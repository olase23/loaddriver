#include "stdafx.h"

#ifdef __cplusplus
extern "C" {
#endif

	typedef BOOL(WINAPI *LD_DIINSTALLDRIVER)(HWND, LPCSTR, DWORD, PBOOL);
	typedef BOOL(WINAPI *LD_DIUNINSTALLDRIVER)(HWND, LPCSTR, DWORD, PBOOL);

	extern BOOL WINAPI	GetFuntionPointer(LPCSTR, LPCSTR, PAPI_HELPER);
	extern INF_FILE inf_file;


	/*
	This funtion trys to install the install a driver with the SetupApi.
	*/
	BOOL WINAPI InstallDriverViaSetupApi(VOID) {
		API_HELPER	SetupHelper;
		LD_DIINSTALLDRIVER FPDiInstallDriver = NULL;
		DWORD		error;
		BOOL		NeedReboot;

		if (inf_file.state == INSTALLED)
			return FALSE;

		if (!GetFuntionPointer("newdev.dll",
			DIINSTALLDRIVER,
			&SetupHelper))
		{
			error = GetLastError();
			return FALSE;
		}

		FPDiInstallDriver = (LD_DIINSTALLDRIVER)SetupHelper.Function;

		if (!FPDiInstallDriver(NULL,
			inf_file.psInfFile,
			0,
			&NeedReboot)) {
			error = GetLastError();

			FreeLibrary(SetupHelper.hDll);
			SetLastError(error);
			return FALSE;
		}


		FreeLibrary(SetupHelper.hDll);

		inf_file.state = INSTALLED;

		return TRUE;
	}

	BOOL WINAPI UnInstallDriverViaSetupApi(VOID) {
		API_HELPER	SetupHelper;
		LD_DIUNINSTALLDRIVER FPDiUnInstallDriver = NULL;
		DWORD		error;
		BOOL		NeedReboot;

		if (inf_file.state != INSTALLED)
			return FALSE;

		if (!GetFuntionPointer("newdev.dll",
			TEXT("DiUninstallDriverW"),
			&SetupHelper))
		{
			error = GetLastError();
			return FALSE;
		}

		FPDiUnInstallDriver = (LD_DIUNINSTALLDRIVER)SetupHelper.Function;

		if (!FPDiUnInstallDriver(NULL,
			inf_file.psInfFile,
			0,
			&NeedReboot)) {
			error = GetLastError();

			FreeLibrary(SetupHelper.hDll);
			SetLastError(error);
			return FALSE;
		}


		FreeLibrary(SetupHelper.hDll);

		inf_file.state = UNINSTALLED;

		return TRUE;
	}

#ifdef __cplusplus
}
#endif