#include "stdafx.h"
#include "LibWdiDynamicAPI.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CLibWdiDynamicAPI::CLibWdiDynamicAPI(void)
{
	HMODULE dllKernel = GetModuleHandle(_T("kernel32"));
	fnFreeLibrary = (_FREE_LIBRARY_T*) GetProcAddress(dllKernel,"FreeLibrary");
	LoadLib();
}

CLibWdiDynamicAPI::~CLibWdiDynamicAPI(void)
{
	Free();
}

BOOL CLibWdiDynamicAPI::LoadLib(void)
{

	// M: DYN_INIT_MEMBER\({[A-Za-z_]+},,{[A-Za-z_]+}.+
	// R: \1=\2;
#ifndef LIBWDI_DYNAMIC_ONLY
	StrError=wdi_strerror;
	IsDriverSupported=wdi_is_driver_supported;
	IsFileEmbedded=wdi_is_file_embedded;
	GetVendorName=wdi_get_vendor_name;
	CreateList=wdi_create_list;
	DestroyList=wdi_destroy_list;
	PrepareDriver=wdi_prepare_driver;
	InstallDriver=wdi_install_driver;
	InstallCertificate=wdi_install_trusted_certificate;
	SetLogLevel=wdi_set_log_level;
	RegisterLogger=wdi_register_logger;
	UnregisterLogger=wdi_unregister_logger;
	ReadLogger=wdi_read_logger;
	m_IsApiLoaded=TRUE;
	return TRUE;
#endif
	m_IsApiLoaded = FALSE;
	return FALSE;
}

BOOL CLibWdiDynamicAPI::LoadDll(CString wdiDllFullPath)
{
	m_IsApiLoaded = FALSE;

	HMODULE wdiDllInstance = LoadLibraryEx(wdiDllFullPath,NULL,LOAD_WITH_ALTERED_SEARCH_PATH);
	if (!wdiDllInstance)
		return FALSE;

	DYN_INIT_MEMBER(StrError,,wdi_strerror,_dynfn_t,wdiDllInstance,"wdi_strerror",fnFreeLibrary(wdiDllInstance); return FALSE);
	DYN_INIT_MEMBER(IsDriverSupported,,wdi_is_driver_supported,_dynfn_t,wdiDllInstance,"wdi_is_driver_supported",fnFreeLibrary(wdiDllInstance); return FALSE);
	DYN_INIT_MEMBER(IsFileEmbedded,,wdi_is_file_embedded,_dynfn_t,wdiDllInstance,"wdi_is_file_embedded",fnFreeLibrary(wdiDllInstance); return FALSE);
	DYN_INIT_MEMBER(GetVendorName,,wdi_get_vendor_name,_dynfn_t,wdiDllInstance,"wdi_get_vendor_name",fnFreeLibrary(wdiDllInstance); return FALSE);
	DYN_INIT_MEMBER(CreateList,,wdi_create_list,_dynfn_t,wdiDllInstance,"wdi_create_list",fnFreeLibrary(wdiDllInstance); return FALSE);
	DYN_INIT_MEMBER(DestroyList,,wdi_destroy_list,_dynfn_t,wdiDllInstance,"wdi_destroy_list",fnFreeLibrary(wdiDllInstance); return FALSE);
	DYN_INIT_MEMBER(PrepareDriver,,wdi_prepare_driver,_dynfn_t,wdiDllInstance,"wdi_prepare_driver",fnFreeLibrary(wdiDllInstance); return FALSE);
	DYN_INIT_MEMBER(InstallDriver,,wdi_install_driver,_dynfn_t,wdiDllInstance,"wdi_install_driver",fnFreeLibrary(wdiDllInstance); return FALSE);
	DYN_INIT_MEMBER(InstallCertificate,,wdi_install_trusted_certificate,_dynfn_t,wdiDllInstance,"wdi_install_trusted_certificate",fnFreeLibrary(wdiDllInstance); return FALSE);
	DYN_INIT_MEMBER(SetLogLevel,,wdi_set_log_level,_dynfn_t,wdiDllInstance,"wdi_set_log_level",fnFreeLibrary(wdiDllInstance); return FALSE);
	DYN_INIT_MEMBER(RegisterLogger,,wdi_register_logger,_dynfn_t,wdiDllInstance,"wdi_register_logger",fnFreeLibrary(wdiDllInstance); return FALSE);
	DYN_INIT_MEMBER(UnregisterLogger,,wdi_unregister_logger,_dynfn_t,wdiDllInstance,"wdi_unregister_logger",fnFreeLibrary(wdiDllInstance); return FALSE);
	DYN_INIT_MEMBER(ReadLogger,,wdi_read_logger,_dynfn_t,wdiDllInstance,"wdi_read_logger",fnFreeLibrary(wdiDllInstance); return FALSE);

	Free();

	m_WdiDll = wdiDllInstance;
	m_IsApiLoaded = TRUE;
	return m_IsApiLoaded;
}

void CLibWdiDynamicAPI::Free(void)
{
	if (m_WdiDll)
	{
		fnFreeLibrary(m_WdiDll);
		m_WdiDll = NULL;
		m_IsApiLoaded=FALSE;
	}
}
