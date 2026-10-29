/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _SOC_GOOGLE_SAMSUNG_IOMMU_H
#define _SOC_GOOGLE_SAMSUNG_IOMMU_H

#include <linux/bits.h>
#include <linux/types.h>

struct device;

enum samsung_iommu_fault_type {
	SAMSUNG_IOMMU_FAULT_DMA_UNRECOV,
	/* A SysMMU translation fault, not a PCI PRI page request. */
	SAMSUNG_IOMMU_FAULT_PAGE_REQ,
};

enum samsung_iommu_fault_reason {
	SAMSUNG_IOMMU_FAULT_REASON_UNKNOWN,
	SAMSUNG_IOMMU_FAULT_REASON_WALK_EABT,
	SAMSUNG_IOMMU_FAULT_REASON_PTE_FETCH,
	SAMSUNG_IOMMU_FAULT_REASON_ACCESS,
	SAMSUNG_IOMMU_FAULT_REASON_PASID_FETCH,
};

#define SAMSUNG_IOMMU_FAULT_PASID_VALID	BIT(0)
#define SAMSUNG_IOMMU_FAULT_ADDR_VALID	BIT(1)
#define SAMSUNG_IOMMU_FAULT_FETCH_ADDR_VALID BIT(2)

#define SAMSUNG_IOMMU_FAULT_PERM_READ	BIT(0)
#define SAMSUNG_IOMMU_FAULT_PERM_WRITE	BIT(1)

struct samsung_iommu_fault {
	enum samsung_iommu_fault_type type;
	/* The hardware fault index and security state, before translation. */
	u32 sysmmu_type;
	u32 transaction_info;
	bool secure;
	struct {
		enum samsung_iommu_fault_reason reason;
		u32 flags;
		u32 pasid;
		u32 perm;
		u64 addr;
		u64 fetch_addr;
	} event;
};

typedef int (*samsung_iommu_fault_handler_t)(const struct samsung_iommu_fault *fault,
					    void *data);

/*
 * Handlers run in the SysMMU interrupt thread and may sleep. A return value of
 * -EAGAIN requests that the SysMMU driver clear the fault and continue; other
 * results preserve its fatal-fault policy. Registration is per device, not per
 * domain, so devices sharing a DMA domain retain independent diagnostics.
 *
 * Unregister waits for active handlers before returning. The caller must
 * unregister before freeing handler data or unloading its code. Neither
 * register nor unregister may be called from a fault handler.
 */
int samsung_iommu_register_fault_handler(struct device *dev,
					samsung_iommu_fault_handler_t handler, void *data);
int samsung_iommu_unregister_fault_handler(struct device *dev);
int samsung_iommu_report_fault(struct device *dev, const struct samsung_iommu_fault *fault);
int devm_samsung_iommu_register_fault_handler(struct device *dev,
					     samsung_iommu_fault_handler_t handler, void *data);

#endif /* _SOC_GOOGLE_SAMSUNG_IOMMU_H */
