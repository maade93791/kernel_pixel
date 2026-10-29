/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _SOC_GOOGLE_EXYNOS_USB_PHY_H_
#define _SOC_GOOGLE_EXYNOS_USB_PHY_H_

#include <linux/types.h>

struct device;
struct phy;

extern void __iomem *phycon_base_addr;

int exynos_usbdrd_set_s2mpu_pm_ops(int (*cb)(struct device *dev, bool on));
int exynos_usbdrd_s2mpu_manual_control(bool on);
int exynos_usbdrd_phy_tune(struct phy *phy, int phy_state);
void exynos_usbdrd_phy_conn(struct phy *phy, int is_conn);
int exynos_usbdrd_dp_ilbk(struct phy *phy);
int exynos_usbdrd_phy_vendor_set(struct phy *phy, int is_enable, int is_cancel);
int exynos_usbdrd_phy_set(struct phy *phy, int option, void *info);
int exynos_usbdrd_pipe3_enable(struct phy *phy);
int exynos_usbdrd_pipe3_disable(struct phy *phy);
void exynos_usbdrd_usbdp_tca_set(struct phy *phy, int mux, int low_power_en);
void exynos_usbdrd_dp_use_notice(int lane);

#endif /* _SOC_GOOGLE_EXYNOS_USB_PHY_H_ */
