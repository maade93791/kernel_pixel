/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __GBMS_GPIO_H
#define __GBMS_GPIO_H

#include <dt-bindings/gpio/gpio.h>
#include <linux/gpio/driver.h>
#include <linux/of.h>
#include <linux/of_gpio.h>

#if IS_ENABLED(CONFIG_OF_GPIO)
struct gbms_gpio_lookup {
	struct of_phandle_args spec;
	int gpio;
	bool active_low;
};

static int gbms_gpio_match(struct gpio_chip *chip, const void *data)
{
	struct gbms_gpio_lookup *lookup = (struct gbms_gpio_lookup *)data;
	u32 flags = 0;
	int offset;

	if (!device_match_of_node(gpio_device_to_device(chip->gpiodev), lookup->spec.np) ||
	    !chip->of_xlate || chip->of_xlate(chip, &lookup->spec, NULL) < 0)
		return 0;

	if (chip->of_gpio_n_cells != lookup->spec.args_count) {
		lookup->gpio = -EINVAL;
		return 1;
	}

	offset = chip->of_xlate(chip, &lookup->spec, &flags);
	if (offset < 0)
		lookup->gpio = offset;
	else if (offset >= chip->ngpio)
		lookup->gpio = -EINVAL;
	else {
		lookup->gpio = gpio_device_get_base(chip->gpiodev) + offset;
		lookup->active_low = !!(flags & GPIO_ACTIVE_LOW);
	}

	return 1;
}
#endif

/* Keep legacy property names and raw GPIO I/O while obtaining translated polarity. */
static inline int gbms_of_get_gpio_polarity(const struct device_node *node,
					  const char *property, int index,
					  bool *active_low)
{
#if IS_ENABLED(CONFIG_OF_GPIO)
	struct gbms_gpio_lookup lookup = { .gpio = -EPROBE_DEFER };
	struct gpio_device *gdev;
	int ret;

	ret = of_parse_phandle_with_args_map(node, property, "gpio", index, &lookup.spec);
	if (ret)
		return ret;

	/* gpio_device_find() protects the chip throughout the translation callback. */
	gdev = gpio_device_find(&lookup, gbms_gpio_match);
	if (gdev)
		gpio_device_put(gdev);
	of_node_put(lookup.spec.np);

	if (lookup.gpio >= 0)
		*active_low = lookup.active_low;

	return lookup.gpio;
#else
	return of_get_named_gpio(node, property, index);
#endif
}

#endif /* __GBMS_GPIO_H */
