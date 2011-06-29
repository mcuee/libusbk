/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * AVR Software Framework (ASF).
 */
#include <asf.h>
#include "conf_board.h"

volatile uint8_t g_IsUsbAttached = 0;

extern void RunApplication(void);

void Bm_VBus_Handler(bool bIsAttached)
{
	if (bIsAttached)
	{
		// USB device connected.
		udc_attach ();
		g_IsUsbAttached=1;
		LED0_ON();
	}
	else
	{
		// USB device dis-connected.
		LED0_OFF();
		g_IsUsbAttached=0;
		udc_detach ();
	}
}

int main (void)
{
	uint32_t counter = UINT32_MAX/2;
	/* Initialize basic board support features.
	 * - Initialize system clock sources according to device-specific
	 *   configuration parameters supplied in a conf_clock.h file.
	 * - Set up GPIO and board-specific features using additional configuration
	 *   parameters, if any, specified in a conf_board.h file.
	 */
	sysclk_init();
	board_init();

	// Initialize interrupt vector table support.
	irq_initialize_vectors();
	
restart:
	// Enable interrupts
	cpu_irq_enable();

	udc_start ();

	// Start the device (attach it) if it can't be attached/detached
	// automatically via an interrupt.
	if (! udc_include_vbus_monitoring ())
	{
		Bm_VBus_Handler (true);
	}
	
	while(!g_IsUsbAttached && ++counter!=0);
	if(g_IsUsbAttached)
	{
		RunApplication();
	}
	
	udc_stop();
	
	cpu_irq_disable();
	
	goto restart;
}
