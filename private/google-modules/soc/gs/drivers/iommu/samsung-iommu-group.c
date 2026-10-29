// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 */

#include <linux/iommu.h>
#include <linux/kmemleak.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/rculist.h>
#include <linux/srcu.h>

#include <soc/google/samsung-iommu.h>

struct samsung_iommu_fault_observer {
	struct list_head node;
	struct device *dev;
	samsung_iommu_fault_handler_t handler;
	void *data;
};

static LIST_HEAD(samsung_iommu_fault_observers);
static DEFINE_MUTEX(samsung_iommu_fault_lock);
DEFINE_STATIC_SRCU(samsung_iommu_fault_srcu);

int samsung_iommu_register_fault_handler(struct device *dev,
					samsung_iommu_fault_handler_t handler, void *data)
{
	struct samsung_iommu_fault_observer *observer, *entry;
	int ret = 0;

	if (!dev || !handler)
		return -EINVAL;

	observer = kzalloc(sizeof(*observer), GFP_KERNEL);
	if (!observer)
		return -ENOMEM;

	observer->dev = get_device(dev);
	observer->handler = handler;
	observer->data = data;

	mutex_lock(&samsung_iommu_fault_lock);
	list_for_each_entry(entry, &samsung_iommu_fault_observers, node) {
		if (entry->dev == dev) {
			ret = -EBUSY;
			goto out;
		}
	}
	list_add_rcu(&observer->node, &samsung_iommu_fault_observers);
out:
	mutex_unlock(&samsung_iommu_fault_lock);
	if (ret) {
		put_device(observer->dev);
		kfree(observer);
	}
	return ret;
}
EXPORT_SYMBOL_GPL(samsung_iommu_register_fault_handler);

int samsung_iommu_unregister_fault_handler(struct device *dev)
{
	struct samsung_iommu_fault_observer *observer;

	mutex_lock(&samsung_iommu_fault_lock);
	list_for_each_entry(observer, &samsung_iommu_fault_observers, node) {
		if (observer->dev != dev)
			continue;
		list_del_rcu(&observer->node);
		synchronize_srcu(&samsung_iommu_fault_srcu);
		mutex_unlock(&samsung_iommu_fault_lock);
		put_device(observer->dev);
		kfree(observer);
		return 0;
	}
	mutex_unlock(&samsung_iommu_fault_lock);
	return 0;
}
EXPORT_SYMBOL_GPL(samsung_iommu_unregister_fault_handler);

int samsung_iommu_report_fault(struct device *dev, const struct samsung_iommu_fault *fault)
{
	struct samsung_iommu_fault_observer *observer;
	int idx, ret = -ENOSYS;

	idx = srcu_read_lock(&samsung_iommu_fault_srcu);
	list_for_each_entry_rcu(observer, &samsung_iommu_fault_observers, node,
			       srcu_read_lock_held(&samsung_iommu_fault_srcu)) {
		if (observer->dev == dev) {
			ret = observer->handler(fault, observer->data);
			break;
		}
	}
	srcu_read_unlock(&samsung_iommu_fault_srcu, idx);
	return ret;
}
EXPORT_SYMBOL_GPL(samsung_iommu_report_fault);

static void samsung_iommu_unregister_fault_action(void *dev)
{
	samsung_iommu_unregister_fault_handler(dev);
}

int devm_samsung_iommu_register_fault_handler(struct device *dev,
					     samsung_iommu_fault_handler_t handler, void *data)
{
	int ret;

	ret = samsung_iommu_register_fault_handler(dev, handler, data);
	if (ret)
		return ret;
	return devm_add_action_or_reset(dev, samsung_iommu_unregister_fault_action, dev);
}
EXPORT_SYMBOL_GPL(devm_samsung_iommu_register_fault_handler);

static int sysmmu_group_device_probe(struct platform_device *pdev)
{
	struct iommu_group *data;
	struct device *dev = &pdev->dev;

	data = iommu_group_alloc();
	if (IS_ERR(data)) {
		dev_err(dev, "Failed to alloc group, ret:%d\n", (int)PTR_ERR(data));
		return PTR_ERR(data);
	}

	platform_set_drvdata(pdev, data);
	iommu_group_set_name(data, dev->of_node->name);

	dev_dbg(dev, "Initialized IOMMU group[%s]\n", dev->of_node->name);

	return 0;
}

static void sysmmu_group_device_shutdown(struct platform_device *pdev)
{
}

static const struct of_device_id sysmmu_group_of_match[] = {
	{ .compatible = "samsung,sysmmu-group" },
	{ }
};
MODULE_DEVICE_TABLE(of, sysmmu_group_of_match);

static struct platform_driver sysmmu_group_driver = {
	.driver	= {
		.name			= "sysmmu-group",
		.of_match_table		= of_match_ptr(sysmmu_group_of_match),
		.suppress_bind_attrs	= true,
	},
	.probe = sysmmu_group_device_probe,
	.shutdown = sysmmu_group_device_shutdown,
};
module_platform_driver(sysmmu_group_driver);
MODULE_LICENSE("GPL v2");
