/*!********************************************************************
libusbK - WDF USB driver.
Copyright (C) 2011 All Rights Reserved.
libusb-win32.sourceforge.net

Development : Travis Robinson  (libusbdotnet@gmail.com)
Testing     : Xiaofan Chen     (xiaofanc@gmail.com)

At the discretion of the user of this library, this software may be
licensed under the terms of the GNU Public License v3 or a BSD-Style
license as outlined in the following files:
* LICENSE-gpl3.txt
* LICENSE-bsd.txt

License files are located in a license folder at the root of source and
binary distributions.
********************************************************************!*/

#include "drv_common.h"

CONST PCHAR PipeTypeStrings[8] = {"Invalid", "Control", "Isochronous", "Bulk", "Interrupt", "invalid", "invalid", "invalid"};
CONST PCHAR DeviceSpeedStrings[8] = {"Low", "Full", "High", "Super", "invalid", "invalid", "invalid", "invalid"};
CONST PCHAR BoolStrings[2] = {"False", "True"};
CONST PCHAR EndpointDirStrings[2] = {"Write", "Read"};
CONST PCHAR BmRequestDirStrings[2] = {"HostToDevice(0)", "DeviceToHost(1)"};
CONST PCHAR BmRequestTypeStrings[4] = {"Standard(0)", "Class(1)", "Vendor(2)", "Unknown(3)"};
CONST PCHAR BmRequestRecipientStrings[4] = {"Device(0)", "Interface(1)", "Endpoint(2)", "Other(3)"};

NTSTATUS GUIDFromWdfString(__in WDFSTRING guidString, __out PGUID guid)
{
	UNICODE_STRING guidStringW;

	WdfStringGetUnicodeString(guidString, &guidStringW);

	return RtlGUIDFromString (&guidStringW, guid);
}

NTSTATUS GetTransferMdl(__in WDFREQUEST Request,
                        __in WDF_REQUEST_TYPE RequestType,
                        __out PMDL* wdmMdl)
{
	return (RequestType == WdfRequestTypeWrite)
	       ? WdfRequestRetrieveInputWdmMdl(Request, wdmMdl)
	       : WdfRequestRetrieveOutputWdmMdl(Request, wdmMdl);
}

NTSTATUS GetTransferMemory(__in WDFREQUEST Request,
                           __in WDF_REQUEST_TYPE RequestType,
                           __out WDFMEMORY* wdfMemory)
{
	return (RequestType == WdfRequestTypeWrite)
	       ? WdfRequestRetrieveInputMemory(Request, wdfMemory)
	       : WdfRequestRetrieveOutputMemory(Request, wdfMemory);
}

NTSTATUS GetTransferBuffer(__in WDFREQUEST Request,
                           __in WDF_REQUEST_TYPE RequestType,
                           __in size_t MinimumRequiredLength,
                           __deref_out_bcount(*Length) PVOID* Buffer,
                           __out_opt size_t* Length)
{
	return (RequestType == WdfRequestTypeWrite)
	       ? WdfRequestRetrieveInputBuffer(Request, MinimumRequiredLength, Buffer, Length)
	       : WdfRequestRetrieveOutputBuffer(Request, MinimumRequiredLength, Buffer, Length);

}
