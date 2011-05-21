/*! \file lusbk_overlapped.h
* \brief structs, typedefs, enums, defines, and functions for asynchronous usb transfers and \c OVERLAPPED I/O management.
*/

#ifndef __LUSBK_OVERLAPPED_
#define __LUSBK_OVERLAPPED_

#include <windows.h>
#include <stddef.h>
#include <objbase.h>


#define DEFAULT_POOL_MAX_COUNT (0x100)

/*! \addtogroup ovlk
 *  @{
 */

//! pointer to an OverlappedK structure.
/*!
* To obtain a pointer to an OverlappedK structure call \ref OvlK_Acquire.
* To release an OverlappedK structure back to its parent pool,
* call \ref OvlK_Release.
*
* \remarks
* OverlappedK structures contain a "standard" windows \c OVERLAPPED
* structure as a first field. This makes them compatible with
* windows api function which take a \ref LPOVERLAPPED as a parameter.
* However, in-order to make use of the OverlappedK functions (such as \ref
* OvlK_WaitComplete and \ref OvlK_IsComplete) the \ref POVERLAPPED_K must
* pass through one of the libusbK \ref usbk transfer functions. e.g. \ref UsbK_ReadPipe
* and \ref UsbK_WritePipe
*
* \note
* With the exception of the following, all overlapped parameters exposed
* by the libusbK API support a \ref POVERLAPPED_K.
* - \ref UsbK_GetOverlappedResult
*
*/
typedef LPOVERLAPPED POVERLAPPED_K;


//! pointer to an OverlappedK pool structure.
/*!
* An OverlappedK pool encompasses an array of OverlappedK structures.
*
*/
typedef VOID* POVERLAPPED_K_POOL;

typedef BOOL OVERLAPPED_K_CANCEL_CB (__in POVERLAPPED_K Overlapped);
typedef OVERLAPPED_K_CANCEL_CB* POVERLAPPED_K_CANCEL_CB;

//! \c WaitFlags used by \ref OvlK_WaitComplete.
/*!
 *
*/
typedef enum _OVERLAPPEDK_WAIT_FLAGS
{
	//! Do not perform any additional actions upon exiting \ref OvlK_WaitComplete.
	WAIT_FLAGS_NONE							= 0,

	//! If the i/o operation completes successfully, release the OverlappedK back to it's pool.
	WAIT_FLAGS_ON_SUCCESS_RELEASE			= 0x0001,

	//! If the i/o operation fails, release the OverlappedK back to it's pool.
	WAIT_FLAGS_ON_FAIL_RELEASE				= 0x0002,

	//! If the i/o operation fails or completes successfully, release the OverlappedK back to its pool. Perform no actions if it times-out.
	WAIT_FLAGS_ON_SUCCESS_OR_FAIL_RELEASE	= 0x0003,

	//! If the i/o operation times-out cancel it, but do not release the OverlappedK back to its pool.
	WAIT_FLAGS_ON_TIMEOUT_CANCEL			= 0x0004,

	//! If the i/o operation times-out, cancel it and release the OverlappedK back to its pool.
	WAIT_FLAGS_ON_TIMEOUT_RELEASE			= WAIT_FLAGS_ON_TIMEOUT_CANCEL | 0x0008,

	//! Always release the OverlappedK back to its pool.  If the operation timed-out, cancel it before releasing back to its pool.
	WAIT_FLAGS_ALWAYS_RELEASE				= WAIT_FLAGS_ON_SUCCESS_OR_FAIL_RELEASE | WAIT_FLAGS_ON_TIMEOUT_RELEASE,

} OVERLAPPEDK_WAIT_FLAGS;

//! Structure representing internal \ref POVERLAPPED_K information.
/* \sa OvlK_GetInfo
 * \note
 * This structure is populated after it is passes through one of the libusbK \ref usbk transfer functions.
 * e.g. \ref UsbK_ReadPipe and \ref UsbK_WritePipe
*/
typedef struct _OVERLAPPED_K_INFO
{
	//! Device file handle.
	HANDLE DeviceHandle;

	//! Back-end specific interface handle.
	HANDLE InterfaceHandle;

	//! USB endpoint address for the i/o operation. (if applicable)
	UCHAR PipeID;

	//! Data buffer used for i/o operations.
	PUCHAR DataBuffer;

	//! Size in bytes of \c DataBuffer.
	ULONG DataBufferSize;

	//! Reserved storage space for the various libusbK driver back-ends. (for internal use only).
	UCHAR BackendContext[KUSB_CONTEXT_SIZE];

	//! Callback function used to abort the i/o operation. (for internal use only).
	POVERLAPPED_K_CANCEL_CB Cancel;

}* POVERLAPPED_K_INFO, OVERLAPPED_K_INFO;

#ifdef __cplusplus
extern "C" {
#endif

//! Gets a pre-allocated K overlapped structure from the specified/default pool.
	/*!
	* \param Pool
	* The overlapped pool used to retrieve the next free K overlapped.
	* \note To acquire an OverlappedK from the \b default pool, pass \c NULL for the \c Pool parameter.
	*
	* After calling \ref OvlK_Acquire or \ref OvlK_ReUse the OverlappedK is ready for use.
	* Simply pass it into one of the libusbK \ref usbk transfer functions.
	* e.g. \ref UsbK_ReadPipe and \ref UsbK_WritePipe
	*
	* If possible, the \ref OvlK_Acquire function will choose an overlapped
	* from the refurbished list first. The refurbished list will be skipped under the following conditions:
	* - It is empty.
	* - Another thread is busy accessing it.
	*   In this scenario the OverlappedK is acquired directly from its pools master overlapped array.
	*
	* \returns On success, the next unused overlappedK available in the pool.  Otherwise NULL.
	* Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP POVERLAPPED_K KUSB_API OvlK_Acquire(
	    __in_opt POVERLAPPED_K_POOL Pool);

//! Returns a used overlapped to it's parent pool.
	/*!
	* \param OverlappedK
	* The overlappedK to release.
	*
	* When an overlapped is returned to pool, it resources are \b not freed.
	* Instead, it is added to an internal list of refurbishable OverlappedKs.
	* This list is maintained in its \ref POVERLAPPED_K_POOL
	* see also \ref OvlK_Acquire
	*
	* \warning
	* Only call this function if the OverlappedK is not in-use.
	* This does not include an OverlappedK which has timed-out (unless it has also been cancelled).
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API OvlK_Release(
	    __in POVERLAPPED_K OverlappedK);

//! Destroys the specified pool and all resources it created.
	/*!
	*
	* \param Pool
	* The overlapped pool used to destroy.  Once destroyed, the pool and all resources which
	* belong to it can no longer be used.
	*
	* \warning
	* A pool should not be destroyed until all OverlappedKs acquired from it
	* are no longer in-use. For more information see \ref OvlK_Release.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API OvlK_DestroyPool(
	    __in POVERLAPPED_K_POOL Pool);

//! Creates a new overlapped pool.
	/*!
	*
	* \param MaxOverlappedCount
	* Number of overkappedK structures to allocate. If \c MaxOverlappedCount
	* exceeds 512, \c OvlK_CreatePool sets last error to ERROR_RANGE_NOT_FOUND
	* and returns NULL.
	*
	* \note
	* Additional pools may increase performance for multithreaded applications.
	* Because all pools are thread safe, some \c OvlK functions must lock access to internal
	* data at certain points of operation.
	* Each OverlappedK pool has its own lock, if each thread has its own pool it can be guaranteed
	* of no lock collisions. (when one thread must wait for another)
	*
	* \returns On success, the newly created overlapped pool.  Otherwise NULL.
	* Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP POVERLAPPED_K_POOL KUSB_API OvlK_CreatePool(
	    __in USHORT MaxOverlappedCount);

//! creates the default overlapped pool.
	/*!
	* The default pool is automatically initialized when the first OverlappedK is acquired from it.
	* The default pool resides in static memory.
	* There can be only one (1) default pool per application.
	*
	* It is generally not neccessary to create the default pool.
	*
	* \returns The default overlapped pool.
	*/
	KUSB_EXP POVERLAPPED_K_POOL KUSB_API OvlK_CreateDefaultPool();

//! Frees the default overlapped pool and resources it is currently using.
	/*!
	* The default pool resources are released when the users application exits.
	*
	* It is generally not neccessary to free the default pool.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API OvlK_DestroyDefaultPool();

//! Returns the internal event handle used to signal IO operations.
	/*!
	*
	* \param OverlappedK
	* The overlappedK used to return the internal event handle.
	*
	* \ref OvlK_GetEventHandle is useful for applications that must to their own event handling.
	* It exposes the windows \c OVERLAPPED \c hEvent used for i/o completion signaling.
	* This event handle can be used by the standard event wait functions;
	* /c WaitForMultipleObjectsEx for example.
	*
	* \warning
	* Use \ref OvlK_GetEventHandle with caution. Event handles returned by this function should never
	* be used unless the OverlappedK has been \b acquired by the application.
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
	* Internal overlapped information is semi-backend specific.
	* /sa OVERLAPPED_K_INFO
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

//! Checks for overlapped completion returns immediately. (polling)
	/*!
	*
	* \param OverlappedK
	* The overlappedK to check for completion.
	*
	* \warning
	* \ref OvlK_IsComplete does \b no validation on the OverlappedK.
	* It's purpose is to check the event signal state as fast as possible.
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
	* \note
	* Re-using OverlappedKs is the most efficient method in terms of OverlappedK management.
	* When an OverlappedK is "re-used" it is not returned to the pool.  Intead, the application
	* retains owner ship for use in another i/o operation.
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
