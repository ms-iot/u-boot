/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2017-2018 Microsoft Corporation
 */

#ifndef _ECC_H
#define _ECC_H

#include <errno.h>
#include <image.h>

struct image_sign_info;

#define   ECC_NIST_P256				"ecc-prime256v1"
#define   ECC_NIST_P256_ID			0x1
#define   ECC_P256_BYTES			(256 / 8)

#if IMAGE_ENABLE_SIGN
/**
 * ecc_sign() - calculate and return signature for given input data
 *
 * @info:	Specifies key and FIT information
 * @region:	Pointer to the input region
 * @sigp:	Set to an allocated buffer holding the signature
 * @sig_len:	Set to length of the calculated hash
 *
 * This computes input data signature according to selected algorithm.
 * Resulting signature value is placed in an allocated buffer, the
 * pointer is returned as *sigp. The length of the calculated
 * signature is returned via the sig_len pointer argument. The caller
 * should free *sigp.
 *
 * @return: 0, on success, -ve on error
 */
int ecc_sign(struct image_sign_info *info,
		const struct image_region region[],
		int region_count, uint8_t **sigp, uint *sig_len);

/**
 * add_verify_data() - Add verification information to FDT
 *
 * Add public key information to the FDT node, suitable for
 * verification at run-time. The information added depends on the
 * algorithm being used.
 *
 * @info:	Specifies key and FIT information
 * @keydest:	Destination FDT blob for public key data
 * @return: 0, on success, -ENOSPC if the keydest FDT blob ran out of space,
		other -ve value on error
*/
int ecc_add_verify_data(struct image_sign_info *info, void *keydest);
#else
static inline int ecc_sign(struct image_sign_info *info,
		const struct image_region region[], int region_count,
		uint8_t **sigp, uint *sig_len)
{
	return -ENXIO;
}

static inline int ecc_add_verify_data(struct image_sign_info *info,
		void *keydest)
{
	return -ENXIO;
}
#endif /* IMAGE_ENABLE_SIGN */

#if IMAGE_ENABLE_VERIFY
/**
 * ecc_verify() - Verify a signature against some data
 *
 * Verify a ECC signature against an expected hash.
 *
 * @info:	Specifies key and FIT information
 * @data:	Pointer to the input data
 * @data_len:	Data length
 * @sig:	Signature
 * @sig_len:	Number of bytes in signature
 * @return 0 if verified, -ve on error
 */
int ecc_verify(struct image_sign_info *info,
		const struct image_region region[], int region_count,
		uint8_t *sig, uint sig_len);
#else
static inline int ecc_verify(struct image_sign_info *info,
		const struct image_region region[], int region_count,
		uint8_t *sig, uint sig_len)
{
	return -ENXIO;
}
#endif /* IMAGE_ENABLE_VERIFY */

#endif /* _ECC_H */
