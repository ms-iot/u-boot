/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * SPDX-License-Identifier:   BSD-3-Clause
 */
#include <common.h>
#include <cyres.h>
#include <fsl_sec.h>
#include <fsl_sec_mon.h>

/*
 * Verify that the SSM is in trusted or secure mode and that PRIBLOB is 00
 */
static int verify_security_state(void)
{
	u32 val;

	struct ccsr_sec_mon_regs *sec_mon_regs = (void *)
						(CONFIG_SYS_SEC_MON_ADDR);
	ccsr_sec_t __iomem *sec = (ccsr_sec_t __iomem *)CONFIG_SYS_FSL_SEC_ADDR;

	val = sec_mon_in32(&sec_mon_regs->hp_stat) & HPSR_SSM_ST_MASK;
	if (val != HPSR_SSM_ST_TRUST && val != HPSR_SSM_ST_SECURE) {
		printf("SSM not in secure/trusted state: 0x%x\n", val >> 8);
		return -1;
	}

	val = sec_in32(&sec->scfgr);
	if ((val & SEC_SCFGR_PRIBLOB_MASK) != 0) {
		printf("PRIBLOB not in SBS mode: 0x%x\n",
		       val & SEC_SCFGR_PRIBLOB_MASK);
		return -1;
	}

	return 0;
}

/*
 * Set the PRIBLOB to normal mode to hide the secret device identity
 */
static void roll_forward_master_key(void)
{
	u32 val;
	ccsr_sec_t __iomem *sec = (ccsr_sec_t __iomem *)CONFIG_SYS_FSL_SEC_ADDR;

	val = sec_in32(&sec->scfgr);
	val |= SEC_SCFGR_PRIBLOB_NORMAL;
	sec_out32(&sec->scfgr, val);
}

static int read_master_key(struct cyres_hw_identity *identity)
{
	return blob_key_verif((u32 *)identity->data);
}

int read_and_hide_cyres_identity(struct cyres_hw_identity *identity)
{
	int ret;

	ret = sec_init();
	if (ret != 0) {
		printf("sec init failed");
		goto exit;
	}

	ret = verify_security_state();
	if (ret != 0) {
		printf("Security state failure\n");
		printf("Continuing with non-secret testing identity\n");
	}

	ret = read_master_key(identity);
	if (ret != 0) {
		printf("read_master_key failed. ret = 0x%x\n", ret);
		goto exit;
	}

#if 0
	{
		int i;
		struct cyres_hw_identity identity2;

		roll_forward_master_key();
		ret = read_master_key(&identity2);
		if (ret != 0)
			printf("Master Key read failed after roll forward\n");

		if (memcmp(identity, &identity2,
		    sizeof(struct cyres_hw_identity)) == 0) {
			printf("Master Key didn't change after roll forward\n");
		} else {
			printf("Master Key read and rolled forward\n");
		}

		printf("Original identity:\n");
		for (i = 0; i < sizeof(identity->data); ++i) {
			printf("%02x", identity->data[i]);
			if (i == 15)
				printf("\n");
		}

		printf("\n");
		printf("Rolled forward identity:\n");
		for (i = 0; i < sizeof(identity2.data); ++i) {
			printf("%02x", identity2.data[i]);
			if (i == 15)
				printf("\n");
		}
		printf("\n");
	}
#endif

exit:
	/* Always roll the master key forward to priblob normal state */
	roll_forward_master_key();

	return ret;
}
