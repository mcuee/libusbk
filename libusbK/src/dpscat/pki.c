/*
 * libwdi: Library for automated Windows Driver Installation - PKI part
 * Copyright (c) 2012 Pete Batard <pbatard@gmail.com>
 * For more info, please visit http://libwdi.akeo.ie
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

// Undefine the following to run the program as standalone
//#define CATSIGN_STANDALONE

// warning C4127: conditional expression is constant
#pragma warning(disable: 4127)

// warning C4201: nonstandard extension used : nameless struct/union
#pragma warning(disable: 4201)

#include "dpscat.h"
#include <wincrypt.h>
#include <string.h>
#include <shlwapi.h>
#include "mssign32.h"
#include "inf_parser.h"

#define CATSIGN_STANDALONE

static WCHAR* windows_error_str(unsigned int retval)
{
	static WCHAR err_string[1024];
	unsigned int error_code;
	int len;

	error_code = retval ? retval : GetLastError();
	len = _snwprintf(err_string, _countof(err_string), L"ErrorCode(%08Xh):", error_code);
	if (FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error_code, 0, &err_string[len], _countof(err_string) - len, NULL) > 0)
	{
		return err_string;
	}
	_snwprintf(err_string, _countof(err_string), L"ErrorCode(%08Xh)", error_code);
	err_string[_countof(err_string) - 1] = 0;

	return err_string;
}

#define PF_INIT_OR_OUT(proc, dllname) \
	PF_INIT(proc, dllname); if (pf##proc == NULL) { \
		USBWRNN(L"unable to access %s DLL", L#dllname); goto out; }

#define KEY_CONTAINER L"libwdi key container"
#ifndef CERT_STORE_PROV_SYSTEM_A
#define CERT_STORE_PROV_SYSTEM_A ((LPCSTR) 9)
#endif

/*
 * Crypt32.dll
 */
typedef HCERTSTORE (WINAPI* CertOpenStore_t)(
    LPCSTR lpszStoreProvider,
    DWORD dwMsgAndCertEncodingType,
    ULONG_PTR hCryptProv,
    DWORD dwFlags,
    const void* pvPara
);

typedef PCCERT_CONTEXT (WINAPI* CertCreateCertificateContext_t)(
    DWORD dwCertEncodingType,
    const BYTE* pbCertEncoded,
    DWORD cbCertEncoded
);

typedef PCCERT_CONTEXT (WINAPI* CertFindCertificateInStore_t)(
    HCERTSTORE hCertStore,
    DWORD dwCertEncodingType,
    DWORD dwFindFlags,
    DWORD dwFindType,
    const void* pvFindPara,
    PCCERT_CONTEXT pfPrevCertContext
);

typedef BOOL (WINAPI* CertAddCertificateContextToStore_t)(
    HCERTSTORE hCertStore,
    PCCERT_CONTEXT pCertContext,
    DWORD dwAddDisposition,
    PCCERT_CONTEXT* pStoreContext
);

typedef BOOL (WINAPI* CertSetCertificateContextProperty_t)(
    PCCERT_CONTEXT pCertContext,
    DWORD dwPropId,
    DWORD dwFlags,
    const void* pvData
);

typedef BOOL (WINAPI* CertDeleteCertificateFromStore_t)(
    PCCERT_CONTEXT pCertContext
);

typedef BOOL (WINAPI* CertFreeCertificateContext_t)(
    PCCERT_CONTEXT pCertContext
);

typedef BOOL (WINAPI* CertCloseStore_t)(
    HCERTSTORE hCertStore,
    DWORD dwFlags
);

typedef DWORD (WINAPI* CertGetNameStringA_t)(
    PCCERT_CONTEXT pCertContext,
    DWORD dwType,
    DWORD dwFlags,
    void* pvTypePara,
    LPCSTR pszNameString,
    DWORD cchNameString
);

typedef BOOL (WINAPI* CryptEncodeObject_t)(
    DWORD dwCertEncodingType,
    LPCSTR lpszStructType,
    const void* pvStructInfo,
    BYTE* pbEncoded,
    DWORD* pcbEncoded
);

typedef BOOL (WINAPI* CryptDecodeObject_t)(
    DWORD dwCertEncodingType,
    LPCSTR lpszStructType,
    const BYTE* pbEncoded,
    DWORD cbEncoded,
    DWORD dwFlags,
    void* pvStructInfo,
    DWORD* pcbStructInfo
);

typedef BOOL (WINAPI* CertStrToNameW_t)(
    DWORD dwCertEncodingType,
    LPCWSTR pszX500,
    DWORD dwStrType,
    void* pvReserved,
    BYTE* pbEncoded,
    DWORD* pcbEncoded,
    LPCWSTR* ppszError
);

typedef BOOL (WINAPI* CryptAcquireCertificatePrivateKey_t)(
    PCCERT_CONTEXT pCert,
    DWORD dwFlags,
    void* pvReserved,
    ULONG_PTR* phCryptProvOrNCryptKey,
    DWORD* pdwKeySpec,
    BOOL* pfCallerFreeProvOrNCryptKey
);

typedef BOOL (WINAPI* CertAddEncodedCertificateToStore_t)(
    HCERTSTORE hCertStore,
    DWORD dwCertEncodingType,
    const BYTE* pbCertEncoded,
    DWORD cbCertEncoded,
    DWORD dwAddDisposition,
    PCCERT_CONTEXT* ppCertContext
);

// MiNGW32 doesn't know CERT_EXTENSIONS => redef
typedef struct _CERT_EXTENSIONS_ARRAY
{
	DWORD cExtension;
	PCERT_EXTENSION rgExtension;
} CERT_EXTENSIONS_ARRAY, *PCERT_EXTENSIONS_ARRAY;

typedef PCCERT_CONTEXT (WINAPI* CertCreateSelfSignCertificate_t)(
    ULONG_PTR hCryptProvOrNCryptKey,
    PCERT_NAME_BLOB pSubjectIssuerBlob,
    DWORD dwFlags,
    PCRYPT_KEY_PROV_INFO pKeyProvInfo,
    PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm,
    LPSYSTEMTIME pStartTime,
    LPSYSTEMTIME pEndTime,
    PCERT_EXTENSIONS_ARRAY pExtensions
);

// MinGW32 doesn't have these ones either
#ifndef CERT_ALT_NAME_URL
#define CERT_ALT_NAME_URL 7
#endif
#ifndef CERT_RDN_IA5_STRING
#define CERT_RDN_IA5_STRING 7
#endif
#ifndef szOID_PKIX_POLICY_QUALIFIER_CPS
#define szOID_PKIX_POLICY_QUALIFIER_CPS "1.3.6.1.5.5.7.2.1"
#endif
typedef struct _CERT_ALT_NAME_ENTRY_URL
{
	DWORD   dwAltNameChoice;
	union
	{
		LPWSTR  pwszURL;
	};
} CERT_ALT_NAME_ENTRY_URL, *PCERT_ALT_NAME_ENTRY_URL;

typedef struct _CERT_ALT_NAME_INFO_URL
{
	DWORD                    cAltEntry;
	PCERT_ALT_NAME_ENTRY_URL rgAltEntry;
} CERT_ALT_NAME_INFO_URL, *PCERT_ALT_NAME_INFO_URL;

typedef struct _CERT_POLICY_QUALIFIER_INFO_REDEF
{
	LPSTR            pszPolicyQualifierId;
	CRYPT_OBJID_BLOB Qualifier;
} CERT_POLICY_QUALIFIER_INFO_REDEF, *PCERT_POLICY_QUALIFIER_INFO_REDEF;

typedef struct _CERT_POLICY_INFO_ALT
{
	LPSTR                             pszPolicyIdentifier;
	DWORD                             cPolicyQualifier;
	PCERT_POLICY_QUALIFIER_INFO_REDEF rgPolicyQualifier;
} CERT_POLICY_INFO_REDEF, *PCERT_POLICY_INFO_REDEF;

typedef struct _CERT_POLICIES_INFO_ARRAY
{
	DWORD                   cPolicyInfo;
	PCERT_POLICY_INFO_REDEF rgPolicyInfo;
} CERT_POLICIES_INFO_ARRAY, *PCERT_POLICIES_INFO_ARRAY;

/*
 * WinTrust.dll
 */
#define CRYPTCAT_OPEN_CREATENEW			0x00000001
#define CRYPTCAT_OPEN_ALWAYS			0x00000002

#define CRYPTCAT_ATTR_AUTHENTICATED		0x10000000
#define CRYPTCAT_ATTR_UNAUTHENTICATED	0x20000000
#define CRYPTCAT_ATTR_NAMEASCII			0x00000001
#define CRYPTCAT_ATTR_NAMEOBJID			0x00000002
#define CRYPTCAT_ATTR_DATAASCII			0x00010000
#define CRYPTCAT_ATTR_DATABASE64		0x00020000
#define CRYPTCAT_ATTR_DATAREPLACE		0x00040000

#define SPC_UUID_LENGTH					16
#define SPC_URL_LINK_CHOICE				1
#define SPC_MONIKER_LINK_CHOICE			2
#define SPC_FILE_LINK_CHOICE			3
#define SHA1_HASH_LENGTH				20
#define SPC_PE_IMAGE_DATA_OBJID			"1.3.6.1.4.1.311.2.1.15"
#define SPC_CAB_DATA_OBJID				"1.3.6.1.4.1.311.2.1.25"

typedef BYTE SPC_UUID[SPC_UUID_LENGTH];
typedef struct _SPC_SERIALIZED_OBJECT
{
	SPC_UUID ClassId;
	CRYPT_DATA_BLOB SerializedData;
} SPC_SERIALIZED_OBJECT, *PSPC_SERIALIZED_OBJECT;

typedef struct SPC_LINK_
{
	DWORD dwLinkChoice;
	union
	{
		LPWSTR pwszUrl;
		SPC_SERIALIZED_OBJECT Moniker;
		LPWSTR pwszFile;
	};
} SPC_LINK, *PSPC_LINK;

typedef struct _SPC_PE_IMAGE_DATA
{
	CRYPT_BIT_BLOB Flags;
	PSPC_LINK pFile;
} SPC_PE_IMAGE_DATA, *PSPC_PE_IMAGE_DATA;

// MinGW32 doesn't know this one either
typedef struct _CRYPT_ATTRIBUTE_TYPE_VALUE_REDEF
{
	LPSTR            pszObjId;
	CRYPT_OBJID_BLOB Value;
} CRYPT_ATTRIBUTE_TYPE_VALUE_REDEF;

typedef struct SIP_INDIRECT_DATA_
{
	CRYPT_ATTRIBUTE_TYPE_VALUE_REDEF Data;
	CRYPT_ALGORITHM_IDENTIFIER       DigestAlgorithm;
	CRYPT_HASH_BLOB                  Digest;
} SIP_INDIRECT_DATA, *PSIP_INDIRECT_DATA;

typedef struct CRYPTCATSTORE_
{
	DWORD      cbStruct;
	DWORD      dwPublicVersion;
	LPWSTR     pwszP7File;
	HCRYPTPROV hProv;
	DWORD      dwEncodingType;
	DWORD      fdwStoreFlags;
	HANDLE     hReserved;
	HANDLE     hAttrs;
	HCRYPTMSG  hCryptMsg;
	HANDLE     hSorted;
} CRYPTCATSTORE;

typedef struct CRYPTCATMEMBER_
{
	DWORD              cbStruct;
	LPWSTR             pwszReferenceTag;
	LPWSTR             pwszFileName;
	GUID               gSubjectType;
	DWORD              fdwMemberFlags;
	PSIP_INDIRECT_DATA pIndirectData;
	DWORD              dwCertVersion;
	DWORD              dwReserved;
	HANDLE             hReserved;
	CRYPT_ATTR_BLOB    sEncodedIndirectData;
	CRYPT_ATTR_BLOB    sEncodedMemberInfo;
} CRYPTCATMEMBER;

typedef struct CRYPTCATATTRIBUTE_
{
	DWORD  cbStruct;
	LPWSTR pwszReferenceTag;
	DWORD  dwAttrTypeAndAction;
	DWORD  cbValue;
	BYTE*   pbValue;
	DWORD  dwReserved;
} CRYPTCATATTRIBUTE;

typedef HANDLE (WINAPI* CryptCATOpen_t)(
    LPWSTR pwszFileName,
    DWORD fdwOpenFlags,
    ULONG_PTR hProv,
    DWORD dwPublicVersion,
    DWORD dwEncodingType
);

typedef BOOL (WINAPI* CryptCATClose_t)(
    HANDLE hCatalog
);

typedef CRYPTCATSTORE* (WINAPI* CryptCATStoreFromHandle_t)(
    HANDLE hCatalog
);

typedef CRYPTCATATTRIBUTE* (WINAPI* CryptCATEnumerateCatAttr_t)(
    HANDLE hCatalog,
    CRYPTCATATTRIBUTE* pPrevAttr
);

typedef CRYPTCATATTRIBUTE* (WINAPI* CryptCATPutCatAttrInfo_t)(
    HANDLE hCatalog,
    LPWSTR pwszReferenceTag,
    DWORD dwAttrTypeAndAction,
    DWORD cbData,
    BYTE* pbData
);

typedef CRYPTCATMEMBER* (WINAPI* CryptCATEnumerateMember_t)(
    HANDLE hCatalog,
    CRYPTCATMEMBER* pPrevMember
);

typedef CRYPTCATMEMBER* (WINAPI* CryptCATPutMemberInfo_t)(
    HANDLE hCatalog,
    LPWSTR pwszFileName,
    LPWSTR pwszReferenceTag,
    GUID* pgSubjectType,
    DWORD dwCertVersion,
    DWORD cbSIPIndirectData,
    BYTE* pbSIPIndirectData
);

typedef CRYPTCATATTRIBUTE* (WINAPI* CryptCATEnumerateAttr_t)(
    HANDLE hCatalog,
    CRYPTCATMEMBER* pCatMember,
    CRYPTCATATTRIBUTE* pPrevAttr
);

typedef CRYPTCATATTRIBUTE* (WINAPI* CryptCATPutAttrInfo_t)(
    HANDLE hCatalog,
    CRYPTCATMEMBER* pCatMember,
    LPWSTR pwszReferenceTag,
    DWORD dwAttrTypeAndAction,
    DWORD cbData,
    BYTE* pbData
);

typedef BOOL (WINAPI* CryptCATPersistStore_t)(
    HANDLE hCatalog
);

typedef BOOL (WINAPI* CryptCATAdminCalcHashFromFileHandle_t)(
    HANDLE hFile,
    DWORD* pcbHash,
    BYTE* pbHash,
    DWORD dwFlags
);

/*
 * Parts of the following functions are based on:
 * http://blogs.msdn.com/b/alejacma/archive/2009/03/16/how-to-create-a-self-signed-certificate-with-cryptoapi-c.aspx
 * http://blogs.msdn.com/b/alejacma/archive/2008/12/11/how-to-sign-exe-files-with-an-authenticode-certificate-part-2.aspx
 * http://www.jensign.com/hash/index.html
 */

/*
 * Add a certificate, identified by its pCertContext, to the system store 'szStoreName'
 */
BOOL AddCertToStore(PCCERT_CONTEXT pCertContext, LPCSTR szStoreName)
{
	PF_DECL(CertOpenStore);
	PF_DECL(CertSetCertificateContextProperty);
	PF_DECL(CertAddCertificateContextToStore);
	PF_DECL(CertCloseStore);
	HCERTSTORE hSystemStore = NULL;
	CRYPT_DATA_BLOB libwdiNameBlob = {14, (BYTE*)L"libwdi"};
	BOOL r = FALSE;

	PF_INIT_OR_OUT(CertOpenStore, crypt32);
	PF_INIT_OR_OUT(CertSetCertificateContextProperty, crypt32);
	PF_INIT_OR_OUT(CertAddCertificateContextToStore, crypt32);
	PF_INIT_OR_OUT(CertCloseStore, crypt32);

	hSystemStore = pfCertOpenStore(CERT_STORE_PROV_SYSTEM_A, X509_ASN_ENCODING,
	                               0, CERT_SYSTEM_STORE_LOCAL_MACHINE, szStoreName);
	if (hSystemStore == NULL)
	{
		USBWRNN(L"failed to open system store '%S': %s", szStoreName, windows_error_str(0));
		goto out;
	}

	if (!pfCertSetCertificateContextProperty(pCertContext, CERT_FRIENDLY_NAME_PROP_ID, 0, &libwdiNameBlob))
	{
		USBWRNN(L"coud not set friendly name: %s", windows_error_str(0));
		goto out;
	}

	if (!pfCertAddCertificateContextToStore(hSystemStore, pCertContext, CERT_STORE_ADD_REPLACE_EXISTING, NULL))
	{
		USBWRNN(L"failed to add certificate to system store '%S': %s", szStoreName, windows_error_str(0));
		goto out;
	}
	r = TRUE;

out:
	if (hSystemStore != NULL) pfCertCloseStore(hSystemStore, 0);
	return r;
}

/*
 * Remove a certificate, identified by its subject, to the system store 'szStoreName'
 */
BOOL RemoveCertFromStore(LPCWSTR szCertSubject, LPCSTR szStoreName)
{
	PF_DECL(CertOpenStore);
	PF_DECL(CertFindCertificateInStore);
	PF_DECL(CertDeleteCertificateFromStore);
	PF_DECL(CertCloseStore);
	PF_DECL(CertStrToNameW);
	HCERTSTORE hSystemStore = NULL;
	PCCERT_CONTEXT pCertContext;
	CERT_NAME_BLOB certNameBlob = {0, NULL};
	BOOL r = FALSE;

	PF_INIT_OR_OUT(CertOpenStore, crypt32);
	PF_INIT_OR_OUT(CertFindCertificateInStore, crypt32);
	PF_INIT_OR_OUT(CertDeleteCertificateFromStore, crypt32);
	PF_INIT_OR_OUT(CertCloseStore, crypt32);
	PF_INIT_OR_OUT(CertStrToNameW, crypt32);

	hSystemStore = pfCertOpenStore(CERT_STORE_PROV_SYSTEM_A, X509_ASN_ENCODING,
	                               0, CERT_SYSTEM_STORE_LOCAL_MACHINE, szStoreName);
	if (hSystemStore == NULL)
	{
		USBWRNN(L"failed to open system store '%S': %s", szStoreName, windows_error_str(0));
		goto out;
	}

	// Encode Cert Name
	if ( (!pfCertStrToNameW(X509_ASN_ENCODING, szCertSubject, CERT_X500_NAME_STR, NULL, NULL, &certNameBlob.cbData, NULL))
	        || ((certNameBlob.pbData = (BYTE*)malloc(certNameBlob.cbData)) == NULL)
	        || (!pfCertStrToNameW(X509_ASN_ENCODING, szCertSubject, CERT_X500_NAME_STR, NULL, certNameBlob.pbData, &certNameBlob.cbData, NULL)) )
	{
		USBWRNN(L"failed to encode'%s': %s", szCertSubject, windows_error_str(0));
		goto out;
	}

	pCertContext = NULL;
	while ((pCertContext = pfCertFindCertificateInStore(hSystemStore, X509_ASN_ENCODING, 0,
	                       CERT_FIND_SUBJECT_NAME, (const void*)&certNameBlob, NULL)) != NULL)
	{
		pfCertDeleteCertificateFromStore(pCertContext);
		USBMSGN(L"Deleted existing certificate: %s (%s store)", szCertSubject, MbsToTempWcs(szStoreName));
	}
	r = TRUE;

out:
	if (certNameBlob.pbData != NULL) free (certNameBlob.pbData);
	if (hSystemStore != NULL) pfCertCloseStore(hSystemStore, 0);
	return r;
}

/*
 * Add certificate data to the TrustedPublisher system store
 * Unless bDisableWarning is set, warn the user before install
 */
BOOL AddCertToTrustedPublisher(BYTE* pbCertData, DWORD dwCertSize, BOOL bDisableWarning, HWND hWnd)
{
	PF_DECL(CertOpenStore);
	PF_DECL(CertCreateCertificateContext);
	PF_DECL(CertFindCertificateInStore);
	PF_DECL(CertAddCertificateContextToStore);
	PF_DECL(CertFreeCertificateContext);
	PF_DECL(CertGetNameStringA);
	PF_DECL(CertCloseStore);
	BOOL r = FALSE;
	int user_input;
	HCERTSTORE hSystemStore = NULL;
	PCCERT_CONTEXT pCertContext = NULL, pStoreCertContext = NULL;
	char org[MAX_PATH], org_unit[MAX_PATH];
	char msg_string[1024];

	PF_INIT_OR_OUT(CertOpenStore, crypt32);
	PF_INIT_OR_OUT(CertCreateCertificateContext, crypt32);
	PF_INIT_OR_OUT(CertFindCertificateInStore, crypt32);
	PF_INIT_OR_OUT(CertAddCertificateContextToStore, crypt32);
	PF_INIT_OR_OUT(CertFreeCertificateContext, crypt32);
	PF_INIT_OR_OUT(CertGetNameStringA, crypt32);
	PF_INIT_OR_OUT(CertCloseStore, crypt32);

	hSystemStore = pfCertOpenStore(CERT_STORE_PROV_SYSTEM_A, X509_ASN_ENCODING,
	                               0, CERT_SYSTEM_STORE_LOCAL_MACHINE, "TrustedPublisher");

	if (hSystemStore == NULL)
	{
		USBWRNN(L"unable to open system store: %s", windows_error_str(0));
		goto out;
	}

	/* Check whether certificate already exists
	 * We have to do this manually, so that we can produce a warning to the user
	 * before any certificate is added to the store (first time or update)
	 */
	pCertContext = pfCertCreateCertificateContext(X509_ASN_ENCODING, pbCertData, dwCertSize);

	if (pCertContext == NULL)
	{
		USBWRNN(L"could not create context for certificate: %s", windows_error_str(0));
		pfCertCloseStore(hSystemStore, 0);
		goto out;
	}

	pStoreCertContext = pfCertFindCertificateInStore(hSystemStore, X509_ASN_ENCODING, 0,
	                    CERT_FIND_EXISTING, (const void*)pCertContext, NULL);
	if (pStoreCertContext == NULL)
	{
		user_input = IDOK;
		if (!bDisableWarning)
		{
			org[0] = 0;
			org_unit[0] = 0;
			pfCertGetNameStringA(pCertContext, CERT_NAME_ATTR_TYPE, 0, szOID_ORGANIZATION_NAME, org, sizeof(org));
			pfCertGetNameStringA(pCertContext, CERT_NAME_ATTR_TYPE, 0, szOID_ORGANIZATIONAL_UNIT_NAME, org_unit, sizeof(org_unit));
			_snprintf(msg_string, sizeof(msg_string), "Warning: this software is about to install the following organization\n"
			          "as a Trusted Publisher on your system:\n\n '%s%s%s%s'\n\n"
			          "This will allow this Publisher to run software with elevated privileges,\n"
			          "as well as install driver packages, without further security notices.\n\n"
			          "If this is not what you want, you can cancel this operation now.", org,
			          (org_unit[0] != 0) ? " (" : "", org_unit, (org_unit[0] != 0) ? ")" : "");
			user_input = MessageBoxA(hWnd, msg_string,
			                         "Warning: Trusted Certificate installation", MB_OKCANCEL | MB_ICONWARNING);
		}
		if (user_input != IDOK)
		{
			USBMSGN(L"operation cancelled by the user");
		}
		else
		{
			if (!pfCertAddCertificateContextToStore(hSystemStore, pCertContext, CERT_STORE_ADD_NEWER, NULL))
			{
				USBWRNN(L"could not add certificate: %s", windows_error_str(0));
			}
			else
			{
				r = TRUE;
			}
		}
	}
	else
	{
		r = TRUE;	// Cert already exists
	}

out:
	if (pCertContext != NULL) pfCertFreeCertificateContext(pCertContext);
	if (pStoreCertContext != NULL) pfCertFreeCertificateContext(pStoreCertContext);
	if (hSystemStore) pfCertCloseStore(hSystemStore, 0);
	return r;
}

/*
 * Create a self signed certificate for code signing
 */
PCCERT_CONTEXT CreateSelfSignedCert(LPCWSTR szCertSubject)
{
	PF_DECL(CryptEncodeObject);
	PF_DECL(CertStrToNameW);
	PF_DECL(CertCreateSelfSignCertificate);
	PF_DECL(CertFreeCertificateContext);

	BOOL success = FALSE;
	DWORD dwSize;
	HCRYPTPROV hCSP = 0;
	HCRYPTKEY hKey = 0;
	PCCERT_CONTEXT pCertContext = NULL;
	CERT_NAME_BLOB SubjectIssuerBlob = {0, NULL};
	CRYPT_KEY_PROV_INFO KeyProvInfo;
	CRYPT_ALGORITHM_IDENTIFIER SignatureAlgorithm;
	LPWSTR wszKeyContainer = KEY_CONTAINER;
	LPBYTE pbEnhKeyUsage = NULL, pbAltNameInfo = NULL, pbCPSNotice = NULL, pbPolicyInfo = NULL;
	SYSTEMTIME sExpirationDate = { 2029, 01, 01, 01, 00, 00, 00, 000 };
	CERT_EXTENSION certExtension[3];
	CERT_EXTENSIONS_ARRAY certExtensionsArray;
	// Code Signing Enhanced Key Usage
	LPSTR szCertPolicyElementId = "1.3.6.1.5.5.7.3.3"; // szOID_PKIX_KP_CODE_SIGNING;
	CERT_ENHKEY_USAGE certEnhKeyUsage;
	// Alternate Name (URL)
	CERT_ALT_NAME_ENTRY_URL certAltNameEntry;
	CERT_ALT_NAME_INFO_URL certAltNameInfo;
	// Certificate Policies
	CERT_POLICY_QUALIFIER_INFO_REDEF certPolicyQualifier;
	CERT_POLICY_INFO_REDEF certPolicyInfo;
	CERT_POLICIES_INFO_ARRAY certPolicyInfoArray;
	CHAR szCPSName[] = "http://libwdi-cps.akeo.ie";
	CERT_NAME_VALUE certCPSValue;


	certEnhKeyUsage.cUsageIdentifier		= 1;
	certEnhKeyUsage.rgpszUsageIdentifier	= &szCertPolicyElementId;

	certAltNameEntry.dwAltNameChoice		= CERT_ALT_NAME_URL;
	certAltNameEntry.pwszURL				= L"http://libwdi.akeo.ie";

	certAltNameInfo.cAltEntry				= 1;
	certAltNameInfo.rgAltEntry				= &certAltNameEntry;

	certPolicyInfo.cPolicyQualifier			= 1;
	certPolicyInfo.pszPolicyIdentifier		= "1.3.6.1.5.5.7.2.1";
	certPolicyInfo.rgPolicyQualifier		= &certPolicyQualifier;

	certPolicyInfoArray.cPolicyInfo			= 1;
	certPolicyInfoArray.rgPolicyInfo		= &certPolicyInfo;

	PF_INIT_OR_OUT(CryptEncodeObject, crypt32);
	PF_INIT_OR_OUT(CertStrToNameW, crypt32);
	PF_INIT_OR_OUT(CertCreateSelfSignCertificate, crypt32);
	PF_INIT_OR_OUT(CertFreeCertificateContext, crypt32);

	// Set Enhanced Key Usage extension to Code Signing only
	if ( (!pfCryptEncodeObject(X509_ASN_ENCODING, X509_ENHANCED_KEY_USAGE, (LPVOID)&certEnhKeyUsage, NULL, &dwSize))
	        || ((pbEnhKeyUsage = (BYTE*)malloc(dwSize)) == NULL)
	        || (!pfCryptEncodeObject(X509_ASN_ENCODING, X509_ENHANCED_KEY_USAGE, (LPVOID)&certEnhKeyUsage, pbEnhKeyUsage, &dwSize)) )
	{
		USBWRNN(L"could not setup EKU for code signing: %s", windows_error_str(0));
		goto out;
	}
	certExtension[0].pszObjId = szOID_ENHANCED_KEY_USAGE;
	certExtension[0].fCritical = TRUE;		// only allow code signing
	certExtension[0].Value.cbData = dwSize;
	certExtension[0].Value.pbData = pbEnhKeyUsage;

	// Set URL as Alt Name parameter
	if ( (!pfCryptEncodeObject(X509_ASN_ENCODING, X509_ALTERNATE_NAME, (LPVOID)&certAltNameInfo, NULL, &dwSize))
	        || ((pbAltNameInfo = (BYTE*)malloc(dwSize)) == NULL)
	        || (!pfCryptEncodeObject(X509_ASN_ENCODING, X509_ALTERNATE_NAME, (LPVOID)&certAltNameInfo, pbAltNameInfo, &dwSize)) )
	{
		USBWRNN(L"could not setup URL: %s", windows_error_str(0));
		goto out;
	}
	certExtension[1].pszObjId = szOID_SUBJECT_ALT_NAME;
	certExtension[1].fCritical = FALSE;
	certExtension[1].Value.cbData = dwSize;
	certExtension[1].Value.pbData = pbAltNameInfo;

	// Set the CPS Certificate Policies field - this enables the "Issuer Statement" button on the cert
	certCPSValue.dwValueType = CERT_RDN_IA5_STRING;
	certCPSValue.Value.cbData = sizeof(szCPSName);
	certCPSValue.Value.pbData = (BYTE*)szCPSName;
	if ( (!pfCryptEncodeObject(X509_ASN_ENCODING, X509_NAME_VALUE, (LPVOID)&certCPSValue, NULL, &dwSize))
	        || ((pbCPSNotice = (BYTE*)malloc(dwSize)) == NULL)
	        || (!pfCryptEncodeObject(X509_ASN_ENCODING, X509_NAME_VALUE, (LPVOID)&certCPSValue, pbCPSNotice, &dwSize)) )
	{
		USBWRNN(L"could not setup CPS: %s", windows_error_str(0));
		goto out;
	}

	certPolicyQualifier.pszPolicyQualifierId = szOID_PKIX_POLICY_QUALIFIER_CPS;
	certPolicyQualifier.Qualifier.cbData = dwSize;
	certPolicyQualifier.Qualifier.pbData = pbCPSNotice;
	if ( (!pfCryptEncodeObject(X509_ASN_ENCODING, X509_CERT_POLICIES, (LPVOID)&certPolicyInfoArray, NULL, &dwSize))
	        || ((pbPolicyInfo = (BYTE*)malloc(dwSize)) == NULL)
	        || (!pfCryptEncodeObject(X509_ASN_ENCODING, X509_CERT_POLICIES, (LPVOID)&certPolicyInfoArray, pbPolicyInfo, &dwSize)) )
	{
		USBWRNN(L"could not setup Certificate Policies: %s", windows_error_str(0));
		goto out;
	}
	certExtension[2].pszObjId = szOID_CERT_POLICIES;
	certExtension[2].fCritical = FALSE;
	certExtension[2].Value.cbData = dwSize;
	certExtension[2].Value.pbData = pbPolicyInfo;

	certExtensionsArray.cExtension = ARRAYSIZE(certExtension);
	certExtensionsArray.rgExtension = certExtension;
	USBDBGN(L"Set Enhanced Key Usage, URL and CPS..");

	if (CryptAcquireContextW(&hCSP, wszKeyContainer, NULL, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET | CRYPT_SILENT))
	{
		USBDBGN(L"Acquired existing key container..");
	}
	else if ( (GetLastError() == NTE_BAD_KEYSET)
	          && (CryptAcquireContextW(&hCSP, wszKeyContainer, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET | CRYPT_MACHINE_KEYSET | CRYPT_SILENT)) )
	{
		USBDBGN(L"Created new key container..");
	}
	else
	{
		USBWRNN(L"could not obtain a key container: %s", windows_error_str(0));
		goto out;
	}

	// Generate key pair (0x0400XXXX = RSA 1024 bit)
	if (!CryptGenKey(hCSP, AT_SIGNATURE, 0x04000000 | CRYPT_EXPORTABLE, &hKey))
	{
		USBDBGN(L"could not generate keypair: %s", windows_error_str(0));
		goto out;
	}
	USBDBGN(L"Generated new keypair.");

	// Set the subject
	if ( (!pfCertStrToNameW(X509_ASN_ENCODING, szCertSubject, CERT_X500_NAME_STR, NULL, NULL, &SubjectIssuerBlob.cbData, NULL))
	        || ((SubjectIssuerBlob.pbData = (BYTE*)malloc(SubjectIssuerBlob.cbData)) == NULL)
	        || (!pfCertStrToNameW(X509_ASN_ENCODING, szCertSubject, CERT_X500_NAME_STR, NULL, SubjectIssuerBlob.pbData, &SubjectIssuerBlob.cbData, NULL)) )
	{
		USBWRNN(L"could not encode subject name for self signed cert: %s", windows_error_str(0));
		goto out;
	}

	// Prepare key provider structure for self-signed certificate
	memset(&KeyProvInfo, 0, sizeof(KeyProvInfo));
	KeyProvInfo.pwszContainerName = wszKeyContainer;
	KeyProvInfo.pwszProvName = NULL;
	KeyProvInfo.dwProvType = PROV_RSA_FULL;
	KeyProvInfo.dwFlags = CRYPT_MACHINE_KEYSET;
	KeyProvInfo.cProvParam = 0;
	KeyProvInfo.rgProvParam = NULL;
	KeyProvInfo.dwKeySpec = AT_SIGNATURE;

	// Prepare algorithm structure for self-signed certificate
	memset(&SignatureAlgorithm, 0, sizeof(SignatureAlgorithm));
	SignatureAlgorithm.pszObjId = szOID_RSA_SHA1RSA;

	// Create self-signed certificate
	pCertContext = pfCertCreateSelfSignCertificate((ULONG_PTR)NULL,
	               &SubjectIssuerBlob, 0, &KeyProvInfo, &SignatureAlgorithm, NULL, &sExpirationDate, &certExtensionsArray);
	if (pCertContext == NULL)
	{
		USBWRNN(L"could not create self signed certificate: %s", windows_error_str(0));
		goto out;
	}
	USBMSGN(L"Created new self-signed certificate: %s", szCertSubject);
	success = TRUE;

out:
	if (pbEnhKeyUsage != NULL) free(pbEnhKeyUsage);
	if (pbAltNameInfo != NULL) free(pbAltNameInfo);
	if (pbCPSNotice != NULL) free(pbCPSNotice);
	if (pbPolicyInfo != NULL) free(pbPolicyInfo);
	if (SubjectIssuerBlob.pbData != NULL) free(SubjectIssuerBlob.pbData);
	if (hKey) CryptDestroyKey(hKey);
	if (hCSP) CryptReleaseContext(hCSP, 0);
	if ((!success) && (pCertContext != NULL))
	{
		pfCertFreeCertificateContext(pCertContext);
		pCertContext = NULL;
	}
	return pCertContext;
}

/*
 * Delete the private key associated with a specific cert
 */
BOOL DeletePrivateKey(PCCERT_CONTEXT pCertContext)
{
	PF_DECL(CryptAcquireCertificatePrivateKey);
	PF_DECL(CertOpenStore);
	PF_DECL(CertCloseStore);
	PF_DECL(CertAddEncodedCertificateToStore);
	PF_DECL(CertSetCertificateContextProperty);
	PF_DECL(CertFreeCertificateContext);

	LPWSTR wszKeyContainer = KEY_CONTAINER;
	HCRYPTPROV hCSP = 0;
	DWORD dwKeySpec;
	BOOL bFreeCSP = FALSE, r = FALSE;
	HCERTSTORE hSystemStore;
	LPCSTR szStoresToUpdate[2] = { "Root", "TrustedPublisher" };
	CRYPT_DATA_BLOB libwdiNameBlob = {14, (BYTE*)L"libwdi"};
	PCCERT_CONTEXT pCertContextUpdate = NULL;
	int i;

	PF_INIT_OR_OUT(CryptAcquireCertificatePrivateKey, crypt32);
	PF_INIT_OR_OUT(CertOpenStore, crypt32);
	PF_INIT_OR_OUT(CertCloseStore, crypt32);
	PF_INIT_OR_OUT(CertAddEncodedCertificateToStore, crypt32);
	PF_INIT_OR_OUT(CertSetCertificateContextProperty, crypt32);
	PF_INIT_OR_OUT(CertFreeCertificateContext, crypt32);

	if (!pfCryptAcquireCertificatePrivateKey(pCertContext, CRYPT_ACQUIRE_SILENT_FLAG, NULL, &hCSP, &dwKeySpec, &bFreeCSP))
	{
		USBWRNN(L"error getting CSP: %s", windows_error_str(0));
		goto out;
	}

	if (!CryptAcquireContextW(&hCSP, wszKeyContainer, NULL, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET | CRYPT_SILENT | CRYPT_DELETEKEYSET))
	{
		USBWRNN(L"failed to delete private key: %s", windows_error_str(0));
	}

	// This is optional, but unless we don't reimport the cert data after having deleted the key
	// end users will still see a "You have a private key that corresponds to this certificate" message.
	for (i = 0; i < ARRAYSIZE(szStoresToUpdate); i++)
	{
		hSystemStore = pfCertOpenStore(CERT_STORE_PROV_SYSTEM_A, X509_ASN_ENCODING,
		                               0, CERT_SYSTEM_STORE_LOCAL_MACHINE, szStoresToUpdate[i]);
		if (hSystemStore == NULL) continue;

		if (!pfCertAddEncodedCertificateToStore(hSystemStore, X509_ASN_ENCODING, pCertContext->pbCertEncoded,
		                                        pCertContext->cbCertEncoded, CERT_STORE_ADD_REPLACE_EXISTING, &pCertContextUpdate))
		{
			USBWRNN(L"failed to update '%s': %s", MbsToTempWcs(szStoresToUpdate[i]), windows_error_str(0));
		}

		// The friendly name is lost in this operation - restore it
		if (!pfCertSetCertificateContextProperty(pCertContextUpdate, CERT_FRIENDLY_NAME_PROP_ID, 0, &libwdiNameBlob))
		{
			USBWRNN(L"coud not set friendly name: %s", windows_error_str(0));
		}

		pfCertFreeCertificateContext(pCertContextUpdate);

		pfCertCloseStore(hSystemStore, 0);
	}

	r = TRUE;

out:
	if ((bFreeCSP) && (hCSP))
	{
		CryptReleaseContext(hCSP, 0);
	}
	return r;
}

/*
 * Digitally sign a file and make it system-trusted by:
 * - creating a self signed certificate for code signing
 * - adding this certificate to both the Root and TrustedPublisher system stores
 * - signing the file provided
 * - deleting the self signed certificate private key so that it cannot be reused
 */
BOOL SelfSignFile(LPCWSTR szFileName, LPCWSTR szCertSubject)
{
	PF_DECL(SignerSignEx);
	PF_DECL(SignerFreeSignerContext);
	PF_DECL(CertFreeCertificateContext);
	PF_DECL(CertCloseStore);

	BOOL r = FALSE;
	HRESULT hResult = S_OK;
	HCERTSTORE hCertStore = NULL;
	PCCERT_CONTEXT pCertContext = NULL;
	DWORD dwIndex;
	SIGNER_FILE_INFO signerFileInfo;
	SIGNER_SUBJECT_INFO signerSubjectInfo;
	SIGNER_CERT_STORE_INFO signerCertStoreInfo;
	SIGNER_CERT signerCert;
	SIGNER_SIGNATURE_INFO signerSignatureInfo;
	PSIGNER_CONTEXT pSignerContext = NULL;
	CRYPT_ATTRIBUTES_ARRAY cryptAttributesArray;
	CRYPT_ATTRIBUTE cryptAttribute[2];
	CRYPT_INTEGER_BLOB oidSpOpusInfoBlob, oidStatementTypeBlob;
	BYTE pbOidSpOpusInfo[] = SP_OPUS_INFO_DATA;
	BYTE pbOidStatementType[] = STATEMENT_TYPE_DATA;

	PF_INIT_OR_OUT(SignerSignEx, mssign32);
	PF_INIT_OR_OUT(SignerFreeSignerContext, mssign32);
	PF_INIT_OR_OUT(CertFreeCertificateContext, crypt32);
	PF_INIT_OR_OUT(CertCloseStore, crypt32);

	// Delete any previous certificate with the same subject
	RemoveCertFromStore(szCertSubject, "Root");
	RemoveCertFromStore(szCertSubject, "TrustedPublisher");

	pCertContext = CreateSelfSignedCert(szCertSubject);
	if (pCertContext == NULL)
	{
		goto out;
	}
	if ( (!AddCertToStore(pCertContext, "Root"))
	        || (!AddCertToStore(pCertContext, "TrustedPublisher")) )
	{
		goto out;
	}
	USBMSGN(L"Added %s certificate to 'Root' and 'TrustedPublisher' stores..", szCertSubject);

	// Setup SIGNER_FILE_INFO struct
	signerFileInfo.cbSize = sizeof(SIGNER_FILE_INFO);
	signerFileInfo.pwszFileName = szFileName;
	signerFileInfo.hFile = NULL;

	// Prepare SIGNER_SUBJECT_INFO struct
	signerSubjectInfo.cbSize = sizeof(SIGNER_SUBJECT_INFO);
	dwIndex = 0;
	signerSubjectInfo.pdwIndex = &dwIndex;
	signerSubjectInfo.dwSubjectChoice = SIGNER_SUBJECT_FILE;
	signerSubjectInfo.pSignerFileInfo = &signerFileInfo;

	// Prepare SIGNER_CERT_STORE_INFO struct
	signerCertStoreInfo.cbSize = sizeof(SIGNER_CERT_STORE_INFO);
	signerCertStoreInfo.pSigningCert = pCertContext;
	signerCertStoreInfo.dwCertPolicy = SIGNER_CERT_POLICY_CHAIN;
	signerCertStoreInfo.hCertStore = NULL;

	// Prepare SIGNER_CERT struct
	signerCert.cbSize = sizeof(SIGNER_CERT);
	signerCert.dwCertChoice = SIGNER_CERT_STORE;
	signerCert.pCertStoreInfo = &signerCertStoreInfo;
	signerCert.hwnd = NULL;

	// Prepare the additional Authenticode OIDs
	oidSpOpusInfoBlob.cbData = sizeof(pbOidSpOpusInfo);
	oidSpOpusInfoBlob.pbData = pbOidSpOpusInfo;
	oidStatementTypeBlob.cbData = sizeof(pbOidStatementType);
	oidStatementTypeBlob.pbData = pbOidStatementType;
	cryptAttribute[0].cValue = 1;
	cryptAttribute[0].rgValue = &oidSpOpusInfoBlob;
	cryptAttribute[0].pszObjId = "1.3.6.1.4.1.311.2.1.12"; // SPC_SP_OPUS_INFO_OBJID in wintrust.h
	cryptAttribute[1].cValue = 1;
	cryptAttribute[1].rgValue = &oidStatementTypeBlob;
	cryptAttribute[1].pszObjId = "1.3.6.1.4.1.311.2.1.11"; // SPC_STATEMENT_TYPE_OBJID in wintrust.h
	cryptAttributesArray.cAttr = 2;
	cryptAttributesArray.rgAttr = cryptAttribute;

	// Prepare SIGNER_SIGNATURE_INFO struct
	signerSignatureInfo.cbSize = sizeof(SIGNER_SIGNATURE_INFO);
	signerSignatureInfo.algidHash = CALG_SHA1;
	signerSignatureInfo.dwAttrChoice = SIGNER_NO_ATTR;
	signerSignatureInfo.pAttrAuthcode = NULL;
	signerSignatureInfo.psAuthenticated = &cryptAttributesArray;
	signerSignatureInfo.psUnauthenticated = NULL;

	// Sign file with cert
	hResult = pfSignerSignEx(0, &signerSubjectInfo, &signerCert, &signerSignatureInfo, NULL, NULL, NULL, NULL, &pSignerContext);
	if (hResult != S_OK)
	{
		USBWRNN(L"SignerSignEx failed. hResult #%X, error %s", hResult, windows_error_str(0));
		goto out;
	}
	r = TRUE;
	USBMSGN(L"Signed file: %s", szFileName);

	// Clean up
out:
	/*
	 * Because we installed our certificate as a Root CA as well as a Trusted Publisher
	 * we *MUST* ensure that the private key is destroyed, so that it cannot be reused
	 * by an attacker to self sign a malicious applications.
	 */
	if ((pCertContext != NULL) && (DeletePrivateKey(pCertContext)))
	{
		USBMSGN(L"Deleted private key..");
	}

	if (r == TRUE)
	{
		USBMSGN(L"Success!");
	}
	if (pSignerContext != NULL) pfSignerFreeSignerContext(pSignerContext);
	if (pCertContext != NULL) pfCertFreeCertificateContext(pCertContext);
	if (hCertStore) pfCertCloseStore(hCertStore, 0);

	return r;
}

/*
 * Opens a file and computes the SHA1 Authenticode Hash
 */
static BOOL CalcHash(BYTE* pbHash, LPCWSTR szfilePath)
{
	PF_DECL(CryptCATAdminCalcHashFromFileHandle);
	BOOL r = FALSE;
	HANDLE hFile = NULL;
	DWORD cbHash = SHA1_HASH_LENGTH;

	PF_INIT_OR_OUT(CryptCATAdminCalcHashFromFileHandle, wintrust);

	// Compute the SHA1 hash
	hFile = CreateFileW(szfilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) goto out;
	if ( (!pfCryptCATAdminCalcHashFromFileHandle(hFile, &cbHash, pbHash, 0)) ) goto out;
	r = TRUE;

out:
	if (hFile) CloseHandle(hFile);
	return r;
}

/*
 * Add a new member to a cat file, containing the hash for the relevant file
 */
static BOOL AddFileHash(HANDLE hCat, LPCWSTR szFileName, BYTE* pbFileHash)
{
	const GUID inf_guid = {0xDE351A42, 0x8E59, 0x11D0, {0x8C, 0x47, 0x00, 0xC0, 0x4F, 0xC2, 0x95, 0xEE}};
	const GUID pe_guid = {0xC689AAB8, 0x8E78, 0x11D0, {0x8C, 0x47, 0x00, 0xC0, 0x4F, 0xC2, 0x95, 0xEE}};
	const BYTE fImageData = 0xA0;		// Flags used for the SPC_PE_IMAGE_DATA "<<<Obsolete>>>" link
	LPCWSTR wszOSAttr = L"2:5.1,2:5.2,2:6.0,2:6.1";

	PF_DECL(CryptCATPutMemberInfo);
	PF_DECL(CryptCATPutAttrInfo);
	PF_DECL(CryptEncodeObject);

	BOOL bPEType = TRUE;
	CRYPTCATMEMBER* pCatMember = NULL;
	SIP_INDIRECT_DATA sSIPData;
	SPC_LINK sSPCLink;
	SPC_PE_IMAGE_DATA sSPCImageData;
	WCHAR wszHash[2 * SHA1_HASH_LENGTH + 1];
	BYTE pbEncoded[64];
	DWORD cbEncoded;
	int i;
	BOOL r = FALSE;

	PF_INIT_OR_OUT(CryptCATPutMemberInfo, wintrust);
	PF_INIT_OR_OUT(CryptCATPutAttrInfo, wintrust);
	PF_INIT_OR_OUT(CryptEncodeObject, crypt32);

	// Create the required UTF-16 strings
	for (i = 0; i < SHA1_HASH_LENGTH; i++)
	{
		_snwprintf((wchar_t*)(&wszHash[2 * i]), 3, L"%02X", pbFileHash[i]);
	}


	// Set the PE or CAB/INF type according to the extension
	if (PathMatchSpecW(szFileName, L"*.dll") ||
	        PathMatchSpecW(szFileName, L"*.sys") ||
	        PathMatchSpecW(szFileName, L"*.exe"))
	{
		USBDBGN(L"Using PE guid..");
	}
	else if (PathMatchSpecW(szFileName, L"*.inf"))
	{
		USBDBGN(L"Using INF guid..");
		bPEType = FALSE;
	}
	else
	{
		USBWRNN(L"unhandled file type: '%s' - ignoring", szFileName);
		goto out;
	}

	// An "<<<Obsolete>>>" Authenticode link must be populated for each entry
	sSPCLink.dwLinkChoice = SPC_FILE_LINK_CHOICE;
	sSPCLink.pwszUrl = L"<<<Obsolete>>>";
	cbEncoded = sizeof(pbEncoded);
	// PE and INF encode the link differently
	if (bPEType)
	{
		sSPCImageData.Flags.cbData = 1;
		sSPCImageData.Flags.cUnusedBits = 0;
		sSPCImageData.Flags.pbData = (BYTE*)&fImageData;
		sSPCImageData.pFile = &sSPCLink;
		if (!pfCryptEncodeObject(X509_ASN_ENCODING, SPC_PE_IMAGE_DATA_OBJID, &sSPCImageData, pbEncoded, &cbEncoded))
		{
			USBWRNN(L"unable to encode SPC Image Data: %s", windows_error_str(0));
			goto out;
		}
	}
	else
	{
		if (!pfCryptEncodeObject(X509_ASN_ENCODING, SPC_CAB_DATA_OBJID, &sSPCLink, pbEncoded, &cbEncoded))
		{
			USBWRNN(L"unable to encode SPC Image Data: %s", windows_error_str(0));
			goto out;
		}
	}

	// Populate the SHA1 Hash OID
	sSIPData.Data.pszObjId = (bPEType) ? SPC_PE_IMAGE_DATA_OBJID : SPC_CAB_DATA_OBJID;
	sSIPData.Data.Value.cbData = cbEncoded;
	sSIPData.Data.Value.pbData = pbEncoded;
	sSIPData.DigestAlgorithm.pszObjId = szOID_OIWSEC_sha1;
	sSIPData.DigestAlgorithm.Parameters.cbData = 0;
	sSIPData.Digest.cbData = SHA1_HASH_LENGTH;
	sSIPData.Digest.pbData = pbFileHash;

	// Create the new member
	if ((pCatMember = pfCryptCATPutMemberInfo(hCat, NULL, wszHash, (GUID*)((bPEType) ? &pe_guid : &inf_guid),
	                  0x200, sizeof(sSIPData), (BYTE*)&sSIPData)) == NULL)
	{
		USBWRNN(L"unable to create cat entry for file '%s': %s", szFileName, windows_error_str(0));
		goto out;
	}

	// Add the "File" and "OSAttr" attributes to the newly created member
	if ( (pfCryptCATPutAttrInfo(hCat, pCatMember, L"File",
	                            CRYPTCAT_ATTR_AUTHENTICATED | CRYPTCAT_ATTR_NAMEASCII | CRYPTCAT_ATTR_DATAASCII,
	                            2 * ((DWORD)wcslen(szFileName) + 1), (BYTE*)szFileName) == NULL)
	        || (pfCryptCATPutAttrInfo(hCat, pCatMember, L"OSAttr",
	                                  CRYPTCAT_ATTR_AUTHENTICATED | CRYPTCAT_ATTR_NAMEASCII | CRYPTCAT_ATTR_DATAASCII,
	                                  2 * ((DWORD)wcslen(wszOSAttr) + 1), (BYTE*)wszOSAttr) == NULL) )
	{
		USBWRNN(L"unable to create attributes for file '%s': %s", szFileName, windows_error_str(0));
		goto out;
	}
	r = TRUE;

out:
	return r;
}

BOOL CreateCatEx(PKINF_EL infEL)
{
	PF_DECL(CryptCATOpen);
	PF_DECL(CryptCATClose);
	PF_DECL(CryptCATPersistStore);
	PF_DECL(CryptCATStoreFromHandle);
	PF_DECL(CryptCATPutCatAttrInfo);

	HCRYPTPROV hProv = 0;
	HANDLE hCat = NULL;
	BOOL r = FALSE;
	DWORD i;
	LPCWSTR wszOS = L"XPX86,XPX64,VistaX86,VistaX64,7X86,7X64";
	PKINF_DEVICE_EL deviceEL;
	PKINF_FILE_EL sourceFileEL;
	BYTE pbHash[SHA1_HASH_LENGTH];

	PF_INIT_OR_OUT(CryptCATOpen, wintrust);
	PF_INIT_OR_OUT(CryptCATClose, wintrust);
	PF_INIT_OR_OUT(CryptCATPersistStore, wintrust);
	PF_INIT_OR_OUT(CryptCATStoreFromHandle, wintrust);
	PF_INIT_OR_OUT(CryptCATPutCatAttrInfo, wintrust);

	if (!CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
	{
		USBWRNN(L"unable to acquire crypt context for cat creation");
		goto out;
	}

	hCat = pfCryptCATOpen(infEL->CatFullPath, CRYPTCAT_OPEN_CREATENEW, hProv, 0, 0);
	if (hCat == INVALID_HANDLE_VALUE)
	{
		USBWRNN(L"unable to create file '%s': %s", infEL->CatFullPath, windows_error_str(0));
		goto out;
	}

	// Setup the general Cat attributes
	i = 0;
	DL_FOREACH(infEL->Devices, deviceEL)
	{
		_wcslwr(deviceEL->HardwareID);	// Most of the cat strings are converted to lowercase
		wsprintfW(deviceEL->CatKey, L"HWID%d", ++i);

		if (pfCryptCATPutCatAttrInfo(hCat, deviceEL->CatKey, CRYPTCAT_ATTR_AUTHENTICATED | CRYPTCAT_ATTR_NAMEASCII | CRYPTCAT_ATTR_DATAASCII,
		                             2 * ((DWORD)wcslen(deviceEL->HardwareID) + 1), (BYTE*)deviceEL->HardwareID) ==  NULL)
		{
			USBWRNN(L"failed to set HWID%d cat attribute: %s", i, windows_error_str(0));
			goto out;
		}
	}

	if (pfCryptCATPutCatAttrInfo(hCat, L"OS", CRYPTCAT_ATTR_AUTHENTICATED | CRYPTCAT_ATTR_NAMEASCII | CRYPTCAT_ATTR_DATAASCII,
	                             2 * ((DWORD)wcslen(wszOS) + 1), (BYTE*)wszOS) == NULL)
	{
		USBWRNN(L"failed to set OS cat attribute: %s", windows_error_str(0));
		goto out;
	}

	DL_FOREACH(infEL->Files, sourceFileEL)
	{
		static WCHAR szFilePath[MAX_PATH];
		static WCHAR szEntry[MAX_PATH];
		static WCHAR szRelPath[MAX_PATH];

		wcscpy_s(szFilePath, _countof(szFilePath), infEL->BaseDir);
		wcscpy_s(szEntry, _countof(szEntry), sourceFileEL->Filename);
		PathAppendW(szFilePath, sourceFileEL->SubDir);
		PathAppendW(szFilePath, szEntry);

		//_wcslwr(szFilePath);
		//_wcslwr(szEntry);

		if (GetFileAttributesW(szFilePath) == INVALID_FILE_ATTRIBUTES)
		{
			USBWRNN(L"SourceDiskFile %s does not exists.", szFilePath);
		}
		else
		{
			if (CalcHash(pbHash, szFilePath))
			{
				szRelPath[0] = '\0';
				PathRelativePathToW(szRelPath, infEL->BaseDir, FILE_ATTRIBUTE_DIRECTORY, szFilePath, FILE_ATTRIBUTE_NORMAL);
				USBMSGN(L"Hash calculated for: %s",  szRelPath);
				if (AddFileHash(hCat, szEntry, pbHash))
				{
					USBMSGN(L"Hash added..");
				}
				else
				{
					USBWRNN(L"Failed adding hash for '%s' - ignored", szEntry);
				}
			}
			else
			{
				USBWRNN(L"Failed calculating hash for '%s' - ignored", szFilePath);
			}
		}
	}

	// The cat needs to be sorted before being saved
	if (!pfCryptCATPersistStore(hCat))
	{
		USBWRNN(L"unable to sort file: %s",  windows_error_str(0));
		goto out;
	}
	USBMSGN(L"Catalog file '%s' created..", infEL->CatTitle);
	r = TRUE;

out:
	if (hProv) (CryptReleaseContext(hProv, 0));
	if ((hCat)) pfCryptCATClose(hCat);
	return r;
}
