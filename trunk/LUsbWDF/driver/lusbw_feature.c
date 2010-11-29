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

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SetClearFeature)
#endif

NTSTATUS SetClearFeature(IN	 PDEVICE_CONTEXT deviceContext,
                         IN	 WDF_USB_BMREQUEST_RECIPIENT recipient,
                         IN  USHORT featureIndex,
                         IN  USHORT feature,
                         IN  BOOLEAN setFeature,
                         IN  INT timeout)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_USB_CONTROL_SETUP_PACKET controlSetupPacket;
	WDF_REQUEST_SEND_OPTIONS  syncReqOptions;

	PAGED_CODE();

	if (timeout > LIBUSB_MAX_CONTROL_TRANSFER_TIMEOUT || !timeout)
	{
		timeout = LIBUSB_MAX_CONTROL_TRANSFER_TIMEOUT;
	}

	USBMSG("[%s] recipient=%04d featureIndex=%04d feature=%04d timeout=%d\n",
	       setFeature ? "Set" : "Clear", recipient, featureIndex, feature, timeout);

	WDF_USB_CONTROL_SETUP_PACKET_INIT_FEATURE(&controlSetupPacket, recipient, feature, featureIndex, setFeature);

	WDF_REQUEST_SEND_OPTIONS_INIT(&syncReqOptions, 0);
	WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&syncReqOptions, WDF_REL_TIMEOUT_IN_MS(timeout));

	status = WdfUsbTargetDeviceSendControlTransferSynchronously(
	             deviceContext->WdfUsbTargetDevice,
	             WDF_NO_HANDLE,
	             &syncReqOptions,
	             &controlSetupPacket,
	             NULL,
	             NULL);

	if (!NT_SUCCESS(status))
	{
		USBERR("failed status=%Xh\n", status);
	}
	return status;
}
