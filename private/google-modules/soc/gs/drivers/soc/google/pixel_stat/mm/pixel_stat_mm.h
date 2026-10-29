/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __PIXEL_STAT_MM_H__
#define __PIXEL_STAT_MM_H__

#include <linux/gfp_types.h>
#include <linux/types.h>

struct address_space;
struct folio;

void vh_filemap_get_folio_mod(void *data, struct address_space *mapping,
			    pgoff_t index, int fgp_flags, gfp_t gfp_mask,
			    struct folio *folio);
void rvh_mapping_shrinkable(void *data, bool *shrinkable);
int pixel_mm_sysfs(void);
int create_mm_procfs_node(void);

#endif
