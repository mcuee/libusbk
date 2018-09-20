/*!********************************************************************
libusbK - Multi-driver USB library.
Copyright (C) 2012 Travis Lee Robinson. All Rights Reserved.
libusb-win32.sourceforge.net

Development : Travis Lee Robinson  (libusbdotnet@gmail.com)
Testing     : Xiaofan Chen         (xiaofanc@gmail.com)

At the discretion of the user of this library, this software may be
licensed under the terms of the GNU Public License v3 or a BSD-Style
license as outlined in the following files:
* LICENSE-gpl3.txt
* LICENSE-bsd.txt

License files are located in a license folder at the root of source and
binary distributions.
********************************************************************!*/

#define UNUSED(x) (void)(x)

#ifndef ENOMEM
#define ENOMEM          12
#endif

#ifndef EINVAL
#define EINVAL          22
#endif

int _usb_io_sync(HANDLE dev, unsigned int code, void *out, int out_size,
	void *in, int in_size, int *ret);

int usb_control_msg(HANDLE *dev, int requesttype, int request,
	int value, int index, PUCHAR bytes, int size, int timeout);

int usb_set_configuration(HANDLE *dev, int configuration);