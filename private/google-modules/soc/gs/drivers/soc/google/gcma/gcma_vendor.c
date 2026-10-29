// SPDX-License-Identifier: GPL-2.0-only

#include <linux/ktime.h>
#include <linux/module.h>
#include <soc/google/gcma.h>

#include "gcma_debug.h"
#include "gcma_sysfs.h"
#include "gcma_vh.h"

#define CREATE_TRACE_POINTS
#include "gcma_trace.h"

void gcma_vendor_alloc_range(unsigned long start_pfn, unsigned long end_pfn,
			     gfp_t gfp)
{
	unsigned long count = end_pfn - start_pfn + 1;
	u64 start_time, latency;

	trace_gcma_alloc_start(start_pfn, count);
	start_time = ktime_get_ns();
	gcma_alloc_range(start_pfn, end_pfn, gfp);
	latency = ktime_get_ns() - start_time;
	account_gcma_per_page_alloc_latency(count, latency);
	trace_gcma_heap_alloc_finish(count, latency);
}
EXPORT_SYMBOL_GPL(gcma_vendor_alloc_range);

static int __init gcma_vendor_init(void)
{
	int ret;

	ret = gcma_vh_init();
	if (ret)
		return ret;

	ret = gcma_sysfs_init();
	if (ret) {
		gcma_vh_exit();
		return ret;
	}

	gcma_debugfs_init();
	return 0;
}
module_init(gcma_vendor_init);

static void __exit gcma_vendor_exit(void)
{
	gcma_debugfs_exit();
	gcma_sysfs_exit();
	gcma_vh_exit();
}
module_exit(gcma_vendor_exit);

MODULE_DESCRIPTION("Pixel GCMA accounting and diagnostics");
MODULE_LICENSE("GPL");
