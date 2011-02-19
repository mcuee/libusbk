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

#ifndef __DRV_DEVICE_H__
#define __DRV_DEVICE_H__

#include "drv_private.h"

//////////////////////////////////////////////////////////////////////////////
// lusbw_device.c function prototypes.
//
EVT_WDF_DRIVER_DEVICE_ADD Device_OnAdd;
EVT_WDF_DEVICE_PREPARE_HARDWARE Device_OnPrepareHardware;
EVT_WDF_DEVICE_D0_ENTRY Device_OnD0Entry;
EVT_WDF_DEVICE_D0_EXIT Device_OnD0Exit;
EVT_WDF_DEVICE_FILE_CREATE Device_OnFileCreate;
EVT_WDF_FILE_CLOSE Device_OnFileClose;

NTSTATUS Device_Reset(__in WDFDEVICE Device);
NTSTATUS Device_ReadAndSelectDescriptors(__in WDFDEVICE Device);
NTSTATUS Device_Configure(__in WDFDEVICE Device);
NTSTATUS Device_ConfigureInterfaces(__in WDFDEVICE Device);

#endif