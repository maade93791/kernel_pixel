/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __GCMA_SYSFS_H__
#define __GCMA_SYSFS_H__

int gcma_sysfs_init(void);
void gcma_sysfs_exit(void);
void account_gcma_per_page_alloc_latency(unsigned long count,
					 unsigned long latency_ns);
#endif
