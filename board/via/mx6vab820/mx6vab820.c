// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 VIA Technologies, Inc.
 */

#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/iomux.h>
#include <asm/arch/mx6-pins.h>
#include <asm/mach-imx/spi.h>
#include <linux/errno.h>
#include <asm/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/video.h>
#include <mmc.h>
#include <fsl_esdhc.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/arch/mxc_hdmi.h>
#include <asm/arch/crm_regs.h>
#include <asm/io.h>
#include <asm/arch/sys_proto.h>
#include <i2c.h>
#include <input.h>
#include <usb.h>
#include <usb/ehci-ci.h>
#include <micrel.h>
#include "../common/via.h"	/* common function */

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL  (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm |			\
	PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC_PAD_CTRL (PAD_CTL_PUS_47K_UP |			\
	PAD_CTL_SPEED_LOW | PAD_CTL_DSE_80ohm |			\
	PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define EHCI_PAD_CTRL (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_LOW | PAD_CTL_DSE_40ohm |			\
	PAD_CTL_SRE_SLOW  | PAD_CTL_HYS)

#define EHCI_OTG_ID_PAD_CTRL (PAD_CTL_PUS_100K_DOWN |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_80ohm |			\
	PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define ENET_PAD_CTRL  (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS)

#define SPI_PAD_CTRL (PAD_CTL_HYS | PAD_CTL_SPEED_MED | \
	PAD_CTL_DSE_40ohm | PAD_CTL_SRE_FAST)

#define AUDIO_PAD_CTRL (PAD_CTL_HYS | PAD_CTL_PUS_100K_UP | \
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm)

#define I2C_PAD_CTRL  (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS |	\
	PAD_CTL_ODE | PAD_CTL_SRE_FAST)

#define I2C_PAD MUX_PAD_CTRL(I2C_PAD_CTRL)

int dram_init(void)
{
	gd->ram_size = imx_ddr_size();
	return 0;
}

static iomux_v3_cfg_t const uart_pads[] = {
	IOMUX_PADS(PAD_EIM_D26__UART2_TX_DATA	| MUX_PAD_CTRL(UART_PAD_CTRL)),
	IOMUX_PADS(PAD_EIM_D27__UART2_RX_DATA	| MUX_PAD_CTRL(UART_PAD_CTRL)),
	IOMUX_PADS(PAD_SD3_DAT6__UART1_TX_DATA	| MUX_PAD_CTRL(UART_PAD_CTRL)),
	IOMUX_PADS(PAD_SD3_DAT7__UART1_RX_DATA	| MUX_PAD_CTRL(UART_PAD_CTRL)),
	IOMUX_PADS(PAD_SD3_DAT0__UART1_RTS_B	| MUX_PAD_CTRL(UART_PAD_CTRL)),
	IOMUX_PADS(PAD_EIM_D20__UART1_CTS_B	| MUX_PAD_CTRL(UART_PAD_CTRL)),
	IOMUX_PADS(PAD_EIM_D24__UART1_DTR_B	| MUX_PAD_CTRL(UART_PAD_CTRL)),
	IOMUX_PADS(PAD_EIM_D25__UART1_DSR_B	| MUX_PAD_CTRL(UART_PAD_CTRL)),
};

static iomux_v3_cfg_t const enet_pads[] = {
	IOMUX_PADS(PAD_ENET_MDIO__ENET_MDIO | MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_ENET_MDC__ENET_MDC	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_TXC__RGMII_TXC | MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_TD0__RGMII_TD0 | MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_TD1__RGMII_TD1 | MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_TD2__RGMII_TD2 | MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_TD3__RGMII_TD3 | MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_TX_CTL__RGMII_TX_CTL |
		   MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_ENET_REF_CLK__ENET_TX_CLK | MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_RXC__GPIO6_IO30 | MUX_PAD_CTRL(NO_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_RD0__GPIO6_IO25 | MUX_PAD_CTRL(NO_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_RD1__GPIO6_IO27 | MUX_PAD_CTRL(NO_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_RD2__GPIO6_IO28 | MUX_PAD_CTRL(NO_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_RD3__GPIO6_IO29 | MUX_PAD_CTRL(NO_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_RX_CTL__GPIO6_IO24 | MUX_PAD_CTRL(NO_PAD_CTRL)),
	IOMUX_PADS(PAD_ENET_CRS_DV__GPIO1_IO25 | MUX_PAD_CTRL(NO_PAD_CTRL)),
};

static iomux_v3_cfg_t enet_pads_final[] = {
	IOMUX_PADS(PAD_RGMII_RXC__RGMII_RXC	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_RD0__RGMII_RD0	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_RD1__RGMII_RD1	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_RD2__RGMII_RD2	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_RD3__RGMII_RD3	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_RX_CTL__RGMII_RX_CTL |
		   MUX_PAD_CTRL(ENET_PAD_CTRL)),
};

static void __maybe_unused setup_iomux_enet(void)
{
	/* reset pin = gpio1_io25 = low */
	gpio_direction_output(IMX_GPIO_NR(1, 25), 0);

	gpio_direction_output(IMX_GPIO_NR(6, 30), 1);
	gpio_direction_output(IMX_GPIO_NR(6, 25), 1);
	gpio_direction_output(IMX_GPIO_NR(6, 27), 1);
	gpio_direction_output(IMX_GPIO_NR(6, 28), 1);
	gpio_direction_output(IMX_GPIO_NR(6, 29), 1);

	SETUP_IOMUX_PADS(enet_pads);
	gpio_direction_output(IMX_GPIO_NR(6, 24), 1);

	/* Reset PHY, Need delay 10ms, gpio1_io25 = high */
	mdelay(10);
	gpio_set_value(IMX_GPIO_NR(1, 25), 1);

	SETUP_IOMUX_PADS(enet_pads_final);
}

static iomux_v3_cfg_t const usdhc2_pads[] = {
	IOMUX_PADS(PAD_SD2_CLK__SD2_CLK	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD2_CMD__SD2_CMD	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD2_DAT0__SD2_DATA0	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD2_DAT1__SD2_DATA1	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD2_DAT2__SD2_DATA2	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD2_DAT3__SD2_DATA3	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	/* CD */
	IOMUX_PADS(PAD_NANDF_D2__GPIO2_IO02	| MUX_PAD_CTRL(NO_PAD_CTRL)),
};

static iomux_v3_cfg_t const usdhc4_pads[] = {
	IOMUX_PADS(PAD_SD4_CLK__SD4_CLK   | MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD4_CMD__SD4_CMD   | MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD4_DAT0__SD4_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD4_DAT1__SD4_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD4_DAT2__SD4_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD4_DAT3__SD4_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD4_DAT4__SD4_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD4_DAT5__SD4_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD4_DAT6__SD4_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD4_DAT7__SD4_DATA7 | MUX_PAD_CTRL(USDHC_PAD_CTRL)),
};

static iomux_v3_cfg_t const ecspi1_pads[] = {
	IOMUX_PADS(PAD_EIM_D16__ECSPI1_SCLK	| MUX_PAD_CTRL(SPI_PAD_CTRL)),
	IOMUX_PADS(PAD_EIM_D17__ECSPI1_MISO	| MUX_PAD_CTRL(SPI_PAD_CTRL)),
	IOMUX_PADS(PAD_EIM_D18__ECSPI1_MOSI	| MUX_PAD_CTRL(SPI_PAD_CTRL)),
	IOMUX_PADS(PAD_EIM_D19__GPIO3_IO19	| MUX_PAD_CTRL(NO_PAD_CTRL)),
};

static iomux_v3_cfg_t const ecspi3_pads[] = {
	IOMUX_PADS(PAD_DISP0_DAT0__ECSPI3_SCLK	| MUX_PAD_CTRL(SPI_PAD_CTRL)),
	IOMUX_PADS(PAD_DISP0_DAT2__ECSPI3_MISO	| MUX_PAD_CTRL(SPI_PAD_CTRL)),
	IOMUX_PADS(PAD_DISP0_DAT1__ECSPI3_MOSI	| MUX_PAD_CTRL(SPI_PAD_CTRL)),
	IOMUX_PADS(PAD_DISP0_DAT3__ECSPI3_SS0	| MUX_PAD_CTRL(SPI_PAD_CTRL)),
	IOMUX_PADS(PAD_DISP0_DAT4__ECSPI3_SS1	| MUX_PAD_CTRL(SPI_PAD_CTRL)),
};

static iomux_v3_cfg_t const audio_pads[] = {
	IOMUX_PADS(PAD_DISP0_DAT20__AUD4_TXC	| MUX_PAD_CTRL(AUDIO_PAD_CTRL)),
	IOMUX_PADS(PAD_DISP0_DAT21__AUD4_TXD	| MUX_PAD_CTRL(AUDIO_PAD_CTRL)),
	IOMUX_PADS(PAD_DISP0_DAT22__AUD4_TXFS	| MUX_PAD_CTRL(AUDIO_PAD_CTRL)),
	IOMUX_PADS(PAD_DISP0_DAT23__AUD4_RXD	| MUX_PAD_CTRL(AUDIO_PAD_CTRL)),
	IOMUX_PADS(PAD_GPIO_0__CCM_CLKO1	| MUX_PAD_CTRL(AUDIO_PAD_CTRL)),
};

/* I2C1: adv7180=0x21  */
I2C_PADS(i2c_pad_info0,
	 PAD_CSI0_DAT9__I2C1_SCL	| I2C_PAD,
	 PAD_CSI0_DAT9__GPIO5_IO27	| I2C_PAD,
	 IMX_GPIO_NR(5, 27),
	 PAD_CSI0_DAT8__I2C1_SDA	| I2C_PAD,
	 PAD_CSI0_DAT8__GPIO5_IO26	| I2C_PAD,
	 IMX_GPIO_NR(5, 26));

/* I2C2: HDMI */
I2C_PADS(i2c_pad_info1,
	 PAD_KEY_COL3__I2C2_SCL		| I2C_PAD,
	 PAD_KEY_COL3__GPIO4_IO12	| I2C_PAD,
	 IMX_GPIO_NR(4, 12),
	 PAD_KEY_ROW3__I2C2_SDA		| I2C_PAD,
	 PAD_KEY_ROW3__GPIO4_IO13	| I2C_PAD,
	 IMX_GPIO_NR(4, 13));

/* I2C3: sgtl5000=0x0a */
I2C_PADS(i2c_pad_info2,
	 PAD_GPIO_3__I2C3_SCL		| I2C_PAD,
	 PAD_GPIO_3__GPIO1_IO03		| I2C_PAD,
	 IMX_GPIO_NR(1, 3),
	 PAD_GPIO_6__I2C3_SDA		| I2C_PAD,
	 PAD_GPIO_6__GPIO1_IO06		| I2C_PAD,
	 IMX_GPIO_NR(1, 6));

#if defined(CONFIG_MULTI_DTB_FIT) || defined(CONFIG_SPL_LOAD_FIT)
int board_fit_config_name_match(const char *name)
{
	return 0;
}
#endif

static void __maybe_unused setup_spi(void)
{
	SETUP_IOMUX_PADS(ecspi1_pads);
	SETUP_IOMUX_PADS(ecspi3_pads);
}

iomux_v3_cfg_t const pcie_pads[] = {
	/* RESET */
	IOMUX_PADS(PAD_GPIO_17__GPIO7_IO12 | MUX_PAD_CTRL(NO_PAD_CTRL)),
};

static void __maybe_unused setup_pcie(void)
{
	SETUP_IOMUX_PADS(pcie_pads);
}

static iomux_v3_cfg_t  const __maybe_unused di0_pads[] = {
	IOMUX_PADS(PAD_DI0_DISP_CLK__IPU1_DI0_DISP_CLK),  /* DISP0_CLK */
	IOMUX_PADS(PAD_DI0_PIN2__IPU1_DI0_PIN02),	  /* DISP0_HSYNC */
	IOMUX_PADS(PAD_DI0_PIN3__IPU1_DI0_PIN03),	  /* DISP0_VSYNC */
};

static void setup_iomux_uart(void)
{
	/* set ring pin to low */
	gpio_direction_output(IMX_GPIO_NR(3, 4), 0);
	SETUP_IOMUX_PADS(uart_pads);
}

#ifdef CONFIG_FSL_ESDHC
struct fsl_esdhc_cfg usdhc_cfg[2] = {
	{USDHC2_BASE_ADDR},
	{USDHC4_BASE_ADDR},
};

#define USDHC2_CD_GPIO	IMX_GPIO_NR(2, 2)

int board_mmc_get_env_dev(int devno)
{
	/* dev 0 for SD, dev 1 for eMMC */
	return (devno == 1) ? 0 : 1;
}

int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = 0;

	switch (cfg->esdhc_base) {
	case USDHC2_BASE_ADDR:
		ret = !gpio_get_value(USDHC2_CD_GPIO);
		break;
	case USDHC4_BASE_ADDR:
		ret = 1; /* eMMC/uSDHC4 is always present */
		break;
	}

	return ret;
}

int board_mmc_init(bd_t *bis)
{
	SETUP_IOMUX_PADS(usdhc2_pads);
	SETUP_IOMUX_PADS(usdhc4_pads);
	gpio_direction_input(USDHC2_CD_GPIO);

#ifndef CONFIG_SPL_BUILD
	int ret;
	int i;

	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-Boot device node)    (Physical Port)
	 * mmc0                    SD2
	 * mmc1                    eMMC
	 */
	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
			/* sd2 = 4bit */
			usdhc_cfg[0].max_bus_width = 4;
			break;
		case 1:
			usdhc_cfg[1].sdhc_clk = mxc_get_clock(MXC_ESDHC4_CLK);
			break;
		default:
			printf("Unsupported USDHC controller %d limit is %d!\n",
			       i + 1, CONFIG_SYS_FSL_USDHC_NUM);
			return -EINVAL;
		}

		ret = fsl_esdhc_initialize(bis, &usdhc_cfg[i]);
		if (ret)
			return ret;
	}

	return 0;
#else
	struct src *psrc = (struct src *)SRC_BASE_ADDR;
	unsigned int reg = readl(&psrc->sbmr1) >> 11;
	/*
	 * Upon reading BOOT_CFG register the following map is done:
	 * Bit 11 and 12 of BOOT_CFG register can determine the current
	 * mmc port
	 * 0x1                  SD2
	 * 0x3                  SD4
	 */

	switch (reg & 0x3) {
	case 0x1:
		usdhc_cfg[0].esdhc_base = USDHC2_BASE_ADDR;
		usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
		/* sd2 = 4bit */
		usdhc_cfg[0].max_bus_width = 4;
		gd->arch.sdhc_clk = usdhc_cfg[0].sdhc_clk;
		break;
	default:
		usdhc_cfg[0].esdhc_base = USDHC4_BASE_ADDR;
		usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC4_CLK);
		gd->arch.sdhc_clk = usdhc_cfg[0].sdhc_clk;
		break;
	}

	return fsl_esdhc_initialize(bis, &usdhc_cfg[0]);
#endif /* CONFIG_SPL_BUILD */
}
#endif /* CONFIG_FSL_ESDHC */

int board_phy_config(struct phy_device *phydev)
{
	ksz9031_rgmii_rework(phydev);

	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}

#if defined(CONFIG_VIDEO_IPUV3)
static void disable_lvds(struct display_info_t const *dev)
{
	struct iomuxc *iomux = (struct iomuxc *)IOMUXC_BASE_ADDR;

	int reg = readl(&iomux->gpr[2]);

	reg &= ~(IOMUXC_GPR2_LVDS_CH0_MODE_MASK |
		 IOMUXC_GPR2_LVDS_CH1_MODE_MASK);

	writel(reg, &iomux->gpr[2]);
}

static void do_enable_hdmi(struct display_info_t const *dev)
{
	disable_lvds(dev);
	imx_enable_hdmi_phy();
}

struct display_info_t const displays[] = {{
	.bus	= -1,
	.addr	= 0,
	.pixfmt	= IPU_PIX_FMT_RGB24,
	.detect	= detect_hdmi,
	.enable	= do_enable_hdmi,
	.mode	= {
		.name           = "HDMI",
		.refresh        = 60,
		.xres           = 1024,
		.yres           = 768,
		.pixclock       = 15384,
		.left_margin    = 160,
		.right_margin   = 24,
		.upper_margin   = 29,
		.lower_margin   = 3,
		.hsync_len      = 136,
		.vsync_len      = 6,
		.sync           = FB_SYNC_EXT,
		.vmode          = FB_VMODE_NONINTERLACED
} } };
size_t display_count = ARRAY_SIZE(displays);

static void __maybe_unused setup_display(void)
{
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;
	struct iomuxc *iomux = (struct iomuxc *)IOMUXC_BASE_ADDR;
	int reg;

	/* Setup HSYNC, VSYNC, DISP_CLK for debugging purposes */
	SETUP_IOMUX_PADS(di0_pads);

	enable_ipu_clock();
	imx_setup_hdmi();

	/* Turn on LDB0, LDB1, IPU,IPU DI0 clocks */
	reg = readl(&mxc_ccm->CCGR3);
	reg |=  MXC_CCM_CCGR3_LDB_DI0_MASK | MXC_CCM_CCGR3_LDB_DI1_MASK;
	writel(reg, &mxc_ccm->CCGR3);

	/* set LDB0, LDB1 clk select to 011/011 */
	reg = readl(&mxc_ccm->cs2cdr);
	reg &= ~(MXC_CCM_CS2CDR_LDB_DI0_CLK_SEL_MASK
		 | MXC_CCM_CS2CDR_LDB_DI1_CLK_SEL_MASK);
	reg |= (3 << MXC_CCM_CS2CDR_LDB_DI0_CLK_SEL_OFFSET)
	      | (3 << MXC_CCM_CS2CDR_LDB_DI1_CLK_SEL_OFFSET);
	writel(reg, &mxc_ccm->cs2cdr);

	reg = readl(&mxc_ccm->cscmr2);
	reg |= MXC_CCM_CSCMR2_LDB_DI0_IPU_DIV | MXC_CCM_CSCMR2_LDB_DI1_IPU_DIV;
	writel(reg, &mxc_ccm->cscmr2);

	reg = readl(&mxc_ccm->chsccdr);
	reg |= (CHSCCDR_CLK_SEL_LDB_DI0
		<< MXC_CCM_CHSCCDR_IPU1_DI0_CLK_SEL_OFFSET);
	reg |= (CHSCCDR_CLK_SEL_LDB_DI0
		<< MXC_CCM_CHSCCDR_IPU1_DI1_CLK_SEL_OFFSET);
	writel(reg, &mxc_ccm->chsccdr);

	reg = IOMUXC_GPR2_BGREF_RRMODE_EXTERNAL_RES
	     | IOMUXC_GPR2_DI1_VS_POLARITY_ACTIVE_LOW
	     | IOMUXC_GPR2_DI0_VS_POLARITY_ACTIVE_LOW
	     | IOMUXC_GPR2_BIT_MAPPING_CH1_SPWG
	     | IOMUXC_GPR2_DATA_WIDTH_CH1_18BIT
	     | IOMUXC_GPR2_BIT_MAPPING_CH0_SPWG
	     | IOMUXC_GPR2_DATA_WIDTH_CH0_18BIT
	     | IOMUXC_GPR2_LVDS_CH0_MODE_DISABLED
	     | IOMUXC_GPR2_LVDS_CH1_MODE_ENABLED_DI0;
	writel(reg, &iomux->gpr[2]);

	reg = readl(&iomux->gpr[3]);
	reg = (reg & ~(IOMUXC_GPR3_LVDS1_MUX_CTL_MASK
			| IOMUXC_GPR3_HDMI_MUX_CTL_MASK))
	    | (IOMUXC_GPR3_MUX_SRC_IPU1_DI0
	       << IOMUXC_GPR3_LVDS1_MUX_CTL_OFFSET);
	writel(reg, &iomux->gpr[3]);
}
#endif /* CONFIG_VIDEO_IPUV3 */

/*
 * Do not overwrite the console
 * Use always serial for U-Boot console
 */
int overwrite_console(void)
{
	return 1;
}

int board_eth_init(bd_t *bis)
{
	setup_iomux_enet();
	setup_pcie();

	return cpu_eth_init(bis);
}

#ifdef CONFIG_USB_EHCI_MX6
#define USB_OTHERREGS_OFFSET	0x800
#define UCTRL_PWR_POL		BIT(9)

static iomux_v3_cfg_t const usb_otg_pads[] = {
	IOMUX_PADS(PAD_EIM_D22__GPIO3_IO22 | MUX_PAD_CTRL(EHCI_PAD_CTRL)),
	IOMUX_PADS(PAD_EIM_D21__USB_OTG_OC | MUX_PAD_CTRL(EHCI_PAD_CTRL)),
	IOMUX_PADS(PAD_ENET_RX_ER__USB_OTG_ID | MUX_PAD_CTRL(EHCI_OTG_ID_PAD_CTRL)),
};

static iomux_v3_cfg_t const usb_hc1_pads[] = {
	IOMUX_PADS(PAD_EIM_DA0__GPIO3_IO00	| MUX_PAD_CTRL(EHCI_PAD_CTRL)),
	IOMUX_PADS(PAD_GPIO_18__GPIO7_IO13	| MUX_PAD_CTRL(NO_PAD_CTRL)),
	IOMUX_PADS(PAD_EIM_D30__USB_H1_OC	| MUX_PAD_CTRL(EHCI_PAD_CTRL)),
};

static void setup_usb(void)
{
	SETUP_IOMUX_PADS(usb_otg_pads);

	gpio_direction_output(IMX_GPIO_NR(3, 22), 1);

	/*
	 * set daisy chain for otg_pin_id on 6q.
	 * for 6dl, this bit is reserved
	 */
	imx_iomux_set_gpr_register(1, 13, 1, 0);

	SETUP_IOMUX_PADS(usb_hc1_pads);

	gpio_direction_output(IMX_GPIO_NR(3, 0), 0);
	mdelay(2);
	/* Reset USB hub: USB_HUB_RESET_B=GPIO_18=GPIO7_IO13 */
	gpio_direction_output(IMX_GPIO_NR(7, 13), 0);
	mdelay(2);
	gpio_set_value(IMX_GPIO_NR(7, 13), 1);
}

int board_ehci_hcd_init(int port)
{
	u32 *usbnc_usb_ctrl;

	if (port > 1)
		return -EINVAL;

	usbnc_usb_ctrl = (u32 *)(USB_BASE_ADDR + USB_OTHERREGS_OFFSET +
				 port * 4);

	setbits_le32(usbnc_usb_ctrl, UCTRL_PWR_POL);

	return 0;
}

int board_ehci_power(int port, int on)
{
	switch (port) {
	case 0:
		break;
	case 1:
		break;
	default:
		printf("MXC USB port %d not yet supported\n", port);
		return -EINVAL;
	}

	return 0;
}
#endif

int board_early_init_f(void)
{
	setup_iomux_uart();
	setup_spi();

	return 0;
}

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, I2C_PADS_INFO(i2c_pad_info0));
	setup_i2c(1, CONFIG_SYS_I2C_SPEED, 0x7f, I2C_PADS_INFO(i2c_pad_info1));
	setup_i2c(2, CONFIG_SYS_I2C_SPEED, 0x7f, I2C_PADS_INFO(i2c_pad_info2));

#if defined(CONFIG_VIDEO_IPUV3)
	setup_display();
#endif
	setup_usb();

	SETUP_IOMUX_PADS(audio_pads);

	return 0;
}

#ifdef CONFIG_MXC_SPI
int board_spi_cs_gpio(unsigned bus, unsigned cs)
{
	return (bus == 0 && cs == 0) ? (IMX_GPIO_NR(3, 19)) : -1;
}
#endif

#ifdef CONFIG_CMD_BMODE
static const struct boot_mode board_boot_modes[] = {
	/* 4 bit bus width */
	{"sd2",	 MAKE_CFGVAL(0x40, 0x28, 0x00, 0x00)},
	/* 8 bit bus width */
	{"emmc", MAKE_CFGVAL(0x60, 0x58, 0x00, 0x00)},
	{NULL,	 0},
};
#endif

int board_late_init(void)
{
#ifdef CONFIG_CMD_BMODE
	add_board_boot_modes(board_boot_modes);
#endif

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	env_set("board_name", "VAB820");

	if (is_mx6dqp())
		env_set("board_rev", "MX6QP");
	else if (is_mx6dq())
		env_set("board_rev", "MX6Q");
	else if (is_mx6sdl())
		env_set("board_rev", "MX6DL");
#endif

	return 0;
}

int checkboard(void)
{
	puts("Board: MX6-VAB-820\n");
	return 0;
}

#ifdef CONFIG_SPL_BUILD
#include <asm/arch/mx6-ddr.h>
#include <spl.h>
#include <linux/libfdt.h>
#include "vab820_dcd_table.h"

static void ccgr_init(void)
{
	struct mxc_ccm_reg *ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;

	writel(0x00C03F3F, &ccm->CCGR0);
	writel(0x0030FC03, &ccm->CCGR1);
	writel(0x0FFFC000, &ccm->CCGR2);
	writel(0x3FF00000, &ccm->CCGR3);
	writel(0x00FFF300, &ccm->CCGR4);
	writel(0x0F0000C3, &ccm->CCGR5);
	writel(0x000003FF, &ccm->CCGR6);
}

static void ddr_init(int *table, int size)
{
	int i;

	for (i = 0; i < size / 2 ; i++)
		writel(table[2 * i + 1], table[2 * i]);
}

static void spl_dram_init(void)
{
	switch (via_ddr_size()) {
	case RAMSIZE_1G:
		if (is_mx6dq()) {
			ddr_init(mx6dq_1g_dcd_table,
				 ARRAY_SIZE(mx6dq_1g_dcd_table));
		} else if (is_mx6dqp()) {
			ddr_init(mx6dqp_1g_dcd_table,
				 ARRAY_SIZE(mx6dqp_1g_dcd_table));
		} else if (is_mx6sdl()) {
			ddr_init(mx6dl_1g_dcd_table,
				 ARRAY_SIZE(mx6dl_1g_dcd_table));
		}
		break;
	case RAMSIZE_2G:
		if (is_mx6dq()) {
			ddr_init(mx6dq_2g_dcd_table,
				 ARRAY_SIZE(mx6dq_2g_dcd_table));
		} else if (is_mx6dqp()) {
			ddr_init(mx6dqp_2g_dcd_table,
				 ARRAY_SIZE(mx6dqp_2g_dcd_table));
		} else if (is_mx6sdl()) {
			ddr_init(mx6dl_2g_dcd_table,
				 ARRAY_SIZE(mx6dl_2g_dcd_table));
		}
		break;
	default:
		if (is_mx6dq()) {
			ddr_init(mx6dq_4g_dcd_table,
				 ARRAY_SIZE(mx6dq_4g_dcd_table));
		}
		break;
	}
}

void board_init_f(ulong dummy)
{
	/* DDR initializatoin */
	spl_dram_init();

	/* setup AIPS and disable watchdog */
	arch_cpu_init();

	ccgr_init();
	gpr_init();

	/* iomux and setup of i2c */
	board_early_init_f();

	/* setup GP timer */
	timer_init();

	/* UART clocks enabled and gd valid - init serial console */
	preloader_console_init();
}
#endif /* CONFIG_SPL_BUILD */
