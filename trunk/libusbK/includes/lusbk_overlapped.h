/*! \file lusbk_overlapped.h
* \brief structs, typedefs, enums, defines, and functions for asynchronous usb transfers and \c OVERLAPPED I/O management.
*/

#ifndef __LUSBK_OVERLAPPED_
#define __LUSBK_OVERLAPPED_

#include <windows.h>
#include <stddef.h>
#include <objbase.h>


#define KOVL_MAX_DEFAULT_POOL_COUNT (0x100)

/*! \addtogroup ovlk
 *  @{
 */

//! pointer to an OverlappedK structure.
/*!
*
* To acquire an OverlappedK for use, call \ref OvlK_Acquire. To release an
* OverlappedK back to its associated pool, call \ref OvlK_Release.
*
* \remarks
* OverlappedK structures contain a "standard" windows \c OVERLAPPED
* structure as thier first member. This makes them compatible with windows
* api function which take a \ref LPOVERLAPPED as a parameter. However,
* in-order to make use of the OverlappedK functions (such as \ref
* OvlK_Wait and \ref OvlK_IsComplete) the \ref PKOVL_OVERLAPPED must pass
* through one of the libusbK \ref usbk transfer functions. e.g. \ref
* UsbK_ReadPipe and \ref UsbK_WritePipe
*
*/
typedef LPOVERLAPPED PKOVL_OVERLAPPED;


//! pointer to an OverlappedK pool structure.
/*!
* An OverlappedK pool encompasses an array of OverlappedK structures.
*
*/
typedef VOID* PKOVL_OVERLAPPED_POOL;

typedef BOOL KUSB_API KOVL_OVERLAPPED_CANCEL_CB (__in PKOVL_OVERLAPPED Overlapped);
typedef KOVL_OVERLAPPED_CANCEL_CB* PKOVL_OVERLAPPED_CANCEL_CB;

//! \c WaitFlags used by \ref OvlK_Wait.
/*!
 *
*/
typedef enum _KOVL_WAIT_FLAGS
{
    //! Do not perform any additional actions upon exiting \ref OvlK_Wait.
    WAIT_FLAGS_NONE							= 0,

    //! If the i/o operation completes successfully, release the OverlappedK back to it's pool.
    WAIT_FLAGS_RELEASE_ON_SUCCESS			= 0x0001,

    //! If the i/o operation fails, release the OverlappedK back to it's pool.
    WAIT_FLAGS_RELEASE_ON_FAIL				= 0x0002,

    //! If the i/o operation fails or completes successfully, release the OverlappedK back to its pool. Perform no actions if it times-out.
    WAIT_FLAGS_RELEASE_ON_SUCCESS_FAIL		= 0x0003,

    //! If the i/o operation times-out cancel it, but do not release the OverlappedK back to its pool.
    WAIT_FLAGS_CANCEL_ON_TIMEOUT			= 0x0004,

    //! If the i/o operation times-out, cancel it and release the OverlappedK back to its pool.
    WAIT_FLAGS_RELEASE_ON_TIMEOUT			= WAIT_FLAGS_CANCEL_ON_TIMEOUT | 0x0008,

    //! Always release the OverlappedK back to its pool.  If the operation timed-out, cancel it before releasing back to its pool.
    WAIT_FLAGS_RELEASE_ALWAYS				= WAIT_FLAGS_RELEASE_ON_SUCCESS_FAIL | WAIT_FLAGS_RELEASE_ON_TIMEOUT,

} KOVL_WAIT_FLAGS;

//! Structure representing internal \ref PKOVL_OVERLAPPED information.
/*
*
* \sa OvlK_GetInfo
*
* \note
* This structure is populated after it is passes through one of the
* libusbK \ref \c UsbK core transfer functions such as \ref UsbK_ReadPipe
* or \ref UsbK_WritePipe
*
*/
typedef struct _KOVL_OVERLAPPED_INFO
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
	PKOVL_OVERLAPPED_CANCEL_CB Cancel;

}* PKOVL_OVERLAPPED_INFO, KOVL_OVERLAPPED_INFO;

#ifdef __cplusplus
extern "C" {
#endif

//! Gets a pre-allocated \c OverlappedK structure from the specified/default pool.
	/*!
	*
	* \param Pool
	* The overlapped pool used to retrieve the next available \c OverlappedK,
	* or NULL for the default pool.
	*
	* \c Pool parameter.
	*
	* \returns On success, the next unused overlappedK available in the pool.
	* Otherwise NULL. Use \c GetLastError() to get extended error information.
	*
	* After calling \ref OvlK_Acquire or \ref OvlK_ReUse the \c OverlappedK is
	* ready to be used in an I/O operation. See one of the \c UsbK core
	* transfer functions such as \ref UsbK_ReadPipe or \ref UsbK_WritePipe for
	* more information.
	*
	* If the pools internal refurbished list (a re-usable list of \c
	* OverlappedK structures) is not empty, the \ref OvlK_Acquire function
	* will choose an overlapped from the refurbished list.
	*
	*/
	KUSB_EXP PKOVL_OVERLAPPED KUSB_API OvlK_Acquire(
	    __in_opt PKOVL_OVERLAPPED_POOL Pool);

//! Returns an \c OverlappedK structure to it's pool.
	/*!
	*
	* \param OverlappedK
	* The overlappedK to release.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*
	* When an overlapped is returned to pool, it resources are \b not freed.
	* Instead, it is added to an internal refurbished list (a re-usable list of \c
	* OverlappedK structures).
	*
	* \warning
	* This function must not be called when the OverlappedK is in-use. If
	* unsure, consider using \ref OvlK_WaitAndRelease instead.
	*/
	KUSB_EXP BOOL KUSB_API OvlK_Release(
	    __in PKOVL_OVERLAPPED OverlappedK);


//! Creates a new overlapped pool.
	/*!
	*
	* \param MaxOverlappedCount
	* Number of overkappedK structures to allocate. If \c MaxOverlappedCount
	* exceeds 512, \c OvlK_CreatePool sets last error to ERROR_RANGE_NOT_FOUND
	* and returns NULL.
	*
	* \returns On success, the newly created overlapped pool.  Otherwise NULL.
	* Use \c GetLastError() to get extended error information.
	*
	* \note
	* Additional pools are generally not neccessary, but may increase
	* performance for multithreaded applications.
	*
	* \c OverlappedK pools use a high-speed spin-lock to achieve thread
	* safety. Some of the \c OvlK function hold this spin-lock for a short
	* period to protect internal data.
	*
	*/
	KUSB_EXP PKOVL_OVERLAPPED_POOL KUSB_API OvlK_CreatePool(
	    __in USHORT MaxOverlappedCount);

//! Destroys the specified pool and all resources it created.
	/*!
	*
	*
	* \param Pool
	* The overlapped pool to destroy. Once destroyed, the pool and all
	* resources which belong to it can no longer be used.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*
	* \warning
	* A pool should not be destroyed until all OverlappedKs acquired from it
	* are no longer in-use. For more information see \ref OvlK_WaitAndRelease or \ref OvlK_Release.
	*
	*/
	KUSB_EXP BOOL KUSB_API OvlK_DestroyPool(
	    __in PKOVL_OVERLAPPED_POOL Pool);

//! creates the default overlapped pool.
	/*!
	*
	*
	* \returns The default overlapped pool.
	*
	* The default pool is automatically initialized when the first OverlappedK
	* is acquired from it.
	*
	* The default pool resides in static memory and there is only one (1) per
	* application.
	*
	* It is generally not neccessary to create the default pool.
	*
	*/
	KUSB_EXP PKOVL_OVERLAPPED_POOL KUSB_API OvlK_CreateDefaultPool();

//! Frees the default overlapped pool and resources it is currently using.
	/*!
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*
	* \note
	* The default pool resources are released when the users application
	* exits.
	*
	*/
	KUSB_EXP BOOL KUSB_API OvlK_DestroyDefaultPool();

//! Returns the internal event handle used to signal IO operations.
	/*!
	*
	* \param OverlappedK
	* The overlappedK used to return the internal event handle.
	*
	* \returns On success, The manual reset event handle being used by this
	* overlappedK. Otherwise NULL. Use \c GetLastError() to get extended error
	* information.
	*
	* \ref OvlK_GetEventHandle is useful for applications that must to their
	* own event handling. It exposes the windows \c OVERLAPPED \c hEvent used
	* for i/o completion signaling. This event handle can be used by the
	* standard event wait functions; /c WaitForMultipleObjectsEx for example.
	*
	* \warning Use \ref OvlK_GetEventHandle with caution. Event handles
	* returned by this function should never be used unless the OverlappedK
	* has been \b acquired by the application.
	*
	*/
	KUSB_EXP HANDLE KUSB_API OvlK_GetEventHandle(
	    __in PKOVL_OVERLAPPED OverlappedK);

//! Returns detailed overlappedK information.
	/*!
	*
	* \param OverlappedK
	* The overlappedK to return information for.
	*
	* \returns On success, A pointer to the internal overlappedK information
	* structure. Otherwise NULL. Use \c GetLastError() to get extended error
	* information.
	*
	* \c OvlK_GetInfo is used by the various backends to configure \c
	* OverlappedK internal information. Not all fields in \ref
	* KOVL_OVERLAPPED_INFO are applicable to \b all backends.
	*/
	KUSB_EXP PKOVL_OVERLAPPED_INFO KUSB_API OvlK_GetInfo(
	    __in PKOVL_OVERLAPPED OverlappedK);

//! Returns the overlappedK context space.
	/*!
	*
	* \param OverlappedK
	* The overlappedK used to return the user context space pointer.
	*
	* \returns On success, A pointer to the user context space. Otherwise
	* NULL. Use \c GetLastError() to get extended error information.
	*
	* The user context can be used for a user defined purpose.
	* (up to 32 byes of storage)
	*/
	KUSB_EXP PKUSB_USER_CONTEXT KUSB_API OvlK_GetContext(
	    __in PKOVL_OVERLAPPED OverlappedK);

//! Returns the pool context space.
	/*!
	*
	* \param Pool
	* The pool used to return the user context space pointer.
	* If this parameter is NULL, a pointer to the default pool
	* context space is returned.
	*
	* \returns On success, A pointer to the user context space. Otherwise
	* NULL. Use \c GetLastError() to get extended error information.
	*
	* The user context can be used for a user defined purpose.
	* (up to 32 byes of storage)
	*
	* \note
	* User context space is never erased; even when the overlapped is
	* released.
	*/
	KUSB_EXP PKUSB_USER_CONTEXT KUSB_API OvlK_GetPoolContext(
	    __in_opt PKOVL_OVERLAPPED_POOL Pool);

//! Waits for an OverlappedK i/o operation to complete.
	/*!
	*
	* \param OverlappedK
	* The overlappedK to wait on.
	*
	* \param TimeoutMS
	* Number of milliseconds to wait for overlapped completion.
	*
	* \param WaitFlags
	* See /ref KOVL_WAIT_FLAGS
	*
	* \param TransferredLength
	* On success, returns the number of bytes transferred by this overlappedK.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*
	* \c OvlK_Wait waits the the time interval specified by \c TimeoutMS for
	* an overlapped result. If the transfer does not complete in time, on
	* operation specified by \c WaitFlags can be performed on the \c
	* OverlappedK.
	*/
	KUSB_EXP BOOL KUSB_API OvlK_Wait(
	    __in PKOVL_OVERLAPPED OverlappedK,
	    __in_opt DWORD TimeoutMS,
	    __in_opt KOVL_WAIT_FLAGS WaitFlags,
	    __out PULONG TransferredLength);

//! Waits for an OverlappedK i/o operation to complete; cancels if it fails to complete within the specified time.
	/*!
	*
	* \note
	* When \c OvlK_WaitOrCancel returns, the \c OverlappedK is ready for
	* re-use or release.
	*
	* \param OverlappedK
	* The overlappedK to wait on.
	*
	* \param TimeoutMS
	* Number of milliseconds to wait for overlapped completion.
	*
	* \param TransferredLength
	* On success, returns the number of bytes transferred by this overlappedK.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*
	* \c OvlK_WaitOrCancel waits the the time interval specified by \c
	* TimeoutMS for an overlapped result. If the transfer does not complete in
	* time, the overlapped operation is cancelled.
	*
	* This convience function calls \ref OvlK_Wait with \ref
	* WAIT_FLAGS_CANCEL_ON_TIMEOUT
	*
	*/
	KUSB_EXP BOOL KUSB_API OvlK_WaitOrCancel(
	    __in PKOVL_OVERLAPPED OverlappedK,
	    __in_opt DWORD TimeoutMS,
	    __out PULONG TransferredLength);

//! Waits for completion, cancels the transfer if it fails to complete within the specified time. Always releases the \c OverlappedK back to it pool.
	/*!
	*
	*
	* \param OverlappedK
	* The overlappedK to wait on.
	*
	* \param TimeoutMS
	* Number of milliseconds to wait for overlapped completion.
	*
	* \param TransferredLength
	* On success, returns the number of bytes transferred by this overlappedK.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*
	* \c OvlK_WaitAndRelease waits the the time interval specified by \c
	* TimeoutMS for an overlapped result. If the transfer does not complete in
	* time, the overlapped operation is cancelled. \c OverlappedK is always
	* released back to its pool.
	*
	* This convience function calls \ref OvlK_Wait with \ref
	* WAIT_FLAGS_RELEASE_ALWAYS
	*
	* \note
	* When \c OvlK_WaitOrCancel returns, the i/o operation has either been
	* completed or cancelled and \c OverlappedK has been released back to the
	* pool.
	*
	*/
	KUSB_EXP BOOL KUSB_API OvlK_WaitAndRelease(
	    __in PKOVL_OVERLAPPED OverlappedK,
	    __in_opt DWORD TimeoutMS,
	    __out PULONG TransferredLength);

//! Checks for i/o completion; returns immediately. (polling)
	/*!
	*
	* \param OverlappedK
	* The overlappedK to check for completion.
	*
	* \warning
	* \ref OvlK_IsComplete does \b no validation on the OverlappedK.
	* It's purpose is to check the event signal state as fast as possible.
	*
	* \returns TRUE if the \c OverlappedK has completed, otherwise FALSE.
	*
	* \c OvlK_IsComplete quickly checks if the \c OverlappedK i/o operation has completed.
	*/
	KUSB_EXP BOOL KUSB_API OvlK_IsComplete(
	    __in PKOVL_OVERLAPPED OverlappedK);

//! Initializes an overlappedK for re-use. The overlappedK is not return to its pool.
	/*!
	*
	* \param OverlappedK
	* The overlappedK to re-use.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*
	*  This function performs the following actions:
	*  - Resets the overlapped event to non-signaled via ResetEvent().
	*  - Clears the internal overlapped information.
	*  - Clears the 'Internal' and 'InternalHigh' members of the windows
	*    overlapped structure.
	*
	* \note
	* Re-using OverlappedKs is the most efficient means of OverlappedK
	* management. When an OverlappedK is "re-used" it is not returned to the
	* pool. Intead, the application retains ownership for use in another i/o
	* operation.
	*
	*/
	KUSB_EXP BOOL KUSB_API KUSB_API OvlK_ReUse(
	    __in PKOVL_OVERLAPPED OverlappedK);

	/*! @} */

#ifdef __cplusplus
}
#endif

#endif
