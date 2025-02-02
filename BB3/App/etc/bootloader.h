/*
 * bootloader.h
 *
 *  Created on: Oct 13, 2021
 *      Author: horinek
 */

#ifndef GUI_BOOTLOADER_H_
#define ETC_BOOTLOADER_H_

#include "common.h"

typedef enum
{
	bl_update_ok,
	bl_file_not_found,
	bl_file_invalid,
	bl_same_version,
} bootloader_res_t;

typedef struct
{
	uint32_t build_number;
	uint32_t size;
	uint32_t reserved;
	uint32_t crc;
} bootloader_header_t;

bootloader_res_t bootloader_update(char * path);

#endif /* GUI_BOOTLOADER_H_ */
