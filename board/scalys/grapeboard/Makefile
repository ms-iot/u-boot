#
# Copyright 2018 Scalys B.V.
# opensource@scalys.com
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y	+= grapeboard.o
obj-y   += eth.o
obj-y   += usb_grapeboard.o
obj-y	+= board_configuration_data.o

obj-$(CONFIG_LAYERSCAPE_NS_ACCESS)	+= ../../freescale/common/ns_access.o

ifdef CONFIG_SECURE_BOOT
obj-$(CONFIG_CMD_ESBC_VALIDATE) += fsl_validate.o cmd_esbc_validate.o
endif
obj-$(CONFIG_CHAIN_OF_TRUST) += fsl_chain_of_trust.o
