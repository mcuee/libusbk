#pragma once
#include "libwdi.h"

typedef  BOOL WINAPI _FREE_LIBRARY_T(HMODULE hModule);

#ifndef DECL_DYN_MACROS________________________________________________
#define DECL_DYN_MACROS________________________________________________

#define DECL_DYN_TYPEDEF(Type_Name,Type_Name_Suffix,Func_Export,Type_ReturnType,Type_Params)					\
	typedef Type_ReturnType Func_Export Type_Name##Type_Name_Suffix Type_Params

#define DECL_DYN_MEMBER(Memb_Name,Memb_Name_Suffix,Func_Export,Memb_ReturnType,Memb_Params)						\
	Memb_ReturnType (Func_Export*Memb_Name##Memb_Name_Suffix) Memb_Params

#define DYN_INIT_MEMBER(Memb_Name,Memb_Name_Suffix,Func_Name,Func_Name_Suffix, Api_Dll, Api_Func_Name,ErrorAction)	do {	\
		Memb_Name##Memb_Name_Suffix = (Func_Name##Func_Name_Suffix*) GetProcAddress(Api_Dll, Api_Func_Name);				\
		if ((Memb_Name##Memb_Name_Suffix) == NULL)																			\
		{																													\
			ErrorAction;																									\
		}																													\
	}while(0)
#endif

#define wdi_strerror_dynargs (int errcode)
#define wdi_is_driver_supported_dynargs (int driver_type, VS_FIXEDFILEINFO* driver_info)
#define wdi_is_file_embedded_dynargs (char* path, char* name)
#define wdi_get_vendor_name_dynargs (unsigned short vid)
#define wdi_create_list_dynargs (struct wdi_device_info** list, struct wdi_options_create_list* options)
#define wdi_destroy_list_dynargs (struct wdi_device_info* list)
#define wdi_prepare_driver_dynargs (struct wdi_device_info* device_info, char* path, char* inf_name, struct wdi_options_prepare_driver* options)
#define wdi_install_driver_dynargs (struct wdi_device_info* device_info, char* path, char* inf_name, struct wdi_options_install_driver* options)
#define wdi_install_trusted_certificate_dynargs (char* cert_name, struct wdi_options_install_cert* options)
#define wdi_set_log_level_dynargs (int level)
#define wdi_register_logger_dynargs (HWND hWnd, UINT message, DWORD buffsize)
#define wdi_unregister_logger_dynargs (HWND hWnd)
#define wdi_read_logger_dynargs (char* buffer, DWORD buffer_size, DWORD* message_size)

DECL_DYN_TYPEDEF(wdi_strerror, _dynfn_t, LIBWDI_API, const char*, wdi_strerror_dynargs);
DECL_DYN_TYPEDEF(wdi_is_driver_supported, _dynfn_t, LIBWDI_API, BOOL, wdi_is_driver_supported_dynargs);
DECL_DYN_TYPEDEF(wdi_is_file_embedded, _dynfn_t, LIBWDI_API, BOOL, wdi_is_file_embedded_dynargs);
DECL_DYN_TYPEDEF(wdi_get_vendor_name, _dynfn_t, LIBWDI_API, const char*, wdi_get_vendor_name_dynargs);
DECL_DYN_TYPEDEF(wdi_create_list, _dynfn_t, LIBWDI_API, int, wdi_create_list_dynargs);
DECL_DYN_TYPEDEF(wdi_destroy_list, _dynfn_t, LIBWDI_API, int, wdi_destroy_list_dynargs);
DECL_DYN_TYPEDEF(wdi_prepare_driver, _dynfn_t, LIBWDI_API, int, wdi_prepare_driver_dynargs);
DECL_DYN_TYPEDEF(wdi_install_driver, _dynfn_t, LIBWDI_API, int, wdi_install_driver_dynargs);
DECL_DYN_TYPEDEF(wdi_install_trusted_certificate, _dynfn_t, LIBWDI_API, int, wdi_install_trusted_certificate_dynargs);
DECL_DYN_TYPEDEF(wdi_set_log_level, _dynfn_t, LIBWDI_API, int, wdi_set_log_level_dynargs);
DECL_DYN_TYPEDEF(wdi_register_logger, _dynfn_t, LIBWDI_API, int, wdi_register_logger_dynargs);
DECL_DYN_TYPEDEF(wdi_unregister_logger, _dynfn_t, LIBWDI_API, int, wdi_unregister_logger_dynargs);
DECL_DYN_TYPEDEF(wdi_read_logger, _dynfn_t, LIBWDI_API, int, wdi_read_logger_dynargs);

class CLibWdiDynamicAPI
{

private:
	HMODULE m_WdiDll;

	// NOTE: We are not supposed to use Load/Free Library in MFC because the reference counts are not
	// properly maintained.
	_FREE_LIBRARY_T* fnFreeLibrary;

	BOOL m_IsApiLoaded;

public:

	BOOL LoadDll(CString wdiDllFullPath);
	BOOL LoadLib(void);
	BOOL IsApiLoaded(void)
	{
		return m_IsApiLoaded;
	}

	void Free(void);

	DECL_DYN_MEMBER(StrError, , LIBWDI_API, const char*, wdi_strerror_dynargs);
	DECL_DYN_MEMBER(IsDriverSupported, , LIBWDI_API, BOOL, wdi_is_driver_supported_dynargs);
	DECL_DYN_MEMBER(IsFileEmbedded, , LIBWDI_API, BOOL, wdi_is_file_embedded_dynargs);
	DECL_DYN_MEMBER(GetVendorName, , LIBWDI_API, const char*, wdi_get_vendor_name_dynargs);
	DECL_DYN_MEMBER(CreateList, , LIBWDI_API, int, wdi_create_list_dynargs);
	DECL_DYN_MEMBER(DestroyList, , LIBWDI_API, int, wdi_destroy_list_dynargs);
	DECL_DYN_MEMBER(PrepareDriver, , LIBWDI_API, int, wdi_prepare_driver_dynargs);
	DECL_DYN_MEMBER(InstallDriver, , LIBWDI_API, int, wdi_install_driver_dynargs);
	DECL_DYN_MEMBER(InstallCertificate, , LIBWDI_API, int, wdi_install_trusted_certificate_dynargs);
	DECL_DYN_MEMBER(SetLogLevel, , LIBWDI_API, int, wdi_set_log_level_dynargs);
	DECL_DYN_MEMBER(RegisterLogger, , LIBWDI_API, int, wdi_register_logger_dynargs);
	DECL_DYN_MEMBER(UnregisterLogger, , LIBWDI_API, int, wdi_unregister_logger_dynargs);
	DECL_DYN_MEMBER(ReadLogger, , LIBWDI_API, int, wdi_read_logger_dynargs);

	CLibWdiDynamicAPI(void);
	~CLibWdiDynamicAPI(void);
};
