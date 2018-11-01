/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 *
 * SPDX-License-Identifier:	BSD-3-Clause
 */
#include "imagetool.h"
#include <image.h>
#include "pblimage.h"
#include "pbl_crc32.h"

void ls1012a_pbl_load_uboot(int ifd, struct image_tool_params *params)
{
        printf("hello from ls1012a load image\n");
        return;
}


int ls1012a_pblimage_check_params(struct image_tool_params *params)
{
        printf("hello from ls1012a check params\n");
        return 0;
}
