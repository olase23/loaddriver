#include "stdafx.h"
#include <Softpub.h>
#include <wincrypt.h>
#include <wintrust.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
  This function gets the version information resources from the current driver image.
*/
BOOL WINAPI GetImageVersionInfo(LP_DRIVER_FILE devdrv) {
  DWORD    length;
  LPVOID    VersionBlock;
  VS_FIXEDFILEINFO 	*pFixedVersionInfo;
  UINT    DataLength;
  BOOL    status;
  DWORD    Ignored;

  status = FALSE;
  pFixedVersionInfo = NULL;

  length = GetFileVersionInfoSize((PTSTR)devdrv->psDriverFile, &Ignored);

  if (!length) {
    devdrv->last_error = GetLastError();
    return status;
  }

  VersionBlock = LocalAlloc(LPTR, length * sizeof(TCHAR));

  if (!VersionBlock) {
    devdrv->last_error = GetLastError();
    return status;
  }

  if (!GetFileVersionInfo((PTSTR)devdrv->psDriverFile,
    0,
    length * sizeof(TCHAR),
    VersionBlock)) {
    devdrv->last_error = GetLastError();
    goto err_out;
  }

  if (!VerQueryValue(VersionBlock,
    TEXT("\\"),
    (LPVOID *)&pFixedVersionInfo,
    &DataLength)) {
    devdrv->last_error = GetLastError();
    goto err_out;
  }

  // save the version information
  if (DataLength) {
    devdrv->version = pFixedVersionInfo->dwFileVersionMS;
    devdrv->flags = pFixedVersionInfo->dwFileFlags;
    devdrv->ftype = pFixedVersionInfo->dwFileType;
    devdrv->fos = pFixedVersionInfo->dwFileOS;
    devdrv->fsubtype = pFixedVersionInfo->dwFileSubtype;
    status = TRUE;
  }

	err_out:
  LocalFree(VersionBlock);
  return status;
	}

/*
  This funtion determines the architecture type of the driver.
*/
BOOL WINAPI GetDriverArchitecture(LPCSTR psDriverFile, PDWORD lpBinaryType) {
  BOOL   status;
  HANDLE f_handle;
  HANDLE m_handle;
  DWORD  error;
  LPVOID image;
  PIMAGE_NT_HEADERS nt_header;


  f_handle = CreateFile(psDriverFile,
    GENERIC_READ,
    0, NULL,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL,
    NULL);

  if (f_handle == INVALID_HANDLE_VALUE)
    return FALSE;

  status = TRUE;

  m_handle = CreateFileMapping(f_handle,
    NULL,
    PAGE_READONLY,
    0,
    0,
    NULL);

  if (m_handle == NULL) {
    error = GetLastError();
    status = FALSE;
    goto err_out1;
  }

   image = MapViewOfFile(m_handle,
    FILE_MAP_READ,
    0,
    0,
    0);

  if (!image) {
    error = GetLastError();
    status = FALSE;
    goto err_out2;
  }

  // try to find the target machine type
  __try {

    // check if we have a DOS header
    if (!((PIMAGE_DOS_HEADER)image)->e_magic == IMAGE_DOS_SIGNATURE) {
        error = ERROR_INVALID_DATA;
        status = FALSE;
        goto err_out3;
    }

    // get the nt header section
    nt_header = (PIMAGE_NT_HEADERS)((PBYTE)image + ((PIMAGE_DOS_HEADER)image)->e_lfanew);
    
	// check PE signature to ensure we have the proper file format
    if (nt_header->Signature == IMAGE_NT_SIGNATURE) {
	  if (nt_header->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
        *lpBinaryType = ARCH_X64;
      else if (nt_header->FileHeader.Machine == IMAGE_FILE_MACHINE_I386)
        *lpBinaryType = ARCH_I386;
      else
        *lpBinaryType = 0;
    } else {
      error = ERROR_INVALID_DATA;
      status = FALSE;
    }
  }
  __except (EXCEPTION_EXECUTE_HANDLER) {
    error = ERROR_INVALID_DATA;
    status = FALSE; 
  }

  err_out3:
  UnmapViewOfFile(image);

  err_out2:
  CloseHandle(m_handle);

  err_out1:
  CloseHandle(f_handle);

  if (status == FALSE)
  	SetLastError(error);

  return status;
}

/*
  Try to determine whether the driver has a valid signature. 	
 */

BOOL WINAPI	VerifyDriverSignatur(LP_DRIVER_FILE driver_file) {
  WINTRUST_FILE_INFO WintrustFileInfo;
  WINTRUST_DATA WintrustData;
  GUID DriverVerifyGuid = DRIVER_ACTION_VERIFY;
  WCHAR UnicodeKey[MAX_PATH];
  DWORD dwError;
  LONG lStatus;

  ZeroMemory(&WintrustFileInfo, sizeof(WINTRUST_FILE_INFO));

  WintrustFileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
  WintrustData.dwUnionChoice = WTD_CHOICE_FILE;
  WintrustData.pFile = &WintrustFileInfo;

#ifdef UNICODE
  WintrustFileInfo.pcwszFilePath = (LPCWSTR)driver_file->psDriverFile;
#else
  MultiByteToWideChar(CP_ACP,
    0,
    driver_file->psDriverFile,
    -1,
    UnicodeKey,
    sizeof(WCHAR) * MAX_PATH);
  WintrustFileInfo.pcwszFilePath = UnicodeKey;
#endif

  WintrustFileInfo.hFile = NULL;
  WintrustFileInfo.pgKnownSubject = NULL;

  lStatus = WinVerifyTrust(
  	NULL,
  	&DriverVerifyGuid,
  	&WintrustData);

  switch (lStatus) {
  case ERROR_SUCCESS:
  	driver_file->flags |= DRV_VALID_SIGNATURE;
  	return TRUE;
  case TRUST_E_NOSIGNATURE:
  	dwError = GetLastError();
    if (dwError == TRUST_E_SUBJECT_FORM_UNKNOWN || dwError == TRUST_E_PROVIDER_UNKNOWN)
      driver_file->flags |= DRV_SIGNATURE_UNKNOWN;
    else
      driver_file->flags |= DRV_NO_SIGNATURE;
    break;
  case CERT_E_UNTRUSTED_ROOT:
  	driver_file->flags |= DRV_SIGNATURE_UNKNOWN;
  	break;
  case TRUST_E_SUBJECT_NOT_TRUSTED:
  	driver_file->flags |= DRV_SIGNATURE_NOT_TRUSTED;
  	break;
  default:
  	driver_file->flags |= DRV_SIGNATURE_ERROR;
  }
  return FALSE;
}

#ifdef __cplusplus
}
#endif