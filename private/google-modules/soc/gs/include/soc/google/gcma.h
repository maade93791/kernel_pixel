/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _SOC_GOOGLE_GCMA_H_
#define _SOC_GOOGLE_GCMA_H_

#include <linux/gcma.h>

void gcma_vendor_alloc_range(unsigned long start_pfn, unsigned long end_pfn,
			     gfp_t gfp);
#endif
