/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _SOC_GOOGLE_GS_CHIPID_H
#define _SOC_GOOGLE_GS_CHIPID_H

#include <linux/types.h>

u32 gs_chipid_get_type(void);
s32 gs_chipid_get_dvfs_version(void);
u32 gs_chipid_get_revision(void);
u32 gs_chipid_get_product_id(void);
int gs_chipid_get_ap_hw_tune_array(const u8 **array);

#endif /* _SOC_GOOGLE_GS_CHIPID_H */
