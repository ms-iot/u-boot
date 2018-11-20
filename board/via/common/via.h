/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2018 VIA Technologies, Inc.
 */

#ifndef __VIA_H_
#define __VIA_H_

#include <miiphy.h>

#if defined(CONFIG_TARGET_MX6VAB820) || defined(CONFIG_TARGET_MX6ARTIGOA820)
#define	RAMSIZE_IO0	IMX_GPIO_NR(2, 0)
#define	RAMSIZE_IO1	IMX_GPIO_NR(2, 1)
#elif defined(CONFIG_TARGET_MX6QSM8Q60)
/* QSM-8Q60: GPIO6_IO09 & GPIO6_IO10 for RAM size */
#define	RAMSIZE_IO0	IMX_GPIO_NR(6, 9)
#define	RAMSIZE_IO1	IMX_GPIO_NR(6, 10)
#endif

#define RAMSIZE_4G	(3840u * 1024 * 1024) - 4096
#define RAMSIZE_2G	(2u * 1024 * 1024 * 1024)
#define RAMSIZE_1G	(1u * 1024 * 1024 * 1024)

unsigned int via_ddr_size(void);

int ksz9031_rgmii_rework(struct phy_device *phydev);

int ksz9021_rgmii_rework(struct phy_device *phydev);

#endif	/* __VIA_H_ */
