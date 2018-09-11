// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 Microsoft Corporation
 */

#include <common.h>
#include <malloc.h>
#include <memalign.h>
#include <fsl_sec.h>
#include <linux/errno.h>
#include "jobdesc.h"
#include "desc.h"
#include "jr.h"

/* Buffer cannot exceed 0xFFFF bytes due to job descriptor format */
#define RNG_BUFFER_SIZE (ARCH_DMA_MINALIGN * 2)

int fsl_get_random_bytes(uint8_t *buf, size_t len)
{
	int completed = 0;
	int ret = 0;
	size_t call_length = 0;

	ALLOC_CACHE_ALIGN_BUFFER(uint32_t, desc, MAX_CAAM_DESCSIZE);
	memset((void *)desc, 0, MAX_CAAM_DESCSIZE*sizeof(uint32_t));
	ALLOC_CACHE_ALIGN_BUFFER(uint8_t, temp_buffer, RNG_BUFFER_SIZE);

	while (completed < len)
	{
		/* Populate the buffer with random bytes, RNG_BUFFER_SIZE bytes at a time */
		call_length = min((len - completed), (size_t)RNG_BUFFER_SIZE);
		inline_cnstr_jobdesc_rng_get_random_bytes(desc, temp_buffer, call_length);

		invalidate_dcache_range((unsigned long)temp_buffer, (unsigned long)(temp_buffer + RNG_BUFFER_SIZE));
		ret = run_descriptor_jr(desc);
		if (ret) {
			debug("%s Error generating random bytes. Completed %d of %d. Error: 0x%x\n", __func__, completed, len, ret);
			return -EFAULT;
		} else {
			memcpy((buf + completed), temp_buffer, call_length);
			completed += call_length;
		}
	}

	/* Clear the buffer to prevent snooping */
	invalidate_dcache_range((unsigned long)temp_buffer, (unsigned long)(temp_buffer + RNG_BUFFER_SIZE));
	memset((void *)temp_buffer, 0, RNG_BUFFER_SIZE);
	flush_dcache_range((unsigned long)temp_buffer, (unsigned long)(temp_buffer + RNG_BUFFER_SIZE));
	return 0;
}
