/**
 * \file
 *
 * \brief UC3-A3 Xplained board configuration template
 *
 */

#ifndef CONF_BOARD_H
#define CONF_BOARD_H

#include "board.h"
#include "gpio.h"

#define LED0_ON() gpio_set_pin_low(LED0_GPIO)
#define LED0_OFF() gpio_set_pin_high(LED0_GPIO)

#define LED1_ON() gpio_set_pin_low(LED1_GPIO)
#define LED1_OFF() gpio_set_pin_high(LED1_GPIO)
#define LED1_TGL() gpio_tgl_gpio_pin(LED1_GPIO)

#define LED2_ON() gpio_set_pin_low(LED2_GPIO)
#define LED2_OFF() gpio_set_pin_high(LED2_GPIO)

#define LED3_ON() gpio_set_pin_low(LED3_GPIO)
#define LED3_OFF() gpio_set_pin_high(LED3_GPIO)

#endif // CONF_BOARD_H
