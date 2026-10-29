/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __EXYNOS_CORESIGHT_H__
#define __EXYNOS_CORESIGHT_H__

#include <linux/kernel.h>
#include <linux/types.h>

#define CORESIGHT_CPUPM_PRIORITY	(INT_MAX / 2)

int gs_coresight_etm_external_etr_on(u64 buf_addr, u32 buf_size);
int gs_coresight_etm_external_etr_off(void);

#endif
