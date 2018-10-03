// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 Microsoft Corporation
 */

#include <common.h>
#include <fsl_sec.h>
#include <asm/arch-mx6/clock.h>
#include <asm/mach-imx/hab.h>
#include <spl.h>

#include <RiotStatus.h>
#include <RiotEcc.h>
#include <RiotCrypt.h>

DECLARE_GLOBAL_DATA_PTR;

#define LP_MKEYCTL_MK_OTP	0
#define LP_LOCK_MKEYSEL_LCK	0x00000200
#define HP_LOCK_MKEYSEL_LCK	0x00000200

#define SNVS_HPLOCK	0x0
#define SNVS_LPLOCK	0x34
#define SNVS_LPMKEYCTL	0x3C

#define CYRES_IDENTITY_SEED_ADDR SEC_MEM_PAGE2
#define CYRES_IDENTITY_SEED_MAGIC 0x43434449 // CCDI
#define CYRES_IDENTITY_SEED_VERSION 1

struct cyres_identity_seed {
	uint32_t magic;
	uint32_t version;
	uint8_t hashed_cdi[RIOT_DIGEST_LENGTH];
	uint8_t hashed_cdi_optee[RIOT_DIGEST_LENGTH];
	bool cdi_valid;
};

static int set_master_key_source_otpmk(void)
{
	uint32_t val;

	val = readl(SNVS_BASE_ADDR + SNVS_LPMKEYCTL);
	if ((val & ~0x3) == LP_MKEYCTL_MK_OTP) {
		return 0;
	}

	/* check MKS_SL and MKS_HL bits */
	val = readl(SNVS_BASE_ADDR + SNVS_LPLOCK);
	if ((val & LP_LOCK_MKEYSEL_LCK) != 0) {
		printf("MASTER_KEY_SEL is hard locked!\n");
		return -1;
	}

	val = readl(SNVS_BASE_ADDR + SNVS_HPLOCK);
	if ((val & HP_LOCK_MKEYSEL_LCK) != 0) {
		printf("MASTER_KEY_SEL is soft locked!\n");
		return -1;
	}

	/* Set master key source to OTPMK */
	val = readl(SNVS_BASE_ADDR + SNVS_LPMKEYCTL);
	val = (val & ~0x3) | LP_MKEYCTL_MK_OTP;
	writel(val, SNVS_BASE_ADDR + SNVS_LPMKEYCTL);

	return 0;
}

/* Verify that priblob has not been rolled forward to normal mode yet */
static int verify_priblob_trusted(void)
{
	u32 val;
	ccsr_sec_t __iomem *sec = (ccsr_sec_t __iomem *)CONFIG_SYS_FSL_SEC_ADDR;

	/* We can only read true OTPMK when CAAM is operating in secure
		or trusted mode. This is a warning and does not prevent boot. */
	val = sec_in32(&sec->csta);
	switch (val & SEC_CSTA_MOO_MASK) {
	case SEC_CSTA_MOO_SECURE:
	case SEC_CSTA_MOO_TRUSTED:
		break;
	default:
		printf("CAAM not secure/trusted; OTPMK inaccessible\n");
		return -1;
	}

	/* Read SCFGR */
	val = sec_in32(&sec->scfgr);
	if ((val & SEC_SCFGR_PRIBLOB_MASK) == SEC_SCFGR_PRIBLOB_NORMAL) {
		printf("PRIBLOB is rolled forward\n");
		return -1;
	}

	return 0;
}

static int init_trusted_master_key(void)
{
	int ret;

	ret = set_master_key_source_otpmk();
	if (ret != 0) {
		debug("Failed to set master key source to otpmk");
		return ret;
	}

	ret = verify_priblob_trusted();
	if (ret != 0) {
		debug("PRIBLOB verification failed.\n");
		return ret;
	}

	return 0;
}


/*
 * Set PRIBLOB to 0b11 to change derivation of master key.
 * Returns 0 if key is successfully rolled forward.
 * Returns -1 if key was already rolled forward
 */
static int roll_forward_master_key(void)
{
	u32 val;
	ccsr_sec_t __iomem *sec = (ccsr_sec_t __iomem *)CONFIG_SYS_FSL_SEC_ADDR;

	/* Read SCFGR */
	val = sec_in32(&sec->scfgr);
	if ((val & SEC_SCFGR_PRIBLOB_MASK) == SEC_SCFGR_PRIBLOB_NORMAL) {
		printf("PRIBLOB already 0x3\n");
		return -1;
	}

	val |= SEC_SCFGR_PRIBLOB_NORMAL;
	sec_out32(&sec->scfgr, val);

	return 0;
}

static int read_master_key(u32 key[8])
{
	return blob_key_verif(key);
}

int cyres_read_and_hide_cdi()
{
	int ret;
	struct cyres_cdi cdi;
	struct cyres_identity_seed* cyres_identity;

	cyres_identity = (struct cyres_identity_seed*) CYRES_IDENTITY_SEED_ADDR;

	hab_caam_clock_enable(1);
	ret = sec_init();
	if (ret != 0) {
		goto exit;
	}

	memset(cyres_identity, 0, sizeof(struct cyres_identity_seed));
	cyres_identity->magic = CYRES_IDENTITY_SEED_MAGIC;
	cyres_identity->version = CYRES_IDENTITY_SEED_VERSION;

	ret = init_trusted_master_key();
	if (ret != 0) {
		goto exit;
	}

	ret = read_master_key(cdi.cdi);
	if (ret != 0) {
		goto exit;
	}

	ret = roll_forward_master_key();
	if (ret != 0) {
		goto exit;
	}

#if 0
	{
		int i;
		struct cyres_cdi cdi2;
		ret = read_master_key(cdi2.cdi);
		if (ret != 0) {
			printf("Failed to read master key after rolling forward!\n");
		}

		if (memcmp(cdi, &cdi2, sizeof(struct cyres_cdi)) == 0) {
			printf("Master key did not change after rolling forward!\n");
		}

		printf("Successfully read and rolled forward master key:\n");
		printf("CDI1:\n");
		for (i = 0; i < sizeof(cdi->cdi) / sizeof(cdi->cdi[0]); ++i) {
			printf("0x%08x ", cdi->cdi[i]);
			if ((i > 0) && (i % 4) == 0)
				printf("\n");
		}

		printf("\n");
		printf("CDI2:\n");
		for (i = 0; i < sizeof(cdi2.cdi) / sizeof(cdi2.cdi[0]); ++i) {
			printf("0x%08x ", cdi2.cdi[i]);
			if ((i > 0) && (i % 4) == 0)
				printf("\n");
		}
		printf("\n");
	}
#endif

	ret = RiotCrypt_Hash((uint8_t*)&(cyres_identity->hashed_cdi),
			     sizeof(cyres_identity->hashed_cdi),
			     (const uint8_t *)&cdi,
			     sizeof(struct cyres_cdi));

	if (ret != RIOT_SUCCESS) {
		goto exit;
	}

	cyres_identity->cdi_valid = imx_hab_is_enabled();

exit:
	fsl_get_random_bytes((uint8_t *)&cdi, sizeof(struct cyres_cdi));

	return ret;
}

int cyres_save_optee_measurement(uint8_t *hash_value, size_t hash_value_len) {
	int ret;

	struct cyres_identity_seed* cyres_identity;

	cyres_identity = (struct cyres_identity_seed*) CYRES_IDENTITY_SEED_ADDR;

	ret = RiotCrypt_Hash2((uint8_t*)&(cyres_identity->hashed_cdi_optee),
			      sizeof(cyres_identity->hashed_cdi_optee),
			      (uint8_t*)&(cyres_identity->hashed_cdi),
			      sizeof(cyres_identity->hashed_cdi),
			      hash_value,
			      hash_value_len);

#if 0
	int i;
	printf("\nHashed CDI:");
	for (i = 0; i < sizeof(cyres_identity->hashed_cdi); i++) {
		if (i%8 == 0)
			printf("\n");
		printf("%02x ", cyres_identity->hashed_cdi[i]);
	}

	printf("\nHashed OPTEE:");
	for (i = 0; i < sizeof(cyres_identity->hashed_cdi_optee); i++) {
		if (i%8 == 0)
			printf("\n");
		printf("%02x ", cyres_identity->hashed_cdi_optee[i]);
	}
	printf("\n");

	printf("magic: %x, ver: %d\n", cyres_identity->magic,
				       cyres_identity->version);
#endif

	return ret;
}