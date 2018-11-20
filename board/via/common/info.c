// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 VIA Technologies, Inc.
 */

#include <asm/io.h>
#include <common.h>
#include <asm/gpio.h>
#include "via.h"

unsigned int via_ddr_size(void)
{
	unsigned int nRamSize;

	if (!gpio_get_value(RAMSIZE_IO1) &&
	    gpio_get_value(RAMSIZE_IO0)) {  /* 4GB: 01 */
		/* 3.75GB */
		nRamSize = RAMSIZE_4G;
	} else if (gpio_get_value(RAMSIZE_IO1) &&
		   !gpio_get_value(RAMSIZE_IO0)) {  /* 2GB: 10 */
		nRamSize = RAMSIZE_2G;
	} else {  /* 1GB: 11 */
		nRamSize = RAMSIZE_1G;
	}

	return nRamSize;
}
