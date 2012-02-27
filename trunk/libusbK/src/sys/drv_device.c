/*!********************************************************************
libusbK - WDF USB driver.
Copyright (C) 2012 Travis Lee Robinson. All Rights Reserved.
libusb-win32.sourceforge.net

Development : Travis Lee Robinson  (libusbdotnet@gmail.com)
Testing     : Xiaofan Chen         (xiaofanc@gmail.com)

At the discretion of the user of this library, this software may be
licensed under the terms of the GNU Public License v3 or a BSD-Style
license as outlined in the following files:
* LICENSE-gpl3.txt
* LICENSE-bsd.txt

License files are located in a license folder at the root of source and
binary distributions.
********************************************************************!*/

#include "drv_common.h"

extern volatile LONG DeviceInstanceNumber;

#if (defined(ALLOC_PRAGMA) && defined(PAGING_ENABLED))
#pragma alloc_text(PAGE, Device_OnAdd)
#pragma alloc_text(PAGE, Device_OnPrepareHardware)
#pragma alloc_text(PAGE, Device_OnFileCreate)
#pragma alloc_text(PAGE, Device_OnFileClose)
#pragma alloc_text(PAGE, Device_Create)
#pragma alloc_text(PAGE, Device_FetchConfigDescriptor)
#pragma alloc_text(PAGE, Device_Configure)
#pragma alloc_text(PAGE, Device_Reset)
#endif

NTSTATUS
Device_OnAdd(
    __in WDFDRIVER        Driver,
    __in PWDFDEVICE_INIT  DeviceInit)
/*++
Routine Description:

    EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of the device. All the software resources
    should be allocated in this callback.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
	WDF_FILEOBJECT_CONFIG     fileConfig;
	WDF_PNPPOWER_EVENT_CALLBACKS        pnpPowerCallbacks;
	WDF_OBJECT_ATTRIBUTES               fdoAttributes;
	WDF_OBJECT_ATTRIBUTES               fileObjectAttributes;
	WDF_OBJECT_ATTRIBUTES               requestAttributes;
	NTSTATUS                            status;
	WDFDEVICE                           device;
	WDF_DEVICE_PNP_CAPABILITIES         pnpCaps;
	WDF_IO_QUEUE_CONFIG                 ioQueueConfig;
	PDEVICE_CONTEXT                     deviceContext;
	WDFQUEUE                            queue;
	UNICODE_STRING						symbolicLinkName;
	WCHAR								tmpName[128];
	INT									i;

	UNREFERENCED_PARAMETER(Driver);

	PAGED_CODE();

	//
	// Initialize the pnpPowerCallbacks structure.  Callback events for PNP
	// and Power are specified here.  If you don't supply any callbacks,
	// the Framework will take appropriate default actions based on whether
	// DeviceInit is initialized to be an FDO, a PDO or a filter device
	// object.
	//

	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

	pnpPowerCallbacks.EvtDevicePrepareHardware = Device_OnPrepareHardware;
	pnpPowerCallbacks.EvtDeviceD0Entry = Device_OnD0Entry;
	pnpPowerCallbacks.EvtDeviceD0Exit = Device_OnD0Exit;

	WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

	//
	// Initialize the request attributes to specify the context size and type
	// for every request created by framework for this device.
	//
	WDF_OBJECT_ATTRIBUTES_INIT(&requestAttributes);
	WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&requestAttributes, REQUEST_CONTEXT);

	WdfDeviceInitSetRequestAttributes(DeviceInit, &requestAttributes);

	//
	// Initialize WDF_FILEOBJECT_CONFIG_INIT struct to tell the
	// framework whether you are interested in handle Create, Close and
	// Cleanup requests that gets genereate when an application or another
	// kernel component opens an handle to the device. If you don't register
	// the framework default behaviour would be complete these requests
	// with STATUS_SUCCESS. A driver might be interested in registering these
	// events if it wants to do security validation and also wants to maintain
	// per handle (fileobject) context.
	//

	WDF_FILEOBJECT_CONFIG_INIT(
	    &fileConfig,
	    Device_OnFileCreate,
	    Device_OnFileClose,
	    WDF_NO_EVENT_CALLBACK
	);

	//
	// Specify a context for FileObject. If you register FILE_EVENT callbacks,
	// the framework by default creates a framework FILEOBJECT corresponding
	// to the WDM fileobject. If you want to track any per handle context,
	// use the context for FileObject. Driver that typically use FsContext
	// field should instead use Framework FileObject context.
	//
	WDF_OBJECT_ATTRIBUTES_INIT(&fileObjectAttributes);
	WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&fileObjectAttributes, FILE_CONTEXT);

	WdfDeviceInitSetFileObjectConfig(DeviceInit,
	                                 &fileConfig,
	                                 &fileObjectAttributes);

#if !defined(BUFFERED_READ_WRITE)
	//
	// I/O type is Buffered by default. We want to do direct I/O for Reads
	// and Writes so set it explicitly.
	WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoDirect);

#endif


	WdfDeviceInitSetIoInCallerContextCallback(DeviceInit, Request_PreIoInitialize);

	//
	// Now specify the size of device extension where we track per device
	// context.DeviceInit is completely initialized. So call the framework
	// to create the device and attach it to the lower stack.
	//
	WDF_OBJECT_ATTRIBUTES_INIT(&fdoAttributes);
	WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&fdoAttributes, DEVICE_CONTEXT);

	status = WdfDeviceCreate(&DeviceInit, &fdoAttributes, &device);
	if (!NT_SUCCESS(status))
	{
		USBERR("WdfDeviceCreate failed. status=%Xh\n", status);
		return status;
	}

	//
	// Get the DeviceObject context by using accessor function specified in
	// the WDF_DECLARE_CONTEXT_TYPE_WITH_NAME macro for DEVICE_CONTEXT.
	//

	deviceContext = GetDeviceContext(device);
	deviceContext->WdfDevice = device;
	deviceContext->InstanceNumber = InterlockedIncrement(&DeviceInstanceNumber);

	status = Registry_ReadAllDeviceKeys(deviceContext);
	if (!NT_SUCCESS(status))
	{
		USBERR("Registry_ReadAllDeviceKeys failed. status=%Xh\n", status);
		return status;
	}

	/* create the libusb-win32 legacy symbolic link */
	for (i = LIBUSB_MAX_NUMBER_OF_DEVICES - 1; i > 1; i--)
	{
		RtlZeroMemory(tmpName, sizeof(tmpName));

		/* initialize some unicode strings */
		_snwprintf(tmpName,
		           sizeof(tmpName) / sizeof(WCHAR),
		           L"%s%04d", LIBUSB_SYMBOLIC_LINK_NAME, i);

		RtlInitUnicodeString(&symbolicLinkName, tmpName);

		status = WdfDeviceCreateSymbolicLink(device, &symbolicLinkName);
		if (NT_SUCCESS(status))
		{
			USBMSG("[dev-id=#%d] SymbolicLink=%s%04d\n",
			       deviceContext->InstanceNumber, LIBUSB_SYMBOLIC_LINK_NAMEA, i);
			break;
		}

		/* continue until an unused device name is found */
	}

	if (!NT_SUCCESS(status))
	{
		USBERR("[dev-id=#%d] creating symbolic link failed. status=%Xh\n", deviceContext->InstanceNumber, status);
		return status;
	}

	//
	// Tell the framework to set the SurpriseRemovalOK in the DeviceCaps so
	// that you don't get the popup in usermode (on Win2K) when you surprise
	// remove the device.
	//
	WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCaps);
	pnpCaps.SurpriseRemovalOK = WdfTrue;

	WdfDeviceSetPnpCapabilities(device, &pnpCaps);

	/*
	Create the default queue. The default queue will be a power managed parallel queue.
	While requests are pending in the default queue the device will not suspend.
	*/
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchParallel);

	ioQueueConfig.EvtIoRead = DefaultQueue_OnRead;
	ioQueueConfig.EvtIoWrite = DefaultQueue_OnWrite;
	ioQueueConfig.EvtIoDeviceControl = DefaultQueue_OnIoControl;

	status = WdfIoQueueCreate(device,
	                          &ioQueueConfig,
	                          WDF_NO_OBJECT_ATTRIBUTES,
	                          &queue);// pointer to default queue
	if (!NT_SUCCESS(status))
	{
		USBERR("WdfIoQueueCreate failed  for Default Queue. status=%Xh\n", status);
		return status;
	}

	USBD_GetUSBDIVersion(&deviceContext->UsbVersionInfo);

	//
	// Register the DeviceInterfaceGUIDs set in the devices registry key
	//
	if (deviceContext->DeviceRegSettings.DeviceInterfaceGUIDs != WDF_NO_HANDLE)
	{
		WDFCOLLECTION collection = deviceContext->DeviceRegSettings.DeviceInterfaceGUIDs;
		ULONG count = WdfCollectionGetCount(collection);
		ULONG index;
		for (index = 0; index < count; index++)
		{
			WDFSTRING guidString = WdfCollectionGetItem(collection, index);
			if (guidString != WDF_NO_HANDLE)
			{
				GUID guid = {0};
				if (NT_SUCCESS(GUIDFromWdfString(guidString, &guid)))
				{
					status = WdfDeviceCreateDeviceInterface(device,
					                                        (LPGUID) &guid,
					                                        NULL);// Reference String
					if (!NT_SUCCESS(status))
					{
						USBERR("WdfDeviceCreateDeviceInterface failed. status=%Xh\n", status);
						return status;
					}
					else
					{
						USBMSG("[dev-id=#%d] assigned DeviceInterfaceGUID {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}.\n",
						       deviceContext->InstanceNumber,
						       guid.Data1,
						       guid.Data2,
						       guid.Data3,
						       guid.Data4[0], guid.Data4[1],
						       guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
					}
				}
			}
		}
	}

	return status;
}

NTSTATUS
Device_OnPrepareHardware(
    __in WDFDEVICE Device,
    __in WDFCMRESLIST ResourceList,
    __in WDFCMRESLIST ResourceListTranslated)
/*++

Routine Description:

    In this callback, the driver does whatever is necessary to make the
    hardware ready to use.  In the case of a USB device, this involves
    reading and selecting descriptors.

    //TODO:

Arguments:

    Device - handle to a device

Return Value:

    NT status value

--*/
{
	NTSTATUS                    status = STATUS_SUCCESS;
	PDEVICE_CONTEXT             deviceContext;

	UNREFERENCED_PARAMETER(ResourceList);
	UNREFERENCED_PARAMETER(ResourceListTranslated);

	deviceContext = GetDeviceContext(Device);

	// Hardware is already prepared.
	if (deviceContext->WdfUsbTargetDevice)
	{
		USBMSGN("Hardware is already prepared. Skipping..");
		return STATUS_SUCCESS;
	}

	// Create target device; fetch device descriptior and information.
	status = Device_Create(Device);
	if (!NT_SUCCESS(status))
	{
		USBERRN("Device_Create Failed. Status=%08Xh", status);
		return status;
	}

	// Fetch and store config descriptor to device context.
	status = Device_FetchConfigDescriptor(Device);
	if (!NT_SUCCESS(status))
	{
		USBERRN("Device_FetchConfigDescriptor Failed. Status=%08Xh", status);
		return status;
	}

	// Select configuration and initialize interfaces.
	status = Device_Configure(Device);
	if (!NT_SUCCESS(status))
	{
		USBERRN("Device_Configure Failed. Status=%08Xh", status);
		return status;
	}

	// Apply power policy settings presented in the registry.
	status = Policy_InitPower(deviceContext);
	if (!NT_SUCCESS(status))
	{
		USBWRNN("Policy_InitPower Failed. Status=%08Xh", status);
		status = STATUS_SUCCESS;
	}
	return status;
}

NTSTATUS Device_OnD0Entry(WDFDEVICE Device, WDF_POWER_DEVICE_STATE WdfPowerDeviceState)
{
	UNREFERENCED_PARAMETER(Device);
	UNREFERENCED_PARAMETER(WdfPowerDeviceState);

	USBMSGN("Entering D0. Leaving D%u.", WdfPowerDeviceState - 1);

	return STATUS_SUCCESS;
}

NTSTATUS Device_OnD0Exit(WDFDEVICE Device, WDF_POWER_DEVICE_STATE WdfPowerDeviceState)
{
	UNREFERENCED_PARAMETER(Device);
	UNREFERENCED_PARAMETER(WdfPowerDeviceState);

	USBMSGN("Leaving D0. Entering D%u.", WdfPowerDeviceState - 1);

	return STATUS_SUCCESS;
}

VOID Device_OnFileCreate(__in WDFDEVICE            Device,
                         __in WDFREQUEST           Request,
                         __in WDFFILEOBJECT        FileObject)
/*++

Routine Description:

    The framework calls a driver's EvtDeviceFileCreate callback
    when the framework receives an IRP_MJ_CREATE request.
    The system sends this request when a user application opens the
    device to perform an I/O operation, such as reading or writing a file.
    This callback is called synchronously, in the context of the thread
    that created the IRP_MJ_CREATE request.

Arguments:

    Device - Handle to a framework device object.
    FileObject - Pointer to fileobject that represents the open handle.
    CreateParams - copy of the create IO_STACK_LOCATION

Return Value:

   NT status code

--*/
{
	NTSTATUS                    status = STATUS_UNSUCCESSFUL;
	PUNICODE_STRING             fileName;
	PFILE_CONTEXT               pFileContext;
	PDEVICE_CONTEXT             deviceContext;
	PPIPE_CONTEXT               pipeContext;

	PAGED_CODE();

	USBMSG("begins\n");

	//
	// initialize variables
	//
	deviceContext = GetDeviceContext(Device);
	pFileContext = GetFileContext(FileObject);
	pFileContext->DeviceContext = deviceContext;

	fileName = WdfFileObjectGetFileName(FileObject);

	if (0 == fileName->Length)
	{
		//
		// opening a device as opposed to pipe.
		//
		status = STATUS_SUCCESS;
	}
	else
	{
		//
		// using direct pipe access.
		//
		pipeContext = Pipe_GetContextFromName(deviceContext, fileName);

		if (pipeContext != NULL)
		{
			//
			// found a match
			//
			pFileContext->PipeID = pipeContext->PipeInformation.EndpointAddress;

			WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pipeContext->Pipe);

			status = STATUS_SUCCESS;
		}
		else
		{
			status = STATUS_INVALID_DEVICE_REQUEST;
		}
	}

	InterlockedIncrement(&deviceContext->OpenedFileHandleCount);
	WdfRequestComplete(Request, status);

	USBMSG("ends\n");

	return;
}

VOID Device_OnFileClose(__in WDFFILEOBJECT FileObject)
{
	PFILE_CONTEXT               pFileContext;

	PAGED_CODE();

	pFileContext = GetFileContext(FileObject);

	if (pFileContext && pFileContext->DeviceContext)
	{
		Interface_ReleaseAll(pFileContext->DeviceContext, WdfFileObjectWdmGetFileObject(FileObject));
		InterlockedDecrement(&pFileContext->DeviceContext->OpenedFileHandleCount);
		USBDBGN("OpenedFileHandleCount=%u", pFileContext->DeviceContext->OpenedFileHandleCount);
	}
}


NTSTATUS Device_Create(__in WDFDEVICE Device)
/*++

Routine Description:

    This routine configures the USB device.
    In this routines we get the device descriptor,
    the configuration descriptor and select the
    configuration.

Arguments:

    Device - Handle to a framework device

Return Value:

    NTSTATUS - NT status value.

--*/
{
	NTSTATUS               status;
	PDEVICE_CONTEXT        deviceContext;
	WDF_USB_DEVICE_INFORMATION  info;

	PAGED_CODE();

	//
	// initialize variables
	//
	deviceContext = GetDeviceContext(Device);

	//
	// Create a usb handle so that we can communicate with the
	// underlying USB stack. The WDFUSBDEVICE handle is used to query,
	// configure, and manage all aspects of the USB device.
	// These aspects include device properties, bus properties,
	// and I/O creation and synchronization. We only create device the first
	// the PrepareHardware is called. If the device is restarted by pnp manager
	// for resource rebalance, we will use the same device handle but then select
	// the interfaces again because the USB stack could reconfigure the device on
	// restart.
	//
	if (deviceContext->WdfUsbTargetDevice == NULL)
	{
		status = WdfUsbTargetDeviceCreate(Device,
		                                  WDF_NO_OBJECT_ATTRIBUTES,
		                                  &deviceContext->WdfUsbTargetDevice);
		if (!NT_SUCCESS(status))
		{
			USBERR("WdfUsbTargetDeviceCreate failed. status=%08Xh\n", status);
			return status;
		}
	}

	WdfUsbTargetDeviceGetDeviceDescriptor(deviceContext->WdfUsbTargetDevice,
	                                      &deviceContext->UsbDeviceDescriptor);

	ASSERT(deviceContext->UsbDeviceDescriptor.bNumConfigurations);

	WDF_USB_DEVICE_INFORMATION_INIT(&info);

	//
	// Retrieve USBD version information, port driver capabilites and device
	// capabilites such as speed, power, etc.
	//
	deviceContext->DeviceSpeed = UsbLowSpeed;
	deviceContext->RemoteWakeCapable = FALSE;
	deviceContext->SelfPowered = FALSE;

	status = WdfUsbTargetDeviceRetrieveInformation(deviceContext->WdfUsbTargetDevice, &info);
	if (NT_SUCCESS(status))
	{
		if ((info.Traits & WDF_USB_DEVICE_TRAIT_AT_HIGH_SPEED))
		{
			deviceContext->DeviceSpeed = UsbHighSpeed;
		}
		if ((info.Traits & WDF_USB_DEVICE_TRAIT_REMOTE_WAKE_CAPABLE))
		{
			deviceContext->RemoteWakeCapable = TRUE;
		}
		if ((info.Traits & WDF_USB_DEVICE_TRAIT_SELF_POWERED))
		{
			deviceContext->SelfPowered = TRUE;
		}
	}
	else
	{
		USBWRN("WdfUsbTargetDeviceRetrieveInformation failed, status=%Xh. Cannot properly determine device speed.\n", status);
	}

	// The winusb device speed policy is offet by speed+1.
	GetPolicyValue(DEVICE_SPEED, deviceContext->DevicePolicy) = deviceContext->DeviceSpeed + 1;

	USBMSG("DeviceSpeed=%s RemoteWakeCapable=%s SelfPowered=%s\n",
	       GetDeviceSpeedString(deviceContext->DeviceSpeed),
	       GetBoolString(deviceContext->RemoteWakeCapable),
	       GetBoolString(deviceContext->SelfPowered));

	return status;
}

NTSTATUS Device_FetchConfigDescriptor(__in WDFDEVICE Device)
/*++

Routine Description:

    This helper routine reads the configuration descriptor
    for the device in couple of steps.

Arguments:

    Device - Handle to a framework device

Return Value:

    NTSTATUS - NT status value

--*/
{
	USHORT                        size = 0;
	NTSTATUS                      status;
	PDEVICE_CONTEXT               deviceContext;
	PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor;
	WDF_OBJECT_ATTRIBUTES attributes;
	WDFMEMORY   memory;

	PAGED_CODE();

	//
	// initialize the variables
	//
	configurationDescriptor = NULL;
	deviceContext = GetDeviceContext(Device);

	//
	// Read the first configuration descriptor
	// This requires two steps:
	// 1. Ask the WDFUSBDEVICE how big it is
	// 2. Allocate it and get it from the WDFUSBDEVICE
	//
	status = WdfUsbTargetDeviceRetrieveConfigDescriptor(deviceContext->WdfUsbTargetDevice,
	         NULL,
	         &size);

	if (status != STATUS_BUFFER_TOO_SMALL || size == 0)
	{
		USBERR("WdfUsbTargetDeviceRetrieveConfigDescriptor failed. status=%08Xh\n", status);
		return status;
	}

	//
	// Create a memory object and specify usbdevice as the parent so that
	// it will be freed automatically.
	//
	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

	attributes.ParentObject = deviceContext->WdfUsbTargetDevice;
	deviceContext->ConfigurationDescriptorSize = 0;

	status = WdfMemoryCreate(&attributes,
	                         NonPagedPool,
	                         POOL_TAG,
	                         size,
	                         &memory,
	                         &configurationDescriptor);
	if (!NT_SUCCESS(status))
	{
		USBERR("WdfMemoryCreate failed. size=%u status=%08Xh\n", size, status);
		return status;
	}

	status = WdfUsbTargetDeviceRetrieveConfigDescriptor(deviceContext->WdfUsbTargetDevice,
	         configurationDescriptor,
	         &size);

	if (!NT_SUCCESS(status))
	{
		USBERR("WdfUsbTargetDeviceRetrieveConfigDescriptor failed. status=%08Xh\n", status);
		return status;
	}

	deviceContext->ConfigurationDescriptorSize = size;
	deviceContext->UsbConfigurationDescriptor = configurationDescriptor;
	deviceContext->ConfigDescriptorIndex = 0;

	return status;
}

NTSTATUS Device_Configure(WDFDEVICE Device)
{
	WDF_USB_DEVICE_SELECT_CONFIG_PARAMS		params;
	PWDF_USB_INTERFACE_SETTING_PAIR			settingPairs = NULL;
	NTSTATUS								status;
	PDEVICE_CONTEXT							deviceContext;
	PINTERFACE_CONTEXT						interfaceContext;
	UCHAR									interfaceIndex;

	PAGED_CODE();

	deviceContext = GetDeviceContext(Device);

	deviceContext->InterfaceCount = WdfUsbTargetDeviceGetNumInterfaces(deviceContext->WdfUsbTargetDevice);

	if (deviceContext->InterfaceCount == 1)
	{
		USBDBG("Using single interface configuration..\n");
		WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE(&params);
	}
	else
	{
		USBDBG("Using multiple interface configuration..\n");
		settingPairs = ExAllocatePoolWithTag(
		                   PagedPool,
		                   sizeof(WDF_USB_INTERFACE_SETTING_PAIR) * deviceContext->InterfaceCount,
		                   POOL_TAG);

		if (settingPairs == NULL)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			USBERR("ExAllocatePoolWithTag failed. size=%u status=%08Xh\n",
			       sizeof(WDF_USB_INTERFACE_SETTING_PAIR) * deviceContext->InterfaceCount,
			       status);
			return status;
		}

		for (interfaceIndex = 0; interfaceIndex < deviceContext->InterfaceCount; interfaceIndex++)
		{
			settingPairs[interfaceIndex].UsbInterface =
			    WdfUsbTargetDeviceGetInterface(deviceContext->WdfUsbTargetDevice, interfaceIndex);

			if (settingPairs[interfaceIndex].UsbInterface == WDF_NO_HANDLE)
			{
				USBDBG("WdfUsbTargetDeviceGetInterface failed. interfaceIndex=%u\n", interfaceIndex);
			}
			// Select alternate setting zero on all interfaces.
			settingPairs[interfaceIndex].SettingIndex = 0;
		}

		WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_MULTIPLE_INTERFACES(&params, deviceContext->InterfaceCount, settingPairs);
	}
	status = WdfUsbTargetDeviceSelectConfig(deviceContext->WdfUsbTargetDevice, NULL, &params);
	if (!NT_SUCCESS(status))
	{
		USBERR("WdfUsbTargetDeviceSelectConfig failed. status=%Xh", status);
		goto Error;
	}
	else
	{
		// get the interface count again; this should be the same as before since KMDF only supports
		// the first configuration.
		//
		deviceContext->InterfaceCount = WdfUsbTargetDeviceGetNumInterfaces(deviceContext->WdfUsbTargetDevice);
		status = Pipe_InitDefaultContext(deviceContext);
		if (!NT_SUCCESS(status))
		{
			USBERR("failed initializing default pipe! status=%Xh", status);
			goto Error;
		}
		for (interfaceIndex = 0; interfaceIndex < deviceContext->InterfaceCount; interfaceIndex++)
		{
			interfaceContext = &deviceContext->InterfaceContext[interfaceIndex];
			interfaceContext->InterfaceIndex = interfaceIndex;

			status = Interface_InitContext(deviceContext, interfaceContext);
			if (!NT_SUCCESS(status))
			{
				USBERR("Interface_InitContext failed. status=%Xh", status);
				deviceContext->InterfaceCount = interfaceIndex;
				goto Error;
			}

			status = Interface_Start(deviceContext, interfaceContext);
			if (!NT_SUCCESS(status))
			{
				USBERR("Interface_Start failed. status=%Xh", status);
				deviceContext->InterfaceCount = interfaceIndex;
				goto Error;
			}
		}
	}

Error:
	if (settingPairs)
	{
		ExFreePoolWithTag(settingPairs, POOL_TAG);
		settingPairs = NULL;
	}
	return status;
}

// This routine calls WdfUsbTargetDeviceResetPortSynchronously to reset the device if it's still connected.
//
NTSTATUS Device_Reset(__in WDFDEVICE Device)
{
	PDEVICE_CONTEXT deviceContext;
	NTSTATUS status;

	PAGED_CODE();

	deviceContext = GetDeviceContext(Device);

	//
	// A reset-device
	// request will be stuck in the USB until the pending transactions
	// have been canceled. Similarly, if there are pending tranasfers on the BULK
	// IN/OUT pipe cancel them.
	// To work around this issue, the driver should stop the continuous reader
	// (by calling WdfIoTargetStop) before resetting the device, and restart the
	// continuous reader (by calling WdfIoTargetStart) after the request completes.
	//
	Pipe_StopAll(deviceContext);

	//
	// It may not be necessary to check whether device is connected before
	// resetting the port.
	//
	status = WdfUsbTargetDeviceIsConnectedSynchronous(deviceContext->WdfUsbTargetDevice);

	if(NT_SUCCESS(status))
	{
		status = WdfUsbTargetDeviceResetPortSynchronously(deviceContext->WdfUsbTargetDevice);
		USBMSG("WdfUsbTargetDeviceResetPortSynchronously status=%Xh\n", status);
	}
	else
	{
		USBMSG("WdfUsbTargetDeviceIsConnectedSynchronous status=%Xh\n", status);
	}

	Pipe_StartAll(deviceContext);


	return status;
}


