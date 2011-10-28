#pragma once

#include "LibWdiDynamicAPI.h"
#include "libwdisession.h"
#include "afxcoll.h"
#include "afxtempl.h"

class CLibWdiManager: public CLibWdiDynamicAPI
{
public:

	CTypedPtrArray<CPtrArray, PWDI_DEVICE_INFO> DeviceItemArray;

public:
	CLibWdiManager(void);
	~CLibWdiManager(void);

	BOOL CreateList(BOOL listAll, BOOL listHubs);
	VOID TryDestroyList(void);
	BOOL SaveSession(LPCTSTR fileName);
	BOOL OpenSession(LPCTSTR fileName);
	BOOL SaveSession(CWnd* parent);
	BOOL OpenSession(CWnd* parent);

	BOOL LoadDll(CWnd* parent);

public:
	inline CLibWdiSession*	Session()
	{
		if (m_pSession == NULL) m_pSession = new CLibWdiSession();
		return m_pSession;
	}

	CString m_SessionFile;

private:
	BOOL m_IsApiLoaded;
	PWDI_DEVICE_INFO m_pDeviceInfoMaster;
	CLibWdiSession*	m_pSession;
	CString m_LibWdiDllPath;

};
