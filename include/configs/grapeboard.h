/*
 * Copyright 2018 Scalys B.V.
 * opensource@scalys.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __GRAPEBOARD_H__
#define __GRAPEBOARD_H__

#define CONFIG_FSL_LAYERSCAPE
#define CONFIG_GICV2

#include <asm/arch/config.h>
#include <asm/arch/stream_id_lsch2.h>
#include <asm/arch/soc.h>
#include <../../../include/generated/autoconf.h>

#define CONFIG_SUPPORT_RAW_INITRD

#define CONFIG_BOARD_LATE_INIT				/* scsi/sata init */
#define CONFIG_DISPLAY_BOARDINFO_LATE
#define CONFIG_MISC_INIT_R

#define CONFIG_SYS_TEXT_BASE 				0x40001000 /* QSPI0_AMBA_BASE + CONFIG_UBOOT_TEXT_OFFSET */

#define CONFIG_SYS_CLK_FREQ					125000000	/* 125MHz */

#define CONFIG_SKIP_LOWLEVEL_INIT

#define CONFIG_SYS_INIT_SP_ADDR				(CONFIG_SYS_FSL_OCRAM_BASE + 0xfff0)
#define CONFIG_SYS_LOAD_ADDR				(CONFIG_SYS_DDR_SDRAM_BASE + 0x10000000)

/* DDR */
#define CONFIG_SYS_DDR_SDRAM_BASE			0x80000000
#define CONFIG_SYS_FSL_DDR_SDRAM_BASE_PHY	0
#define CONFIG_SYS_SDRAM_BASE				CONFIG_SYS_DDR_SDRAM_BASE
#define CONFIG_SYS_DDR_BLOCK2_BASE     		0x880000000ULL
#define CONFIG_DIMM_SLOTS_PER_CTLR			1
#define CONFIG_CHIP_SELECTS_PER_CTRL		1
#define CONFIG_NR_DRAM_BANKS				2
#define CONFIG_SYS_SDRAM_SIZE				0x40000000
#define CONFIG_CMD_MEMINFO
#define CONFIG_CMD_MEMTEST
#define CONFIG_SYS_MEMTEST_START			0x80000000
#define CONFIG_SYS_MEMTEST_END				0x9fffffff

/* Generic Timer Definitions */
#define COUNTER_FREQUENCY					25000000	/* 25MHz */

/* CSU */
#define CONFIG_LAYERSCAPE_NS_ACCESS

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN				(0x40000 + 128 * 1024)

/* QSPI */
#ifdef CONFIG_QSPI_BOOT
#define CONFIG_SYS_QE_FW_IN_SPIFLASH
#define CONFIG_SYS_FMAN_FW_ADDR				0x400d0000
#define CONFIG_ENV_SPI_BUS					0
#define CONFIG_ENV_SPI_CS					0
#define CONFIG_ENV_SPI_MAX_HZ				1000000
#define CONFIG_ENV_SPI_MODE					0x03
#define CONFIG_SPI_FLASH_SPANSION
#define CONFIG_FSL_SPI_INTERFACE
#define CONFIG_SF_DATAFLASH
#define CONFIG_FSL_QSPI
#define QSPI0_AMBA_BASE						0x40000000
#define CONFIG_SPI_FLASH_SPANSION
#define CONFIG_SPI_FLASH_SST
#define FSL_QSPI_FLASH_SIZE					SZ_64M
#define FSL_QSPI_FLASH_NUM					1

/* QSPI Environment */
#define CONFIG_ENV_SIZE						0x40000          /* 256KB */

#if CONFIG_RESCUE_UBOOT_CONFIG
/* Rescue flash size is at minimum 1MBytes.
 * I.e. PBL/U-boot/PPA/PFE/BCD must fit within 0x100000. */
#define CONFIG_ENV_IS_NOWHERE
#else
#define CONFIG_ENV_OVERWRITE
/*#define CONFIG_ENV_IS_IN_SPI_FLASH*/
#define CONFIG_ENV_OFFSET					0x200000        /* 2MB */
#define CONFIG_ENV_SECT_SIZE				0x40000
#endif
#endif /* CONFIG_QSPI_BOOT */

/* SPI */
#define CONFIG_FSL_DSPI1
#define CONFIG_DEFAULT_SPI_BUS				0
#define CONFIG_CMD_SPI
#define MMAP_DSPI							DSPI1_BASE_ADDR
#define CONFIG_SYS_DSPI_CTAR0   			1
#define CONFIG_SYS_DSPI_CTAR1				1
#define CONFIG_SF_DEFAULT_SPEED				10000000
#define CONFIG_SF_DEFAULT_MODE				SPI_MODE_0
#define CONFIG_SF_DEFAULT_BUS				0
#define CONFIG_SF_DEFAULT_CS				0

/* I2C */
#define CONFIG_SYS_I2C
#define CONFIG_SYS_I2C_MXC
#define CONFIG_SYS_I2C_MXC_I2C1

/* UART */
#define CONFIG_CONS_INDEX       			1
#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_REG_SIZE     	1
#define CONFIG_SYS_NS16550_CLK          	(get_serial_clock())
#define CONFIG_SYS_BAUDRATE_TABLE			{ 9600, 19200, 38400, 57600, 115200 }

/* Command line configuration */
#undef CONFIG_CMD_IMLS

#define CONFIG_SYS_HZ						1000

#define CONFIG_HWCONFIG
#define HWCONFIG_BUFFER_SIZE				128

#include <config_distro_defaults.h>
#include <config_distro_bootcmd.h>

/* Monitor Command Prompt */
#define CONFIG_SYS_CBSIZE					512	/* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE					(CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_BARGSIZE					CONFIG_SYS_CBSIZE /* Boot args buffer */
#define CONFIG_SYS_LONGHELP
#define CONFIG_AUTO_COMPLETE
#define CONFIG_SYS_MAXARGS					64	/* max command args */

#define CONFIG_PANIC_HANG
#define CONFIG_SYS_BOOTM_LEN   				(64 << 20)      /* Increase max gunzip size */

/* PFE Ethernet */
#ifdef CONFIG_FSL_PFE
#define EMAC1_PHY_ADDR          			0x1
#define EMAC2_PHY_ADDR          			0x2
#define CONFIG_SYS_LS_PFE_FW_ADDR			0x40280000
#endif

/*  MMC  */
#ifdef CONFIG_MMC
#define CONFIG_FSL_ESDHC
#define CONFIG_SYS_FSL_MMC_HAS_CAPBLT_VS33
#endif

/* SATA */
#define CONFIG_LIBATA
#define CONFIG_SCSI
#define CONFIG_SCSI_AHCI
#define CONFIG_SCSI_AHCI_PLAT
#define CONFIG_CMD_SCSI
#define CONFIG_SYS_SATA						AHCI_BASE_ADDR
#define CONFIG_SYS_SCSI_MAX_SCSI_ID			1
#define CONFIG_SYS_SCSI_MAX_LUN				1
#define CONFIG_SYS_SCSI_MAX_DEVICE			(CONFIG_SYS_SCSI_MAX_SCSI_ID * CONFIG_SYS_SCSI_MAX_LUN)

/* PCIE */
#define CONFIG_PCIE1
#define CONFIG_NET_MULTI
#define CONFIG_PCI_SCAN_SHOW
#define CONFIG_CMD_PCI

/* Mtdparts configuration */
#define CONFIG_MTD_DEVICE
#define CONFIG_MTD_PARTITIONS
#define CONFIG_CMD_MTDPARTS
#define CONFIG_USE_SPIFLASH
#define CONFIG_SPI_FLASH_MTD

#define MTDIDS_DEFAULT \
	"nor0=qspi@40000000.0"

#define MTDPARTS_DEFAULT    \
	"mtdparts=qspi@40000000.0:" \
		"2M@0x0(u-boot)," \
		"256k(env)," \
		"256k(ppa)," \
		"256k(pfe)," \
		"-(rootfs)"

/* Default environment variables */
#define COMMON_UBOOT_CONFIG \
	"verify=no\0"				\
	"fdt_high=0xffffffffffffffff\0"		\
	"initrd_high=0xffffffffffffffff\0"	\
	"fdt_addr=0x00f00000\0"			\
	"kernel_addr=0x01000000\0"		\
	"kernelheader_addr=0x800000\0"		\
	"scriptaddr=0x80000000\0"		\
	"scripthdraddr=0x80080000\0"		\
	"fdtheader_addr_r=0x80100000\0"		\
	"kernelheader_addr_r=0x80200000\0"	\
	"kernel_addr_r=0x81000000\0"		\
	"fdt_addr_r=0x90000000\0"		\
	"load_addr=0xa0000000\0"		\
	"kernel_size=0x2800000\0"		\
	"kernelheader_size=0x40000\0"		\
	"console=ttyS0,115200\0"		\
	"ethprime=pfe_eth0\0" \
	"ethaddr=02:00:00:ba:be:01\0" \
	"eth1addr=02:00:00:ba:be:02\0" \
	"update_files_path=.\0" \
	"update_source_file_system=ext4\0" \
	"autoload=no\0" \
	"update_tftp_uboot_qspi_nor=" \
        "dhcp;" \
        "tftp $load_addr $update_files_path/u-boot-with-pbl.bin;" \
        "if test $? = \"0\"; then " \
        	"sf probe 0:0;" \
        	"sf erase u-boot 200000;" \
        	"sf write $load_addr u-boot $filesize;" \
        "fi\0" \
	"update_tftp_ppa_qspi_nor=" \
        "dhcp;" \
        "tftp $load_addr $update_files_path/ppa.itb;" \
        "if test $? = \"0\"; then " \
        	"sf probe 0:0;" \
		    "sf erase ppa 40000;" \
            "sf write $load_addr ppa $filesize;" \
        "fi\0" \
	"update_tftp_pfe_qspi_nor=" \
        "dhcp;" \
        "tftp $load_addr $update_files_path/pfe_fw_sbl.itb;" \
        "if test $? = \"0\"; then " \
        	"sf probe 0:0;" \
		    "sf erase pfe 40000;" \
            "sf write $load_addr pfe $filesize;" \
        "fi\0" \
	"update_usb_uboot_qspi_nor=" \
        "usb start;" \
        "fatload usb 0:1 $load_addr $update_files_path/u-boot-with-pbl.bin;" \
        "if test $? = \"0\"; then " \
        	"sf probe 0:0;" \
        	"sf erase u-boot 200000;" \
        	"sf write $load_addr u-boot $filesize;" \
        "fi\0" \
	"update_usb_ppa_qspi_nor=" \
        "usb start;" \
        "fatload usb 0:1 $load_addr $update_files_path/ppa.itb;" \
        "if test $? = \"0\"; then " \
        	"sf probe 0:0;" \
		    "sf erase ppa 40000;" \
            "sf write $load_addr ppa $filesize;" \
        "fi\0" \
	"update_usb_pfe_qspi_nor=" \
        "usb start;" \
        "fatload usb 0:1 $load_addr $update_files_path/pfe_fw_sbl.itb;" \
        "if test $? = \"0\"; then " \
        	"sf probe 0:0;" \
		    "sf erase pfe 40000;" \
            "sf write $load_addr pfe $filesize;" \
        "fi\0" \
	"update_mmc_uboot_qspi_nor=" \
        "mmc rescan;" \
        "ext4load mmc 0:1 $load_addr $update_files_path/u-boot-with-pbl.bin;" \
        "if test $? = \"0\"; then " \
        	"sf probe 0:0;" \
        	"sf erase u-boot 200000;" \
        	"sf write $load_addr u-boot $filesize;" \
        "fi\0" \
	"update_mmc_ppa_qspi_nor=" \
        "mmc rescan;" \
        "ext4load mmc 0:1 $load_addr $update_files_path/ppa.itb;" \
        "if test $? = \"0\"; then " \
        	"sf probe 0:0;" \
		    "sf erase ppa 40000;" \
            "sf write $load_addr ppa $filesize;" \
        "fi\0" \
	"update_mmc_pfe_qspi_nor=" \
        "mmc rescan;" \
        "ext4load mmc 0:1 $load_addr $update_files_path/pfe_fw_sbl.itb;" \
        "if test $? = \"0\"; then " \
        	"sf probe 0:0;" \
		    "sf erase pfe 40000;" \
            "sf write $load_addr pfe $filesize;" \
        "fi\0" \

/* Default flash specific environment variables */
#if CONFIG_RESCUE_UBOOT_CONFIG
#define CONFIG_EXTRA_ENV_SETTINGS		\
		COMMON_UBOOT_CONFIG
#undef CONFIG_BOOTCOMMAND
#if defined(CONFIG_QSPI_BOOT) || defined(CONFIG_SD_BOOT_QSPI)
/* recover from sd card */
#define CONFIG_BOOTCOMMAND "run update_mmc_uboot_pbl_qspi_nor; run update_mmc_pfe_qspi_nor; run update_mmc_ppa_qspi_nor"
#endif

#else /* if CONFIG_STANDARD_UBOOT_CONFIG */
#define CONFIG_EXTRA_ENV_SETTINGS		\
	COMMON_UBOOT_CONFIG \
	"mmcboot=" \
		"ext4load mmc 0:1 $fdt_addr_r /boot/grapeboard.dtb;" \
		"ext4load mmc 0:1 $kernel_addr_r /boot/uImage;" \
		"if test $? = \"0\"; then " \
			"pfe stop;" \
			"bootm $kernel_addr_r - $fdt_addr_r;" \
		"fi\0" \
	"scsiboot=" \
		"ext4load scsi 0:1 $fdt_addr_r /boot/grapeboard.dtb;" \
		"ext4load scsi 0:1 $kernel_addr_r /boot/uImage;" \
		"if test $? = \"0\"; then " \
			"pfe stop;" \
			"bootm $kernel_addr_r - $fdt_addr_r;" \
		"fi\0" \
	"netboot=" \
		"dhcp;" \
		"tftp $fdt_addr_r $tftp_path/grapeboard.dtb;" \
		"tftp $kernel_addr_r $tftp_path/uImage;" \
		"if test $? = \"0\"; then " \
			"pfe stop;" \
			"bootm $kernel_addr_r - $fdt_addr_r;" \
		"fi\0"

#undef CONFIG_BOOTCOMMAND
#if defined(CONFIG_QSPI_BOOT) || defined(CONFIG_SD_BOOT_QSPI)
#define CONFIG_BOOTCOMMAND "run mmcboot"
#endif
#endif

#include <asm/fsl_secure_boot.h>

#endif /* __GRAPEBOARD_H__ */
