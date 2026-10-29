/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __GCMA_DEBUG_FS_H__
#define __GCMA_DEBUG_FS_H__

#ifdef CONFIG_DEBUG_FS
void gcma_debugfs_init(void);
void gcma_debugfs_exit(void);
bool workingset_filter_enabled(void);
#else
static inline void gcma_debugfs_init(void) {}
static inline void gcma_debugfs_exit(void) {}
static inline bool workingset_filter_enabled(void) { return true; }
#endif
#endif
