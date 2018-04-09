/*
 * Copyright 2018 Scalys B.V.
 * opensource@scalys.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <i2c.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/fsl_serdes.h>
#ifdef CONFIG_FSL_LS_PPA
#include <asm/arch/ppa.h>
#endif
#include <asm/arch/mmu.h>
#include <asm/arch/soc.h>
#include <hwconfig.h>
#include <ahci.h>
#include <mmc.h>
#include <scsi.h>
#include <fsl_esdhc.h>
#include <environment.h>
#include <fsl_mmdc.h>
#include <netdev.h>
#include <fsl_sec.h>
#include <fsl_csu.h>
#include "usb_grapeboard.h"
#include <../../../include/generated/autoconf.h>
#include <usb.h>
#include "gpio_grapeboard.h"
#include "board_configuration_data.h"

DECLARE_GLOBAL_DATA_PTR;

int checkboard(void)
{
	struct ccsr_gpio *pgpio = (void *)(CONFIG_SYS_GPIO2);

	int m2_config = 0;
	int serdes_cfg = get_serdes_protocol();

	puts("Board: Grape board\n");

	/* set QSPI chip select muxing to 0 */
	setbits_be32(&pgpio->gpdir, QSPI_MUX_N_MASK);
	clrbits_be32(&pgpio->gpdat, QSPI_MUX_N_MASK);

	/* Configure USB hub */
	usb_hx3_hub_init();

	/* Check for an M.2 module */
	clrbits_be32(&pgpio->gpdir, (M2_CFG0 | M2_CFG1 | M2_CFG2 | M2_CFG3));
	m2_config = (in_be32(&pgpio->gpdat) & (M2_CFG0 | M2_CFG1 | M2_CFG2 | M2_CFG3));

	switch(m2_config) {
	case 0:
		printf("M.2: SATA SSD module found\n");
		if (serdes_cfg & 0x5)
			printf("Warning: SERDES has not been configured in RCW for SATA!\n");
		break;
	case M2_CFG1:
		printf("M.2: PCIe SSD module found\n");
		if (serdes_cfg & 0x8)
			printf("Warning: SERDES has not been configured in RCW for PCIe!\n");
		break;
	case (M2_CFG0 | M2_CFG1 | M2_CFG2 | M2_CFG3):
		printf("M.2: No module detected\n");
		break;
	default:
		printf("M.2: A module has been detected (TODO: support new module type)\n");
		break;
	}

	return 0;
}

int misc_init_r(void)
{
	const void* bcd_dtc_blob;
	int ret;

	bcd_dtc_blob = get_boardinfo_rescue_flash();
	if (bcd_dtc_blob != NULL) {
		/* Board Configuration Data is intact, ready for parsing */
		ret = add_mac_addressess_to_env(bcd_dtc_blob);
		if (ret != 0) {
			printf("Error adding BCD data to environment\n");
		}
	}

	return 0;
}

int dram_init(void)
{
	static const struct fsl_mmdc_info mparam = {
		0x05180000,	/* mdctl */
		0x00030035,	/* mdpdc */
		0x12554000,	/* mdotc */
		0xbabf7954,	/* mdcfg0 */
		0xdb328f64,	/* mdcfg1 */
		0x01ff00db,	/* mdcfg2 */
		0x00001680,	/* mdmisc */
		0x0f3c8000,	/* mdref */
		0x00002000,	/* mdrwd */
		0x00bf1023,	/* mdor */
		0x0000003f,	/* mdasp */
		0x0000022a,	/* mpodtctrl */
		0xa1390003,	/* mpzqhwctrl */
	};

	mmdc_init(&mparam);

	gd->ram_size = CONFIG_SYS_SDRAM_SIZE;
#if !defined(CONFIG_SPL) || defined(CONFIG_SPL_BUILD)
	/* This will break-before-make MMU for DDR */
	update_early_mmu_table();
#endif

	return 0;
}

int board_early_init_f(void)
{
	fsl_lsch2_early_init_f();

	return 0;
}

int board_init(void)
{
	struct ccsr_cci400 *cci = (struct ccsr_cci400 *)(CONFIG_SYS_IMMR +
					CONFIG_SYS_CCI400_OFFSET);
	/*
	 * Set CCI-400 control override register to enable barrier
	 * transaction
	 */
	out_le32(&cci->ctrl_ord, CCI400_CTRLORD_EN_BARRIER);

#ifdef CONFIG_SYS_FSL_ERRATUM_A010315
	erratum_a010315();
#endif

#ifdef CONFIG_ENV_IS_NOWHERE
	gd->env_addr = (ulong)&default_environment[0];
#endif

#ifdef CONFIG_FSL_CAAM
	sec_init();
#endif

#ifdef CONFIG_FSL_LS_PPA
	ppa_init();
#endif
	return 0;
}

int esdhc_status_fixup(void *blob, const char *compat)
{
	char esdhc0_path[] = "/soc/esdhc@1560000";

	do_fixup_by_path(blob, esdhc0_path, "status", "okay",
			 sizeof("okay"), 1);

	return 0;
}

int ft_board_setup(void *blob, bd_t *bd)
{
	arch_fixup_fdt(blob);

	ft_cpu_setup(blob, bd);

	return 0;
}

void scsi_init(void) {
	printf("\r"); /* SCSI init already completed in board_late_init, so skip message */
}

