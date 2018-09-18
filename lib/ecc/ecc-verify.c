// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 Microsoft Corporation
 */

#ifndef USE_HOSTCC
#include <common.h>
#include <fdtdec.h>
#include <asm/types.h>
#include <asm/byteorder.h>
#include <linux/errno.h>
#include <asm/types.h>
#include <asm/unaligned.h>
#include <dm.h>
#else
#include "fdt_host.h"
#include "mkimage.h"
#include <fdt_support.h>
#endif

#include <u-boot/ecc.h>
#include <RiotTarget.h>
#include <RiotStatus.h>
#include <RiotEcc.h>
#include <RiotDerEnc.h>
#include <RiotCrypt.h>
#include <RiotSha256.h>


/**
 * ecc_verify_with_keynode() - Verify a signature against some data using
 * information in node with a specified ECC Key and Curve.
 *
 * @info:	Specifies key and FIT information
 * @hash:	Pointer to the expected hash
 * @sig:	RIoT Signature
 * @node:	Node having the ECC Key properties
 * @return 0 if verified, -ve on error
 */
static int ecc_verify_with_keynode(struct image_sign_info *info,
				const void *hash, RIOT_ECC_SIGNATURE *sig,
				int node)
{
	RIOT_ECC_PUBLIC ecc = { 0 };
	const void *blob = info->fdt_blob;
	RIOT_STATUS status;
	const void *enc_key;
	int curve_id, enc_key_len;

	if (node < 0 || blob == 0) {
		debug("%s: Skipping invalid node", __func__);
		return -EBADF;
	}

	curve_id = fdtdec_get_int(blob, node, "ecc,curve_id", 0);
	enc_key =  fdt_getprop(blob, node, "ecc,pub-key", NULL);
	enc_key_len = fdtdec_get_int(blob, node, "ecc,pub-key-len", 0);

	/* Only NIST P256 keys are supported. */
	if (!enc_key ||
		enc_key_len <= (ECC_P256_BYTES + 1) ||		/* Encoding adds a single octet. (rfc5480) */
		curve_id != ECC_NIST_P256_ID) {
		debug("%s: Missing ECC key info", __func__);
		return -EFAULT;
	}

	status = RiotCrypt_ImportEccPub( enc_key, enc_key_len, &ecc );
	if (status != 0) {
		debug("%s: Failed to import ECC public key: %d", __func__, status);
		return -EBADF;
	}

	status = RiotCrypt_VerifyDigest( hash, RIOT_DIGEST_LENGTH, sig, &ecc );
	if (status != 0) {
		debug("%s: Hash validation failed: %d", __func__, status);
		return -EPERM;
	}

	return 0;
}

int ecc_verify(struct image_sign_info *info,
		const struct image_region region[], int region_count,
		uint8_t *sig, uint sig_len)
{
	const void *blob = info->fdt_blob;
	int ndepth, noffset;
	int sig_node, node;
	char name[100];
	int ret = -1;
	uint8_t digest[RIOT_DIGEST_LENGTH] = { 0 };
	RIOT_ECC_SIGNATURE riot_sig = { 0 };

	/* SHA256 required.  */
	if (info->checksum->checksum_len > RIOT_DIGEST_LENGTH) {
		debug("%s: invlaid checksum-algorith m %s for %s\n",
				__func__, info->checksum->name, info->crypto->name);
		return -EINVAL;
	}

	sig_node = fdt_subnode_offset(blob, 0, FIT_SIG_NODENAME);
	if (sig_node < 0) {//FDT_ERR_NOTFOUND
		debug("%s: No signature node found\n", __func__);
		return -ENOENT;
	}

	/* Convert der encoded signature to RIoT structure */
	ret = RiotCrypt_DERDecodeECCSignature( sig, sig_len, &riot_sig );
	if (ret != 0) {
		return ret;
	}

	/* Compute the digest */
	ret = info->checksum->calculate(info->checksum->name,
					region, region_count, digest);
	if (ret < 0) {
		debug("%s: Error in checksum calculation\n", __func__);
		return -EINVAL;
	}

	/* See if we must use a particular key */
	if (info->required_keynode != -1) {
		ret = ecc_verify_with_keynode(info, digest, &riot_sig, info->required_keynode);
		if (!ret)
			return ret;
	}

	/* Look for a key that matches our hint */
	snprintf(name, sizeof(name), "key-%s", info->keyname);
	node = fdt_subnode_offset(blob, sig_node, name);
	ret = ecc_verify_with_keynode(info, digest, &riot_sig, node);
	if (!ret)
		return ret;

	/* No luck, so try each of the keys in turn */
	for (ndepth = 0, noffset = fdt_next_node(info->fit, sig_node, &ndepth);
			(noffset >= 0) && (ndepth > 0);
			noffset = fdt_next_node(info->fit, noffset, &ndepth)) {
		if (ndepth == 1 && noffset != node) {
			ret = ecc_verify_with_keynode(info, digest, &riot_sig, noffset);
			if (!ret)
				break;
		}
	}

	return ret;
}
