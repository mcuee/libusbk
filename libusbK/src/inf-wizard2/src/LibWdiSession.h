#pragma once

#include "afx.h"
#include "libwdi.h"
#include <GuidDef.h>

typedef struct wdi_device_info WDI_DEVICE_INFO, *PWDI_DEVICE_INFO;

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

public:
	CString m_VendorName;
	CHAR chVendorName[512];
	CHAR chDeviceGuid[64];

public:
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
	CString m_InterfaceGuid;

	// Implementation
protected:
	DECLARE_SERIAL(CLibWdiSession)

};
