/* SPDX-License-Identifier: GPL-2.0-only */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM power

#if !defined(_TRACE_PIXEL_CLOCK_RATE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_PIXEL_CLOCK_RATE_H

#include <linux/tracepoint.h>

/* Preserve the DFS notification formerly provided by trace/events/power.h. */
TRACE_EVENT(clock_set_rate,
	TP_PROTO(const char *name, unsigned int state, unsigned int cpu_id),
	TP_ARGS(name, state, cpu_id),
	TP_STRUCT__entry(
		__string(name, name)
		__field(u64, state)
		__field(u64, cpu_id)
	),
	TP_fast_assign(
		__assign_str(name);
		__entry->state = state;
		__entry->cpu_id = cpu_id;
	),
	TP_printk("%s state=%lu cpu_id=%lu", __get_str(name),
		  (unsigned long)__entry->state, (unsigned long)__entry->cpu_id)
);

#endif

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH trace/events
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE gs_clock_rate

#include <trace/define_trace.h>

/* Do not leak the include path into other trace event headers. */
#ifndef TRACE_HEADER_MULTI_READ
#undef TRACE_INCLUDE_PATH
#undef TRACE_INCLUDE_FILE
#endif
