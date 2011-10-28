#pragma once

typedef void CALLBACK EXPORT DEVICE_NOTIFIER_CB(
    HWND hWnd,      // handle of CWnd that called SetTimer
    UINT nMsg,      // WM_TIMER
    UINT nIDEvent,   // timer identification
    DWORD dwTime    // system time
);

#define DEVICE_NOTIFIER_TIMER_ID 0xF

class CDeviceNotifier
{
public:
	CDeviceNotifier(void);
	~CDeviceNotifier(void);

	static inline long SetDirty(long isDirty)
	{
		return InterlockedExchange(&m_IsDirty, isDirty);
	}
	static inline CWnd* RegisterWnd(CWnd* pwndToRecieveMsg)
	{
		return ( CWnd*)InterlockedExchangePointer((void**)&m_pRegisteredWnd, pwndToRecieveMsg);
	}

private:
	static CWnd*			m_pMainWnd;
	static HDEVNOTIFY		m_NotifyHandle;
	static UINT_PTR			m_NotifyTimer;
	volatile static long	m_IsDirty;
	volatile static CWnd*	m_pRegisteredWnd;

	static void CALLBACK EXPORT OnDeviceNotifyTimerElapsed(HWND hWnd, UINT nMsg, UINT nIDEvent, DWORD dwTime);

protected:
	void OnWmDeviceChange(WPARAM wParam, LPARAM lParam);
	BOOL RegisterNotifier(void);
	void UnRegisterNotifier(void);
};
