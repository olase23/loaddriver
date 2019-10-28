#include "stdafx.h"
#include "loaddriver.h"

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

	
	f_handle = CreateFile(psDriverFile,
				GENERIC_READ,
				0, NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL
	);

	if (f_handle == INVALID_HANDLE_VALUE)
		return FALSE;

	status = TRUE;

	m_handle = CreateFileMapping(f_handle,
				NULL,
				PAGE_READONLY,
				0,
				0,
				NULL
	);

	if (m_handle == NULL) {
		error = GetLastError();
		status = FALSE;
		goto err_out1;
	}

	image = MapViewOfFile(m_handle,
				FILE_MAP_READ,
				0,
				0,
				0
	);

	if (!image) {
		error = GetLastError();
		status = FALSE;
		goto err_out2;
	}

	// try to find the target machine type
	__try {
		nt_header = (PIMAGE_NT_HEADERS)((PBYTE)image + ((PIMAGE_DOS_HEADER)image)->e_lfanew);
		if (nt_header->Signature == IMAGE_NT_SIGNATURE) {
			if (nt_header->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
				*lpBinaryType = ARCH_X64;
			else if (nt_header->FileHeader.Machine == IMAGE_FILE_MACHINE_I386)
				*lpBinaryType = ARCH_I386;
			else
				*lpBinaryType = 0;
		}
		else {
			error = ERROR_INVALID_DATA;
			status = FALSE;
		}
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		error = ERROR_INVALID_DATA;
		status = FALSE;
	}
	
	UnmapViewOfFile(image);

err_out2:
	CloseHandle(m_handle);

err_out1:
	CloseHandle(f_handle);

	if (status == FALSE)
		SetLastError(error);

	return status;
}


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

#ifdef __cplusplus
}
#endif
