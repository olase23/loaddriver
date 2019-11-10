#include "stdafx.h"

#ifdef __cplusplus
extern "C" {
#endif

	/*
	This function gets the version information resources from the current driver image.
	*/
	BOOL WINAPI GetImageVersionInfo(LP_DRIVER_FILE devdrv) {
		DWORD				length;
		LPVOID				VersionBlock;
		VS_FIXEDFILEINFO 	*pFixedVersionInfo;
		UINT				DataLength;
		BOOL				status;
		PWORD				Translation;
		DWORD				Ignored;

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
			}
			else {
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
#ifdef __cplusplus
}
#endif