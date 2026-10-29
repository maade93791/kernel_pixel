/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _PIXEL_METRICS_H
#define _PIXEL_METRICS_H

struct kobject;

int thermal_metrics_init(struct kobject *metrics_kobj);
int perf_metrics_init(struct kobject *metrics_kobj);

#endif /* _PIXEL_METRICS_H */
