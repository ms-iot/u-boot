/*
 * Copyright 2018 Scalys B.V.
 * opensource@scalys.com
 *
 * SPDX-License-Identifier:GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <asm/io.h>
#include <netdev.h>
#include <fm_eth.h>
#include <fsl_mdio.h>
#include <malloc.h>
#include <asm/types.h>
#include <fsl_dtsec.h>
#include <asm/arch/soc.h>
#include <asm/arch-fsl-layerscape/config.h>
#include <asm/arch-fsl-layerscape/immap_lsch2.h>
#include <asm/arch/fsl_serdes.h>
#include <net/pfe_eth/pfe_eth.h>
#include <dm/platform_data/pfe_dm_eth.h>
#include <i2c.h>

#define DEFAULT_PFE_MDIO_NAME "PFE_MDIO"
#define DEFAULT_PFE_MDIO1_NAME "PFE_MDIO1"


void reset_phy(void)
{
	/* No PHY reset control from LS1012A */
}

int pfe_eth_board_init(struct udevice *dev)
{
	static int init_done;
	struct mii_dev *bus;
	struct pfe_mdio_info mac_mdio_info;
	struct pfe_eth_dev *priv = dev_get_priv(dev);

	if (!init_done) {
		reset_phy();

		mac_mdio_info.reg_base = (void *)EMAC1_BASE_ADDR;
		mac_mdio_info.name = DEFAULT_PFE_MDIO_NAME;

		bus = pfe_mdio_init(&mac_mdio_info);
		if (!bus) {
			printf("Failed to register mdio\n");
			return -1;
		}

		init_done = 1;
	}

	if (priv->gemac_port) {
		mac_mdio_info.reg_base = (void *)EMAC2_BASE_ADDR;
		mac_mdio_info.name = DEFAULT_PFE_MDIO1_NAME;
		bus = pfe_mdio_init(&mac_mdio_info);
		if (!bus) {
			printf("Failed to register mdio\n");
			return -1;
		}
	}

	pfe_set_mdio(priv->gemac_port,
		     miiphy_get_dev_by_name(DEFAULT_PFE_MDIO_NAME));
	if (!priv->gemac_port)
		/* MAC1 */
		pfe_set_phy_address_mode(priv->gemac_port,
					 CONFIG_PFE_EMAC1_PHY_ADDR,
					 PHY_INTERFACE_MODE_SGMII);
	else
		/* MAC2 */
		pfe_set_phy_address_mode(priv->gemac_port,
					 CONFIG_PFE_EMAC2_PHY_ADDR,
					 PHY_INTERFACE_MODE_SGMII);
                
        /* Initialize TI DP83867CS PHY LEDs as:
	 * LED_3 = 0x0: Link established (not connected)
	 * LED_2 = 0x0: Link established (not connected)
	 * LED_1 = 0xB: Link established, blink for activity (green LED)
	 * LED_0 = 0x5: 1000BT link established (orange LED)
	 */
	miiphy_write(DEFAULT_PFE_MDIO_NAME,CONFIG_PFE_EMAC1_PHY_ADDR,0x18,0x00B5);
	miiphy_write(DEFAULT_PFE_MDIO_NAME,CONFIG_PFE_EMAC2_PHY_ADDR,0x18,0x00B5);

	/* Enable PHY Power save mode */
	miiphy_write(DEFAULT_PFE_MDIO_NAME,CONFIG_PFE_EMAC1_PHY_ADDR,0x10,0x0200);
	miiphy_write(DEFAULT_PFE_MDIO_NAME,CONFIG_PFE_EMAC2_PHY_ADDR,0x10,0x0200);
        
	return 0;
}

static struct pfe_eth_pdata pfe_pdata0 = {
	.pfe_eth_pdata_mac = {
		.iobase = (phys_addr_t)EMAC1_BASE_ADDR,
		.phy_interface = 0,
	},

	.pfe_ddr_addr = {
		.ddr_pfe_baseaddr = (void *)CONFIG_DDR_PFE_BASEADDR,
		.ddr_pfe_phys_baseaddr = CONFIG_DDR_PFE_PHYS_BASEADDR,
	},
};

static struct pfe_eth_pdata pfe_pdata1 = {
	.pfe_eth_pdata_mac = {
		.iobase = (phys_addr_t)EMAC2_BASE_ADDR,
		.phy_interface = 1,
	},

	.pfe_ddr_addr = {
		.ddr_pfe_baseaddr = (void *)CONFIG_DDR_PFE_BASEADDR,
		.ddr_pfe_phys_baseaddr = CONFIG_DDR_PFE_PHYS_BASEADDR,
	},
};

U_BOOT_DEVICE(ls1012a_pfe0) = {
	.name = "pfe_eth",
	.platdata = &pfe_pdata0,
};

U_BOOT_DEVICE(ls1012a_pfe1) = {
	.name = "pfe_eth",
	.platdata = &pfe_pdata1,
};
