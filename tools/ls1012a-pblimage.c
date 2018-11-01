/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 *
 * SPDX-License-Identifier:	BSD-3-Clause
 */
#include "imagetool.h"
#include <image.h>
#include "pblimage.h"
#include "pbl_crc32.h"

/*
 * need to store all bytes in memory for calculating crc32, then write the
 * bytes to image file for PBL boot.
 */
static unsigned char mem_buf[1000000];
static unsigned char *pmem_buf = mem_buf;
static int pbl_size;
static char *fname = "Unknown";
static int lineno = -1;

static size_t round_up(size_t x, size_t align)
{
	return ((x + (align - 1)) / align) * align;
}

/*
 * The PBL can load up to 64 bytes at a time, so we split the U-Boot
 * image into 64 byte chunks. PBL needs a command for each piece, of
 * the form "81xxxxxx", where "xxxxxx" is the offset. Calculate the
 * start offset by subtracting the size of the u-boot image from the
 * top of the allowable 24-bit range.
 */

static void swap_byte(uint8_t *a, uint8_t *b)
{
	uint8_t t = *a;
	*a = *b;
	*b = t;
}

/*
 * Swap words within dwords, and bytes within words.
 * This equates to reversing all bytes per 8-byte chunk
 */
static void swap_bytes(uint8_t *buf, size_t len)
{
	int i;

	if ((len % 8) != 0) {
		printf("Error: buffer must be multiple of 8-bytes long\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < len; i += 8) {
		swap_byte(&buf[i], &buf[i + 7]);
		swap_byte(&buf[i + 1], &buf[i + 6]);
		swap_byte(&buf[i + 2], &buf[i + 5]);
		swap_byte(&buf[i + 3], &buf[i + 4]);
	}
}

static void write_altcbar(uint64_t addr)
{
	*pmem_buf++ = 0x09;
	*pmem_buf++ = 0x57;
	*pmem_buf++ = 0x01;
	*pmem_buf++ = 0x58;

	*pmem_buf++ = (addr >> 40) & 0xff;
	*pmem_buf++ = (addr >> 32) & 0xff;
	*pmem_buf++ = (addr >> 24) & 0xff;
	*pmem_buf++ = 0;

	pbl_size += 8;
}

/* load split u-boot with PBI command 81xxxxxx. */
static void load_uboot(FILE *fp_uboot, uint64_t load_addr)
{
	int i;
	uint64_t dest_addr;
	uint64_t prev_dest_addr_high;
	uint32_t dest_addr_low;
	uint8_t *binary;
	size_t padded_len;
	size_t uboot_size;
	int fd;
	struct stat st;

	/* make sure PBL is starting on an 8-byte boundary */
	if ((pbl_size % 8) != 0) {
		printf("Error: invalid alignment in PBL (pbl_size=0x%x)\n",
		       pbl_size);
		exit(EXIT_FAILURE);
	}

	/* determine u-boot size */
	fd = fileno(fp_uboot);
	if (fstat(fd, &st) == -1) {
		printf("Error: Could not determine u-boot image size. %s\n",
		       strerror(errno));
		exit(EXIT_FAILURE);
	}

	uboot_size = st.st_size;

	/* pad input binary to 128 bytes to satisfy alignment requirements */
	padded_len = round_up(uboot_size, 128);
	binary = (uint8_t *)malloc(padded_len);
	if (fread(binary, 1, uboot_size, fp_uboot) != uboot_size) {
		printf("Error: failed to read uboot from file\n");
		exit(EXIT_FAILURE);
	}
	memset(binary + uboot_size, 0xff, padded_len - uboot_size);

	prev_dest_addr_high = 0xffULL << 40;

	/* process file in 64-byte chunks */
	dest_addr = load_addr;
	for (i = 0; i < (padded_len / 64); i++) {
		dest_addr_low = (uint32_t)(dest_addr & 0x00ffffff);
		if ((dest_addr & 0x0000ffffff000000ULL) !=
		    prev_dest_addr_high) {

			write_altcbar(dest_addr);
			prev_dest_addr_high = dest_addr & 0x0000ffffff000000ULL;
		}

		/* write command into buffer */
		*pmem_buf++ = 0x81;	// ALT, SIZE=64, CONT
		*pmem_buf++ = (dest_addr_low >> 16) & 0xff;
		*pmem_buf++ = (dest_addr_low >> 8) & 0xff;
		*pmem_buf++ = dest_addr_low & 0xff;
		pbl_size += 4;

		/* copy data to buffer */
		memcpy(pmem_buf, &binary[i * 64], 64);
		pmem_buf += 64;
		pbl_size += 64;
		dest_addr += 64;
	}

	free(binary);
}

static void check_get_hexval(char *token)
{
	uint32_t hexval;
	char *end;
	int i;

	hexval = strtoul(token, &end, 16);
	if (*end != '\0') {
		printf("Error:%s[%d] - Invalid hex data(%s)\n", fname,
		       lineno, token);
		exit(EXIT_FAILURE);
	}

	for (i = 3; i >= 0; i--) {
		*pmem_buf++ = (hexval >> (i * 8)) & 0xff;
		pbl_size++;
	}
}

static void pbl_parser(char *name)
{
	FILE *fd = NULL;
	char *line = NULL;
	char *token, *saveptr1, *saveptr2;
	size_t len = 0;

	fname = name;
	fd = fopen(name, "r");
	if (!fd) {
		printf("Error:%s - Can't open\n", fname);
		exit(EXIT_FAILURE);
	}

	while ((getline(&line, &len, fd)) > 0) {
		lineno++;
		token = strtok_r(line, "\r\n", &saveptr1);
		/* drop all lines with zero tokens (= empty lines) */
		if (!token)
			continue;
		for (line = token;; line = NULL) {
			token = strtok_r(line, " \t", &saveptr2);
			if (!token)
				break;
			/* Drop all text starting with '#' as comments */
			if (token[0] == '#')
				break;

			/* Drop all text starting with '.' as directives */
			if (token[0] == '.')
				break;

			check_get_hexval(token);
		}
	}
	if (line)
		free(line);
	fclose(fd);
}

static void parse_rcw(char *name)
{
	pbl_parser(name);

	if (pbl_size != (8 + 64)) {
		printf("Error:%s RCW must be exactly 64 bytes\n", fname);
		exit(EXIT_FAILURE);
	}
}

static void add_preamble(void)
{
	*pmem_buf++ = 0xaa;
	*pmem_buf++ = 0x55;
	*pmem_buf++ = 0xaa;
	*pmem_buf++ = 0x55;

	*pmem_buf++ = 0x01;
	*pmem_buf++ = 0xee;
	*pmem_buf++ = 0x01;
	*pmem_buf++ = 0x00;

	pbl_size += 8;
}

/* write end command and crc command to memory. */
static void add_end_cmd(uint32_t load_addr)
{
	uint32_t crc32_pbl;

	/* write load address SCRATCHRW1 */
	*pmem_buf++ = 0x09;
	*pmem_buf++ = 0x57;
	*pmem_buf++ = 0x06;
	*pmem_buf++ = 0x04;
	*pmem_buf++ = (load_addr >> 24) & 0xff;
	*pmem_buf++ = (load_addr >> 16) & 0xff;
	*pmem_buf++ = (load_addr >> 8) & 0xff;
	*pmem_buf++ = load_addr & 0xff;
	pbl_size += 8;

	/* Add mystery end command. Fails if this does not go last */
	*pmem_buf++ = 0x09;
	*pmem_buf++ = 0x55;
	*pmem_buf++ = 0x00;
	*pmem_buf++ = 0x00;
	*pmem_buf++ = 0x00;
	*pmem_buf++ = 0x0f;
	*pmem_buf++ = 0x40;
	*pmem_buf++ = 0x0c;
	pbl_size += 8;

	/* Add PBI CRC command. */
	*pmem_buf++ = 0x08;
	*pmem_buf++ = 0x61;
	*pmem_buf++ = 0;
	*pmem_buf++ = 0x40;
	pbl_size += 4;

	/* calculated CRC32 and write it to memory. */
	crc32_pbl = pbl_crc32(0, (const char *)mem_buf, pbl_size);
	*pmem_buf++ = (crc32_pbl >> 24) & 0xff;
	*pmem_buf++ = (crc32_pbl >> 16) & 0xff;
	*pmem_buf++ = (crc32_pbl >> 8) & 0xff;
	*pmem_buf++ = (crc32_pbl) & 0xff;
	pbl_size += 4;
}

void ls1012a_pbl_load_uboot(int ifd, struct image_tool_params *params)
{
	FILE *fp_uboot;
	int size;

	add_preamble();

	/* parse the rcw.cfg file */
	parse_rcw(params->imagename);

	/* parse the pbi.cfg file. */
	if (params->imagename2[0] != '\0')
		pbl_parser(params->imagename2);

	if (params->datafile) {
		fp_uboot = fopen(params->datafile, "r");
		if (!fp_uboot) {
			printf("Error: %s open failed\n", params->datafile);
			exit(EXIT_FAILURE);
		}

		load_uboot(fp_uboot, params->addr);
		fclose(fp_uboot);
	}
	add_end_cmd(params->addr);
	lseek(ifd, 0, SEEK_SET);

	/* swap all but the CRC command */
	swap_bytes(mem_buf, pbl_size - 8);

	size = pbl_size;
	if (write(ifd, (const void *)&mem_buf, size) != size) {
		fprintf(stderr, "Write error on %s: %s\n",
			params->imagefile, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

int ls1012a_pblimage_check_params(struct image_tool_params *params)
{
	/* destination address must be 64-byte aligned */
	if (params->addr & 0x3f) {
		printf("Error:0x%x load address must be 64-byte aligned\n",
		       params->addr);
		return EXIT_FAILURE;
	}
	return 0;
}
