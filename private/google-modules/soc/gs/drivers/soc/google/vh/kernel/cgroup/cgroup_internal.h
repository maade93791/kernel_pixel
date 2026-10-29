/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef PIXEL_VH_CGROUP_INTERNAL_H
#define PIXEL_VH_CGROUP_INTERNAL_H

#include <linux/types.h>

struct cgroup;
struct task_struct;

void rvh_cgroup_force_kthread_migration_pixel_mod(void *data, struct task_struct *tsk,
						struct cgroup *dst_cgrp, bool *force_migration);

#endif
