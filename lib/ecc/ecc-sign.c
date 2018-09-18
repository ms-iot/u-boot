// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 Microsoft Corporation
 */

#include "mkimage.h"
#include <stdio.h>
#include <string.h>
#include <image.h>
#include <time.h>

#include <u-boot/ecc.h>

#include <openssl/ec.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>

#include <RiotTarget.h>
#include <RiotStatus.h>
#include <RiotEcc.h>
#include <RiotDerEnc.h>
#include <RiotCrypt.h>
#include <RiotX509Bldr.h>
#include <RiotSha256.h>


struct ecc_curve {
	const char *name;		/* Name of algorithm */
	const int curve_id;		/* Curve ID */
	const int group_name;	/* Ossl group NID */
};

/* ecc_verify has support for a limited set of curves */
struct ecc_curve supported_ecc_curves[] = {
	{
		.name = ECC_NIST_P256,
		.curve_id = ECC_NIST_P256_ID,
		.group_name = NID_X9_62_prime256v1,
	}
};

static int ecc_err(const char *msg)
{
	unsigned long sslErr = ERR_get_error();

	fprintf(stderr, "%s", msg);
	fprintf(stderr, ": %s\n",
		ERR_error_string(sslErr, 0));

	return -1;
}

static int ec_init(void)
{
	int ret;

#if OPENSSL_VERSION_NUMBER < 0x10100000L
	ret = SSL_library_init();
#else
	ret = OPENSSL_init_ssl(0, NULL);
#endif
	if (!ret) {
		fprintf(stderr, "Failure to init SSL library\n");
		return -1;
	}
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	SSL_load_error_strings();

	OpenSSL_add_all_algorithms();
	OpenSSL_add_all_digests();
	OpenSSL_add_all_ciphers();
#endif

	return 0;
}

static void ec_remove(void)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	CRYPTO_cleanup_all_ex_data();
	ERR_free_strings();
#ifdef HAVE_ERR_REMOVE_THREAD_STATE
	ERR_remove_thread_state(NULL);
#else
	ERR_remove_state(0);
#endif
	EVP_cleanup();
#endif
}

/**
 * ecc_sign_with_key() - sign a provided data region with a specified key
 *
 * @ec:				ECC key object to sign with, containing the private signing key
 * @checksum_algo	Hashing algorithum
 * @region			Array of regions to sign
 * @region_count	Number of regions
 * @sigp			Returns signature buffer, or NULL on failure
 * @sig_size		Returns size signature buffer
 * @return 0 if ok, -ve on error (in which case *sigp will be set to NULL)
 */
static int ecc_sign_with_key(EC_KEY *ec, struct checksum_algo *checksum_algo,
		const struct image_region region[], int region_count,
		uint8_t **sigp, uint *sig_size)
{
	EVP_PKEY *key = NULL;
	EVP_MD_CTX *context = NULL;
	size_t der_size, ret = 0;
	uint8_t *der_sig = NULL;
	int i;

	/* Query size, allocate buffer and finalize signature. */
	key = EVP_PKEY_new();
	if (!key) {
		ret =  ecc_err("EVP_PKEY object creation failed");
		goto out;
	}

	if (!EVP_PKEY_set1_EC_KEY(key, ec)) {
		ret = ecc_err("EVP key setup failed");
		goto out;
	}

	context = EVP_MD_CTX_create();
	if (!context) {
		ret = ecc_err("EVP context creation failed");
		goto out;
	}

	if (!EVP_DigestSignInit(context, NULL, checksum_algo->calculate_sign(), NULL, key)) {
		ret = ecc_err("Signer setup failed");
		goto out;
	}

	/* Extend digest with each region. */
	for (i = 0; i < region_count; i++) {
		if (!EVP_DigestSignUpdate(context, region[i].data, region[i].size)) {
			ret = ecc_err("Signing data failed");
			goto out;
		}
	}

	/* Query size, allocate buffer and finalize signature. */
	EVP_DigestSignFinal( context, NULL, &der_size );
	der_sig = malloc(sizeof(unsigned char) * der_size);
	if (!der_sig) {
		fprintf(stderr, "Out of memory for signature (%d bytes)\n",
			(int)der_size);
		ret = -ENOMEM;
		goto out;
	}
	ret = EVP_DigestSignFinal( context, der_sig, &der_size );
	if(ret != 1) {
		ret = ecc_err("Could not obtain signature");
		goto out;
	}

	/* Transfer reference to caller, clear local pointer */
	debug("Got signature: %d bytes\n", *sig_size);
	*sigp = der_sig;
	der_sig = NULL;
	*sig_size = der_size;
	ret = 0;

out:

	if (der_sig != NULL) free( der_sig );

	if (context) {
		#if OPENSSL_VERSION_NUMBER < 0x10100000L
			EVP_MD_CTX_cleanup(context);
		#else
			EVP_MD_CTX_reset(context);
		#endif
		EVP_MD_CTX_destroy(context);
	}

	if (key) EVP_PKEY_free(key);

	return ret;
}

/**
 * ecc_pem_get_key() - read a ecc key from a .key file
 *
 * @keydir:	Directory containing the key
 * @name	Name of key file (will have a .key extension)
 * @ecp		Returns EC object, or NULL on failure
 * @return 0 if ok, -ve on error (in which case *ecp will be set to NULL)
 */
static int ecc_pem_get_key(const char *keydir, const char *name,
		EC_KEY **ecp)
{
	char path[1024];
	EC_KEY *ec;
	FILE *f;

	*ecp = NULL;
	snprintf(path, sizeof(path), "%s/%s.key", keydir, name);
	f = fopen(path, "r");
	if (!f) {
		fprintf(stderr, "Couldn't open EC private key: '%s': %s\n",
			path, strerror(errno));
		return -ENOENT;
	}

	ec = PEM_read_ECPrivateKey(f, 0, NULL, path);
	if (!ec) {
		ecc_err("Failure reading private key");
		fclose(f);
		return -EPROTO;
	}
	fclose(f);
	*ecp = ec;

	return 0;
}
/**
 * ecc_validate_key_support() - verifies if the ECC key is supported and matches
 *		the requirments defined in the image. ecc_verify is limited on ECC
 *		algorithms to maintain a small footprint.
 *
 * @info:	Image info containing the selected signing algo
 * @ecp		ECC Key object to validate
 * @return true if valid
 */
bool ecc_validate_key_support(struct image_sign_info *info, EC_KEY *ecp)
{
	struct checksum_algo *checksum_algo = info->checksum;
	const EC_GROUP  *group = NULL;
	int group_name;
	int i;
	const char *name;


	/* ecc_verify only supports SHA256 */
	if (strcmp(checksum_algo->name, "sha256")) {
		fprintf( stderr, "Hashing algo not supported for ECC: %s\n", checksum_algo->name );
		return false;
	}

	/* Get the required signing curve as defined by the image */
	name = strchr(info->name, ',');
	if (!name) {
		fprintf( stderr, "Invalid image singing info: %s\n", info->name );
		return false;
	}
	name += 1;

	/* Get the singing curve as defined by the ECC key */
	group = EC_KEY_get0_group(ecp);
	if (group == NULL) {
		fprintf( stderr, "Invalid ECC key group\n");
		return false;
	}
	group_name = EC_GROUP_get_curve_name( group );

	/* Validate the image signing curve against the curve used to create the key. */
	for (i = 0; i < ARRAY_SIZE( supported_ecc_curves ); i++) {
		if (!strcmp(supported_ecc_curves[i].name, name))
			return (group_name == supported_ecc_curves[i].group_name) ? true : false;
	}

	fprintf( stderr, "ECC Key not supported: %s\n", info->name );
	return false;
}

/**
 * ecc_get_params(): - Get the important parameters of an ECC public key
  *
 * @key:		Pointer to the ECC public key to parse
 * @curve_id	Returns the curve used to create the key
 * @pub_asn1	Returns ASN1 encoded key buffer, or NULL on failure
 * @pub_size	Returns size of the encoded buffer
 * @return 0 on success, -ve on error (in which case *pub_asn1 will be set to NULL)
 */
int ecc_get_params(EC_KEY *key, int *curve_id,
		uint8_t **pub_asn1, int *pub_size)
{
	const EC_GROUP  *group = NULL;
	const EC_POINT  *raw_pub;
	BN_CTX *ctx = NULL;
	int ret = -1, i, size, nid;
	uint8_t *buf = NULL;

	ctx = BN_CTX_new();

	/* Get the base types */
	raw_pub = EC_KEY_get0_public_key( key );
	if (raw_pub == NULL){
		goto out;
	}
	group = EC_KEY_get0_group(key);
	if (group == NULL) {
		goto out;
	}

	/* Curve NID. Must be a standard ASN1 curve */
	nid = EC_GROUP_get_curve_name( group );

	*curve_id = -1;
	for (i = 0; i < ARRAY_SIZE( supported_ecc_curves ); i++) {
		if (nid == supported_ecc_curves[i].group_name)
			*curve_id = supported_ecc_curves[i].curve_id;
	}
	if (*curve_id == -1) {
		goto out;
	}

	/* Public key in uncompressed format (0x04||x||y). Consumable by ecc_verify. see rfc5480*/
	size = EC_POINT_point2oct( group, raw_pub, POINT_CONVERSION_UNCOMPRESSED, NULL, 0, ctx );
	if (size == 0) {
		goto out;
	}

	buf = malloc( size );
	if (buf == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	size = EC_POINT_point2oct( group, raw_pub, POINT_CONVERSION_UNCOMPRESSED, buf, size, ctx );
	if (size == 0) {
		goto out;
	}

	*pub_size = size;
	*pub_asn1 = buf;
	buf = NULL;
	ret = 0;

out:

	if (buf) free(buf);
	BN_CTX_free( ctx );
	return ret;
}

/**
 * ecc_add_pubinfo_data(): - Adds the public ECC key as a Signature/Key node
  *
 * @info:		Image specifies key and FIT information
 * @keydest		Destination FDT blob for public key data
 * @noffset		Component image node offset
 * @ec			Key data to record
 * @return 0 on success, -ve on error
 */
int ecc_add_pubinfo_data(struct image_sign_info *info,
		void *keydest, int noffset,
		EC_KEY *ec)
{
	int parent, node, curve_id;
	char name[100];
	int ret, pub_size;
	uint8_t *pub = NULL;

	ret = ecc_get_params( ec, &curve_id, &pub, &pub_size );
	if (ret)
		goto out;

	/* Get the appropriate parent node if no offset was specified. */
	parent = fdt_subnode_offset( keydest, noffset, FIT_SIG_NODENAME );
	if (parent == -FDT_ERR_NOTFOUND) {
		parent = fdt_add_subnode( keydest, noffset, FIT_SIG_NODENAME );
		if (parent < 0) {
			ret = parent;
			if (ret != -FDT_ERR_NOSPACE) {
				fprintf( stderr, "Couldn't create signature node: %s\n",
							fdt_strerror( parent ) );
			}
		}
	}
	if (ret)
		goto out;

	/* Either create or overwrite the named key node */
	snprintf( name, sizeof( name ), "key-%s", info->keyname );
	node = fdt_subnode_offset( keydest, parent, name );
	if (node == -FDT_ERR_NOTFOUND) {
		node = fdt_add_subnode( keydest, parent, name );
		if (node < 0) {
			ret = node;
			if (ret != -FDT_ERR_NOSPACE) {
				fprintf( stderr, "Could not create key subnode: %s\n",
							fdt_strerror( node ) );
			}
		}
	}
	else if (node < 0) {
		fprintf( stderr, "Cannot select keys parent: %s\n",
					fdt_strerror( node ) );
		ret = node;
	}

	if (!ret)
		ret = fdt_setprop_u32(keydest, node, "ecc,curve_id", curve_id);
	if (!ret)
		ret = fdt_setprop(keydest, node, "ecc,pub-key", pub, pub_size);
	if (!ret)
		ret = fdt_setprop_u32(keydest, node, "ecc,pub-key-len", pub_size);
	if (!ret) {
		ret = fdt_setprop_string(keydest, node, "key-name-hint",
				 info->keyname);
	}
	if (!ret) {
		ret = fdt_setprop_string(keydest, node, FIT_ALGO_PROP,
					 info->name);
	}
	if (!ret && info->require_keys) {
		ret = fdt_setprop_string(keydest, node, "required",
					 info->require_keys);
	}

	ret = 0;

out:

	if (pub != NULL) free(pub);

	return ret;
}

int ecc_sign(struct image_sign_info *info,
		const struct image_region region[], int region_count,
		uint8_t **sigp, uint *sig_len)
{
	EC_KEY *ec = NULL;
	int ret;

	ret = ec_init();
	if (ret)
		goto out;

	if (info->engine_id) {
		/* Hardware keys currently not supported */
		/* Future: Engine support implemented in rsa-sign.c can be generalized for ECC */
		fprintf(stderr, "Engine not supported\n");
		ret = -ENOTSUP;
		goto out;
	}

	ret = ecc_pem_get_key(info->keydir, info->keyname, &ec);
	if (ret)
		goto out;
	if (!ecc_validate_key_support(info, ec)) {
		fprintf(stderr, "EC error: Curve not supported.\n");
		ret = -ENOTSUP;
		goto out;
	}

	ret = ecc_sign_with_key(ec, info->checksum, region,
				region_count, sigp, sig_len);
	if (ret)
		goto out;

out:
	if (ec) EC_KEY_free(ec);

	ec_remove();
	return ret;
}

int ecc_add_verify_data(struct image_sign_info *info, void *keydest)
{
	EC_KEY *ec = NULL;
	int ret;

	debug("%s: Getting verification data\n", __func__);
	if (info->engine_id) {
		fprintf(stderr, "Engine not supported\n");
		ret = -ENOTSUP;
		goto out;
	}

	ret = ecc_pem_get_key(info->keydir, info->keyname, &ec);
	if (ret)
		goto out;

	ret = ecc_add_pubinfo_data(info, keydest, 0, ec);
	if (ret)
		goto out;

out:
	if (ec) EC_KEY_free(ec);

	if (ret)
		ret = ret == -FDT_ERR_NOSPACE ? -ENOSPC : -EIO;

	return ret;
}
