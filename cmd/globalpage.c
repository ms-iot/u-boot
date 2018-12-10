// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2018 Microsoft Corporation
 */

#include <common.h>
#include <command.h>
#include <environment.h>

struct global_page_header {
	u32 signature;
	u8 revision;
	u8 reserved[3];
} __packed;

struct global_mac_entry {
	u8 enet_id;
	u8 valid;
	u8 mac[6];
} __packed;

struct imx_global_page {
	struct global_page_header header;
	struct global_mac_entry mac_entry[2];
} __packed;

struct imx_global_page *global_page = 0;

int globalpage_init(char* const arg)
{
	global_page = (struct imx_global_page *) simple_strtol(arg, NULL, 16);

	printf("Initializing 4KB of memory at 0x%p as the global page.\n", global_page);

	/* Clear global page */
	memset(global_page, 0, 0x1000);

	/* Set signature to 'GLBL' */
	global_page->header.signature = 0x474c424c;

	/* Set revision */
	global_page->header.revision = 1;
	return 0;
}

int globalpage_add(char* const arg)
{
	uchar mac_id[6];
	int mac;

	if (global_page == 0)
		return -1;

	if (!strncmp(arg, "ethaddr", 7))
		mac = 0;
	else if (!strncmp(arg, "eth1addr", 8))
		mac = 1;
	else
		return -1;

	eth_env_get_enetaddr(arg, mac_id);

	global_page->mac_entry[mac].enet_id = mac;
	global_page->mac_entry[mac].valid = 1;
	memcpy(global_page->mac_entry[mac].mac, mac_id, 6);
	printf("Stashed %s = %02x:%02x:%02x:%02x:%02x:%02x\n", arg,
	mac_id[5], mac_id[4], mac_id[3], mac_id[2], mac_id[1], mac_id[0]);
	return 0;
}

int do_globalpage(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc != 3)
		return CMD_RET_USAGE;

	if (!strncmp(argv[1], "init", 4))
		return globalpage_init(argv[2]);
	else if (!strncmp(argv[1], "add", 3))
		return globalpage_add(argv[2]);
	else
		return CMD_RET_USAGE;
}

U_BOOT_CMD(
	globalpage, 4, 1, do_globalpage,
	"Set up the global configuration page",
	"globalpage init 0x800000\n"
	"globalpage add ethaddr\n"
	"globalpage add eth1addr\n"
);
