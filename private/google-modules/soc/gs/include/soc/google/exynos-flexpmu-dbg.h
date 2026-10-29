/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 */

#ifndef EXYNOS_FLEXPMU_DEBUG_H
#define EXYNOS_FLEXPMU_DEBUG_H

#if IS_REACHABLE(CONFIG_ACPM_FLEXPMU_DBG)
extern void exynos_flexpmu_dbg_log_stop(void);
#else
#define exynos_flexpmu_dbg_log_stop()		do { } while (0)
#endif

#endif
