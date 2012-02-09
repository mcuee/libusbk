#include "stdafx.h"
#include <shlwapi.h>

#include "LibWdiSession.h"
#include "GridDevCfgListCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define WDI_ALLOC_STRING(mDeviceInfo,mMember) do {		\
		mDeviceInfo->mMember = NULL;						\
		if (mMember.GetLength() > 0)						\
		{													\
			mDeviceInfo->mMember = (char*)malloc(256);		\
			memset(mDeviceInfo->mMember,0, 256);			\
			wcstombs(mDeviceInfo->mMember,mMember,255);		\
		}													\
	}while(0)

#define WDI_FREE_STRING(mDeviceInfo,mMember) do {		\
		if (mDeviceInfo->mMember)							\
		{													\
			free(mDeviceInfo->mMember);						\
			mDeviceInfo->mMember = NULL;					\
		}													\
	}while(0)

#define WDI_COPY_STRING(mDeviceInfo,mMember) do {		\
		mMember = mDeviceInfo->mMember;						\
	}while(0)

#define GUID_MAXSIZE 38
#define GUID_FORMAT_STRING _T("%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X")

CLibWdiSession::CLibWdiSession(void)
	: driver_type(WDI_NB_DRIVERS)
	, vid(0)
	, pid(0)
	, is_composite(FALSE)
	, mi(0)
	, pwr_device_idle_enabled(1)
	, pwr_default_idle_state(0)
	, pwr_default_idle_timeout(5000)
	, pwr_user_set_device_idle_enabled(0)
	, pwr_device_idle_ignore_wake_enable(0)
	, pwr_system_wake_enabled(0)
	, m_PackageStatus(0)
{
}

CLibWdiSession::~CLibWdiSession(void)
{
}

DWORD CLibWdiSession::IsValid(void)
{
	GUID guidCheck;

	if (vid <= 0 || vid > 0xFFFF)
		return DEVCFG_FIELD_VID | WDI_SESSION_INVALID_MASK;

	if (pid <= 0 || pid > 0xFFFF)
		return DEVCFG_FIELD_PID | WDI_SESSION_INVALID_MASK;

	if (!StringToGuid(&guidCheck, this->m_InterfaceGuid))
		return DEVCFG_FIELD_GUID | WDI_SESSION_INVALID_MASK;

	return ERROR_SUCCESS;
}

/** (Optional) Pointer to the next element in the chained list. NULL if unused */
void CLibWdiSession::Destroy(PWDI_DEVICE_INFO deviceInfo)
{
	WDI_FREE_STRING(deviceInfo, desc);
	WDI_FREE_STRING(deviceInfo, driver);
	WDI_FREE_STRING(deviceInfo, device_id);
	WDI_FREE_STRING(deviceInfo, hardware_id);
	WDI_FREE_STRING(deviceInfo, compatible_id);
	WDI_FREE_STRING(deviceInfo, upper_filter);
}
void CLibWdiSession::CopyFrom(PWDI_DEVICE_INFO deviceInfo)
{
	vid = deviceInfo->vid;
	pid = deviceInfo->pid;
	is_composite = deviceInfo->is_composite;
	mi = deviceInfo->mi;
	driver_version = deviceInfo->driver_version;

	WDI_COPY_STRING(deviceInfo, desc);
	WDI_COPY_STRING(deviceInfo, driver);
	WDI_COPY_STRING(deviceInfo, device_id);
	WDI_COPY_STRING(deviceInfo, hardware_id);
	WDI_COPY_STRING(deviceInfo, compatible_id);
	WDI_COPY_STRING(deviceInfo, upper_filter);

}
void CLibWdiSession::CopyTo(PWDI_DEVICE_INFO deviceInfo)
{
	memset(deviceInfo, 0, sizeof(*deviceInfo));
	deviceInfo->vid = vid;
	deviceInfo->pid = pid;
	deviceInfo->is_composite = is_composite;
	deviceInfo->mi = mi;
	deviceInfo->driver_version = driver_version;

	WDI_ALLOC_STRING(deviceInfo, desc);
	WDI_ALLOC_STRING(deviceInfo, driver);
	WDI_ALLOC_STRING(deviceInfo, device_id);
	WDI_ALLOC_STRING(deviceInfo, hardware_id);
	WDI_ALLOC_STRING(deviceInfo, compatible_id);
	WDI_ALLOC_STRING(deviceInfo, upper_filter);
}

void CLibWdiSession::RefreshSession(void)
{
	wcstombs(chVendorName, m_VendorName, sizeof(chVendorName));
	wcstombs(chDeviceGuid, m_InterfaceGuid, sizeof(chDeviceGuid));

	wcstombs(chInfClassGuid, m_InfClassGuid, sizeof(chInfClassGuid));
	wcstombs(chInfClassName, m_InfClassName, sizeof(chInfClassName));
	wcstombs(chInfProvider, m_InfProvider, sizeof(chInfProvider));
}

void CLibWdiSession::Serialize(CArchive& archive)
{

	CObject::Serialize(archive);    // Always call base class Serialize.

	if( archive.IsStoring() )
	{
		archive
		        << m_PackageBaseDir
		        << m_PackageName
		        << m_VendorName
		        << m_InterfaceGuid
		        << driver_type
		        << vid
		        << pid
		        << is_composite
		        << mi
		        << desc
		        << driver_version
		        << m_InfClassGuid
		        << m_InfClassName
		        << m_InfProvider
		        << pwr_device_idle_enabled
		        << pwr_default_idle_state
		        << pwr_default_idle_timeout
		        << pwr_user_set_device_idle_enabled
		        << pwr_device_idle_ignore_wake_enable
		        << pwr_system_wake_enabled;

	}
	else
	{
		archive
		        >> m_PackageBaseDir
		        >> m_PackageName
		        >> m_VendorName
		        >> m_InterfaceGuid
		        >> driver_type
		        >> vid
		        >> pid
		        >> is_composite
		        >> mi
		        >> desc
		        >> driver_version
		        >> m_InfClassGuid
		        >> m_InfClassName
		        >> m_InfProvider
		        >> pwr_device_idle_enabled
		        >> pwr_default_idle_state
		        >> pwr_default_idle_timeout
		        >> pwr_user_set_device_idle_enabled
		        >> pwr_device_idle_ignore_wake_enable
		        >> pwr_system_wake_enabled;
	}
}

BOOL CLibWdiSession::StringToGuid(GUID* GuidVal, CString GuidString)
{
	int scanCount;
	UCHAR guidChars[11 * sizeof(int)];
	GUID* Guid = (GUID*)&guidChars;


	GuidString.Remove((TCHAR)'{');
	GuidString.Remove((TCHAR)'}');

	scanCount = _stscanf_s(GuidString, GUID_FORMAT_STRING,
	                       &Guid->Data1,
	                       &Guid->Data2,
	                       &Guid->Data3,
	                       &Guid->Data4[0], &Guid->Data4[1], &Guid->Data4[2], &Guid->Data4[3],
	                       &Guid->Data4[4], &Guid->Data4[5], &Guid->Data4[6], &Guid->Data4[7]);

	if (scanCount == 11)
		memcpy(GuidVal, &guidChars, sizeof(GUID));

	return (scanCount == 11);
}

BOOL CLibWdiSession::GuidToString(GUID* Guid, CString& GuidString)
{
	GuidString.Format(GUID_FORMAT_STRING,
	                  Guid->Data1,
	                  Guid->Data2,
	                  Guid->Data3,
	                  Guid->Data4[0], Guid->Data4[1], Guid->Data4[2], Guid->Data4[3],
	                  Guid->Data4[4], Guid->Data4[5], Guid->Data4[6], Guid->Data4[7]);

	GuidString.Insert(0, (TCHAR)'{');
	GuidString += (TCHAR)'}';

	return (TRUE);
}
CString CLibWdiSession::GetRandomGuid(void)
{
	GUID guid;
	BYTE* pbGuid = (BYTE*)&guid;

	srand((DWORD)time(NULL));
	for (int i = 0; i < sizeof(GUID); i++)
		pbGuid[i] = (BYTE)rand();

	CString guidString;
	GuidToString(&guid, guidString);

	return guidString;
}

CString CLibWdiSession::GetGuid(void)
{
	if (m_InterfaceGuid.GetLength() == 0)
	{
		m_InterfaceGuid = GetRandomGuid();
	}
	return m_InterfaceGuid;
}

BOOL CLibWdiSession::SetGuid(CString& newGuid)
{
	GUID guid;
	m_InterfaceGuid = newGuid;
	if (!StringToGuid(&guid, newGuid))
		return FALSE;

	return GuidToString(&guid, m_InterfaceGuid);
}


CString CLibWdiSession::GetInfClassName(void)
{
	m_InfClassName.Trim();
	if (m_InfClassName.GetLength() == 0)
		m_InfClassName = DefaultInfTags[driver_type].ClassName;

	CString checkValue = m_InfClassName;
	checkValue.MakeLower();
	for (int i = 0; i < WDI_NB_DRIVERS; i++)
	{
		CString s = DefaultInfTags[i].ClassName;
		s.MakeLower();
		if (checkValue.Compare(s) == 0)
		{
			if (i != driver_type)
			{
				m_InfClassName = DefaultInfTags[driver_type].ClassName;
				break;
			}
		}
	}
	return m_InfClassName;
}
BOOL CLibWdiSession::SetInfClassName(CString& className)
{
	m_InfClassName = className;
	GetInfClassName();
	return TRUE;
}

CString CLibWdiSession::GetInfClassGuid(void)
{
	m_InfClassGuid.Trim();
	if (m_InfClassGuid.GetLength() == 0)
		m_InfClassGuid = DefaultInfTags[driver_type].ClassGuid;

	CString checkValue = m_InfClassGuid;
	checkValue.MakeLower();
	for (int i = 0; i < WDI_NB_DRIVERS; i++)
	{
		CString s = DefaultInfTags[i].ClassGuid;
		s.MakeLower();
		if (checkValue.Compare(s) == 0)
		{
			if (i != driver_type)
			{
				m_InfClassGuid = DefaultInfTags[driver_type].ClassGuid;
				break;
			}
		}
	}
	return m_InfClassGuid;
}
BOOL CLibWdiSession::SetInfClassGuid(CString& classGuid)
{
	m_InfClassGuid = classGuid;
	GetInfClassGuid();
	return TRUE;
}

CString CLibWdiSession::GetInfProvider(void)
{
	m_InfProvider.Trim();
	if (m_InfProvider.GetLength() == 0)
		m_InfProvider = DefaultInfTags[driver_type].Provider;

	CString checkValue = m_InfProvider;
	checkValue.MakeLower();

	for (int i = 0; i < WDI_NB_DRIVERS; i++)
	{
		CString s = DefaultInfTags[i].Provider;
		s.MakeLower();
		if (checkValue.Compare(s) == 0)
		{
			if (i != driver_type)
				m_InfProvider = DefaultInfTags[driver_type].Provider;

			break;
		}
	}
	return m_InfProvider;
}
BOOL CLibWdiSession::SetInfProvider(CString& provider)
{
	m_InfProvider = provider;
	GetInfProvider();
	return TRUE;
}



CString CLibWdiSession::GetPackageBaseDir(void)
{
	if (m_PackageBaseDir.IsEmpty())
	{
		CString txtPath;
		SHGetSpecialFolderPath(NULL, txtPath.GetBufferSetLength(4096), CSIDL_MYDOCUMENTS, TRUE);
		PathAppend(txtPath.GetBuffer(4096), _T("DriverPackages"));
		txtPath.ReleaseBuffer();
		m_PackageBaseDir = txtPath;
	}

	return m_PackageBaseDir;
}

CString CLibWdiSession::SetPackageBaseDir(CString baseDir)
{
	m_PackageBaseDir = baseDir;
	return baseDir;
}

CString CLibWdiSession::GetPackageName(void)
{
	if (m_PackageName.IsEmpty())
		m_PackageName = desc;

	if (m_PackageName.IsEmpty())
		m_PackageName = _T("NewPackage");

	return m_PackageName;
}
CString CLibWdiSession::SetPackageName(CString name)
{
	int chPos;
	for (chPos = 1; chPos <= 255; chPos++)
	{
		if (chPos >= 'A' && chPos <= 'Z')
			continue;
		if (chPos >= 'a' && chPos <= 'z')
			continue;
		if (chPos >= '0' && chPos <= '9')
			continue;
		if (chPos == '-' || chPos == '_'  || chPos == '+')
			continue;

		name.Replace((TCHAR)chPos, (TCHAR)'_');
	}

	while (name.Replace(_T("__"), _T("_")));
	name.Trim(_T("_"));

	m_PackageName = name;

	return name;
}

BOOL CLibWdiSession::ShowSavePackageDialog(CWnd* parent, CString baseDir, CString name)
{
	CString txtPath = baseDir;
	LPTSTR szPath = txtPath.GetBufferSetLength(4096);

	if (!PathIsDirectory(szPath))
		SHCreateDirectoryEx(parent->GetSafeHwnd(), szPath, NULL);

	PathAppend(szPath, name);
	txtPath.ReleaseBuffer();


	CFileDialog dlg(FALSE, NULL, txtPath.GetBuffer(4096), 0, _T("Package/Sub-Folder Name||"), parent);

	if (dlg.DoModal() == IDOK)
	{

		m_PackageName		= dlg.m_ofn.lpstrFileTitle;
		m_PackageBaseDir	= dlg.m_ofn.lpstrFile;

		PathRemoveFileSpec(m_PackageBaseDir.GetBuffer(0));
		m_PackageBaseDir.ReleaseBuffer();

		return TRUE;
	}
	return FALSE;
}

IMPLEMENT_SERIAL(CLibWdiSession, CObject, 1)
