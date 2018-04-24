/*
 * Copyright 2018 Scalys B.V.
 * opensource@scalys.com
 *
 * SPDX-License-Identifier:GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <netdev.h>
#include <fm_eth.h>
#include <fsl_mdio.h>
#include <malloc.h>
#include <fsl_dtsec.h>
#include <asm/arch/soc.h>
#include <asm/arch-fsl-layerscape/config.h>
#include <asm/arch/fsl_serdes.h>

#include <pfe_eth/pfe_eth.h>
#include <asm/arch-fsl-layerscape/immap_lsch2.h>
#include <i2c.h>

#define DEFAULT_PFE_MDIO_NAME "PFE_MDIO"
#define DEFAULT_PFE_MDIO1_NAME "PFE_MDIO1"


void reset_phy(void)
{
	/* No PHY reset control from LS1012A */
}

int board_eth_init(bd_t *bis)
{
#ifdef CONFIG_FSL_PFE
	struct mii_dev *bus;
	struct mdio_info mac1_mdio_info;
	struct mdio_info mac2_mdio_info;

	reset_phy();

	init_pfe_scfg_dcfg_regs();

	/* Initialize SGMIIA on MDIO1 */
	mac1_mdio_info.reg_base = (void *)EMAC1_BASE_ADDR;
	mac1_mdio_info.name = DEFAULT_PFE_MDIO_NAME;

	bus = pfe_mdio_init(&mac1_mdio_info);
	if (!bus) {
		printf("Failed to register mdio 1\n");
		return -1;
	}

	/* Initialize SGMIIB on MDIO2 */
	mac2_mdio_info.reg_base = (void *)EMAC2_BASE_ADDR;
	mac2_mdio_info.name = DEFAULT_PFE_MDIO1_NAME;

	bus = pfe_mdio_init(&mac2_mdio_info);
	if (!bus) {
		printf("Failed to register mdio 2\n");
		return -1;
	}

	/* Initialize PHYs on MDIO1 */
	pfe_set_mdio(0, miiphy_get_dev_by_name(DEFAULT_PFE_MDIO_NAME));
	pfe_set_phy_address_mode(0, EMAC1_PHY_ADDR, PHY_INTERFACE_MODE_SGMII);

	pfe_set_mdio(1, miiphy_get_dev_by_name(DEFAULT_PFE_MDIO_NAME));
	pfe_set_phy_address_mode(1, EMAC2_PHY_ADDR, PHY_INTERFACE_MODE_SGMII);

	/* Initialize TI DP83867CS PHY LEDs as:
	 * LED_3 = 0x0: Link established (not connected)
	 * LED_2 = 0x0: Link established (not connected)
	 * LED_1 = 0xB: Link established, blink for activity (green LED)
	 * LED_0 = 0x5: 1000BT link established (orange LED)
	 */
	miiphy_write(DEFAULT_PFE_MDIO_NAME,EMAC1_PHY_ADDR,0x18,0x00B5);
	miiphy_write(DEFAULT_PFE_MDIO_NAME,EMAC2_PHY_ADDR,0x18,0x00B5);

	/* Enable PHY Power save mode */
	miiphy_write(DEFAULT_PFE_MDIO_NAME,EMAC1_PHY_ADDR,0x10,0x0200);
	miiphy_write(DEFAULT_PFE_MDIO_NAME,EMAC2_PHY_ADDR,0x10,0x0200);

	cpu_eth_init(bis);
#endif
	return pci_eth_init(bis);
}
