/*! \file drv_device.h
*/

#ifndef __DRV_DEVICE_H__
#define __DRV_DEVICE_H__

#include "drv_private.h"

//////////////////////////////////////////////////////////////////////////////
// lusbk_device.c function prototypes.
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