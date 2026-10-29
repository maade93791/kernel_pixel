/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2023 Google, Inc.
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM gcma

#if !defined(_GCMA_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _GCMA_TRACE_H

#include <linux/tracepoint.h>

TRACE_EVENT(gcma_alloc_start,
        TP_PROTO(unsigned long start_pfn, unsigned long count),
        TP_ARGS(start_pfn, count),
        TP_STRUCT__entry(
                __field(unsigned long, start_pfn)
                __field(unsigned long, count)
        ),
        TP_fast_assign(
                __entry->start_pfn = start_pfn;
                __entry->count = count;
        ),
        TP_printk("start_pfn=%lu count=%lu",
		  __entry->start_pfn, __entry->count)
);

/* ACK provides cumulative discard statistics rather than a per-call count. */
TRACE_EVENT(gcma_heap_alloc_finish,
        TP_PROTO(unsigned long count, u64 latency_ns),
        TP_ARGS(count, latency_ns),
        TP_STRUCT__entry(
                __field(unsigned long, count)
                __field(u64, latency_ns)
        ),
        TP_fast_assign(
                __entry->count = count;
                __entry->latency_ns = latency_ns;
        ),
        TP_printk("count=%lu latency_ns=%llu", __entry->count, __entry->latency_ns)
);

#endif /* _GCMA_TRACE_H */

/* This part must be outside protection */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE gcma_trace

#include <trace/define_trace.h>
