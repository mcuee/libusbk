/* libusb-win32 WDF, Generic KMDF Windows USB Driver
 * Copyright (c) 2010-2011 Travis Robinson <libusbdotnet@gmail.com>
 * Copyright (c) 2002-2005 Stephan Meyer <ste_meyer@web.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _USER_H
#define _USER_H

#include <initguid.h>

// {D70F9B29-6EF7-4c94-BC7C-AE6755DD76BA}
DEFINE_GUID(GUID_CLASS_USBSAMP_USB,
            0xd70f9b29, 0x6ef7, 0x4c94, 0xbc, 0x7c, 0xae, 0x67, 0x55, 0xdd, 0x76, 0xba);

#define IOCTL_INDEX             0x0000


#define IOCTL_USBSAMP_GET_CONFIG_DESCRIPTOR CTL_CODE(FILE_DEVICE_UNKNOWN,     \
        IOCTL_INDEX,     \
        METHOD_BUFFERED,         \
        FILE_ANY_ACCESS)

#define IOCTL_USBSAMP_RESET_DEVICE          CTL_CODE(FILE_DEVICE_UNKNOWN,     \
        IOCTL_INDEX + 1, \
        METHOD_BUFFERED,         \
        FILE_ANY_ACCESS)

#define IOCTL_USBSAMP_RESET_PIPE            CTL_CODE(FILE_DEVICE_UNKNOWN,     \
        IOCTL_INDEX + 2, \
        METHOD_BUFFERED,         \
        FILE_ANY_ACCESS)


#endif

