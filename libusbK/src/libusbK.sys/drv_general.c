
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

NTSTATUS AddDefaultDeviceInterfaceGUID(__in PDEVICE_CONTEXT deviceContext)
{

	WDF_OBJECT_ATTRIBUTES	stringAttributes;
	UNICODE_STRING defaultDeviceInterfaceGUID_UnicodeString;
	WDFSTRING defaultDeviceInterfaceGUID = NULL;
	GUID guidTest = {0};
	NTSTATUS status;

	WDF_OBJECT_ATTRIBUTES_INIT(&stringAttributes);

	stringAttributes.ParentObject = deviceContext->DeviceRegSettings.DeviceInterfaceGUIDs;

	RtlInitUnicodeString(&defaultDeviceInterfaceGUID_UnicodeString, DEFINE_TO_STRW(Regsitry_Default_DeviceInterfaceGUID));

	status = WdfStringCreate(&defaultDeviceInterfaceGUID_UnicodeString, &stringAttributes, &defaultDeviceInterfaceGUID);
	if (!NT_SUCCESS(status))
	{
		USBERR("WdfStringCreate failed. status=%Xh\n", status);
		return status;
	}

	status = GUIDFromWdfString(defaultDeviceInterfaceGUID, &guidTest);
	if (!NT_SUCCESS(status))
	{
		USBERR("GUIDFromWdfString failed. status=%Xh\n", status);
		return status;
	}

	USBWRN("using default GUID {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}\n",
	       guidTest.Data1,
	       guidTest.Data2,
	       guidTest.Data3,
	       guidTest.Data4[0], guidTest.Data4[1],
	       guidTest.Data4[2], guidTest.Data4[3], guidTest.Data4[4], guidTest.Data4[5], guidTest.Data4[6], guidTest.Data4[7]);

	return WdfCollectionAdd(deviceContext->DeviceRegSettings.DeviceInterfaceGUIDs, defaultDeviceInterfaceGUID);

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
