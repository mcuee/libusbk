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


#include <pshpack1.h>

#define HOTK_PLUG_EVENT VOID

typedef PVOID KHOT_HANDLE;

typedef struct _KHOT_PARAMS
{
	PVOID Context;

	HOTK_PLUG_EVENT (KUSB_API* OnHotPlug)(
	    KHOT_HANDLE HotHandle,
	    struct _KHOT_PARAMS* HotParams,
	    PCKLST_DEV_INFO DeviceInfo,
	    KLST_SYNC_FLAG PlugType);

	HWND UserHwnd;
	UINT UserMessage;

	struct
	{
		CHAR InstanceID[KLST_STRING_MAX_LEN];
		CHAR DeviceInterfaceGUID[KLST_STRING_MAX_LEN];
		CHAR DevicePath[KLST_STRING_MAX_LEN];
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

	PCKLST_DEV_INFO MatchedInfo;

} KHOT_PARAMS;
typedef KHOT_PARAMS* PKHOT_PARAMS;

typedef VOID KUSB_API KHOT_PLUG_EVENT(
    KHOT_HANDLE HotHandle,
    PKHOT_PARAMS HotParams,
    PCKLST_DEV_INFO DeviceInfo,
    KLST_SYNC_FLAG PlugType);
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
