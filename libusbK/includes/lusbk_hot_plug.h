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

typedef struct _KHOT_PARAMS
{
	VOID (KUSB_API* OnHotPlugEvent)(KHOT_HANDLE, struct _KHOT_PARAMS*, PCKLST_DEV_INFO, KHOT_PLUG_TYPE);
	CHAR DevInstIdPatternMatch[MAX_PATH];
	PVOID Context;

	KLST_DEV_INFO Match;
	PCKLST_DEV_INFO Found;

	union
	{
		ULONG _value;
		struct
		{
			unsigned PlugAllOnInit: 1;
			unsigned AllowDupeMatch: 1;
		};
	} Flags;
} KHOT_PARAMS;
typedef KHOT_PARAMS* PKHOT_PARAMS;

typedef VOID KUSB_API KHOT_PLUG_CB(
    __in KHOT_HANDLE Handle,
    __in PKHOT_PARAMS Params,
    __in PKLST_DEV_INFO DeviceInfo,
    __in KHOT_PLUG_TYPE NotificationType);
typedef KHOT_PLUG_CB* PKHOT_PLUG_CB;

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
