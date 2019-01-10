/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * SPDX-License-Identifier:   BSD-3-Clause
 *
 * Utilities related to HAB
 */
#ifndef __FSL_HAB_H__
#define __FSL_HAB_H__

/*
 * Get the public key used to validate SPL
 *
 * key - buffer allocated by caller into which the key is copied
 * size - size of the buffer
 * key_size - on success, receives actual number of bytes copied to buffer
 */
int get_spl_auth_key_pub(u8 *key, size_t size, size_t *key_size);

#endif /* __FSL_HAB_H__ */
