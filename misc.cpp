#include "stdafx.h"

#ifdef __cplusplus
extern "C" {
#endif

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

	
	f_handle = CreateFile(	psDriverFile,
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

	image = MapViewOfFile(	m_handle,
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
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		
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
	This function determines the current OS version.
*/
BOOL WINAPI GetWindowVersion() {



	return TRUE;
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
	This funtion checks the administrator priviledge of the cureent user.
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

	bSuccess = GetTokenInformation(	hAccessToken,
									TokenGroups,
									InfoBuffer,
									1024,
									&dwInfoBufferSize);

	CloseHandle(hAccessToken);

	if (!bSuccess)
		return FALSE;

	if (!AllocateAndInitializeSid(	&siaNtAuthority,
									2,
									SECURITY_BUILTIN_DOMAIN_RID,
									DOMAIN_ALIAS_RID_ADMINS,
									0, 0, 0, 0, 0, 0,
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


	if ((!DriverPath) || (!FileEnding))
		return status;

	__try {

		strcpy_s(FilePath, MAX_PATH, DriverPath);

		token = strtok_s(FilePath, ".", &next);
		if (!token)
			return status;

		do {
			if (! _tcsnccmp((PTCHAR) _totlower((UINT)token), FileEnding, _tcslen(FileEnding))) {
				status = TRUE;
				break;
			}
			
			token = strtok_s(NULL, ".", &next);
			if (!token)
				return status;
		} while (token != NULL);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		
	}
	
	return status;
}

#ifdef __cplusplus
}
#endif
