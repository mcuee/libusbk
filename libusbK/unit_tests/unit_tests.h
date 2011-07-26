/*! \file unit_tests.h
* \brief Common include file used only in unit_tests.
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

#include "libusbk.h"

#ifndef __UNIT_TEST_H_
#define __UNIT_TEST_H_

//! Default example vendor id
#define UNIT_TEST_VID 0x04D8

//! Default example product id
#define UNIT_TEST_PID 0xFA2E

typedef struct _UNIT_TEST_CONFIG
{
	LPCSTR Category;
	LPCSTR Name;
	DWORD Tab;
} UNIT_TEST_CONFIG, *PUNIT_TEST_CONFIG;

//! Helper function for unit_tests; searches a command line argument list for devices matching a specific vid/pid.
/*!
* Arguments are interpreted as follows:
* - vid=<4 digit hex>
*   - hex number is a vendor id.
* - pid=<4 digit hex>
*   - hex number is a product id.
*
* \param DeviceList
* On success, contains a pointer to the device list.
*
* \param DeviceInfo
* On success, contains a pointer to the first device info structure which matched.
*
* \param argc
* The \c argc parameter of the \b main() application function.
*
* \param argv
* The \c argv parameter of the \b main() application function.
*
* \returns
* TRUE if a devices was found, otherwise FALSE.
*/
BOOL UnitK_GetTestDevice( __deref_out KLST_HANDLE* DeviceList,
                          __deref_out KLST_DEVINFO_HANDLE* DeviceInfo,
                          __in int argc,
                          __in char* argv[]);
BOOL UnitK_GetTestDeviceEx( __deref_out KLST_HANDLE* DeviceList,
                            __deref_out KLST_DEVINFO_HANDLE* DeviceInfo,
                            __in int argc,
                            __in char* argv[],
                            __in_opt PKLST_INIT_PARAMS InitParams);

DWORD UnitK_GetError(PCHAR ErrorString, DWORD ErrorStringSize);
VOID UnitK_Init(PUNIT_TEST_CONFIG Config);
VOID UnitK_Ln(CONST CHAR* fmt, ...);

#endif



#ifndef UTLIST_H
#define UTLIST_H

/*
Copyright (c) 2007-2010, Troy D. Hanson   http://uthash.sourceforge.net
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


/******************************************************************************
 * doubly linked list macros (non-circular)                                   *
 *****************************************************************************/

//! Adds an element to the beginning of a linked list.
/*!
* \param head
* First element of the list.
*
* \param add
* Element to add.
*/
#define DL_PREPEND(head,add)                                                                   \
do {                                                                                           \
 (add)->next = head;                                                                           \
 if (head) {                                                                                   \
   (add)->prev = (head)->prev;                                                                 \
   (head)->prev = (add);                                                                       \
 } else {                                                                                      \
   (add)->prev = (add);                                                                        \
 }                                                                                             \
 (head) = (add);                                                                               \
} while (0)

//! Adds an element to the end of a linked list.
/*!
* \param head
* First element of the list.
*
* \param add
* Element to add.
*/
#define DL_APPEND(head,add)                                                                    \
do {                                                                                           \
  if (head) {                                                                                  \
      (add)->prev = (head)->prev;                                                              \
      (head)->prev->next = (add);                                                              \
      (head)->prev = (add);                                                                    \
      (add)->next = NULL;                                                                      \
  } else {                                                                                     \
      (head)=(add);                                                                            \
      (head)->prev = (head);                                                                   \
      (head)->next = NULL;                                                                     \
  }                                                                                            \
} while (0);

//! Removes an element from a linked list.
/*!
*
* \param head
* First element of the list.
*
* \param del
* Element to remove.
*
* \attention
* \c DL_DELETE does not free or de-allocate memory.
* It "de-links" the element specified by \c del from the list.
*/
#define DL_DELETE(head,del)                                                                    \
do {                                                                                           \
  if ((del)->prev == (del)) {                                                                  \
      (head)=NULL;                                                                             \
  } else if ((del)==(head)) {                                                                  \
      (del)->next->prev = (del)->prev;                                                         \
      (head) = (del)->next;                                                                    \
  } else {                                                                                     \
      (del)->prev->next = (del)->next;                                                         \
      if ((del)->next) {                                                                       \
          (del)->next->prev = (del)->prev;                                                     \
      } else {                                                                                 \
          (head)->prev = (del)->prev;                                                          \
      }                                                                                        \
  }                                                                                            \
} while (0);

//! Start a \c foreach like enumeration though a linked list using a \b for loop.
/*!
* \param head
* First element of the list.
*
* \param el
* assigned to an element of the linked list on each iteration.
*/
#define DL_FOREACH(head,el)                                                                    \
    for(el=head;el;el=el->next)

//! \copybrief DL_FOREACH
/*!
*
* \copydetails DL_FOREACH
*
* \param tmp
* A temporary list element used to ensure safe deletion during iteration.
*
* \attention
* This version is safe for deleting the elements during iteration.
*/
#define DL_FOREACH_SAFE(head,el,tmp)                                                           \
  for((el)=(head);(el) && (tmp = (el)->next, 1); (el) = tmp)

/* these are identical to their singly-linked list counterparts */

//! Searches for a scalar field using a \ref DL_FOREACH.
/*!
* \param head
* First element of the list.
*
* \param out
* First element whose \c field value equals \c val.
*
* \param field
* Name of the field member to search.
*
* \param val
* Value to compare with the field member.
*/
#define DL_SEARCH_SCALAR(head,out,field,val)                                                   \
do {                                                                                           \
    DL_FOREACH(head,out) {                                                                     \
      if ((out)->field == (val)) break;                                                        \
    }                                                                                          \
} while(0)

//! Searches for an element using a user-defined compare function such as memcmp or strcmp.
/*!
* \param head
* First element of the list.
*
* \param out
* First matching element that matched (user-defined compare function returned 0).
*
* \param elt
* Matching criteria (passed as a second paramater to the user-defined compare function)
*
* \param cmp
* User-defined compare function or macro.
*/
#define DL_SEARCH(head,out,elt,cmp)                                                            \
do {                                                                                           \
    DL_FOREACH(head,out) {                                                                     \
      if ((cmp(out,elt))==0) break;                                                            \
    }                                                                                          \
} while(0)

#endif /* UTLIST_H */


