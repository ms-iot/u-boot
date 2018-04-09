/*
 * Copyright 2018 Scalys B.V.
 * opensource@scalys.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef GPIO_GRAPEBOARD_H_
#define GPIO_GRAPEBOARD_H_

/* GPIO1 and GPIO2 registers */
#define CONFIG_SYS_GPIO1 0x2300000
#define CONFIG_SYS_GPIO2 0x2310000
#define GPIO_PIN_MASK(shift) (0x80000000 >> shift)

/* =====================================================
 * Grapeboard ExPI mapping (*pin name at ls1012a side)
 * Note: The secondary options require modified RCW.
 * =====================================================
 *                 3V3 -| 1       2|- 5V0
 *             I2C_SDA -| 3       4|- 5V0
 *             I2C_SCL -| 5       6|- GND
 *          CLK0_25MHZ -| 7       8|- UART_TXD
 *                 GND -| 9      10|- UART_RXD
 *   SPI_CE2/GPIO1_27* -|11      12|- GPIO2_04*
 *           GPIO2_05* -|13      14|- GND
 *           GPIO2_06* -|15      16|- GPIO2_07*
 *                 3V3 -|17      18|- GPIO2_09*
 *  SPI_MOSI/GPIO1_24* -|19      20|- GND
 *  SPI_MISO/GPIO1_28* -|21      22|- GPIO2_10*
 *   SPI_CLK/GPIO1_29* -|23      24|- SPI_CE0/GPIO1_25*
 *                 GND -|25      26|- SPI_CE1/GPIO1_26*
 */

/* ExPI gpios */
#define gpio1_24 GPIO_PIN_MASK(24) /* ExPI pin 19 */
#define gpio1_25 GPIO_PIN_MASK(25) /* ExPI pin 24 */
#define gpio1_26 GPIO_PIN_MASK(26) /* ExPI pin 26 */
#define gpio1_27 GPIO_PIN_MASK(27) /* ExPI pin 11 */
#define gpio1_28 GPIO_PIN_MASK(28) /* ExPI pin 21 */
#define gpio1_29 GPIO_PIN_MASK(29) /* ExPI pin 23 */

#define gpio2_04 GPIO_PIN_MASK(4)  /* ExPI pin 12 */
#define gpio2_05 GPIO_PIN_MASK(5)  /* ExPI pin 13 */
#define gpio2_06 GPIO_PIN_MASK(6)  /* ExPI pin 15 */
#define gpio2_07 GPIO_PIN_MASK(7)  /* ExPI pin 16 */
#define gpio2_09 GPIO_PIN_MASK(9)  /* ExPI pin 18 */
#define gpio2_10 GPIO_PIN_MASK(10) /* ExPI pin 22 */

/* M.2 gpios */
#define gpio1_22 GPIO_PIN_MASK(22)
#define gpio2_00 GPIO_PIN_MASK(0)
#define gpio2_01 GPIO_PIN_MASK(1)
#define gpio2_02 GPIO_PIN_MASK(2)
#define M2_CFG1  GPIO_PIN_MASK(11) /* gpio2_11 */
#define M2_CFG0  GPIO_PIN_MASK(12) /* gpio2_12 */
#define M2_CFG2  GPIO_PIN_MASK(13) /* gpio2_13 */
#define M2_CFG3  GPIO_PIN_MASK(14) /* gpio2_14 */

/* Misc gpios */
#define QSPI_MUX_N_MASK GPIO_PIN_MASK(3) /* gpio2_03 */

#endif /* GPIO_GRAPEBOARD_H_ */
