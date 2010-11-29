/* libusb-win32 WDF, Generic KMDF Windows USB Driver
 * Copyright (c) 2010-2011 Travis Robinson <libusbdotnet@gmail.com>
 * Copyright (c) 2002-2005 Stephan Meyer <ste_meyer@web.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "private.h"



NTSTATUS GetStatus(IN  PDEVICE_CONTEXT deviceContext,
                   IN  PWDF_MEMORY_DESCRIPTOR memoryDescriptor,
                   IN  WDF_USB_BMREQUEST_RECIPIENT recipient,
                   IN  USHORT index,
                   IN  INT timeout,
                   OUT PULONG transferred)
{
	NTSTATUS						status = STATUS_SUCCESS;
	WDF_USB_CONTROL_SETUP_PACKET	controlSetupPacket;
	WDF_REQUEST_SEND_OPTIONS		syncReqOptions;

	if (timeout > LIBUSB_MAX_CONTROL_TRANSFER_TIMEOUT || !timeout)
	{
		timeout = LIBUSB_MAX_CONTROL_TRANSFER_TIMEOUT;
	}

	USBMSG("recipient=%04d index=%04d timeout=%d\n", recipient, index, timeout);

	WDF_USB_CONTROL_SETUP_PACKET_INIT_GET_STATUS(&controlSetupPacket, recipient, index);

	WDF_REQUEST_SEND_OPTIONS_INIT(&syncReqOptions, 0);
	WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&syncReqOptions, WDF_REL_TIMEOUT_IN_MS(timeout));

	status = WdfUsbTargetDeviceSendControlTransferSynchronously(
	             deviceContext->WdfUsbTargetDevice,
	             WDF_NO_HANDLE,
	             &syncReqOptions,
	             &controlSetupPacket,
	             memoryDescriptor,
	             transferred);

	if (!NT_SUCCESS(status))
	{
		USBERR("failed status=%Xh\n", status);
	}
	return status;
}
