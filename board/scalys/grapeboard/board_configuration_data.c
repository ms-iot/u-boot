/*
 * Copyright 2018 Scalys B.V.
 * opensource@scalys.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <linux/ctype.h>
#include <libfdt.h>
#include <fdt_support.h>

#include <spi.h>
#include <spi_flash.h>
#include <linux/errno.h>
#include <malloc.h>
#include <asm/io.h>
#include <dm.h>
#include <malloc.h>
#include <mapmem.h>
#include <spi.h>
#include <dm/device-internal.h>

#include <../../../include/generated/autoconf.h>
#include "board_configuration_data.h"
#include "gpio_grapeboard.h"

DECLARE_GLOBAL_DATA_PTR;

int add_mac_addressess_to_env(const void* blob)
{
        const char *propname;
	const void *value;
	int prop_offset, len;
	int count = 0;
	char mac_string[19], eth_string[10];
	uint8_t mac_address[6];

	if (fdt_check_header(blob) != 0) {
		printf( "Board Configuration Data FDT corrupt\n");
		return -1;
	}
	
	int nodeoff = fdt_path_offset(blob, "/network");

	if (nodeoff < 0) {
		printf("Network node not found\n");
		return -1;
	}
        for (prop_offset = fdt_first_property_offset(blob, nodeoff);
             prop_offset > 0;
             prop_offset = fdt_next_property_offset(blob, prop_offset)) {
                value = fdt_getprop_by_offset(blob, prop_offset,
                                              &propname, &len);
                if (!value) {
                        return -EINVAL;
		}
			
		memcpy(mac_address, value, 6);
		
		if (count) {
			snprintf(eth_string, sizeof(eth_string), "eth%iaddr", count);
		}
		else {
			snprintf(eth_string, sizeof(eth_string), "ethaddr");
		}
		
		snprintf(mac_string, sizeof(mac_string), 
			"%02x:%02x:%02x:%02x:%02x:%02x",
			mac_address[0], mac_address[1], mac_address[2], 
			mac_address[3], mac_address[4], mac_address[5]
		);
		
		printf("%s : [ %s ]\n", propname, mac_string );

		env_set( eth_string, mac_string);

		count++;
		
	}
	printf("Done reading BCD\n");
	
	return 0;
}

struct udevice* sel_rescue_qspi_flash(bool sel_rescue) {
	struct ccsr_gpio *pgpio = (void *)(CONFIG_SYS_GPIO2);
	struct udevice *rescue_flash_dev,*bus_dev;
	int ret = 0;
	unsigned int bus = 0;
	unsigned int cs = 0;
	unsigned int speed = 0;
	unsigned int mode = 0;

	/* Remove previous DM device */
	ret = spi_find_bus_and_cs(bus, cs, &bus_dev, &rescue_flash_dev);
	if (!ret) {
		device_remove(rescue_flash_dev, DM_REMOVE_NORMAL);
	}

	setbits_be32(&pgpio->gpdir, QSPI_MUX_N_MASK);
	if (sel_rescue == true) {
		/* Change chip select to rescue QSPI NOR flash */
		setbits_be32(&pgpio->gpdat, QSPI_MUX_N_MASK);
	} else {
		/* Revert chip select muxing to standard QSPI flash */
		clrbits_be32(&pgpio->gpdat, QSPI_MUX_N_MASK);
		/* Delay required (to meet RC time for button debouncing) before probing flash again.
		 * May be removed but the primary flash is only available after delay */
		udelay(75000);
	}

	/* Probe new flash */
	ret = spi_flash_probe_bus_cs(bus, cs, speed, mode, &rescue_flash_dev);
	if (ret != 0) {
		printf("probe failed\n");
		return NULL;
	}

	return rescue_flash_dev;

}



const void* get_boardinfo_rescue_flash(void)
{
	struct udevice *rescue_flash_dev;
	uint32_t bcd_data_length;
	uint8_t *bcd_data = NULL;
	uint32_t calculated_crc, received_crc;
	int dtb_length;
	int ret = 0;

	/* Select and probe rescue flash */
	rescue_flash_dev = sel_rescue_qspi_flash(true);

	if (rescue_flash_dev == NULL)
		goto err_no_free;

	/* Read the last 4 bytes to determine the length of the DTB data */
	ret = spi_flash_read_dm(rescue_flash_dev, (BCD_FLASH_SIZE-4), 4, (uint8_t*) &bcd_data_length);
	if (ret != 0) {
		printf("Error reading bcd length\n");
		errno = -ENODEV;
		goto err_no_free;
	}
	
	/* Convert length from big endianess to architecture endianess */
	bcd_data_length = ntohl(bcd_data_length);
	printf("bcd_data_length = %i\n", bcd_data_length );
	
	if (bcd_data_length > BCD_FLASH_SIZE ) {
		debug("BCD data length error %02x %02x %02x %02x\n",
		      ( (uint8_t*) &bcd_data_length)[0],
		      ( (uint8_t*) &bcd_data_length)[1],
		      ( (uint8_t*) &bcd_data_length)[2],
		      ( (uint8_t*) &bcd_data_length)[3] );
		errno = -EMSGSIZE;
		goto err_no_free;
	}

	/* Allocate, and verify memory for the BCD data */
	bcd_data = (uint8_t*) malloc(bcd_data_length);
	if (bcd_data == NULL) {
		printf("Error locating memory for BCD data\n");
		goto err_no_free;
	}
	debug("Allocated memory for BCD data\n");

	/* Read the DTB BCD data to memory */
	ret = spi_flash_read_dm(rescue_flash_dev, (BCD_FLASH_SIZE-bcd_data_length), bcd_data_length, (uint8_t*) bcd_data);
	debug("Read data from QSPI bus\n");

	if (ret != 0) {
		printf("Error reading complete BCD data from EEPROM\n");
		errno = -ENOMEM;
		goto err_free;
	}
	dtb_length = bcd_data_length - BCD_LENGTH_SIZE - BCD_HASH_SIZE;

	/* Calculate CRC on read DTB data */
	calculated_crc = crc32( 0, bcd_data, dtb_length);

	/* Received CRC is packed after the DTB data */
	received_crc = *((uint32_t*) &bcd_data[dtb_length]);

	/* Convert CRC from big endianess to architecture endianess */
	received_crc = ntohl(received_crc);

   	if (calculated_crc !=  received_crc) {
		printf("Checksum error. expected %08x, got %08x\n",
			calculated_crc, received_crc);
		errno = -EBADMSG;
		goto err_free;
	}

   	/* Select and probe normal flash */
   	rescue_flash_dev = sel_rescue_qspi_flash(false);

	/* Everything checked out, return the BCD data.
	 * The caller is expected to free this data */
	return bcd_data;

err_free:
	/* free the allocated buffer */
	free(bcd_data);
	
err_no_free:	

	/* Select and probe normal flash */
	rescue_flash_dev = sel_rescue_qspi_flash(false);
	return NULL;
}

#ifndef CONFIG_SPL_BUILD

#ifndef CONFIG_CMD_FDT_MAX_DUMP
#define CONFIG_CMD_FDT_MAX_DUMP 64
#endif

/*
 * Heuristic to guess if this is a string or concatenated strings.
 */

static int is_printable_string(const void *data, int len)
{
	const char *s = data;

	/* zero length is not */
	if (len == 0)
		return 0;

	/* must terminate with zero or '\n' */
	if (s[len - 1] != '\0' && s[len - 1] != '\n')
		return 0;

	/* printable or a null byte (concatenated strings) */
	while (((*s == '\0') || isprint(*s) || isspace(*s)) && (len > 0)) {
		/*
		 * If we see a null, there are three possibilities:
		 * 1) If len == 1, it is the end of the string, printable
		 * 2) Next character also a null, not printable.
		 * 3) Next character not a null, continue to check.
		 */
		if (s[0] == '\0') {
			if (len == 1)
				return 1;
			if (s[1] == '\0')
				return 0;
		}
		s++;
		len--;
	}

	/* Not the null termination, or not done yet: not printable */
	if (*s != '\0' || (len != 0))
		return 0;

	return 1;
}

/*
 * Print the property in the best format, a heuristic guess.  Print as
 * a string, concatenated strings, a byte, word, double word, or (if all
 * else fails) it is printed as a stream of bytes.
 */
static void print_data(const void *data, int len)
{
	int j;

	/* no data, don't print */
	if (len == 0)
		return;

	/*
	 * It is a string, but it may have multiple strings (embedded '\0's).
	 */
	if (is_printable_string(data, len)) {
		puts("\"");
		j = 0;
		while (j < len) {
			if (j > 0)
				puts("\", \"");
			puts(data);
			j    += strlen(data) + 1;
			data += strlen(data) + 1;
		}
		puts("\"");
		return;
	}

	if ((len %4) == 0) {
		if (len > CONFIG_CMD_FDT_MAX_DUMP)
			printf("* 0x%p [0x%08x]", data, len);
		else {
			const __be32 *p;

			printf("<");
			for (j = 0, p = data; j < len/4; j++)
				printf("0x%08x%s", fdt32_to_cpu(p[j]),
					j < (len/4 - 1) ? " " : "");
			printf(">");
		}
	} else { /* anything else... hexdump */
		if (len > CONFIG_CMD_FDT_MAX_DUMP)
			printf("* 0x%p [0x%08x]", data, len);
		else {
			const u8 *s;

			printf("[");
			for (j = 0, s = data; j < len; j++)
				printf("%02x%s", s[j], j < len - 1 ? " " : "");
			printf("]");
		}
	}
}

/*
 * Recursively print (a portion of) the working_fdt.  The depth parameter
 * determines how deeply nested the fdt is printed.
 */
#define MAX_LEVEL 4
static int bcd_fdt_print(const void* address, int depth)
{
	static char tabs[MAX_LEVEL+1] =
		"\t\t\t\t\t";
	const void *nodep;	/* property node pointer */
	int  nodeoffset;	/* node offset from libfdt */
	int  nextoffset;	/* next node offset from libfdt */
	uint32_t tag;		/* tag */
	int  len;		/* length of the property */
	int  level = 0;		/* keep track of nesting level */
	const struct fdt_property *fdt_prop;
	const char *pathp;

	nodeoffset = fdt_path_offset (address, "/");
	if (nodeoffset < 0) {
		/*
		 * Not found or something else bad happened.
		 */
		printf ("libfdt fdt_path_offset() returned %s\n",
			fdt_strerror(nodeoffset));
		return 1;
	}

	/*
	 * The user passed in a node path and no property,
	 * print the node and all subnodes.
	 */
	while(level >= 0) {
		tag = fdt_next_tag(address, nodeoffset, &nextoffset);
		switch(tag) {
		case FDT_BEGIN_NODE:
			pathp = fdt_get_name(address, nodeoffset, NULL);
			if (level <= depth) {
				if (pathp == NULL)
					pathp = "/* NULL pointer error */";
				if (*pathp == '\0')
					pathp = "/";	/* root is nameless */
				printf("%s%s {\n",
					&tabs[MAX_LEVEL - level], pathp);
			}
			level++;
			if (level >= MAX_LEVEL) {
				printf("Nested too deep, aborting.\n");
				return 1;
			}
			break;
		case FDT_END_NODE:
			level--;
			if (level <= depth)
				printf("%s};\n", &tabs[MAX_LEVEL - level]);
			if (level == 0) {
				level = -1;		/* exit the loop */
			}
			break;
		case FDT_PROP:
			fdt_prop = fdt_offset_ptr(address, nodeoffset,
					sizeof(*fdt_prop));
			pathp    = fdt_string(address,
					fdt32_to_cpu(fdt_prop->nameoff));
			len      = fdt32_to_cpu(fdt_prop->len);
			nodep    = fdt_prop->data;
			if (len < 0) {
				printf ("libfdt fdt_getprop(): %s\n",
					fdt_strerror(len));
				return 1;
			} else if (len == 0) {
				/* the property has no value */
				if (level <= depth)
					printf("%s%s;\n",
						&tabs[MAX_LEVEL - level],
						pathp);
			} else {
				if (level <= depth) {
					printf("%s%s = ",
						&tabs[MAX_LEVEL - level],
						pathp);
					print_data (nodep, len);
					printf(";\n");
				}
			}
			break;
		case FDT_NOP:
			printf("%s/* NOP */\n", &tabs[MAX_LEVEL - level]);
			break;
		case FDT_END:
			return 1;
		default:
			if (level <= depth)
				printf("Unknown tag 0x%08X\n", tag);
			return 1;
		}
		nodeoffset = nextoffset;
	}
	return 0;
}

int do_bcdinfo(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	const void* bcd_dtc_blob;
	int ret = 0;
	
	bcd_dtc_blob = get_boardinfo_rescue_flash();

	if (bcd_dtc_blob != NULL) {
		bcd_fdt_print(bcd_dtc_blob, 4);
	}
	
	return ret;
}

/* U_BOOT_CMD(name,maxargs,repeatable,command,"usage","help") */
U_BOOT_CMD(
	bcdinfo, 
	1,
	1,
	do_bcdinfo,
	"Show the Board Configuration Data (stored in rescue flash)",
	""
);
#endif
