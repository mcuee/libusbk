/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief UC3-A3-Xplained board initialization.
 *
 * This file contains board initialization function.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR
 * - Supported devices:  AVR32 UC3-A3 Xplained development board.
 * - AppNote:
 *
 * \author               Atmel Corporation: http://www.atmel.com \n
 *                       Support and FAQ: http://support.atmel.no/
 *
 ******************************************************************************/

/* Copyright (c) 2010 Atmel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an Atmel
 * AVR product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


//! \name Atmel Xplained Development Board Definitions

#include "conf_board.h"
#include "board.h"
#include "gpio.h"



#if defined (CONF_BOARD_AT45DBX)
#   define AT45DBX_MEM_CNT          1
#endif



void board_init (void)
{
    gpio_configure_pin (LED0_GPIO, GPIO_DIR_OUTPUT | GPIO_INIT_HIGH);
    gpio_configure_pin (LED1_GPIO, GPIO_DIR_OUTPUT | GPIO_INIT_HIGH);
    gpio_configure_pin (LED2_GPIO, GPIO_DIR_OUTPUT | GPIO_INIT_HIGH);
    gpio_configure_pin (LED3_GPIO, GPIO_DIR_OUTPUT | GPIO_INIT_HIGH);

    gpio_configure_pin (GPIO_PUSH_BUTTON_0, (GPIO_DIR_INPUT | GPIO_PULL_UP));


#if defined (CONF_BOARD_AT45DBX)

    // SPI GPIO pin configuration.

    const gpio_map_t AT45DBX_SPI_GPIO_MAP = {

        {AT45DBX_SPI_SCK_PIN,  AT45DBX_SPI_SCK_FUNCTION },  // SPI Clock.
        {AT45DBX_SPI_MISO_PIN, AT45DBX_SPI_MISO_FUNCTION},  // MISO.
        {AT45DBX_SPI_MOSI_PIN, AT45DBX_SPI_MOSI_FUNCTION},  // MOSI.

#define AT45DBX_ENABLE_NPCS_PIN(NPCS, unused) \
		{AT45DBX_SPI_NPCS##NPCS##_PIN, AT45DBX_SPI_NPCS##NPCS##_FUNCTION},  // Chip Select NPCS.
		MREPEAT(AT45DBX_MEM_CNT, AT45DBX_ENABLE_NPCS_PIN, ~)
#undef AT45DBX_ENABLE_NPCS_PIN
    };

    gpio_enable_module( AT45DBX_SPI_GPIO_MAP,
        sizeof (AT45DBX_SPI_GPIO_MAP) / sizeof (AT45DBX_SPI_GPIO_MAP[0]));
#endif


#if defined (CONF_BOARD_TWI)
  
    // TWI Master GPIO pin configuration.

    const gpio_map_t TWI_GPIO_MAP =
        {{ TWIMS0_TWD_PIN,  TWIMS0_TWD_FUNCTION  },
         { TWIMS0_TWCK_PIN, TWIMS0_TWCK_FUNCTION }};

    gpio_enable_module (TWI_GPIO_MAP,
        sizeof (TWI_GPIO_MAP) / sizeof (TWI_GPIO_MAP[0]));
#endif

#if defined (CONF_BOARD_COM_PORT)
	// USART GPIO pin configuration.
	static const gpio_map_t COMPORT_GPIO_MAP =
	{
		{USART_RXD_PIN, USART_RXD_FUNCTION },
		{USART_TXD_PIN, USART_TXD_FUNCTION }
	};
	gpio_enable_module(COMPORT_GPIO_MAP,
			sizeof(COMPORT_GPIO_MAP) / sizeof(COMPORT_GPIO_MAP[0]));
#endif

#if defined (SENSORS_XPLAINED_BOARD)
    /* Configure the Xplained Sensor extension board, if any, after
     * the platform Xplained board has configured basic clock settings,
     * GPIO pin mapping, interrupt controller options, etc.
     */
    sensor_board_init ();
#endif
}
