#pragma once

#include "afx.h"
#include "libwdi.h"
#include <GuidDef.h>

typedef struct wdi_device_info WDI_DEVICE_INFO, *PWDI_DEVICE_INFO;
typedef struct wdi_options_prepare_driver WDI_OPTIONS_PREPARE_DRIVER, *PWDI_OPTIONS_PREPARE_DRIVER;

#define WDI_SESSION_INVALID_MASK (0x8000)

class CLibWdiSession :
	public CObject
{

public:
	CLibWdiSession(void);

	virtual ~CLibWdiSession(void);

	DWORD IsValid(void);

	void Destroy(PWDI_DEVICE_INFO deviceInfo);
	void CopyFrom(PWDI_DEVICE_INFO deviceInfo);
	void CopyTo(PWDI_DEVICE_INFO deviceInfo);
	void RefreshSession(void);

	inline void SetDriverType(int driverType)
	{
		this->driver_type = driverType;
	}
	inline int GetDriverType(void)
	{
		return driver_type;
	}

	static BOOL StringToGuid(GUID* GuidVal, CString GuidString);
	static BOOL GuidToString(GUID* Guid, CString& GuidString);
	static CString GetRandomGuid(void);

	CString GetGuid(void);
	BOOL SetGuid(CString& newGuid);

	CString GetInfClassName(void);
	BOOL SetInfClassName(CString& className);

	CString GetInfClassGuid(void);
	BOOL SetInfClassGuid(CString& classGuid);

	CString GetInfProvider(void);
	BOOL SetInfProvider(CString& provider);

	virtual void Serialize( CArchive& archive );

	/** USB VID */
	unsigned short vid;
	/** USB PID */
	unsigned short pid;
	/** Whether the USB device is composite */
	BOOL is_composite;
	/** (Optional) Composite USB interface number */
	unsigned char mi;
	/** USB Device description, usually provided by the device itself */
	CString desc;
	/** Windows' driver (service) name */
	CString driver;
	/** (Optional) Microsoft's device URI string. NULL if unused */
	CString device_id;
	/** (Optional) Microsoft's Hardware ID string. NULL if unused */
	CString hardware_id;
	/** (Optional) Microsoft's Compatible ID string. NULL if unused */
	CString compatible_id;
	/** (Optional) Upper filter. NULL if unused */
	CString upper_filter;
	/** (Optional) Driver version (four WORDS). 0 if unused */
	UINT64 driver_version;

	unsigned int pwr_device_idle_enabled;
	unsigned int pwr_default_idle_state;
	unsigned int pwr_default_idle_timeout;
	unsigned int pwr_user_set_device_idle_enabled;
	unsigned int pwr_device_idle_ignore_wake_enable;
	unsigned int pwr_system_wake_enabled;


public:
	CString m_VendorName;
	CString m_InterfaceGuid;
	CString m_InfClassName;
	CString m_InfClassGuid;
	CString m_InfProvider;

	CHAR chVendorName[512];
	CHAR chDeviceGuid[512];
	CHAR chInfClassName[512];
	CHAR chInfClassGuid[512];
	CHAR chInfProvider[512];

	DWORD m_PackageStatus;

public:
	enum
	{
	    PACKAGE_STATUS_SUCCESS = (1 << 0),
	    PACKAGE_STATUS_FAIL = (1 << 1),
	    PACKAGE_TYPE_CLIENT_INSTALLER = (1 << 2),
	    PACKAGE_TYPE_INSTALL_ONLY = (1 << 3),
	    PACKAGE_TYPE_LEGACY = (1 << 4),
	};

	CString m_PackageBaseDir;
	CString m_PackageName;

	CString GetPackageBaseDir(void);
	CString SetPackageBaseDir(CString baseDir);
	CString GetPackageName(void);
	CString SetPackageName(CString name);

	BOOL ShowSavePackageDialog(CWnd* parent, CString baseDir, CString name);
private:
	/** (Optional) The selected WDI driver type  */
	int driver_type;

	// Implementation
protected:
	DECLARE_SERIAL(CLibWdiSession)

};
