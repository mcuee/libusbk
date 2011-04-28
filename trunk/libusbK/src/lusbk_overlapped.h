/*! \file lusbk_overlapped.h
*/

#ifndef __LUSBK_OVERLAPPED_
#define __LUSBK_OVERLAPPED_

#include <windows.h>
#include <stddef.h>
#include <objbase.h>

/*! \addtogroup overlappedK Asynchronous I/O (OverlappedK)
 *  TODO:
 *  @{
 */

#define DEFAULT_POOL_MAX_COUNT (0x100)

/*! \addtogroup overlappedK_structs Structs
 *  OverlappedK typedefs, structs, enums, and defines.
 *  @{
 */

typedef LPOVERLAPPED POVERLAPPED_K;
typedef VOID* POVERLAPPED_K_POOL;

typedef BOOL OVERLAPPED_K_CANCEL_CB (__in POVERLAPPED_K Overlapped);
typedef OVERLAPPED_K_CANCEL_CB* POVERLAPPED_K_CANCEL_CB;

typedef enum _OVERLAPPEDK_WAIT_FLAGS
{
	WAIT_FLAGS_NONE							= 0,

	WAIT_FLAGS_ON_SUCCESS_RELEASE			= 0x0001,
	WAIT_FLAGS_ON_FAIL_RELEASE				= 0x0002,
	WAIT_FLAGS_ON_SUCCESS_OR_FAIL_RELEASE	= 0x0003,

	WAIT_FLAGS_ON_TIMEOUT_CANCEL			= 0x0004,
	WAIT_FLAGS_ON_TIMEOUT_RELEASE			= WAIT_FLAGS_ON_TIMEOUT_CANCEL | 0x0008,

	WAIT_FLAGS_ALWAYS_RELEASE				= WAIT_FLAGS_ON_SUCCESS_OR_FAIL_RELEASE | WAIT_FLAGS_ON_TIMEOUT_RELEASE,

} OVERLAPPEDK_WAIT_FLAGS, *POVERLAPPEDK_WAIT_FLAGS;

typedef struct _OVERLAPPED_K_INFO
{
	HANDLE DeviceHandle;
	HANDLE InterfaceHandle;
	UCHAR PipeID;
	PUCHAR DataBuffer;
	ULONG DataBufferSize;

	UCHAR BackendContext[KUSB_CONTEXT_SIZE];

	POVERLAPPED_K_CANCEL_CB CancelOverlappedCB;
} OVERLAPPED_K_INFO, *POVERLAPPED_K_INFO;

/*! @} */

#ifdef __cplusplus
extern "C" {
#endif
	/*! \addtogroup overlappedK_API Functions
	*  OverlappedK asynchronous I/O functions.
	*  @{
	*/

//! Gets a pre-allocated K overlapped structure from the specified/default pool.
	/*!
	* \param Pool
	* The overlapped pool used to retrieve the next free K overlapped. If Pool is NULL,
	* the default pool is used.
	*
	* \returns On success, the next unused overlappedK available in the pool.  Otherwise NULL.
	* Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP POVERLAPPED_K KUSB_API OvlK_Acquire(
	    __in_opt POVERLAPPED_K_POOL Pool);

//! Returns a used overlapped to it's parent pool.
	/*!
	* When an overlapped is returned to pool, it resources remain allocated.
	* It is added to the list tail of an internal pool list of refurbished overlaps.
	* The \ref OvlK_Acquire function will always choose an overlapped from the refurbished list first.
	*
	* \param OverlappedK
	* The overlappedK to release.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API OvlK_Release(
	    __in POVERLAPPED_K OverlappedK);

//! Frees the specified pool and resources it is currently using.
	/*!
	*
	* \param Pool
	* The overlapped pool used to free.  Once freed it can no longer be used.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API OvlK_FreePool(
	    __in POVERLAPPED_K_POOL Pool);

//! Creates a new overlapped pool.
	/*!
	*
	* \param MaxOverlappedCount
	* Number of overkappedK structures to allocate.
	* This value must be 2, 4, 8, 16, 32, 64, 128, 256 or 512.
	*
	* \returns On success, the newly created overlapped pool.  Otherwise NULL.
	* Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP POVERLAPPED_K_POOL KUSB_API OvlK_CreatePool(
	    __in USHORT MaxOverlappedCount);

//! creates the default overlapped pool.
	/*!
	* The default pool is always created when the application starts.
	* It is only neccessary to create the default pool when statically linked to libusbK.lib.
	* Applications can have only one (1) default pool.
	*
	* \returns The default overlapped pool.
	*/
	KUSB_EXP POVERLAPPED_K KUSB_API OvlK_CreateDefaultPool();

//! Frees the default overlapped pool and resources it is currently using.
	/*!
	* The internal pool is always freed when the application exists.
	* It is only neccessary to free the default pool when statically linked to libusbK.lib.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API OvlK_FreeDefaultPool();

//! Returns the internal event handle used to signal IO operations.
	/*!
	*
	* \param OverlappedK
	* The overlappedK used to return the internal event handle.
	*
	* \returns On success, The manual reset event handle being used by this overlappedK.  Otherwise NULL.
	* Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP HANDLE KUSB_API OvlK_GetEventHandle(
	    __in POVERLAPPED_K OverlappedK);

//! Returns detailed overlappedK information.
	/*!
	*
	* \param OverlappedK
	* The overlappedK to return information for.
	*
	* \returns On success, A pointer to the internal overlappedK information structure. Otherwise NULL.
	* Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP POVERLAPPED_K_INFO KUSB_API OvlK_GetInfo(
	    __in POVERLAPPED_K OverlappedK);

//! Returns the overlappedK context space.
	/*!
	* The user context can be used in anyway you wish.  (up to 32 byes of storage)
	* User context space is never erased; even when the overlapped is released.
	*
	* \param OverlappedK
	* The overlappedK used to return the user context space pointer.
	*
	* \returns On success, A pointer to the user context space. Otherwise NULL.
	* Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP PKUSB_USER_CONTEXT KUSB_API OvlK_GetContext(
	    __in POVERLAPPED_K OverlappedK);

//! Returns the pool context space.
	/*!
	* The user context can be used in anyway you wish.  (up to 32 byes of storage)
	* User context space is never erased; even when the overlapped is released.
	*
	* \param Pool
	* The pool used to return the user context space pointer.
	* If this parameter is NULL, a pointer to the default pool
	* context space is returned.
	*
	* \returns On success, A pointer to the user context space. Otherwise NULL.
	* Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP PKUSB_USER_CONTEXT KUSB_API OvlK_GetPoolContext(
	    __in_opt POVERLAPPED_K_POOL Pool);

//! Waits for overlapped completion.
	/*!
	*
	* \param OverlappedK
	* The overlappedK to wait on.
	*
	* \param TimeoutMS
	* Number of milliseconds to wait for overlapped completion.
	*
	* \param WaitFlags
	* See /ref OVERLAPPEDK_WAIT_FLAGS
	*
	* \param TransferredLength
	* On success, returns the number of bytes transferred by this overlappedK.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API OvlK_WaitComplete(
	    __in POVERLAPPED_K OverlappedK,
	    __in_opt DWORD TimeoutMS,
	    __in_opt OVERLAPPEDK_WAIT_FLAGS WaitFlags,
	    __out PULONG TransferredLength);

//! Checks for overlapped completion. (polling)
	/*!
	* \note This function returns immediately.
	*
	* \param OverlappedK
	* The overlappedK to check for completion.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API OvlK_IsComplete(
	    __in POVERLAPPED_K OverlappedK);

//! Initializes an overlappedK for re-use. The overlappedK is not return to its pool.
	/*!
	*  This function performs the following actions:
	*    - Resets the overlapped event to non-signaled via ResetEvent().
	*    - Clears the internal overlapped information.
	*    - Clears the 'Internal' and 'InternalHigh' members of the windows overlapped structure.
	*
	* \param OverlappedK
	* The overlappedK to re-use.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API KUSB_API OvlK_ReUse(
	    __in POVERLAPPED_K OverlappedK);

	/*! @} */

#ifdef __cplusplus
}
#endif

#endif

/*! @} */
