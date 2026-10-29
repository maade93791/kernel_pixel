// SPDX-License-Identifier: GPL-2.0-only
/*
 * Google Whitechapel AoC ALSA  Driver on PCM
 *
 * Copyright (c) 2023 Google LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <sound/core.h>
#include <linux/usb.h>
#include <linux/mutex.h>

#include "aoc_alsa.h"
#include "card.h"
#include "usbaudio.h"

struct uaudio_dev {
	int card_num;
	struct snd_usb_audio *chip;
	bool original_allow_runtime_suspend;
};

static struct uaudio_dev uadev[SNDRV_CARDS];

/*
 * USB sound's register_mutex serializes connect/disconnect callbacks. This
 * separate lock also pins the selected chip while an AoC endpoint request or
 * consumer callback runs. Lock order is register_mutex -> this -> chip->mutex.
 */
static DEFINE_MUTEX(aoc_usb_mutex);
static const struct aoc_alsa_usb_ops *aoc_usb_ops;
static bool aoc_usb_stopping = true;

/* Caller holds aoc_usb_mutex and chip->mutex. */
static void aoc_usb_update_suspend_policy(struct uaudio_dev *dev)
{
	if (dev->original_allow_runtime_suspend ||
	    (aoc_usb_ops && aoc_usb_ops->allow_runtime_suspend))
		dev->chip->quirk_flags |= QUIRK_FLAG_PCM_ALLOW_RUNTIME_SUSPEND;
	else
		dev->chip->quirk_flags &= ~QUIRK_FLAG_PCM_ALLOW_RUNTIME_SUSPEND;
}

static bool snd_usb_is_implicit_feedback(struct snd_usb_endpoint *ep)
{
	return  ep->implicit_fb_sync && usb_pipeout(ep->pipe);
}

static struct snd_usb_substream *find_substream(unsigned int card_num,
	unsigned int device, unsigned int direction)
{
	struct snd_usb_stream *as;
	struct snd_usb_substream *subs = NULL;
	struct snd_usb_audio *chip;

	chip = uadev[card_num].chip;

	if (!chip || atomic_read(&chip->shutdown))
		goto done;

	if (device >= chip->pcm_devs)
		goto done;

	if (direction > SNDRV_PCM_STREAM_CAPTURE)
		goto done;

	list_for_each_entry(as, &chip->pcm_list, list) {
		if (as->pcm_index == device) {
			subs = &as->substream[direction];
			goto done;
		}
	}

done:
	return subs;
}

static void usb_audio_offload_connect(struct snd_usb_audio *chip)
{
	int card_num;

	if (!chip || atomic_read(&chip->shutdown))
		return;

	card_num = chip->card->number;
	if (card_num < 0 || card_num >= SNDRV_CARDS)
		return;

	guard(mutex)(&aoc_usb_mutex);
	if (aoc_usb_stopping)
		return;
	pr_info("%s card%d connected", __func__, card_num);
	mutex_lock(&chip->mutex);
	if (uadev[card_num].chip != chip) {
		get_device(&chip->card->card_dev);
		uadev[card_num].original_allow_runtime_suspend =
			chip->quirk_flags & QUIRK_FLAG_PCM_ALLOW_RUNTIME_SUSPEND;
	}
	uadev[card_num].chip = chip;
	uadev[card_num].card_num = card_num;
	chip->quirk_flags &= ~QUIRK_FLAG_GENERIC_IMPLICIT_FB;
	chip->quirk_flags |= QUIRK_FLAG_SKIP_IMPLICIT_FB;
	aoc_usb_update_suspend_policy(&uadev[card_num]);
	mutex_unlock(&chip->mutex);
	if (aoc_usb_ops)
		aoc_usb_ops->connection(true);
}

static void usb_audio_offload_disconnect(struct snd_usb_audio *chip)
{
	struct snd_card *card = NULL;
	int card_num;

	if (!chip)
		return;

	card_num = chip->card->number;
	if (card_num < 0 || card_num >= SNDRV_CARDS)
		return;

	mutex_lock(&aoc_usb_mutex);
	if (aoc_usb_stopping) {
		mutex_unlock(&aoc_usb_mutex);
		return;
	}
	mutex_lock(&chip->mutex);
	pr_info("%s card%d disconnected", __func__, card_num);
	if (uadev[card_num].chip == chip) {
		if (!uadev[card_num].original_allow_runtime_suspend)
			chip->quirk_flags &= ~QUIRK_FLAG_PCM_ALLOW_RUNTIME_SUSPEND;
		card = chip->card;
		uadev[card_num].chip = NULL;
		uadev[card_num].card_num = 0;
	}
	mutex_unlock(&chip->mutex);
	if (aoc_usb_ops)
		aoc_usb_ops->connection(false);
	mutex_unlock(&aoc_usb_mutex);
	if (card)
		put_device(&card->card_dev);
}

int aoc_set_usb_mem_config(struct aoc_chip *achip)
{
	struct usb_host_endpoint *ep = NULL;
	struct usb_host_endpoint *fb_ep = NULL;
	struct snd_usb_substream *subs = NULL;
	struct snd_usb_audio *chip = NULL;
	unsigned int card_num;
	unsigned int device;
	unsigned int direction;
	bool implicit_fb = false;

	if (!achip)
		return -ENODEV;

	guard(mutex)(&aoc_usb_mutex);
	if (aoc_usb_stopping)
		return -ENODEV;
	card_num = achip->usb_card;
	device = achip->usb_device;
	direction = achip->usb_direction;
	if (card_num < SNDRV_CARDS) {
		chip = uadev[card_num].chip;
	}
	if (!chip) {
		pr_err("%s no device connected (card %u device %u, direction %u)",
			__func__, card_num, device, direction);
		return -ENODEV;
	}
	pr_info("%s card %u device %u, direction %u", __func__,
			card_num, device, direction);
	if (direction <= SNDRV_PCM_STREAM_CAPTURE) {
		mutex_lock(&chip->mutex);
		subs = find_substream(card_num, device, direction);
		if (!subs || !subs->data_endpoint) {
			pr_err("%s subs not found", __func__);
			mutex_unlock(&chip->mutex);
			return -ENODEV;
		}
		ep = usb_pipe_endpoint(subs->dev, subs->data_endpoint->pipe);
		implicit_fb = snd_usb_is_implicit_feedback(subs->data_endpoint);
		if (!ep) {
			pr_err("%s data ep # %d context is null\n",
					__func__, subs->data_endpoint->ep_num);
			mutex_unlock(&chip->mutex);
			return -ENODEV;
		}

		if (aoc_usb_ops) {
			aoc_usb_ops->setup_transfer_ring(subs->dev, ep);
		} else {
			pr_info("%s call aoc_alsa_usb_callback_register() first, skip", __func__);
		}

		if (subs->sync_endpoint && !implicit_fb) {
			fb_ep = usb_pipe_endpoint(subs->dev, subs->sync_endpoint->pipe);
			if (!fb_ep) {
				pr_info("%s sync ep # %d context is null\n",
						__func__, subs->sync_endpoint->ep_num);
			} else {
				if (aoc_usb_ops) {
					aoc_usb_ops->setup_transfer_ring(subs->dev, fb_ep);
				}
				aoc_set_usb_feedback_endpoint(achip, subs->dev, fb_ep);
			}
		}
		mutex_unlock(&chip->mutex);
	}

	aoc_set_usb_offload_state(achip, true);
	return 0;
}

int aoc_alsa_usb_callback_register(const struct aoc_alsa_usb_ops *ops)
{
	int i;

	if (!ops || !ops->setup_transfer_ring || !ops->connection)
		return -EINVAL;

	guard(mutex)(&aoc_usb_mutex);
	if (aoc_usb_stopping)
		return -ENODEV;
	if (aoc_usb_ops)
		return -EBUSY;

	aoc_usb_ops = ops;
	/* Replay cards connected before the xHCI consumer became ready. */
	for (i = 0; i < ARRAY_SIZE(uadev); i++) {
		struct snd_usb_audio *chip = uadev[i].chip;

		if (!chip)
			continue;
		mutex_lock(&chip->mutex);
		aoc_usb_update_suspend_policy(&uadev[i]);
		mutex_unlock(&chip->mutex);
		ops->connection(true);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(aoc_alsa_usb_callback_register);

void aoc_alsa_usb_callback_unregister(const struct aoc_alsa_usb_ops *ops)
{
	int i;

	guard(mutex)(&aoc_usb_mutex);
	if (aoc_usb_ops != ops)
		return;

	aoc_usb_ops = NULL;
	for (i = 0; i < ARRAY_SIZE(uadev); i++) {
		struct snd_usb_audio *chip = uadev[i].chip;

		if (!chip)
			continue;
		mutex_lock(&chip->mutex);
		aoc_usb_update_suspend_policy(&uadev[i]);
		mutex_unlock(&chip->mutex);
	}
}
EXPORT_SYMBOL_GPL(aoc_alsa_usb_callback_unregister);

static struct snd_usb_platform_ops aoc_platform_ops = {
	.connect_cb = usb_audio_offload_connect,
	.disconnect_cb = usb_audio_offload_disconnect,
};

int aoc_usb_init(void)
{
	int ret;

	scoped_guard(mutex, &aoc_usb_mutex)
		aoc_usb_stopping = false;
	ret = snd_usb_register_platform_ops(&aoc_platform_ops);
	if (ret) {
		scoped_guard(mutex, &aoc_usb_mutex)
			aoc_usb_stopping = true;
		return ret;
	}
	snd_usb_rediscover_devices();
	return 0;
}

void aoc_usb_exit(void)
{
	struct snd_card *cards[SNDRV_CARDS] = {};
	int i;

	/* Drain endpoint users while native disconnect still obeys our lock. */
	scoped_guard(mutex, &aoc_usb_mutex) {
		aoc_usb_stopping = true;
		for (i = 0; i < ARRAY_SIZE(uadev); i++) {
			struct snd_usb_audio *chip = uadev[i].chip;

			if (!chip)
				continue;
			mutex_lock(&chip->mutex);
			if (!uadev[i].original_allow_runtime_suspend)
				chip->quirk_flags &= ~QUIRK_FLAG_PCM_ALLOW_RUNTIME_SUSPEND;
			mutex_unlock(&chip->mutex);
			cards[i] = chip->card;
			uadev[i].chip = NULL;
		}
	}
	/* SRCU may wait for callbacks taking our mutex: never hold it here. */
	snd_usb_unregister_platform_ops();
	for (i = 0; i < ARRAY_SIZE(cards); i++)
		if (cards[i])
			put_device(&cards[i]->card_dev);
}
