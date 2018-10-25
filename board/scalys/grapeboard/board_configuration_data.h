/*
 * Copyright 2018 Scalys B.V.
 * opensource@scalys.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _BCD_H
#define _BCD_H

#define BCD_LENGTH_SIZE	4

#define BCD_FLASH_SIZE 0x100000 /* 1 MBytes */

#define BCD_HASH_SIZE	4

const void* get_boardinfo_rescue_flash(void);
int add_mac_addressess_to_env(const void* blob);

#endif /* _BCD_H */
