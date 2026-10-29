/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * drivers/media/platform/exynos/mfc/mfc_dec_v4l2.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MFC_DEC_V4L2_H
#define __MFC_DEC_V4L2_H __FILE__

#include "mfc_common.h"

const struct v4l2_ioctl_ops *mfc_get_dec_v4l2_ioctl_ops(void);
int mfc_dec_ctrl_ioctl(struct file *file, unsigned int cmd, void *arg);

#endif /* __MFC_DEC_V4L2_H */
