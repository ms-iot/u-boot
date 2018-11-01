/*
 * Copyright 2012-2014 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include "imagetool.h"
#include <image.h>
#include "pblimage.h"
#include "pbl_crc32.h"

#define roundup(x, y)		((((x) + ((y) - 1)) / (y)) * (y))
#define PBL_ACS_CONT_CMD	0x81000000
#define PBL_ADDR_24BIT_MASK	0x00ffffff

/*
 * need to store all bytes in memory for calculating crc32, then write the
 * bytes to image file for PBL boot.
 */
static unsigned char mem_buf[1000000];
static unsigned char *pmem_buf = mem_buf;
static int pbl_size;
static char *fname = "Unknown";
static int lineno = -1;
static struct pbl_header pblimage_header;
static int uboot_size;
static int arch_flag;

static uint32_t pbi_crc_cmd1;
static uint32_t pbi_crc_cmd2;
static uint32_t pbl_end_cmd[4];

static union
{
	char c[4];
	unsigned char l;
} endian_test = { {'l', '?', '?', 'b'} };

#define ENDIANNESS ((char)endian_test.l)

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

/* swap words within dwords, and bytes within words.
   This equates to reversing all bytes per 8-byte chunk */
static void all_the_swaps(uint8_t *buf, size_t len)
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
	*pmem_buf++ = 0;
	*pmem_buf++ = (addr >> 24) & 0xff;
	*pmem_buf++ = (addr >> 32) & 0xff;
	*pmem_buf++ = (addr >> 40) & 0xff;

	*pmem_buf++ = 0x58;
	*pmem_buf++ = 0x01;
	*pmem_buf++ = 0x57;
	*pmem_buf++ = 0x09;

	pbl_size += 8;
}

/* load split u-boot with PBI command 81xxxxxx. */
static void load_uboot(FILE *fp_uboot, uint64_t load_addr)
{
	int i;
	uint64_t offset;
	uint64_t prev_offset_high;
	uint32_t offset_low;
	uint8_t *binary;
	size_t padded_len;

	// make sure we're starting on an 8-byte boundary
	if ((pbl_size % 8) != 0) {
		printf("Error: invalid alignment in PBL (pbl_size=0x%x)\n",
			pbl_size);
		exit(EXIT_FAILURE);
	}

	// pad input binary to 128 bytes to make life easy
	padded_len = roundup(uboot_size, 128);
	binary = (uint8_t *)malloc(padded_len);
	if (fread(binary, 1, uboot_size, fp_uboot) != uboot_size) {
		printf("Error: failed to read uboot from file\n");
		exit(EXIT_FAILURE);
	}
	memset(binary + uboot_size, 0xff, padded_len - uboot_size);

	prev_offset_high = 0xffULL << 40;

	// process file in 128-byte chunks
	offset = load_addr;
	for (i = 0; i < (padded_len / 128); i++) {
		uint8_t buf[128 + 4 + 4];

		offset_low = (uint32_t)(offset & 0x00ffffff);
		if ((offset & 0x0000ffffff000000ULL) != prev_offset_high) {
			write_altcbar(offset);
			prev_offset_high = offset & 0x0000ffffff000000ULL;
		}

		// copy data to intermediate buffer
		memcpy(&buf[4], &binary[i * 128], 64);
		memcpy(&buf[72], &binary[i * 128 + 64], 64);

		// write commands into buffer
		buf[3] = offset_low & 0xff;
		buf[2] = (offset_low >> 8) & 0xff;
		buf[1] = (offset_low >> 16) & 0xff;
		buf[0] = 0x81;	// ALT, SIZE=64, CONT

		buf[68 + 3] = (offset_low + 64) & 0xff;
		buf[68 + 2] = ((offset_low + 64) >> 8) & 0xff;
		buf[68 + 1] = ((offset_low + 64) >> 16) & 0xff;
		buf[68 + 0] = 0x81;	// ALT, SIZE=64, CONT

		all_the_swaps(buf, sizeof(buf));

		// append to pmem_buf
		memcpy(pmem_buf, buf, sizeof(buf));
		pmem_buf += sizeof(buf);
		pbl_size += sizeof(buf);

		offset += 128;
	}

	free(binary);
}

static void check_get_hexval(char *token)
{
	uint32_t hexval;
	int i;

	if (!sscanf(token, "%x", &hexval)) {
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
	if (fd == NULL) {
		printf("Error:%s - Can't open\n", fname);
		exit(EXIT_FAILURE);
	}

	while ((getline(&line, &len, fd)) > 0) {
		lineno++;
		token = strtok_r(line, "\r\n", &saveptr1);
		/* drop all lines with zero tokens (= empty lines) */
		if (token == NULL)
			continue;
		for (line = token;; line = NULL) {
			token = strtok_r(line, " \t", &saveptr2);
			if (token == NULL)
				break;
			/* Drop all text starting with '#' as comments */
			if (token[0] == '#')
				break;
			check_get_hexval(token);
		}
	}
	if (line)
		free(line);
	fclose(fd);
}

static uint32_t reverse_byte(uint32_t val)
{
	uint32_t temp;
	unsigned char *p1;
	int j;

	temp = val;
	p1 = (unsigned char *)&temp;
	for (j = 3; j >= 0; j--)
		*p1++ = (val >> (j * 8)) & 0xff;
	return temp;
}

/* write end command and crc command to memory. */
static void add_end_cmd(uint32_t load_addr)
{
	uint32_t crc32_pbl;
#if 0
	int i;
	unsigned char *p = (unsigned char *)&pbl_end_cmd;
	if (ENDIANNESS == 'l') {
		for (i = 0; i < 4; i++)
			pbl_end_cmd[i] = reverse_byte(pbl_end_cmd[i]);
	}

	for (i = 0; i < 16; i++) {
		*pmem_buf++ = *p++;
		pbl_size++;
	}
#endif

	/* write load address SCRATCHRW1 */
	*pmem_buf++ = load_addr & 0xff;
	*pmem_buf++ = (load_addr >> 8) & 0xff;
	*pmem_buf++ = (load_addr >> 16) & 0xff;
	*pmem_buf++ = (load_addr >> 24) & 0xff;
	*pmem_buf++ = 0x04;
	*pmem_buf++ = 0x06;
	*pmem_buf++ = 0x57;
	*pmem_buf++ = 0x09;
	pbl_size += 8;

	/* Add mystery end command. Fails if this does not go last */
	*pmem_buf++ = 0x0c;
	*pmem_buf++ = 0x40;
	*pmem_buf++ = 0x0f;
	*pmem_buf++ = 0x00;
	*pmem_buf++ = 0x00;
	*pmem_buf++ = 0x00;
	*pmem_buf++ = 0x55;
	*pmem_buf++ = 0x09;
	pbl_size += 8;

	/* Add PBI CRC command. */
	*pmem_buf++ = 0x08;
	*pmem_buf++ = pbi_crc_cmd1;
	*pmem_buf++ = pbi_crc_cmd2;
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

void pbl_load_uboot(int ifd, struct image_tool_params *params)
{
	FILE *fp_uboot;
	int size;

	/* parse the rcw.cfg file. */
	pbl_parser(params->imagename);

	/* parse the pbi.cfg file. */
	if (params->imagename2[0] != '\0')
		pbl_parser(params->imagename2);

	if (params->datafile) {
		fp_uboot = fopen(params->datafile, "r");
		if (fp_uboot == NULL) {
			printf("Error: %s open failed\n", params->datafile);
			exit(EXIT_FAILURE);
		}

		load_uboot(fp_uboot, params->addr);
		fclose(fp_uboot);
	}
	add_end_cmd(params->addr);
	lseek(ifd, 0, SEEK_SET);

	size = pbl_size;
	if (write(ifd, (const void *)&mem_buf, size) != size) {
		fprintf(stderr, "Write error on %s: %s\n",
			params->imagefile, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

static int pblimage_check_image_types(uint8_t type)
{
	if (type == IH_TYPE_PBLIMAGE)
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;
}

static int pblimage_verify_header(unsigned char *ptr, int image_size,
			struct image_tool_params *params)
{
	struct pbl_header *pbl_hdr = (struct pbl_header *) ptr;

	/* Only a few checks can be done: search for magic numbers */
	if (ENDIANNESS == 'l') {
		if (pbl_hdr->preamble != reverse_byte(RCW_PREAMBLE))
			return -FDT_ERR_BADSTRUCTURE;

		if (pbl_hdr->rcwheader != reverse_byte(RCW_HEADER))
			return -FDT_ERR_BADSTRUCTURE;
	} else {
		if (pbl_hdr->preamble != RCW_PREAMBLE)
			return -FDT_ERR_BADSTRUCTURE;

		if (pbl_hdr->rcwheader != RCW_HEADER)
			return -FDT_ERR_BADSTRUCTURE;
	}
	return 0;
}

static void pblimage_print_header(const void *ptr)
{
	printf("Image Type:   Freescale PBL Boot Image\n");
}

static void pblimage_set_header(void *ptr, struct stat *sbuf, int ifd,
				struct image_tool_params *params)
{
	/*nothing need to do, pbl_load_uboot takes care of whole file. */
}

int pblimage_check_params(struct image_tool_params *params)
{
	FILE *fp_uboot;
	int fd;
	struct stat st;

	if (!params)
		return EXIT_FAILURE;

	if (params->datafile) {
		fp_uboot = fopen(params->datafile, "r");
		if (fp_uboot == NULL) {
			printf("Error: %s open failed\n", params->datafile);
			exit(EXIT_FAILURE);
		}
		fd = fileno(fp_uboot);

		if (fstat(fd, &st) == -1) {
			printf("Error: Could not determine u-boot image size. %s\n",
			       strerror(errno));
			exit(EXIT_FAILURE);
		}

		uboot_size = st.st_size;
		fclose(fp_uboot);
	}

	if (params->arch == IH_ARCH_ARM) {
		arch_flag = IH_ARCH_ARM;
		pbi_crc_cmd1 = 0x61;
		pbi_crc_cmd2 = 0;
		pbl_end_cmd[0] = 0x09610000;
		pbl_end_cmd[1] = 0x00000000;
		pbl_end_cmd[2] = 0x096100c0;
		pbl_end_cmd[3] = 0x00000000;
	} else if (params->arch == IH_ARCH_PPC) {
		arch_flag = IH_ARCH_PPC;
		pbi_crc_cmd1 = 0x13;
		pbi_crc_cmd2 = 0x80;
		pbl_end_cmd[0] = 0x091380c0;
		pbl_end_cmd[1] = 0x00000000;
		pbl_end_cmd[2] = 0x091380c0;
		pbl_end_cmd[3] = 0x00000000;
	}

	return 0;
};

/* pblimage parameters */
U_BOOT_IMAGE_TYPE(
	pblimage,
	"Freescale PBL Boot Image support",
	sizeof(struct pbl_header),
	(void *)&pblimage_header,
	pblimage_check_params,
	pblimage_verify_header,
	pblimage_print_header,
	pblimage_set_header,
	NULL,
	pblimage_check_image_types,
	NULL,
	NULL
);
