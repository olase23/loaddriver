#include "stdafx.h"

#ifdef __cplusplus
extern "C" {
#endif

#define REG_STR_SIZE  64

extern SYSTEM_INFORMATION current_system;
typedef BOOL(WINAPI *LD_ISWOW64PROCESS) (HANDLE, PBOOL);
typedef DWORD(WINAPI *LD_FORMATMESSAGE) (DWORD, LPCVOID, DWORD, DWORD, LPTSTR, DWORD, va_list);

/*
  This function determines the current installed OS env.
*/
VOID WINAPI GetCurrentHostSetup(VOID) {
  LD_ISWOW64PROCESS fnIsWow64Process;
  TCHAR RegistryString[REG_STR_SIZE];
  SYSTEM_INFO SystemInfo;
  DWORD ValueFromReg;
  DWORD RegDataType;
  DWORD RegDataSize;
  BOOL isWow64 = FALSE;
  LONG result = 0;
  HKEY hKey;

  ZeroMemory(&SystemInfo, sizeof(SYSTEM_INFO));
  
  GetSystemInfo(&SystemInfo);
  fnIsWow64Process = (LD_ISWOW64PROCESS)GetProcAddress(
    GetModuleHandle(TEXT("kernel32")),
   "IsWow64Process");

  if (fnIsWow64Process) {
    if (fnIsWow64Process(GetCurrentProcess(), &isWow64)) {
     if (isWow64)
       current_system.ArchType = ARCH_X64;
    else
      current_system.ArchType = ARCH_I386;
    }
  }

  // we still not know, so lets try to get it from the registry
  if (current_system.ArchType == 0) {	
    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
     HKLM_CURRENT_VERSION,
     0,
     KEY_READ,
     &hKey);

    if (result == ERROR_SUCCESS) {
      RegDataSize = sizeof(TCHAR) * REG_STR_SIZE;
    
      result = RegQueryValueEx(hKey,
        PROC_ARCH,
        NULL,
        &RegDataType,
        (PBYTE)&RegistryString,
        &RegDataSize);
      if (result == ERROR_SUCCESS) {
        if(!strcmp(RegistryString,"x86"))
          current_system.ArchType = ARCH_I386;
        else
          current_system.ArchType = ARCH_X64;
      } else {
          current_system.ArchType = ARCH_I386;
      }
      RegCloseKey(hKey);
    }
  }
  // Try tor figure out what the current driver signing policy is.
  // If nothing is available in the registry assume none. 	
  result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
    NON_DRIVER_SIGN_PATH,
    0,
    KEY_READ,
    &hKey);

  if (result == ERROR_SUCCESS) {
    RegDataSize = sizeof(DWORD);
    result = RegQueryValueEx(hKey,
      DRIVER_SIGN_POLICY,
      NULL,
      &RegDataType,
      (PBYTE)&ValueFromReg,
      &RegDataSize);
    if (result == ERROR_SUCCESS)
      current_system.NonDriverSigning = (DWORD)*((PBYTE)&ValueFromReg);
    else
      current_system.NonDriverSigning = DRIVER_SIGNING_NONE;
  	RegCloseKey(hKey);
  }
}

/*
  This funtion trys to get a API function pointer.
*/
BOOL WINAPI GetFuntionPointer(LPCSTR DllName, 
  LPCSTR FuntionName,
  PAPI_HELPER ApiHelper) {

  if (!DllName || !FuntionName)
    return FALSE;

  ApiHelper->hDll = LoadLibrary(DllName);
  if (ApiHelper->hDll == NULL)
    return FALSE;

  ApiHelper->Function = GetProcAddress(ApiHelper->hDll, FuntionName);

  if (!ApiHelper->Function)
    return FALSE;
  return TRUE;
}

/*
  This funtion checks if the current user has administrator priviledges.
*/
BOOL IsAdmin(void) {
  HANDLE hAccessToken;
  UCHAR InfoBuffer[1024];
  PTOKEN_GROUPS ptgGroups = (PTOKEN_GROUPS)InfoBuffer;
  DWORD dwInfoBufferSize;
  PSID psidAdministrators;
  SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
  UINT x;
  BOOL bSuccess;

  if (!OpenThreadToken(GetCurrentThread(),
    TOKEN_QUERY,
    TRUE,
    &hAccessToken)) {
      if (GetLastError() != ERROR_NO_TOKEN)
        return FALSE;

      if (!OpenProcessToken(GetCurrentProcess(),
        TOKEN_QUERY,
        &hAccessToken))
        return FALSE;
    }

  bSuccess = GetTokenInformation(hAccessToken,
    TokenGroups,
    InfoBuffer,
    1024,
    &dwInfoBufferSize);
  CloseHandle(hAccessToken);

  if (!bSuccess)
    return FALSE;

  if (!AllocateAndInitializeSid(&siaNtAuthority,
    2,
    SECURITY_BUILTIN_DOMAIN_RID,
    DOMAIN_ALIAS_RID_ADMINS,
    0,
    0,
    0,
    0,
    0,
    0,
    &psidAdministrators))
    return FALSE;

  bSuccess = FALSE;

  for (x = 0; x < ptgGroups->GroupCount; x++) {
    if (EqualSid(psidAdministrators, ptgGroups->Groups[x].Sid)) {
      bSuccess = TRUE;
      break;
    }
  }

  FreeSid(psidAdministrators);
  return bSuccess;
}

/*
  This file trys to figure out if we have a sys or inf file.
*/
BOOL WINAPI CheckFileEnding(LPCSTR DriverPath, PTCHAR FileEnding) {
  BOOL	status = FALSE;
  PCHAR	token = NULL;
  PCHAR   next = NULL;
  TCHAR	FilePath[MAX_PATH];
  UINT	i;

  if ((!DriverPath) || (!FileEnding))
    return status;

  __try {
    strcpy_s(FilePath, MAX_PATH, DriverPath);
    token = strtok_s(FilePath, ".", &next);
    if (!token)
      return status;

    do {
      for (i = 0; i < 3; i++) {
        if (isupper(token[i]))
          token[i] = (TCHAR)_totlower((UINT)token[i]);
      }
      if (!_tcsnccmp(token, FileEnding, _tcslen(FileEnding))) {
        status = TRUE;
        break;
      }
      token = strtok_s(NULL, ".", &next);
      if (!token)
        return status;
  	} while (token != NULL);
  }
  __except (EXCEPTION_EXECUTE_HANDLER) {}
  return status;
}

/*
  This function flushes all file buffers to the hard drive(s).
*/
VOID WINAPI SyncVolumes(VOID) {
  HANDLE	hVolume;
  TCHAR	szVolume[16];
  UINT	i;

  for (i = 2; i < 26; i++) {
    wsprintf(szVolume, TEXT("\\\\.\\%c:\0"), (TCHAR)((TCHAR)i + (TCHAR)'a'));
    hVolume = CreateFile(
      szVolume,
      GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE,
      NULL,
      OPEN_EXISTING,
      0,
      NULL);
    if (hVolume == INVALID_HANDLE_VALUE) 
      continue;

    FlushFileBuffers(hVolume);
    CloseHandle(hVolume);
  }
}

VOID WINAPI ShowErrorMessage(DWORD dwError, LPSTR msg, LPSTR title) {
  LD_FORMATMESSAGE fnForrmatMessage;
  static char buff[BUFFER_SIZE];
  TCHAR errormsg[MSG_SIZE];

  ZeroMemory(buff, BUFFER_SIZE);
  fnForrmatMessage = (LD_FORMATMESSAGE) GetProcAddress(
    GetModuleHandle(TEXT("kernel32")),
    "FormatMessageA");
  if (fnForrmatMessage) {
    fnForrmatMessage(FORMAT_MESSAGE_IGNORE_INSERTS |
      FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_MAX_WIDTH_MASK,
      NULL,
      dwError,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR)buff,
      BUFFER_SIZE - 1,
      NULL);
  }
  if (strlen(buff))
    wsprintf(errormsg,
      TEXT("%s\n\n%s"),
      msg,
      buff);
  else
    wsprintf(errormsg,
      TEXT("%s\n"),
      msg);

  MessageBox(0,
    errormsg,
    title,
    MB_ICONERROR);
}
#ifdef __cplusplus
}
#endif