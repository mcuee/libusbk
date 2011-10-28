#include "stdafx.h"
#include "DeviceNotifier.h"
#include "dbt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Used for device notification
//DEFINE_GUID(GUID_DEVINTERFACE_USB_HUB, 0xf18a0e88, 0xc30c, 0x11d0, 0x88, 0x15, 0x00, 0xa0, 0xc9, 0x06, 0xbe, 0xd8);
//DEFINE_GUID(GUID_DEVINTERFACE_USB_DEVICE, 0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED);

CWnd*			CDeviceNotifier::m_pMainWnd			= NULL;
HDEVNOTIFY		CDeviceNotifier::m_NotifyHandle		= NULL;
UINT_PTR		CDeviceNotifier::m_NotifyTimer		= 0;
volatile  long	CDeviceNotifier::m_IsDirty			= 0;
volatile CWnd*	CDeviceNotifier::m_pRegisteredWnd	= NULL;

CDeviceNotifier::CDeviceNotifier(void)
{
}

CDeviceNotifier::~CDeviceNotifier(void)
{
	UnRegisterNotifier();
}

BOOL CDeviceNotifier::RegisterNotifier(void)
{
	UnRegisterNotifier();

	m_pMainWnd = AfxGetApp()->GetMainWnd();
	ASSERT(m_pMainWnd);

	DEV_BROADCAST_DEVICEINTERFACE dbv;
	memset(&dbv,0,sizeof(dbv));
	dbv.dbcc_size=sizeof(DEV_BROADCAST_DEVICEINTERFACE);

	dbv.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;

	m_NotifyHandle = RegisterDeviceNotification(m_pMainWnd->GetSafeHwnd(), &dbv, DEVICE_NOTIFY_WINDOW_HANDLE|DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);
	if (!m_NotifyHandle || m_NotifyHandle == INVALID_HANDLE_VALUE)
		return FALSE;

	return TRUE;

}

void CDeviceNotifier::OnWmDeviceChange(WPARAM wParam, LPARAM lParam)
{
	ASSERT(m_pMainWnd);

	InterlockedExchange(&m_IsDirty, TRUE);
	m_NotifyTimer = m_pMainWnd->SetTimer(DEVICE_NOTIFIER_TIMER_ID,100,&CDeviceNotifier::OnDeviceNotifyTimerElapsed);
}

void CDeviceNotifier::UnRegisterNotifier(void)
{
	if (m_NotifyHandle)
	{
		UnregisterDeviceNotification(m_NotifyHandle);
		m_NotifyHandle=NULL;
	}
}

void CDeviceNotifier::OnDeviceNotifyTimerElapsed(HWND hWnd, UINT nMsg, UINT nIDEvent, DWORD dwTime)
{
	KillTimer(hWnd,nIDEvent);
	m_NotifyTimer=NULL;
	CWnd* pWndCb=(CWnd*)m_pRegisteredWnd;
	if (pWndCb)
	{
		pWndCb->PostMessage(WM_DEVICECHANGE,0,0);
		m_IsDirty=FALSE;
	}
}
