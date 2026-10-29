// SPDX-License-Identifier: GPL-2.0-only

#include <linux/debugfs.h>
#include <linux/err.h>
#include <linux/types.h>

#include "gcma_debug.h"

static bool workingset = true;
static struct dentry *gcma_debugfs_root;

bool workingset_filter_enabled(void)
{
	return READ_ONCE(workingset);
}

void __init gcma_debugfs_init(void)
{
	gcma_debugfs_root = debugfs_create_dir("gcma", NULL);
	if (IS_ERR(gcma_debugfs_root)) {
		gcma_debugfs_root = NULL;
		return;
	}

	debugfs_create_bool("workingset", 0644, gcma_debugfs_root, &workingset);
	/* ACK does not export its LRU eviction helper, so expose no evict file. */
}

void gcma_debugfs_exit(void)
{
	debugfs_remove_recursive(gcma_debugfs_root);
}
