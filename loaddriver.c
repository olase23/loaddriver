#include "stdafx.h"
#include <windows.h>
#include <commctrl.h>
#include "resource.h"
#include "loaddriver.h"

#define SECOND_INSTANCE	(WM_APP+100)

BOOL CALLBACK   DlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL WINAPI		SetRegKeys(VOID);
VOID WINAPI		CleanUp(VOID);
BOOL WINAPI		StopDriver(VOID);
BOOL WINAPI		StartDriver(SC_HANDLE);
VOID WINAPI		InitialDriver(VOID);
BOOL WINAPI		CopyDriver(VOID);
BOOL WINAPI		CheckAccess(VOID);
BOOL WINAPI		CreateDriver(SC_HANDLE);
BOOL WINAPI		LaunchDriver(VOID);
BOOL WINAPI		RemoveDriver(VOID);
SC_HANDLE WINAPI OpenServiceDatabase(VOID);
BOOL IsAdmin(void);

DRIVER_FILE	driver_file;
HWND	_hdlg;
TCHAR	psWinSysDir[MAX_PATH];
TCHAR	psStartDir[4];
TCHAR	psSysName[256];

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hprevInstance, LPSTR pCmdLine, int nCmdShow)
{
	HWND parent = FindWindow("#32770", TEXT("Load Driver"));
	if (IsWindow(parent)) {
		SendMessage(parent, SECOND_INSTANCE, 0, 0);
		return(FALSE);
	}

	if (!IsAdmin()) {
		MessageBox(NULL,
			TEXT("User must be Admin!"),
			TEXT("ERROR"),
			MB_ICONSTOP | MB_APPLMODAL);

		exit(1);
	}

	memset(&driver_file, 0, DRIVER_FILE_SIZE);
	return(DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN_DIALOG), NULL, DlgProc));
}

BOOL CALLBACK DlgProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	OPENFILENAME		OpenFileName;

	switch (message) {
	case SECOND_INSTANCE: {
		MessageBox(0, TEXT("There is an instance already running!"), TEXT("Launch Driver Error"), MB_ICONERROR);
		break;
	}

	case WM_INITDIALOG: {
		UINT	x, y, i;

		_hdlg = hdlg;
		x = GetSystemMetrics(SM_CXSCREEN);
		y = GetSystemMetrics(SM_CYSCREEN);

		x = x / 4;
		y = y / 4;
		SetWindowPos(hdlg, HWND_TOP, x, y, 0, 0, SWP_NOSIZE);
		InitCommonControls();

		for (i = 0; DRIVER_TYPE[i] != NULL; i++)
			SendMessage(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(IDC_DRIVER_TYPE)), CB_ADDSTRING, 0, (LPARAM)DRIVER_TYPE[i]);
			SendMessage(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(IDC_DRIVER_TYPE)), CB_SELECTSTRING, 0, (LPARAM)DRIVER_TYPE[0]);

		for (i = 0; DRIVER_START[i] != NULL; i++)
			SendMessage(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(IDC_DRIVER_START)), CB_ADDSTRING, 0, (LPARAM)DRIVER_START[i]);
			SendMessage(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(IDC_DRIVER_START)), CB_SELECTSTRING, 0, (LPARAM)DRIVER_START[3]);

		for (i = 0; ERROR_CONTROL[i] != NULL; i++)
			SendMessage(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(IDC_ERROR_CONTROL)), CB_ADDSTRING, 0, (LPARAM)ERROR_CONTROL[i]);
			SendMessage(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(IDC_ERROR_CONTROL)), CB_SELECTSTRING, 0, (LPARAM)ERROR_CONTROL[1]);

		if (!GetSystemDirectory(psWinSysDir, MAX_PATH)) {
			MessageBox(0, TEXT("Couldn't determine the system directory"), TEXT("Launch Driver Error"), MB_ICONERROR);
			EndDialog(hdlg, 0);
		}

		EnableWindow(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(ID_LAUNCH)), FALSE);
		EnableWindow(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(ID_INSTALL)), FALSE);
		break;
	}

	case WM_COMMAND: {
		switch (wParam) {
		case IDCANCEL:	CleanUp();
			EndDialog(hdlg, 0);
			break;
		case IDC_LOAD_FILE: {
			LONG	lErrMsg = 0;
			PCHAR	tok = NULL, last = NULL;
			TCHAR	psDriverFile[MAX_PATH];

			ZeroMemory(&OpenFileName, sizeof(OPENFILENAME));
			psDriverFile[0] = '\0';
			wsprintf(psStartDir, TEXT("%c:\\"), psWinSysDir[0]);

			OpenFileName.lStructSize = sizeof(OPENFILENAME);
			OpenFileName.hwndOwner = NULL;
			OpenFileName.lpstrFile = psDriverFile;
			OpenFileName.lpstrFilter = TEXT("Device Driver (*.sys)\0*.SYS\0\0");
			OpenFileName.lpstrInitialDir = psStartDir;
			OpenFileName.lpstrFileTitle = TEXT("Please select a File");
			OpenFileName.nMaxFile = MAX_PATH;
			OpenFileName.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

			if (!GetOpenFileName(&OpenFileName)) {

				TCHAR		sErrMsg[256];
				lErrMsg = CommDlgExtendedError();
				if (lErrMsg) {
					wsprintf(sErrMsg, TEXT("Error %d returned from GetOpenFileName()"), lErrMsg);
					MessageBox(NULL, sErrMsg, TEXT("OpenDialog"), MB_ICONERROR);
				}
				break;
			}

			SetWindowText(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(IDC_DRIVER)), psDriverFile);

			ZeroMemory(&driver_file, sizeof(driver_file));
			strcpy(driver_file.psDriverFile, psDriverFile);

			tok = strtok(psDriverFile, "\\");
			do {
				last = tok;
				tok = strtok(NULL, "\\");
			} while (tok);

			if (last) {
				strcpy(psSysName, last);
				tok = strchr(psSysName, (INT)'.');
				tok[0] = '\0';
			}
			else {
				MessageBox(0, TEXT("Couldn't determine the file name!!!"), TEXT("Launch Driver Error"), MB_ICONERROR);
				break;
			}

			strcpy(driver_file.psSysName, psSysName);
			driver_file.state = INITIALIZED;
			EnableWindow(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(ID_INSTALL)), TRUE);
			break;
		}
		case ID_LAUNCH: {
			INT		check_access = 0;

			switch (driver_file.state) {
			case STOPED:
			case INSTALLED:
				if (!LaunchDriver())
					break;
				SetWindowText(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(ID_LAUNCH)), TEXT("Stop"));
				EnableWindow(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(ID_INSTALL)), FALSE);
				driver_file.state = STARTED;

				check_access = (INT)SendMessage(GetDlgItem(_hdlg, (int)MAKEINTRESOURCE(IDC_CHECK_ACCESS)), BM_GETCHECK, 0, 0);
				if (check_access) {
					if (!CheckAccess())
						MessageBox(0, TEXT("Couldn't access the driver!!!"), TEXT("Launch Driver Error"), MB_ICONERROR);
				}

				break;
			case STARTED:
				StopDriver();
				SetWindowText(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(ID_LAUNCH)), TEXT("Launch"));
				EnableWindow(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(ID_INSTALL)), TRUE);
				driver_file.state = STOPED;
				break;
			default:		break;
			}
			break;
		}
		case ID_INSTALL:
			switch (driver_file.state) {
			case STARTED:	  if (!StopDriver()) {
				MessageBox(0, TEXT("Couldn't stop the driver!!!"),
					TEXT("Launch Driver Error"),
					MB_ICONERROR);
				break;
			}
							  SetWindowText(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(ID_LAUNCH)), TEXT("Launch"));
			case STOPED:
			case INSTALLED:
			case INITIALIZED:
				InitialDriver();
				if (!CopyDriver())
					break;
				SetRegKeys();
				driver_file.state = INSTALLED;
				EnableWindow(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(ID_LAUNCH)), TRUE);
				break;
			default:	break;
			}

			break;

		case IDC_DEPENCES:
		{
			if (IsWindowEnabled(
				GetDlgItem(hdlg,
				(int)MAKEINTRESOURCE(IDC_ON_SERVICE))))

				EnableWindow(GetDlgItem(hdlg,
				(int)MAKEINTRESOURCE(IDC_ON_SERVICE)),
					FALSE);
			else
				EnableWindow(GetDlgItem(hdlg,
				(int)MAKEINTRESOURCE(IDC_ON_SERVICE)),
					TRUE);

			if (IsWindowEnabled(GetDlgItem(hdlg,
				(int)MAKEINTRESOURCE(IDC_ON_GROUP))))
				EnableWindow(GetDlgItem(hdlg,
				(int)MAKEINTRESOURCE(IDC_ON_GROUP)),
					FALSE);
			else
				EnableWindow(GetDlgItem(hdlg,
				(int)MAKEINTRESOURCE(IDC_ON_GROUP)),
					TRUE);
			break;
		}
		}
		break;
	}
	default: return(FALSE);
	}
	return(TRUE);
}

VOID WINAPI CleanUp(VOID) {
	INT	    iKeepKey = 0;
	LONG	result = 0;
	DWORD	errorcode = 0;
	TCHAR	psSubKey[MAX_PATH];

	if (!driver_file.psDriverFile[0])
		return;

	iKeepKey = (INT)SendMessage(GetDlgItem(_hdlg,
		(int)MAKEINTRESOURCE(IDC_KEEP_REG_ENTRYS)),
		BM_GETCHECK,
		0,
		0);
	if (!iKeepKey) {
		/**delete all registry keys**/

		//wsprintf(psSubKey,TEXT("%s%s"),DRIVER_REGISTRY_PATH,psSysName);

		RemoveDriver();
	}

}

VOID WINAPI InitialDriver(VOID) {
	LRESULT index = 0;

	index = SendMessage(GetDlgItem(_hdlg,
		(int)MAKEINTRESOURCE(IDC_DRIVER_TYPE)),
		CB_GETCURSEL,
		0,
		0);
	if (index == CB_ERR) {
		MessageBox(0,
			TEXT("Couldn't get the driver type!!!"),
			TEXT("Launch Driver Error"),
			MB_ICONERROR);
		return;
	}

	switch (index) {
	case 0:	
		driver_file.type = 1;
		break;
	case 1: 
		driver_file.type = 2;
		break;
	case 2: 
		driver_file.type = 4;
		break;
	}

	index = SendMessage(GetDlgItem(_hdlg,
		(int)MAKEINTRESOURCE(IDC_DRIVER_START)),
		CB_GETCURSEL,
		0,
		0);
	if (index == CB_ERR) {
		MessageBox(0,
			TEXT("Couldn't get the start type!!!"),
			TEXT("Launch Driver Error"),
			MB_ICONERROR);
		return;
	}
	driver_file.start = index;

	index = SendMessage(GetDlgItem(_hdlg,
		(int)MAKEINTRESOURCE(IDC_ERROR_CONTROL)),
		CB_GETCURSEL,
		0,
		0);

	if (index == CB_ERR) {
		MessageBox(0,
			TEXT("Couldn't get the error control!!!"),
			TEXT("Launch Driver Error"),
			MB_ICONERROR);
		return;
	}
	driver_file.errorcontrol = index;

	GetWindowText(GetDlgItem(_hdlg,
		(int)MAKEINTRESOURCE(IDC_DESCRIPTION)),
		driver_file.description,
		255);

	GetWindowText(GetDlgItem(_hdlg,
		(int)MAKEINTRESOURCE(IDC_DISPLAY_NAME)),
		driver_file.display_name,
		255);

	if (SendMessage(GetDlgItem(_hdlg,
		(int)MAKEINTRESOURCE(IDC_DEPENCES)),
		BM_GETCHECK,
		0,
		0)) {
		GetWindowText(GetDlgItem(_hdlg,
			(int)MAKEINTRESOURCE(IDC_ON_SERVICE)),
			driver_file.depend_service,
			255);

		GetWindowText(GetDlgItem(_hdlg,
			(int)MAKEINTRESOURCE(IDC_ON_GROUP)),
			driver_file.depend_group,
			255);
	}
}

BOOL WINAPI SetRegKeys(VOID) {
	HKEY	hkey;
	LONG	result = 0;
	DWORD	errorcode = 0;
	TCHAR	errormsg[256];
	TCHAR	psSubKey[MAX_PATH];

	wsprintf(psSubKey, TEXT("%s%s"), DRIVER_REGISTRY_PATH, psSysName);

	result = RegCreateKey(HKEY_LOCAL_MACHINE, psSubKey, &hkey);
	if (result != NO_ERROR)
		goto error;

	result = RegSetValueEx(hkey,
		TEXT("Type"),
		0,
		REG_DWORD,
		(PBYTE)&driver_file.type,
		sizeof(driver_file.type));
	if (result != NO_ERROR)
		goto error;

	result = RegSetValueEx(hkey,
		TEXT("Start"),
		0,
		REG_DWORD,
		(PBYTE)&driver_file.start,
		sizeof(driver_file.start));
	if (result != NO_ERROR)
		goto error;

	result = RegSetValueEx(hkey,
		TEXT("ErrorControl"),
		0,
		REG_DWORD,
		(PBYTE)&driver_file.errorcontrol,
		sizeof(driver_file.errorcontrol));
	if (result != NO_ERROR)
		goto error;

	if (driver_file.description[0] != '\0')
	{
		result = RegSetValueEx(hkey,
			TEXT("Description"),
			0,
			REG_SZ,
			(PBYTE)&driver_file.description,
			strlen(driver_file.description));
		if (result != NO_ERROR)
			goto error;
	}

	if (driver_file.display_name[0] != '\0')
	{
		result = RegSetValueEx(hkey,
			TEXT("DisplayName"),
			0,
			REG_SZ,
			(PBYTE)&driver_file.display_name,
			strlen(driver_file.display_name));
		if (result != NO_ERROR)
			goto error;
	}

	if (driver_file.depend_service[0] != '\0')
	{
		result = RegSetValueEx(hkey,
			TEXT("DependOnService"),
			0,
			REG_SZ,
			(PBYTE)&driver_file.depend_service,
			strlen(driver_file.depend_service));
		if (result != NO_ERROR)
			goto error;
	}

	if (driver_file.depend_group[0] != '\0')
	{
		result = RegSetValueEx(hkey,
			TEXT("DependOnGroup"),
			0,
			REG_SZ,
			(PBYTE)&driver_file.depend_group,
			strlen(driver_file.depend_group));
		if (result != NO_ERROR)
			goto error;
	}

	RegFlushKey(hkey);
	RegCloseKey(hkey);

	return(TRUE);

error:

	errorcode = GetLastError();
	wsprintf(errormsg, TEXT("Couldn't create the registry keys!!!\nErrorcode: %d"), errorcode);
	MessageBox(0, errormsg, TEXT("Launch Driver Error"), MB_ICONERROR);
	RegCloseKey(hkey);
	return(FALSE);

}

BOOL WINAPI CopyDriver() {
	TCHAR	dstpath[MAX_PATH];
	DWORD	errorcode = 0;
	TCHAR	errormsg[256];

	wsprintf(dstpath, TEXT("%s\\drivers\\%s.sys"), psWinSysDir, driver_file.psSysName);

	if (!CopyFileExA(driver_file.psDriverFile,
		dstpath,
		NULL,
		NULL,
		FALSE,
		COPY_FILE_NO_BUFFERING)) {
		errorcode = GetLastError();
		wsprintf(errormsg, TEXT("Couldn't copy driver file to destination %s \nErrorcode: %d"), dstpath, errorcode);
		MessageBox(0, errormsg, TEXT("Launch Driver Error"), MB_ICONERROR);
		return(FALSE);
	}
	return(TRUE);
}

BOOL WINAPI CreateDriver(SC_HANDLE SchSCManager) {
	LONG		error;
	SC_HANDLE   schService;
	SERVICE_STATUS  serviceStatus;
	TCHAR		errormsg[128];
	TCHAR	    DriverPath[MAX_PATH];

	wsprintf(DriverPath, TEXT("%s\\drivers\\%s.sys"), psWinSysDir, driver_file.psSysName);

	schService = CreateService(SchSCManager,          // SCManager database
		driver_file.psSysName, // name of service (driver)
		driver_file.psSysName, // name to display
		SERVICE_ALL_ACCESS,    // desired access
		SERVICE_KERNEL_DRIVER, // service type
		SERVICE_DEMAND_START,  // start type
		SERVICE_ERROR_NORMAL,  // error control type
		DriverPath,            // service's binary
		NULL,                  // no load ordering group
		NULL,                  // no tag identifier
		NULL,                  // no dependencies
		NULL,                  // LocalSystem account
		NULL                   // no password
	);
	if (schService == NULL) {
		error = GetLastError();
		// check if service already exist
		// if so just leave
		if (error == ERROR_SERVICE_EXISTS) {
			CloseServiceHandle(schService);
			return TRUE;
		}

		if (error == ERROR_SERVICE_MARKED_FOR_DELETE) {
			ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus);
			CloseServiceHandle(schService);
			return TRUE;
		}


		wsprintf(errormsg, TEXT("Couldn't start driver!\nErrorcode: %d"), error);
		MessageBox(0, errormsg, TEXT("Launch Driver Error"), MB_ICONERROR);
		return FALSE;
	}

	CloseServiceHandle(schService);
	return(TRUE);
}

BOOL WINAPI StartDriver(SC_HANDLE SchSCManager) {
	SC_HANDLE   schService;
	TCHAR		errormsg[128];

	if (driver_file.state == STARTED)
		return TRUE;

	schService = OpenService(SchSCManager,
		driver_file.psSysName,
		SERVICE_ALL_ACCESS
	);
	if (schService == NULL)
		return FALSE;

	if (!StartService(schService, 0, NULL)) {
		switch (GetLastError()) {
		case ERROR_SERVICE_ALREADY_RUNNING: 
			MessageBox(0, TEXT("Driver is already up and running!"),
				TEXT("Launch Driver Error"),
				MB_ICONERROR);
			CloseServiceHandle(schService);
			return(TRUE);

		case ERROR_SERVICE_DISABLED:		
			MessageBox(0, TEXT("Driver is disabled!"),
				TEXT("Launch Driver Error"),
				MB_ICONERROR);
			break;

		default:							
			wsprintf(errormsg, TEXT("Couldn't start driver!\nErrorcode: %d"), GetLastError());
			MessageBox(0, errormsg,
				TEXT("Launch Driver Error"),
				MB_ICONERROR);
			break;
		}
		CloseServiceHandle(schService);
		return(FALSE);
	}

	CloseServiceHandle(schService);
	driver_file.state = STARTED;
	return(TRUE);
}

// stop the driver...
BOOL WINAPI StopDriver(VOID) {
	SC_HANDLE		SchSCManager;
	SC_HANDLE       schService;
	SERVICE_STATUS  serviceStatus;
	TCHAR			errormsg[128];
	DWORD			error;
	BOOL			ret;

	ret = TRUE;

	if (driver_file.state != STARTED)
		return FALSE;

	SchSCManager = OpenServiceDatabase();

	schService = OpenService(SchSCManager,
		driver_file.psSysName,
		SERVICE_ALL_ACCESS);
	if (schService == NULL) {
		error = GetLastError();

		wsprintf(errormsg, TEXT("ERROR: Couldn't get driver!\nErrorcode: %d"), error);
		MessageBox(0, errormsg,
			TEXT("Launch Driver Error"),
			MB_ICONERROR);

		ret = FALSE;
		return(ret);
	}

	if (!ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus)) {
		error = GetLastError();

		if (error != ERROR_SERVICE_NOT_ACTIVE) {// is driver driver already stopped? If not, error...
			wsprintf(errormsg, TEXT("Couldn't stop driver!\nErrorcode: %d"), error);
			MessageBox(0, errormsg,
				TEXT("Launch Driver Error"),
				MB_ICONERROR);
			ret = FALSE;
		}
	}

	DeleteService(schService);
	CloseServiceHandle(schService);
	CloseServiceHandle(SchSCManager);
	driver_file.state = STOPED;
	return(ret);
}

BOOL WINAPI RemoveDriver(VOID) {
	SC_HANDLE	SchSCManager;
	SC_HANDLE	schService;
	TCHAR		errormsg[128];

	if (!StopDriver())
		return(FALSE);

	SchSCManager = OpenServiceDatabase();
	schService = OpenService(SchSCManager,
		driver_file.psSysName,
		SERVICE_ALL_ACCESS
	);
	if (schService == NULL)
		return FALSE;

	if (!DeleteService(schService)) {
		switch (GetLastError()) {
		case ERROR_SERVICE_MARKED_FOR_DELETE:	break;
		default:								wsprintf(errormsg, TEXT("Couldn't remove driver!\nErrorcode: %d"), GetLastError());
			MessageBox(0, errormsg,
				TEXT("Launch Driver Error"),
				MB_ICONERROR);

			CloseServiceHandle(schService);
			return(FALSE);
		}
	}
	CloseServiceHandle(schService);
	CloseServiceHandle(SchSCManager);
	return(TRUE);
}

// try to access the loaded driver and close the handle...
BOOL WINAPI CheckAccess(VOID) {
	TCHAR    completeDeviceName[256];
	HANDLE   hDevice;

	if ((GetVersion() & 0xFF) >= 5) {
		wsprintf(completeDeviceName, TEXT("\\\\.\\Global\\%s"), driver_file.psSysName);
	}
	else {
		wsprintf(completeDeviceName, TEXT("\\\\.\\%s"), driver_file.psSysName);
	}


	hDevice = CreateFile(completeDeviceName,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hDevice == ((HANDLE)-1))
		return FALSE;

	CloseHandle(hDevice);

	return(TRUE);
}

// establishe a connection to the service control manager... 
SC_HANDLE WINAPI OpenServiceDatabase(VOID) {
	SC_HANDLE	SCManagerHandle;
	TCHAR		errormsg[128];

	SCManagerHandle = OpenSCManager(NULL,                 // machine (NULL == local)
		NULL,                 // database (NULL == default)
		SC_MANAGER_ALL_ACCESS // access required
	);
	if (!SCManagerHandle) {
		wsprintf(errormsg, TEXT("Couldn't open the Service Manager!\nErrorcode: %d"), GetLastError());
		MessageBox(0, errormsg, TEXT("Launch Driver Error"), MB_ICONERROR);
		return(NULL);
	}
	return(SCManagerHandle);
}

// start up the driver...
BOOL WINAPI LaunchDriver(VOID)
{
	BOOL		ret;
	SC_HANDLE	SchSCManager;

	SchSCManager = OpenServiceDatabase();
	if (!SchSCManager)
		return(FALSE);

	ret = CreateDriver(SchSCManager);
	if (!ret) goto out;

	ret = StartDriver(SchSCManager);

out:
	CloseServiceHandle(SchSCManager);
	return(ret);
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


	if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hAccessToken)) {
		if (GetLastError() != ERROR_NO_TOKEN)
			return FALSE;

		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hAccessToken))
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
