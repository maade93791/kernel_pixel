// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2021 - Google LLC
 * Author: David Brazdil <dbrazdil@google.com>
 */

#include <linux/ioport.h>
#include <linux/kvm_host.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>

#include <asm/kvm_asm.h>
#include <asm/kvm_host.h>
#include <asm/kvm_pkvm_module.h>

#include "kvm_s2mpu.h"
#include <soc/google/pkvm-s2mpu.h>

extern struct s2mpu_boot_data kvm_nvhe_sym(s2mpu_boot_data);
extern struct kvm_iommu_ops kvm_nvhe_sym(s2mpu_iommu_ops);

struct s2mpu_host_device {
	struct device_node *node;
	phys_addr_t pa;
	u16 sync_first;
	u16 nr_syncs;
	u8 flags;
};

static struct s2mpu_host_device host_devices[S2MPU_MAX_DEVICES];
static phys_addr_t host_sync_pa[S2MPU_MAX_SYNCS];
static unsigned long smpt_pages[NR_GIGABYTES];
static unsigned int nr_host_devices;
static unsigned int nr_host_syncs;
static bool hyp_registered;

static bool ranges_overlap(phys_addr_t a, size_t a_size, phys_addr_t b, size_t b_size)
{
	return a < b + b_size && b < a + a_size;
}

static bool mmio_overlaps(phys_addr_t pa, size_t size)
{
	unsigned int i;

	if (region_intersects(pa, size, IORESOURCE_SYSTEM_RAM, IORES_DESC_NONE) != REGION_DISJOINT)
		return true;
	for (i = 0; i < nr_host_devices; i++)
		if (ranges_overlap(pa, size, host_devices[i].pa, S2MPU_MMIO_SIZE))
			return true;
	for (i = 0; i < nr_host_syncs; i++)
		if (ranges_overlap(pa, size, host_sync_pa[i], SYSMMU_SYNC_S2_MMIO_SIZE))
			return true;
	return false;
}

static int prepare_sysmmu_syncs(struct device_node *parent, struct s2mpu_boot_device *desc)
{
	struct device_node *np;
	struct resource res;
	phys_addr_t pa;
	int i, count, ret;

	desc->sync_first = nr_host_syncs;
	count = of_count_phandle_with_args(parent, "sysmmu_syncs", NULL);
	if (count == -ENOENT)
		return 0;
	if (count < 0)
		return count;
	for (i = 0; i < count; i++) {
		np = of_parse_phandle(parent, "sysmmu_syncs", i);
		if (!np)
			return -EINVAL;
		if (!of_device_is_compatible(np, "google,sysmmu_sync") ||
		    !of_device_is_available(np)) {
			of_node_put(np);
			return -EINVAL;
		}
		ret = of_address_to_resource(np, 0, &res);
		of_node_put(np);
		if (ret)
			return ret;
		if (resource_size(&res) != SYSMMU_SYNC_MMIO_SIZE)
			return -EINVAL;
		pa = res.start + SYSMMU_SYNC_S2_OFFSET;
		if (!PAGE_ALIGNED(pa) || !PAGE_ALIGNED(SYSMMU_SYNC_S2_MMIO_SIZE) ||
		    pa < res.start || pa + SYSMMU_SYNC_S2_MMIO_SIZE < pa ||
		    mmio_overlaps(pa, SYSMMU_SYNC_S2_MMIO_SIZE))
			return -EINVAL;
		if (nr_host_syncs == S2MPU_MAX_SYNCS)
			return -E2BIG;
		host_sync_pa[nr_host_syncs++] = pa;
		desc->nr_syncs++;
	}
	return 0;
}

void pkvm_iommu_s2mpu_release_bootstrap(void)
{
	unsigned int i, gb;

	/* Called only before EL2 driver initialization can retain any pages. */
	for_each_gb(gb) {
		if (smpt_pages[gb])
			free_pages(smpt_pages[gb], SMPT_ORDER(MPT_PROT_BITS));
		smpt_pages[gb] = 0;
	}
	for (i = 0; i < nr_host_devices; i++) {
		of_node_put(host_devices[i].node);
		host_devices[i].node = NULL;
	}
	nr_host_devices = 0;
	nr_host_syncs = 0;
}

int pkvm_iommu_s2mpu_prepare(void)
{
	struct s2mpu_boot_data *boot = &kvm_nvhe_sym(s2mpu_boot_data);
	struct s2mpu_host_device *host;
	struct s2mpu_boot_device *desc;
	struct device_node *np;
	struct resource res;
	unsigned int gb;
	int ret;

	memset(boot, 0, sizeof(*boot));
	for_each_compatible_node(np, NULL, "google," S2MPU_NAME) {
		if (!of_device_is_available(np))
			continue;
		if (nr_host_devices == S2MPU_MAX_DEVICES) {
			ret = -E2BIG;
			goto put_node;
		}
		ret = of_address_to_resource(np, 0, &res);
		if (ret)
			goto put_node;
		if (resource_size(&res) != S2MPU_MMIO_SIZE || !PAGE_ALIGNED(res.start) ||
		    !PAGE_ALIGNED(S2MPU_MMIO_SIZE) || res.start + S2MPU_MMIO_SIZE < res.start ||
		    mmio_overlaps(res.start, S2MPU_MMIO_SIZE)) {
			ret = -EINVAL;
			goto put_node;
		}
		desc = &boot->devices[nr_host_devices];
		host = &host_devices[nr_host_devices];
		desc->pa = res.start;
		desc->off_at_boot = of_property_read_bool(np, "off-at-boot");
		if (of_property_read_bool(np, "built-in-sync"))
			desc->flags |= S2MPU_HAS_SYNC;
		if (of_property_read_bool(np, "deny-all"))
			desc->flags |= S2MPU_DENY_ALL;
		host->node = of_node_get(np);
		host->pa = desc->pa;
		host->flags = desc->flags;
		nr_host_devices++;
		ret = prepare_sysmmu_syncs(np, desc);
		if (ret)
			goto put_node;
		host->sync_first = desc->sync_first;
		host->nr_syncs = desc->nr_syncs;
	}
	if (!nr_host_devices) {
		ret = -ENODEV;
		goto free_bootstrap;
	}
	boot->nr_devices = nr_host_devices;
	boot->nr_syncs = nr_host_syncs;
	memcpy(boot->sync_pa, host_sync_pa, nr_host_syncs * sizeof(host_sync_pa[0]));

	for_each_gb(gb) {
		smpt_pages[gb] = __get_free_pages(GFP_KERNEL | __GFP_ZERO,
						SMPT_ORDER(MPT_PROT_BITS));
		if (!smpt_pages[gb]) {
			ret = -ENOMEM;
			goto free_bootstrap;
		}
		boot->smpt_pa[gb] = __pa(smpt_pages[gb]);
	}
	return nr_host_devices;

put_node:
	of_node_put(np);
free_bootstrap:
	pkvm_iommu_s2mpu_release_bootstrap();
	return ret;
}

int pkvm_iommu_s2mpu_init(void)
{
	pkvm_handle_t driver_id;
	int ret;

	ret = kvm_iommu_register_hyp_ops((void *)pkvm_el2_mod_va(
					    &kvm_nvhe_sym(s2mpu_iommu_ops)), &driver_id);
	if (!ret)
		hyp_registered = true;
	return ret;
}
EXPORT_SYMBOL_GPL(pkvm_iommu_s2mpu_init);

int pkvm_iommu_s2mpu_device_id(struct device *dev)
{
	unsigned int i;

	if (!is_protected_kvm_enabled() || !hyp_registered)
		return -ENODEV;
	for (i = 0; i < nr_host_devices; i++)
		if (host_devices[i].node == dev->of_node)
			return i;
	return -ENODEV;
}

int pkvm_iommu_s2mpu_register(struct device *dev, phys_addr_t addr, u8 flags)
{
	int id = pkvm_iommu_s2mpu_device_id(dev);

	if (id < 0)
		return id;
	if (host_devices[id].pa != addr || host_devices[id].flags != flags)
		return -EINVAL;
	return 0;
}
EXPORT_SYMBOL_GPL(pkvm_iommu_s2mpu_register);

int pkvm_iommu_sysmmu_sync_register(struct device *dev, phys_addr_t addr,
				  struct device *parent)
{
	struct s2mpu_host_device *host;
	unsigned int i;
	int id = pkvm_iommu_s2mpu_device_id(parent);

	if (id < 0)
		return id;
	host = &host_devices[id];
	for (i = host->sync_first; i < host->sync_first + host->nr_syncs; i++)
		if (host_sync_pa[i] == addr + SYSMMU_SYNC_S2_OFFSET)
			return 0;
	return -EINVAL;
}
EXPORT_SYMBOL_GPL(pkvm_iommu_sysmmu_sync_register);
