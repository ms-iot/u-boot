/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 *
 * SPDX-License-Identifier:	BSD-3-Clause
 */
#include <common.h>
#include <fsl_sfp.h>
#include <fsl_validate.h>
#include <isbc_hdr_ta_2_x.h>
#include <fsl_hab.h>

#define CONFIG_DCFG_ADDR	CONFIG_SYS_FSL_GUTS_ADDR

static const u8 barker_code[] = { 0x68, 0x39, 0x27, 0x81 };

/* this function duplicates fsl_check_boot_mode_secure, but it was
   simpler to copy it than refactor */
static bool is_hab_enabled(void)
{
	uint32_t val;
	struct ccsr_sfp_regs *sfp_regs = (void *)(CONFIG_SYS_SFP_ADDR);
	struct ccsr_gur __iomem *gur = (void *)(CONFIG_DCFG_ADDR);

	val = sfp_in32(&sfp_regs->ospr);
	if (val & ITS_MASK)
		return true;

	/* For PBL based platforms check the SB_EN bit in RCWSR */
	val = in_be32(&gur->rcwsr[RCW_SB_EN_REG_INDEX - 1]);
	if (val & RCW_SB_EN_MASK)
		return true;

	return false;
}

/* this function duplicates get_csf_base_addr in fsl_validate.c */
static int get_csf_base_addr(void **csf_addr)
{
	struct ccsr_gur __iomem *gur = (void *)(CONFIG_SYS_FSL_GUTS_ADDR);
	u32 csf_hdr_addr = in_be32(&gur->scratchrw[0]);

	if (memcmp((u8 *)(uintptr_t)csf_hdr_addr,
		   barker_code, ISBC_BARKER_LEN)) {

		printf("CSF barker code mismatch: 0x%x\n",
		       *(u32 *)(uintptr_t)csf_hdr_addr);
		return -1;
	}

	*csf_addr = (void *)(uintptr_t)csf_hdr_addr;
	return 0;
}

int get_spl_auth_key_pub(u8 *key, size_t size, size_t *key_size)
{
	int ret;
	const struct srk_table *srk_tbl;
	u32 offset;
	const u8 *key_ptr;
	size_t key_len;
	const struct isbc_hdr_ta_2_1 *csf_hdr;

	if (!is_hab_enabled()) {
		printf("can't get CSF - HAB disabled\n");
		return -1;
	}

	ret = get_csf_base_addr((void **)&csf_hdr);
	if (ret)
		return ret;

	if (csf_hdr->len_kr.srk_table_flag != 0) {
		offset = csf_hdr->srk_table_offset +
			((csf_hdr->len_kr.key_num_verify - 1) *
			 sizeof(struct srk_table));

		srk_tbl = (struct srk_table *)((u8 *)csf_hdr + offset);
		key_ptr = srk_tbl->pkey;
		key_len = srk_tbl->key_len;
	} else {
		key_ptr = (u8 *)csf_hdr + csf_hdr->pkey;
		key_len = csf_hdr->key_len;
	}

	if (size < key_len) {
		return -EINVAL;
	}

	memcpy(key, key_ptr, key_len);
	*key_size = key_len;

	return 0;
}
