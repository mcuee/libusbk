/*! \file lusbk_hot_plug.h
* \brief structs, typedefs, enums, defines, and functions for the \ref hotk.
*/
#ifndef __LUSBK_HOT_PLUG_H_
#define __LUSBK_HOT_PLUG_H_

#include <windows.h>
#include <objbase.h>
#include "lusbk_common.h"
#include "lusbk_device_list.h"


/*! \addtogroup hotk
* @{
*/

typedef PVOID KHOT_HANDLE;
typedef enum _KHOT_PLUG_TYPE
{
    KHOT_PLUG_INVALID = 0,
    KHOT_PLUG_UNCHANGED = 1 << 0,
    KHOT_PLUG_ARRIVAL = 1 << 1,
    KHOT_PLUG_REMOVAL = 1 << 2,
} KHOT_PLUG_TYPE;

#include <pshpack1.h>

#define HOTK_PLUG_EVENT VOID

typedef struct _KHOT_PARAMS
{
	PVOID Context;

	HOTK_PLUG_EVENT (KUSB_API* OnHotPlug)(
	    KHOT_HANDLE HotHandle,
	    struct _KHOT_PARAMS* HotParams,
	    PCKLST_DEV_INFO DeviceInfo,
	    KHOT_PLUG_TYPE PlugType);

	HWND UserHwnd;
	UINT UserMessage;

	struct
	{
		CHAR InstanceID[MAX_PATH];
		CHAR DeviceInterfaceGUID[MAX_PATH];
		CHAR DevicePath[MAX_PATH];
	} PatternMatch;

	union
	{
		ULONG Value;
		struct
		{
			unsigned PlugAllOnInit: 1;
			unsigned AllowDupeInstanceIDs: 1;
			unsigned PostUserMessage: 1;
		};
	} Flags;

	PCKLST_DEV_INFO DeviceInfo;

} KHOT_PARAMS;
typedef KHOT_PARAMS* PKHOT_PARAMS;

typedef VOID KUSB_API KHOT_PLUG_EVENT(
    KHOT_HANDLE HotHandle,
    PKHOT_PARAMS HotParams,
    PCKLST_DEV_INFO DeviceInfo,
    KHOT_PLUG_TYPE PlugType);
typedef KHOT_PLUG_EVENT* PKHOT_PLUG_EVENT;

#include <poppack.h>

#ifdef __cplusplus
extern "C" {
#endif

	KUSB_EXP BOOL KUSB_API HotK_Init(
	    __deref_out KHOT_HANDLE* Handle,
	    __in PKHOT_PARAMS InitParams);

	KUSB_EXP BOOL KUSB_API HotK_Free(
	    __deref_inout KHOT_HANDLE* Handle);

	KUSB_EXP VOID KUSB_API HotK_FreeAll(VOID);

#ifdef __cplusplus
}
#endif

/*! @} */
#endif // __LUSBK_HOT_PLUG_H_
