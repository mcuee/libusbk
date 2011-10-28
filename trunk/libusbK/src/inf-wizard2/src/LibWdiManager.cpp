#include "stdafx.h"
#include "LibWdiManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CLibWdiManager::CLibWdiManager(void)
	:m_pSession(NULL)
	,m_pDeviceInfoMaster(NULL)
{
}

CLibWdiManager::~CLibWdiManager(void)
{
	TryDestroyList();
	if (m_pSession) delete m_pSession;
}

VOID CLibWdiManager::TryDestroyList(void)
{
	if (m_pDeviceInfoMaster)
	{
		CLibWdiDynamicAPI::DestroyList(m_pDeviceInfoMaster);
		m_pDeviceInfoMaster = NULL;
		DeviceItemArray.RemoveAll();
	}
}

BOOL CLibWdiManager::CreateList(BOOL listAll, BOOL listHubs)
{
	wdi_options_create_list optCreateList;
	memset(&optCreateList,0,sizeof(optCreateList));
	optCreateList.list_all			= listAll;
	optCreateList.list_hubs			= listHubs;
	optCreateList.trim_whitespaces	= TRUE;

	TryDestroyList();

	CLibWdiDynamicAPI::CreateList(&m_pDeviceInfoMaster,&optCreateList);
	if (!m_pDeviceInfoMaster) return FALSE;


	for(PWDI_DEVICE_INFO current=m_pDeviceInfoMaster; current!=NULL; current=current->next)
	{
		DeviceItemArray.Add(current);
	}

	return TRUE;
}

BOOL CLibWdiManager::SaveSession(CWnd* parent)
{

	CFileDialog dlg(
	    FALSE,
	    _T("InfSession"),
	    m_SessionFile.GetBufferSetLength(4096),
	    OFN_PATHMUSTEXIST,
	    _T("Inf Wizard Session (*.InfSession)|*.InfSession||"),
	    parent);

	if (dlg.DoModal()==IDOK)
	{
		if (SaveSession(dlg.m_ofn.lpstrFile))
		{
			
			m_SessionFile = dlg.m_ofn.lpstrFile;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CLibWdiManager::OpenSession(CWnd* parent)
{
	CFileDialog dlg(
	    TRUE,
	    _T("InfSession"),
	    m_SessionFile.GetBufferSetLength(4096),
	    OFN_FILEMUSTEXIST,
	    _T("Inf Wizard Session (*.InfSession)|*.InfSession||"),
	    parent);

	if (dlg.DoModal()==IDOK)
	{
		if (OpenSession(dlg.m_ofn.lpstrFile))
		{
			m_SessionFile = dlg.m_ofn.lpstrFile;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CLibWdiManager::SaveSession(LPCTSTR fileName)
{
	CFile file;

	if (!file.Open(fileName, CFile::modeCreate | CFile::modeWrite, NULL))
		return FALSE;

	CArchive sessionArchive(&file,CArchive::store);

	sessionArchive.WriteObject(Session());
	sessionArchive.Close();
	file.Close();

	return TRUE;
}

BOOL CLibWdiManager::OpenSession(LPCTSTR fileName)
{
	CFile file;

	if (!file.Open(fileName, CFile::modeRead, NULL))
		return FALSE;

	CArchive sessionArchive(&file,CArchive::load);

	CLibWdiSession* newSession = (CLibWdiSession*)sessionArchive.ReadObject(RUNTIME_CLASS(CLibWdiSession));
	if (!newSession || !newSession->IsKindOf(RUNTIME_CLASS(CLibWdiSession))) return FALSE;
	if (m_pSession) delete m_pSession;

	m_pSession = newSession;
	sessionArchive.Close();
	file.Close();

	return TRUE;
}
