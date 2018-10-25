/*
 * Copyright 2018 Scalys B.V.
 * opensource@scalys.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <i2c.h>
#include <asm/io.h>

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

#define I2C_ADDRESS_USB_HUB		0x60
#define MAX_I2C_ATTEMPTS		10

#define HX3_SETTINGS_SIZE		192

/* Cypress HX3 hub settings blob */
const uint8_t hx3_settings[5 + HX3_SETTINGS_SIZE] = {
	'C', 'Y', /* Cypress magic signature */
	0x30, /* I2C speed : 100kHz */
	0xd4, /* Image type: Only settings, no firmware */
	HX3_SETTINGS_SIZE, /* payload size (192) */
	0xb4, 0x04, /* VID */
	0x04, 0x65, /* PID */
	10, 50, /* DID */
	0x00, /* Reserved */
	0x07, /* DS4 has no SuperSpeed port */
	0x32, /* bPwrOn2PwrGood : 100 ms */
	0x7f, /* 4 Downstream ports : DS4 is non-removable (MCU) */
	0x11, /* Ganged power switching */
	0x20, /* suspend indicator disabled, power switch control is active high */
	0x11, /* BC v1.2 and ghost charging enabled */
	0xf0, /* cdp enabled */
	0xf8, /* embedded hub, overcurrent input is active high */
	0x00, /* reserved */
	0x08, /* USB String descriptors enabled (0x08) / disabled (0x00) */
	0x00, 0x00,
	0x12, 0x00, 0x2c,
	0x66, 0x66, /* USB3.0 TX driver de-emphasis */
	0x69, 0x29, 0x29, 0x29, 0x29, /* TX amplitude */
	0x00, /* Reserved */
	0x06, 0x65, /* USB 2.0 PID */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* Reserved */
	0x04, 0x03, 0x09, 0x04, /* LangID = 0x0409 US English */
	0x18, 0x03, /* Manufacturer string descriptor */
	0x32, 0x00, 0x30, 0x00, 0x31, 0x00, 0x37, 0x00,
	0x20, 0x00, 0x53, 0x00, 0x63, 0x00, 0x61, 0x00,
	0x6c, 0x00, 0x79, 0x00, 0x73, 0x00,
	0x2c, 0x03, /* Product string descriptor */
	0x47, 0x00, 0x72, 0x00, 0x61, 0x00, 0x70, 0x00,
	0x65, 0x00, 0x62, 0x00, 0x6f, 0x00, 0x61, 0x00,
	0x72, 0x00, 0x64, 0x00, 0x20, 0x00, 0x43, 0x00,
	0x59, 0x00, 0x2d, 0x00, 0x48, 0x00, 0x58, 0x00,
	0x33, 0x00, 0x20, 0x00, 0x48, 0x00, 0x55, 0x00,
	0x42, 0x00,
	0x1a, 0x03, /* Serial string descriptor */
	0x47, 0x00,	0x72, 0x00, 0x61, 0x00, 0x70, 0x00,
	0x65, 0x00, 0x62, 0x00, 0x6f, 0x00, 0x61, 0x00,
	0x72, 0x00, 0x64, 0x00, 0x20, 0x00, 0x31, 0x00,
	0x39, 0x00, 0x41, 0x00,
	0x00
};

int usb_hx3_hub_init(void) {
	int length, index = 0, i2c_attempts = 0;
	const int settings_size = sizeof(hx3_settings);
	uint8_t *data = (uint8_t *)hx3_settings;

	/*
	 * Configure USB hub slave
	 *
	 * The Hx3 starts in an i2c slave bootloader mode until sufficient and correct data is written to it over I2C.
	 * If transferred data is incorrect then the device will hang until it has been reset.
	 */
	puts("USB: configuring hub....");

	while(index <= settings_size - 1){
		length = MIN(64, (settings_size - index));

		if(i2c_write(I2C_ADDRESS_USB_HUB, index, 2, data, length)) {
			if(i2c_attempts < 1)
				printf("\nWARNING: I2C error during configuring USB hub slave. retrying...\n");
			if(++i2c_attempts >= MAX_I2C_ATTEMPTS){
				printf("ERROR: Maximum USB hub configuration attempts reached. Exiting now\n");
				return 1;
			}
			continue;
		}
		i2c_attempts = 0; /* reset error count */
		index += length;
		data += length;
	}

	puts("Done!\n");
	return 0;
}

int usb_hx3_hub_reset(void) {
	/* USB hub cannot be reset in software without resetting the ls1012a */
	return 1;
}
