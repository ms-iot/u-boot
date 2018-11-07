/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * SPDX-License-Identifier:   BSD-3-Clause
 *
 * U-Boot interface to Cyres.
 */
#ifndef __CYRES_H__
#define __CYRES_H__

#define CYRES_IDENTITY_LEN 32

struct cyres_hw_identity {
	uint8_t data[CYRES_IDENTITY_LEN];
};

/*
 * Builds a Cyres certificate chain and places it at
 * CONFIG_CYRES_CERT_CHAIN_ADDR. Boot loader code should call this when
 * the next boot stage image has been loaded into memory and hashed.
 * next_image_name becomes the certificate subject name.
 */
int build_cyres_cert_chain(const char *next_image_name,
                           const uint8_t *next_image_sha256);

/*
 * Implemented by the platform to read a stable device-unique identity,
 * then hide it so that no subsequent code can extract the identity.
 */
int read_and_hide_cyres_identity(struct cyres_hw_identity *identity);

#endif /* __CYRES_H__ */

