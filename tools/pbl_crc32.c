/*
 * Copyright 2012 Freescale Semiconductor, Inc.
 *
 * Cleaned up and refactored by Charles Manning.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include "pblimage.h"

static uint32_t crc_table[256];
static int crc_table_valid;

static void make_crc_table(void)
{
	uint32_t mask;
	int i, j;
	uint32_t poly; /* polynomial exclusive-or pattern */

	if (crc_table_valid)
		return;

	/*
	 * the polynomial used by PBL is 1 + x1 + x2 + x4 + x5 + x7 + x8 + x10
	 * + x11 + x12 + x16 + x22 + x23 + x26 + x32.
	 */
	poly = 0x04c11db7;

	for (i = 0; i < 256; i++) {
		mask = i << 24;
		for (j = 0; j < 8; j++) {
			if (mask & 0x80000000)
				mask = (mask << 1) ^ poly;
			else
				mask <<= 1;
		}
		crc_table[i] = mask;
	}

	crc_table_valid = 1;
}

uint32_t pbl_crc32(uint32_t in_crc, const char *buf, uint32_t len)
{
	uint32_t crc32_val;
	int i, j;
	uint8_t temp[8];

	make_crc_table();

	crc32_val = ~in_crc;

	for (j = 0; j < (len / 8); j++) {
		// byte-reverse 8-byte chunk of input into temporary buffer
		for (i = 0; i < 8; i++) {
			temp[i] = buf[(j * 8) + (7 - i)];
		}
		for (i = 0; i < 8; i++) {
			//printf("%02x\n", (uint32_t)(temp[i] & 0xff));
			crc32_val = (crc32_val << 8) ^
				crc_table[(crc32_val >> 24) ^ (temp[i] & 0xff)];
		}
	}

	// remaining bytes don't get swapped for whatever reason
	for (i = 0; i < (len % 8); i++) {
		uint8_t ch = buf[((len / 8 ) * 8) + i];
		//printf("%02x\n", (uint32_t)(ch & 0xff));
		crc32_val = (crc32_val << 8) ^
			crc_table[(crc32_val >> 24) ^ (ch & 0xff)];
	}

	return crc32_val;
}
