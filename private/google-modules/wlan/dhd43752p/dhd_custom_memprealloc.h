/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _DHD_CUSTOM_MEMPREALLOC_H_
#define _DHD_CUSTOM_MEMPREALLOC_H_

void *dhd_wlan_mem_prealloc(int section, unsigned long size);
int dhd_init_wlan_mem(void);

#endif /* _DHD_CUSTOM_MEMPREALLOC_H_ */
