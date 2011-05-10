
/*! \page usbk_power_management libusbK Power Management
* libusbK uses the KMDF state machines for power management. Power 
* policies are managed through calls to \ref UsbK_SetPowerPolicy. 
* 
* In order to modify the power behavior of libusbK, default registry 
* settings can be modified in the device's INF. These values must be 
* written to the device specific location in the registry by adding the 
* values in the HW.AddReg section of the INF. 
* 
* The registry values described in the following list can be specified in 
* the device's INF to modify the power behavior. 
* 
* \sa UsbK_GetPowerPolicy
* \sa UsbK_SetPowerPolicy
* 
* \section usbk_system_wake System Wake
* This feature is controlled by the SystemWakeEnabled DWORD registry 
* setting. This value value indicates whether or not the device should be 
* allowed to wake the system from a low power state. 
* 
* \code HKR,,SystemWakeEnabled,0x00010001,1 \endcode
* 
* A value of zero, or the absence of this value indicates that the device 
* is not allowed to wake the system. 
*
* To allow a device to wake the system, set SystemWakeEnabled to a nonzero 
* value. A check box in the device Properties page is automatically 
* enabled so that the user can override the setting. 
* 
* \note Changing the SystemWakeEnabled setting has no affect on selective 
suspend, this registry value only pertains to system suspend. 
* 
* \section usbk_selective_suspend Selective Suspend
* Selective suspend can be disabled by any of several system or libusbK 
* settings. A single setting cannot force libusbK to enable selective 
* suspend. 
* 
* The following power policy settings that are specified in \ref 
* UsbK_SetPowerPolicy's PolicyType parameter affect the behavior of 
* selective suspend: 
* - AUTO_SUSPEND	\n
*   When set to zero, it does not set the device to selective suspend mode. 
* - SUSPEND_DELAY	\n
*   Sets the time between when the device becomes idle and 
*   when libusbK requests the device to go into selective suspend. 
* 
* The following list shows how the registry keys affect the selective 
* suspend feature:
* - DeviceIdleEnabled
*   - This is a DWORD value. This registry value indicates whether or not the 
*     device is capable of being powered down when idle (Selective Suspend). 
*   - A value of zero, or the absence of this value indicates that the device 
*     does not support being powered down when idle. 
*   - A non-zero value indicates that the device supports being powered down 
*     when idle. 
*   - If DeviceIdleEnabled is not set, the value of the AUTO_SUSPEND power 
*     policy setting is ignored. 
*   - \code HKR,,DeviceIdleEnabled,0x00010001,1 \endcode	\n
* - DeviceIdleIgnoreWakeEnable
*   - When set to a nonzero value, it suspends the device even if it does not 
*     support RemoteWake.
*   - \code HKR,,DeviceIdleIgnoreWakeEnable,0x00010001,1 \endcode	\n
* - UserSetDeviceIdleEnabled
*   - This value is a DWORD value. This registry value indicates whether or 
*     not a check box should be enabled in the device Properties page that 
*     allows a user to override the idle defaults. When 
*     UserSetDeviceIdleEnabled is set to a nonzero value the check box is 
*     enabled and the user can disable powering down the device when idle. A 
*     value of zero, or the absence of this value indicates that the check box 
*     is not enabled.
*   - If the user disables device power savings, the value of the AUTO_SUSPEND 
*     power policy setting is ignored. 
*   - If the user enables device power savings, then the value of AUTO_SUSPEND 
*     is used to determine whether or not to suspend the device when idle. 
*   - The UserSetDeviceIdleEnabled is ignored if DeviceIdleEnabled is not set.
*   - \code HKR,,UserSetDeviceIdleEnabled,0x00010001,1 \endcode	\n
* - DefaultIdleState
*   - This is a DWORD value. This registry value sets the default value of the 
*     AUTO_SUSPEND power policy setting. This registry key is used to enable 
*     or disable selective suspend when a handle is not open to the device. 
*   - A value of zero or the absence of this value indicates that by default, 
*     the device is not suspended when idle. The device be allowed to suspend 
*     when idle only when the AUTO_SUSPEND power policy is enabled. 
*   - A non-zero value indicates that by default the device is allowed to be 
*     suspended when idle. 
*   - This value is ignored if DeviceIdleEnabled is not set.
*   - \code HKR,,DefaultIdleState,0x00010001,1 \endcode	\n
* - DefaultIdleTimeout
*   - This is a DWORD value. This registry value sets the default state of the 
*     SUSPEND_DELAY power policy setting. 
*   - The value indicates the amount of time in milliseconds to wait before 
*     determining that a device is idle. 
*   - \code HKR,,DefaultIdleTimeout,0x00010001,100 \endcode	\n
* 
* \section usbk_detecting_idle Detecting Idle     
* All writes and control transfers force the device into the D0 power 
* state and reset the idle timer. The IN endpoint queues are not power 
* managed. Read requests wake the device when they are submitted. However, 
* a device can become idle while a read request waits. 
* 
*/

#ifndef __LUSBK_USBIO_H_
#define __LUSBK_USBIO_H_

#ifndef __WINUSB_COMPAT_IO_H__
#define __WINUSB_COMPAT_IO_H__

#ifndef __WUSBIO_H__

// pipe policy types ///////////////
#define SHORT_PACKET_TERMINATE  0x01
#define AUTO_CLEAR_STALL        0x02
#define PIPE_TRANSFER_TIMEOUT   0x03
#define IGNORE_SHORT_PACKETS    0x04
#define ALLOW_PARTIAL_READS     0x05
#define AUTO_FLUSH              0x06
#define RAW_IO                  0x07
#define MAXIMUM_TRANSFER_SIZE   0x08
#define RESET_PIPE_ON_RESUME    0x09
// [tr] !NEW! (mainly for testing)
#define MAX_TRANSFER_STAGE_SIZE    0x0F

// Power policy types //////////////
#define AUTO_SUSPEND            0x81
#define SUSPEND_DELAY           0x83

// Device Information types ////////
#define DEVICE_SPEED            0x01

// Device Speeds
#define LowSpeed                0x01
#define FullSpeed               0x02
#define HighSpeed               0x03

typedef struct _WINUSB_PIPE_INFORMATION
{
	USBD_PIPE_TYPE  PipeType;
	UCHAR           PipeId;
	USHORT          MaximumPacketSize;
	UCHAR           Interval;
}* PWINUSB_PIPE_INFORMATION, WINUSB_PIPE_INFORMATION;
C_ASSERT(sizeof(WINUSB_PIPE_INFORMATION) == 12);

#include <pshpack1.h>
typedef struct _WINUSB_SETUP_PACKET
{
	UCHAR   RequestType;
	UCHAR   Request;
	USHORT  Value;
	USHORT  Index;
	USHORT  Length;
}* PWINUSB_SETUP_PACKET, WINUSB_SETUP_PACKET;
C_ASSERT(sizeof(WINUSB_SETUP_PACKET) == 8);
#include <poppack.h>

#endif // __WUSBIO_H__

#endif // __WINUSB_COMPAT_IO_H__

#endif
