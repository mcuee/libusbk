/*! \file lusbk_device_list.h
* \brief structs, typedefs, enums, defines, and functions for usb device enumeration and detection.
*/

#ifndef __LUSBK_DEVICE_LIST_H
#define __LUSBK_DEVICE_LIST_H

#include <windows.h>
#include <objbase.h>
#include "lusbk_common.h"

#include <PSHPACK1.H>

/*! \addtogroup lstk
* @{
*/

//! Device list search/filtering/listing parameters.
/*!
* This structure contains fields that modify the behavior of \ref LstK_GetDeviceList.
*/
typedef struct _KUSB_DEV_LIST_SEARCH
{
	//! Enable listings for the raw device interface GUID.{A5DCBF10-6530-11D2-901F-00C04FB951ED}
	BOOL EnableRawDeviceInterfaceGuid;

	//! Enable composite device list mode
	/*!
	* When \c EnableCompositeDeviceMode is TRUE, composite devices are merged into a single \ref KUSB_DEV_LIST and
	* \ref KUSB_DEV_LIST::CompositeList is populated with the individual composite device elements.
	*
	*/
	BOOL EnableCompositeDeviceMode;

} KUSB_DEV_LIST_SEARCH;
//! pointer to a \ref KUSB_DEV_LIST_SEARCH
typedef KUSB_DEV_LIST_SEARCH* PKUSB_DEV_LIST_SEARCH;

//! Linked device list item.
/*!
* Device list elements are stored in a double linked list.
* The \b head element is returned by \ref LstK_GetDeviceList.
*
* Use the macro functions in \ref lusbk_linked_list.h to iterate/search these linked lists.
* \warning Do not remove entries or modify the list manually.
*
* All device list elements contain a \ref KUSB_USER_CONTEXT.
* This 32 bytes of user context space can be used by you, the developer, for any desired purpose.
*/
typedef struct _KUSB_DEV_LIST
{
	//! User context area
	KUSB_USER_CONTEXT UserContext;

	//! Driver id this device element is using
	LONG DrvId;

	//! Device interface GUID
	CHAR DeviceInterfaceGUID[MAX_PATH];

	//! Device instance ID
	CHAR DeviceInstance[MAX_PATH];

	//! Class GUID
	CHAR ClassGUID[MAX_PATH];

	//! Manufaturer name as specified in the INF file
	CHAR Mfg[MAX_PATH];

	//! Device description as specified in the INF file
	CHAR DeviceDesc[MAX_PATH];

	//! Driver service name
	CHAR Service[MAX_PATH];

	//! Unique symbolic link identifier
	/*!
	* The \c SymbolicLink can be used to uniquely distinguish between device list elements.
	*/
	CHAR SymbolicLink[MAX_PATH];

	//! physical device filename.
	/*!
	* This path is used with the Windows \c CreateFile() function to obtain on opened device handle.
	*/
	CHAR DevicePath[MAX_PATH];

	//! Internal use only
	DWORD ReferenceCount;

	//! Internal use only
	DWORD Linked;

	//! Internal use only
	DWORD LUsb0SymbolicLinkIndex;

	//! see \ref KUSB_DEV_LIST_SEARCH::EnableCompositeDeviceMode
	struct _KUSB_DEV_LIST* CompositeList;

	//! The next entry in the list, or \c NULL.
	struct _KUSB_DEV_LIST* next;

	//! The previos entry in the list, or \c NULL.
	struct _KUSB_DEV_LIST* prev;

	//! Internal use only
	volatile LONG refCount;

	//! Internal use only
	DWORD cbSize;

} KUSB_DEV_LIST;
//! pointer to a \ref KUSB_DEV_LIST
typedef KUSB_DEV_LIST* PKUSB_DEV_LIST;

#include <POPPACK.H>

#ifdef __cplusplus
extern "C" {
#endif

//! Creates a new usb device list.
	/*!
	*
	* \param DeviceList
	* Pointer reference that will receive a a populated device list.
	*
	* \param SearchParameters
	* Search, filter, and listing options.
	*
	* \returns
	* - 0 if no device are found
	* - On success, the number of elements in the device list.
	* - On failure, a negative win32 error code.
	*
	*/
	KUSB_EXP LONG KUSB_API LstK_GetDeviceList(
	    __deref_inout PKUSB_DEV_LIST* DeviceList,
	    __in PKUSB_DEV_LIST_SEARCH SearchParameters);

//! Destroys a usb device list created with \ref LstK_GetDeviceList.
	/*!
	*
	* \param DeviceList
	* The list to free.
	*
	* \returns NONE
	*/
	KUSB_EXP VOID KUSB_API LstK_FreeDeviceList(
	    __deref_inout PKUSB_DEV_LIST* DeviceList);

#ifdef __cplusplus
}
#endif

/*!
* \example example-device-list.c
*
* <B>Device list example</B>:
* -# Creates a usb device list.
* -# Prints a single line description for each device list element.
* -# Searches for the example device hardware id
* -# Prints example device connection status.
* -# Destroys the usb device list created in step #1
*

\par Console Output
\verbatim
USB\VID_04D8&PID_FA2E\LUSBW1: Benchmark Device (Travis Robinson)
Example device connected!
\endverbatim

*/

/*! @} */
#endif
