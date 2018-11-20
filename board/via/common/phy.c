// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 VIA Technologies, Inc.
 */

#include <linux/types.h>
#include <micrel.h>
#include "via.h"

/* support KSZ9031 */
int ksz9031_rgmii_rework(struct phy_device *phydev)
{
	phy_write(phydev, MDIO_DEVAD_NONE, MII_CTRL1000, 0x1c00);

	/* control data pad skew - devaddr = 0x02, register = 0x04 */
	ksz9031_phy_extended_write(phydev, 0x02,
				   MII_KSZ9031_EXT_RGMII_CTRL_SIG_SKEW,
				   MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x0000);
	/* rx data pad skew - devaddr = 0x02, register = 0x05 */
	ksz9031_phy_extended_write(phydev, 0x02,
				   MII_KSZ9031_EXT_RGMII_RX_DATA_SKEW,
				   MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x0000);
	/* tx data pad skew - devaddr = 0x02, register = 0x05 */
	ksz9031_phy_extended_write(phydev, 0x02,
				   MII_KSZ9031_EXT_RGMII_TX_DATA_SKEW,
				   MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x0000);
	/* gtx and rx clock pad skew - devaddr = 0x02, register = 0x08 */
	ksz9031_phy_extended_write(phydev, 0x02,
				   MII_KSZ9031_EXT_RGMII_CLOCK_SKEW,
				   MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x03FF);

	return 0;
}

/* support KSZ9021 */
int ksz9021_rgmii_rework(struct phy_device *phydev)
{
	/* min rx data delay */
	ksz9021_phy_extended_write(phydev,
				   MII_KSZ9021_EXT_RGMII_RX_DATA_SKEW, 0x0);
	/* min tx data delay */
	ksz9021_phy_extended_write(phydev,
				   MII_KSZ9021_EXT_RGMII_TX_DATA_SKEW, 0x0);
	/* max rx/tx clock delay, min rx/tx control */
	ksz9021_phy_extended_write(phydev,
				   MII_KSZ9021_EXT_RGMII_CLOCK_SKEW, 0xf0f0);

	return 0;
}
