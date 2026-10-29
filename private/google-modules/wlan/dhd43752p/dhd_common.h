/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _DHD_COMMON_H_
#define _DHD_COMMON_H_

#include <dhd.h>

struct dhd_dump;

int dhd_collect_coredump(dhd_pub_t *dhdp, struct dhd_dump *dump);
uint wl_get_port_num(wl_io_pport_t *io_pport);
int wl_event_process(dhd_pub_t *dhd_pub, int *ifidx, void *pktdata,
	uint pktlen, void **data_ptr, void *raw_event);
int wl_pattern_atoh(char *src, char *dst);
int pattern_atoh_len(char *src, char *dst, int len);

#endif /* _DHD_COMMON_H_ */
