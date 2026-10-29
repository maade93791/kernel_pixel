// SPDX-License-Identifier: GPL-2.0
/*
 * GCMA(Guaranteed Contiguous Memory Allocator) vendor hooks
 *
 */

#include <linux/gcma.h>
#include <trace/hooks/mm.h>

#include "gcma_debug.h"
#include "gcma_vh.h"

static void vh_gcma_si_meminfo_fixup(void *data, unsigned long *totalram,
				unsigned long *freeram)
{
	*totalram += gcma_stat_get(TOTAL_PAGE);
}

static void vh_gcma_allow_nonworkingset(void *data, bool *allow)
{
	if (!workingset_filter_enabled())
		*allow = true;
}

int __init gcma_vh_init(void)
{
	int ret;

	ret = register_trace_android_vh_si_meminfo_adjust(vh_gcma_si_meminfo_fixup, NULL);
	if (ret)
		return ret;

	ret = register_trace_android_vh_gcma_cc_allow_nonworkingset(
			vh_gcma_allow_nonworkingset, NULL);
	if (ret) {
		unregister_trace_android_vh_si_meminfo_adjust(vh_gcma_si_meminfo_fixup, NULL);
		tracepoint_synchronize_unregister();
	}
	return ret;
}

void gcma_vh_exit(void)
{
	unregister_trace_android_vh_gcma_cc_allow_nonworkingset(vh_gcma_allow_nonworkingset, NULL);
	unregister_trace_android_vh_si_meminfo_adjust(vh_gcma_si_meminfo_fixup, NULL);
	tracepoint_synchronize_unregister();
}
