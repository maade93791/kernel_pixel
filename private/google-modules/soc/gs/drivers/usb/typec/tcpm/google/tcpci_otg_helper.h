// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2019, Google LLC
 *
 * FUSB307B OTG HELPER
 */
#ifndef __TCPCI_OTG_HELPER__
#define __TCPCI_OTG_HELPER__

#include <linux/types.h>

struct i2c_client;
struct fusb307b_plat;

int max77729_gpio_set(struct i2c_client *client, unsigned int gpio,
		      bool dir_out, bool out_hi);
int enable_ls(struct i2c_client *client);
int fusb307b_update_bits16(struct fusb307b_plat *chip, unsigned int reg,
			 u16 mask, u16 val);
int fusb307b_update_bits8(struct fusb307b_plat *chip, unsigned int reg,
			u8 mask, u8 val);
int max77729_disable_water_detection(struct i2c_client *client);
#endif
