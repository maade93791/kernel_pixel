/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * PCIe Exynos IOMMU driver header file
 *
 * Copyright (C) 2021 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 */

#ifndef _EXYNOS_PCIE_IOMMU_EXP_H_
#define _EXYNOS_PCIE_IOMMU_EXP_H_

#include <linux/types.h>

struct notifier_block;

int pcie_iommu_map(unsigned long iova, phys_addr_t paddr, size_t size,
		   int prot, int hsi_block_num);
size_t pcie_iommu_unmap(unsigned long iova, size_t size, int hsi_block_num);

void pcie_iommu_tlb_invalidate_all(int hsi_block_num);
void pcie_iommu_tlb_invalidate_range(dma_addr_t iova, size_t size, int hsi_block_num);
int pcie_sysmmu_register_fault_handler(struct notifier_block *pcie_sysmmu_nb, int hsi_block_num);
int pcie_sysmmu_add_fault_handler(struct notifier_block *pcie_sysmmu_nb, int hsi_block_num);

void pcie_sysmmu_set_use_iocc(int hsi_block_num);
void pcie_sysmmu_enable(int hsi_block_num);
void pcie_sysmmu_disable(int hsi_block_num);
void pcie_sysmmu_all_buff_free(int hsi_block_num);
void print_pcie_sysmmu_tlb(int hsi_block_num);

#if IS_ENABLED(CONFIG_SOC_ZUMA)
void pcie_sysmmu_dump_registers(int hsi_block_num);
#endif

#endif /* _EXYNOS_PCIE_IOMMU_EXP_H_ */
