/*
 * embedder : converts binary resources into a .h include
 * "If you can think of a better way to get ice, I'd like to hear it."
 * Copyright (c) 2010 Pete Batard <pbatard@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#pragma once

/////////////////////////////////////////////////////////////////////////////
// DO NOT MODIFY SECTION.  These enums are copied from the header.
enum wdi_package_flag
{
	WDI_PACKAGE_FLAG_CREATE_NORMAL			= (1 << 31),
	WDI_PACKAGE_FLAG_CREATE_USER_INSTALLER	= (1 << 29),
	WDI_PACKAGE_FLAG_FOR_INSTALL			= (1 << 28),

	WDI_PACKAGE_FLAG_MASK					= 0xFFFFFF00,
};

enum wdi_driver_type {
	WDI_WINUSB,
	WDI_LIBUSB0,
	WDI_LIBUSBK,
	WDI_USER,
	WDI_NB_DRIVERS,	// Total number of drivers in the enum
	WDI_ALL_DRIVERS=0x80,
};
///////////////////////////////////////////////////////////////////////////////

/*
 * This include defines the driver files that should be embedded in the library.
 * This file is meant to be used by libwdi developers only.
 * If you want to add extra files from a specific directory (eg signed inf and cat)
 * you should either define the macro USER_DIR in msvc/config.h (MS compilers) or
 * use the --with-userdir option when running configure.
 */

struct emb {
	int reuse_last;
	char* file_name;
	char* extraction_subdir;
	int driver_type;
};

#if defined(DDK_DIR) && !defined(DDK_DPINST_DIR)
#define DDK_DPINST_DIR DDK_DIR "\\redist\\DIFx\\dpinst\\MultiLin"
#endif

#if !defined(DDK_DPINST_DIR)
#error "You must set DDK_DIR or DDK_DPINST_DIR for this flavor of libwdi
#endif
/*
 * files to embed
 */
struct emb embeddable_fixed[] = {

	// DDK files for all drivers
#if defined(DDK_DIR)
	{ 0, DDK_DIR "\\redist\\wdf\\x86\\WdfCoInstaller" WDF_VER ".dll", "x86",WDI_WINUSB|WDI_PACKAGE_FLAG_MASK },
	{ 1, DDK_DIR "\\redist\\wdf\\x86\\WdfCoInstaller" WDF_VER ".dll", "x86",WDI_LIBUSBK|WDI_PACKAGE_FLAG_MASK },

	{ 0, DDK_DIR "\\redist\\wdf\\amd64\\WdfCoInstaller" WDF_VER ".dll", "amd64",WDI_WINUSB|WDI_PACKAGE_FLAG_MASK },
	{ 1, DDK_DIR "\\redist\\wdf\\amd64\\WdfCoInstaller" WDF_VER ".dll", "amd64",WDI_LIBUSBK|WDI_PACKAGE_FLAG_MASK },

	{ 0, DDK_DIR "\\redist\\winusb\\x86\\winusbcoinstaller2.dll", "x86",WDI_WINUSB|WDI_PACKAGE_FLAG_MASK },

	{ 0, DDK_DIR "\\redist\\winusb\\amd64\\winusbcoinstaller2.dll", "amd64",WDI_WINUSB|WDI_PACKAGE_FLAG_MASK },

	{ 0, DDK_DPINST_DIR "\\amd64\\dpinst.exe", "dpinst64.exe",WDI_ALL_DRIVERS|WDI_PACKAGE_FLAG_CREATE_USER_INSTALLER },
	{ 0, DDK_DPINST_DIR "\\x86\\dpinst.exe", "dpinst32.exe",WDI_ALL_DRIVERS|WDI_PACKAGE_FLAG_CREATE_USER_INSTALLER },

#endif

#if defined(LIBUSB0_DIR)
	// libusb-win32 files
	{ 0, LIBUSB0_DIR "\\bin\\amd64\\libusb0.sys", "amd64",WDI_LIBUSB0|WDI_PACKAGE_FLAG_MASK },
	{ 0, LIBUSB0_DIR "\\bin\\x86\\libusb0.sys", "x86",WDI_LIBUSB0|WDI_PACKAGE_FLAG_MASK },

	{ 0, LIBUSB0_DIR "\\bin\\x86\\libusb0_x86.dll", "x86",WDI_LIBUSB0|WDI_PACKAGE_FLAG_MASK },
	{ 1, LIBUSB0_DIR "\\bin\\x86\\libusb0_x86.dll", "x86",WDI_LIBUSBK|WDI_PACKAGE_FLAG_MASK },
	
	{ 0, LIBUSB0_DIR "\\bin\\amd64\\libusb0.dll", "amd64",WDI_LIBUSB0|WDI_PACKAGE_FLAG_MASK },
	{ 1, LIBUSB0_DIR "\\bin\\amd64\\libusb0.dll", "amd64",WDI_LIBUSBK|WDI_PACKAGE_FLAG_MASK },
#endif

#if defined(LIBUSBK_DIR)
	// libusbK files
	{ 0, LIBUSBK_DIR "\\sys\\amd64\\libusbK.sys", "amd64",WDI_LIBUSBK|WDI_PACKAGE_FLAG_MASK },
	{ 0, LIBUSBK_DIR "\\sys\\x86\\libusbK.sys", "x86",WDI_LIBUSBK|WDI_PACKAGE_FLAG_MASK },

	{ 0, LIBUSBK_DIR "\\dll\\amd64\\libusbK.dll", "amd64",WDI_ALL_DRIVERS|WDI_PACKAGE_FLAG_MASK },
	{ 0, LIBUSBK_DIR "\\dll\\x86\\libusbK.dll", "x86\\libusbK_x86.dll",WDI_ALL_DRIVERS|WDI_PACKAGE_FLAG_MASK },

	{ 0, LIBUSBK_DIR "\\exe\\x86\\dpscat.exe", ".",WDI_ALL_DRIVERS|WDI_PACKAGE_FLAG_CREATE_USER_INSTALLER },
#endif

// Common files
#if defined(OPT_M32)
	{ 0, INSTALLER_PATH_32 "\\installer_x86.exe", ".",WDI_ALL_DRIVERS|WDI_PACKAGE_FLAG_FOR_INSTALL },
#endif
#if defined(OPT_M64)
	{ 0, INSTALLER_PATH_64 "\\installer_x64.exe", ".",WDI_ALL_DRIVERS|WDI_PACKAGE_FLAG_FOR_INSTALL },
#endif
// inf templates for the tokenizer ("" directory means no extraction)
	{ 0, "winusb.inf.in", "",WDI_WINUSB | WDI_PACKAGE_FLAG_MASK },
	{ 0, "libusb0.inf.in", "",WDI_LIBUSB0 | WDI_PACKAGE_FLAG_MASK },
	{ 0, "libusbk.inf.in", "",WDI_LIBUSBK |WDI_PACKAGE_FLAG_MASK },
// cat file lists for self signing
	{ 0, "winusb.cat.in", "",WDI_WINUSB|WDI_PACKAGE_FLAG_FOR_INSTALL },
	{ 0, "libusb0.cat.in", "",WDI_LIBUSB0|WDI_PACKAGE_FLAG_FOR_INSTALL },
	{ 0, "libusbk.cat.in", "",WDI_LIBUSBK|WDI_PACKAGE_FLAG_FOR_INSTALL },
};

