/*
 * Copyright (C) 2018 Freescale Semiconductor, Inc.
 * MX6 Sabre Lite boards.
 *
 * Author: David Vescovi <dvescovi@gmail.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <asm/arch/clock.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/iomux.h>
#include <asm/arch/mx6-pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <mmc.h>
#include <fsl_esdhc.h>
#include <spl.h>

DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_SPL_BUILD)
#include <asm/arch/mx6-ddr.h>

/*
 * Driving strength:
 *   0x30 == 40 Ohm
 *   0x28 == 48 Ohm
 */
#define IMX6DQ_DRIVE_STRENGTH	0x30

/* configure MX6Q/DUAL mmdc DDR io registers */
static struct mx6dq_iomux_ddr_regs mx6dq_ddr_ioregs = {
	.dram_sdclk_0 = IMX6DQ_DRIVE_STRENGTH,
	.dram_sdclk_1 = IMX6DQ_DRIVE_STRENGTH,
	.dram_cas = IMX6DQ_DRIVE_STRENGTH,
	.dram_ras = IMX6DQ_DRIVE_STRENGTH,
	.dram_reset = IMX6DQ_DRIVE_STRENGTH,
	.dram_sdcke0 = IMX6DQ_DRIVE_STRENGTH,
	.dram_sdcke1 = IMX6DQ_DRIVE_STRENGTH,
	.dram_sdba2 = 0x00000000,
	.dram_sdodt0 = IMX6DQ_DRIVE_STRENGTH,
	.dram_sdodt1 = IMX6DQ_DRIVE_STRENGTH,
	.dram_sdqs0 = IMX6DQ_DRIVE_STRENGTH,
	.dram_sdqs1 = IMX6DQ_DRIVE_STRENGTH,
	.dram_sdqs2 = IMX6DQ_DRIVE_STRENGTH,
	.dram_sdqs3 = IMX6DQ_DRIVE_STRENGTH,
	.dram_sdqs4 = IMX6DQ_DRIVE_STRENGTH,
	.dram_sdqs5 = IMX6DQ_DRIVE_STRENGTH,
	.dram_sdqs6 = IMX6DQ_DRIVE_STRENGTH,
	.dram_sdqs7 = IMX6DQ_DRIVE_STRENGTH,
	.dram_dqm0 = IMX6DQ_DRIVE_STRENGTH,
	.dram_dqm1 = IMX6DQ_DRIVE_STRENGTH,
	.dram_dqm2 = IMX6DQ_DRIVE_STRENGTH,
	.dram_dqm3 = IMX6DQ_DRIVE_STRENGTH,
	.dram_dqm4 = IMX6DQ_DRIVE_STRENGTH,
	.dram_dqm5 = IMX6DQ_DRIVE_STRENGTH,
	.dram_dqm6 = IMX6DQ_DRIVE_STRENGTH,
	.dram_dqm7 = IMX6DQ_DRIVE_STRENGTH,
};

/* configure MX6Q/DUAL mmdc GRP io registers */
static struct mx6dq_iomux_grp_regs mx6dq_grp_ioregs = {
	.grp_ddr_type = 0x000c0000,
	.grp_ddrmode_ctl = 0x00020000,
	.grp_ddrpke = 0x00000000,
	.grp_addds = IMX6DQ_DRIVE_STRENGTH,
	.grp_ctlds = IMX6DQ_DRIVE_STRENGTH,
	.grp_ddrmode = 0x00020000,
	.grp_b0ds = IMX6DQ_DRIVE_STRENGTH,
	.grp_b1ds = IMX6DQ_DRIVE_STRENGTH,
	.grp_b2ds = IMX6DQ_DRIVE_STRENGTH,
	.grp_b3ds = IMX6DQ_DRIVE_STRENGTH,
	.grp_b4ds = IMX6DQ_DRIVE_STRENGTH,
	.grp_b5ds = IMX6DQ_DRIVE_STRENGTH,
	.grp_b6ds = IMX6DQ_DRIVE_STRENGTH,
	.grp_b7ds = IMX6DQ_DRIVE_STRENGTH,
};

/* MT41K128M16JT-125 */
static struct mx6_ddr3_cfg mt41k128m16jt_125 = {
	/* quad = 1066 */
	.mem_speed = 1066,
	.density = 2,
	.width = 16,
	.banks = 8,
	.rowaddr = 14,
	.coladdr = 10,
	.pagesz = 2,
	.trcd = 1375,
	.trcmin = 4875,
	.trasmin = 3500,
	.SRT = 0,
};

static struct mx6_mmdc_calibration mx6q_1g_mmdc_calib = {
	.p0_mpwldectrl0 = 0x00350035,
	.p0_mpwldectrl1 = 0x001F001F,
	.p1_mpwldectrl0 = 0x00010001,
	.p1_mpwldectrl1 = 0x00010001,
	.p0_mpdgctrl0 = 0x43510360,
	.p0_mpdgctrl1 = 0x0342033F,
	.p1_mpdgctrl0 = 0x033F033F,
	.p1_mpdgctrl1 = 0x03290266,
	.p0_mprddlctl = 0x4B3E4141,
	.p1_mprddlctl = 0x47413B4A,
	.p0_mpwrdlctl = 0x42404843,
	.p1_mpwrdlctl = 0x4C3F4C45,
};

/* DDR 64bit 1GB */
static struct mx6_ddr_sysinfo mem_qdl = {
	.dsize = 2,
	.cs1_mirror = 0,
	/* config for full 4GB range so that get_mem_size() works */
	.cs_density = 32,
	.ncs = 1,
	.bi_on = 1,
	/* quad = 2, duallite = 1 */
	.rtt_nom = 2,
	/* quad = 2, duallite = 1 */
	.rtt_wr = 2,
	.ralat = 5,
	.walat = 0,
	.mif3_mode = 3,
	.rst_to_cke = 0x23,
	.sde_to_rst = 0x10,
	.refsel = 1,	/* Refresh cycles at 32KHz */
	.refr = 7,	/* 8 refresh commands per refresh cycle */
};

static void ccgr_init(void)
{
	struct mxc_ccm_reg *ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;

	/* set the default clock gate to save power */
	writel(0x00C03F3F, &ccm->CCGR0);
	writel(0x0030FC03, &ccm->CCGR1);
	writel(0x0FFFC000, &ccm->CCGR2);
	writel(0x3FF00000, &ccm->CCGR3);
	writel(0x00FFF300, &ccm->CCGR4);
	writel(0x0F0000C3, &ccm->CCGR5);
	writel(0x000003FF, &ccm->CCGR6);
}

static void spl_dram_init(void)
{
	mt41k128m16jt_125.mem_speed = 1066;
	mem_qdl.rtt_nom = 2;
	mem_qdl.rtt_wr = 2;

	mx6dq_dram_iocfg(64, &mx6dq_ddr_ioregs, &mx6dq_grp_ioregs);
	mx6_dram_cfg(&mem_qdl, &mx6q_1g_mmdc_calib, &mt41k128m16jt_125);

	udelay(100);
}

void board_init_f(ulong dummy)
{
	ccgr_init();

	/* setup AIPS and disable watchdog */
	arch_cpu_init();

	gpr_init();

	/* iomux */
	board_early_init_f();

	/* setup GP timer */
	timer_init();

	/* UART clocks enabled and gd valid - init serial console */
	preloader_console_init();

	/* DDR initialization */
	spl_dram_init();
	/* load/boot image from boot device */
	board_init_r(NULL, 0);
}

void board_boot_order(u32 *spl_boot_list)
{
	debug("returned boot dev  %d\n", spl_boot_device());

	spl_boot_list[0] = BOOT_DEVICE_SPI;
	spl_boot_list[1] = BOOT_DEVICE_MMC1;
}

#endif
