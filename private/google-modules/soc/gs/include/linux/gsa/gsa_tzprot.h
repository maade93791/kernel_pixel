/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _LINUX_GSA_TZPROT_H
#define _LINUX_GSA_TZPROT_H

#include <linux/types.h>

struct device;

int trusty_protect_ip(struct device *dev, u32 prot_id, bool enable);

#endif /* _LINUX_GSA_TZPROT_H */
