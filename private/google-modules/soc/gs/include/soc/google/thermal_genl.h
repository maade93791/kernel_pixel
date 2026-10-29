/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _SOC_GOOGLE_THERMAL_GENL_H
#define _SOC_GOOGLE_THERMAL_GENL_H

void register_tz_id_ignore_genl(int tz_id);
void vh_enable_thermal_genl_check(void *data, int event, int tz_id, int *enable_thermal_genl);

#endif /* _SOC_GOOGLE_THERMAL_GENL_H */
