#include "resource.h"
#include "stdafx.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define SECOND_INSTANCE (WM_APP + 100)
#define MAX_RANGE 0x7FFF

    BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
    BOOL WINAPI SetRegKeys(VOID);
    VOID WINAPI CleanUp(VOID);
    BOOL WINAPI IsDriverRunning(SC_HANDLE);
    BOOL WINAPI StopDriver(VOID);
    BOOL WINAPI StartDriver(SC_HANDLE);
    VOID WINAPI InitialDriverObj(VOID);
    BOOL WINAPI CopyDriver(VOID);
    BOOL WINAPI CheckAccess(VOID);
    BOOL WINAPI CreateDriver(SC_HANDLE);
    BOOL WINAPI LaunchDriver(VOID);
    BOOL WINAPI RemoveDriver(VOID);
    SC_HANDLE WINAPI OpenServiceDatabase(VOID);
    VOID UpdateStatusBar(HWND, WORD);

    extern BOOL
    IsAdmin(void);
    extern BOOL WINAPI GetDriverArchitecture(LPCWSTR, PDWORD);
    extern BOOL WINAPI CheckFileEnding(LPCWSTR, PWCHAR);
    extern BOOL WINAPI InstallDriverViaSetupApi(VOID);

    DRIVER_FILE driver_file = {0};
    INF_FILE inf_file;
    BOOL sys_file_present = FALSE;
    BOOL inf_file_present = FALSE;
    HWND _hdlg;
    WCHAR psWinSysDir[MAX_PATH];
    WCHAR psStartDir[4];
    WCHAR psSysName[MAX_PATH];

    /*
        The entry point for this graphical Windows-based application.
    */
#if defined(__GNUC__) && !defined(__REACTOS__)
    INT WINAPI
    WinMain(HINSTANCE hInstance, HINSTANCE hprevInstance, LPSTR pCmdLine, int nCmdShow)
#else
INT WINAPI
_tWinMain(HINSTANCE hInstance, HINSTANCE hprevInstance, LPTSTR pCmdLine, int nCmdShow)
#endif
    {
        HWND parent = FindWindow(L"#32770", L"Load Driver");
        if (IsWindow(parent))
        {
            SendMessage(parent, SECOND_INSTANCE, 0, 0);
            return (FALSE);
        }

        if (!IsAdmin())
        {
            MessageBox(NULL, L"User must be Admin!", L"ERROR", MB_ICONSTOP | MB_APPLMODAL);

            exit(1);
        }

        // memset(&driver_file, 0, sizeof(struct _DRIVER_FILE) * 1);
        return (DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN_DIALOG), NULL, DlgProc));
    }

    /*
        The main dialog box callback function to process UI messages.
    */

    BOOL CALLBACK
    DlgProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
    {
        OPENFILENAME OpenFileName;
        WORD bar_pos;
        WORD bar_delta;
        WCHAR errormsg[128];

        bar_pos = 0;
        bar_delta = (MAX_RANGE / 4);

        switch (message)
        {
            case SECOND_INSTANCE:
            {
                MessageBox(0, L"There is an instance already running!", L"Launch Driver Error", MB_ICONERROR);
                break;
            }

            case WM_INITDIALOG:
            {
                UINT x, y, i;

                _hdlg = hdlg;
                x = GetSystemMetrics(SM_CXSCREEN);
                y = GetSystemMetrics(SM_CYSCREEN);

                x = x / 4;
                y = y / 4;

                SetWindowPos(hdlg, HWND_TOP, x, y, 0, 0, SWP_NOSIZE);
                InitCommonControls();

                for (i = 0; DRIVER_TYPE[i] != NULL; i++)
                    SendMessage(
                        GetDlgItem(hdlg, (int)MAKEINTRESOURCE(IDC_DRIVER_TYPE)), CB_ADDSTRING, 0,
                        (LPARAM)DRIVER_TYPE[i]);

                SendMessage(
                    GetDlgItem(hdlg, (int)MAKEINTRESOURCE(IDC_DRIVER_TYPE)), CB_SELECTSTRING, 0,
                    (LPARAM)DRIVER_TYPE[0]);

                for (i = 0; DRIVER_START[i] != NULL; i++)
                    SendMessage(
                        GetDlgItem(hdlg, (int)MAKEINTRESOURCE(IDC_DRIVER_START)), CB_ADDSTRING, 0,
                        (LPARAM)DRIVER_START[i]);

                SendMessage(
                    GetDlgItem(hdlg, (int)MAKEINTRESOURCE(IDC_DRIVER_START)), CB_SELECTSTRING, 0,
                    (LPARAM)DRIVER_START[3]);

                for (i = 0; ERROR_CONTROL[i] != NULL; i++)
                    SendMessage(
                        GetDlgItem(hdlg, (int)MAKEINTRESOURCE(IDC_ERROR_CONTROL)), CB_ADDSTRING, 0,
                        (LPARAM)ERROR_CONTROL[i]);

                SendMessage(
                    GetDlgItem(hdlg, (int)MAKEINTRESOURCE(IDC_ERROR_CONTROL)), CB_SELECTSTRING, 0,
                    (LPARAM)ERROR_CONTROL[1]);

                if (!GetSystemDirectory(psWinSysDir, MAX_PATH))
                {

                    MessageBox(0, L"Couldn't determine the system directory", L"Launch Driver Error", MB_ICONERROR);

                    EndDialog(hdlg, 0);
                }

                EnableWindow(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(ID_LAUNCH)), FALSE);
                EnableWindow(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(ID_INSTALL)), FALSE);
                break;
            }

            case WM_COMMAND:
            {
                switch (wParam)
                {
                    case IDCANCEL:
                        CleanUp();
                        EndDialog(hdlg, 0);
                        break;
                    case IDC_LOAD_FILE:
                    {
                        LONG lErrMsg = 0;
                        DWORD lpBinaryType = 0;
                        DWORD errorcode = 0;
                        PWCHAR tok = NULL;
                        PWCHAR image = NULL;
                        // PWCHAR   context = NULL;
                        WCHAR psDriverFile[MAX_PATH];
                        WCHAR errormsg[256];
                        WCHAR sErrMsg[256];

                        ZeroMemory(&OpenFileName, sizeof(OPENFILENAME));
                        psDriverFile[0] = '\0';

                        wsprintf(psStartDir, L"%c:\\", psWinSysDir[0]);

                        OpenFileName.lStructSize = sizeof(OPENFILENAME);
                        OpenFileName.hwndOwner = NULL;
                        OpenFileName.lpstrFile = psDriverFile;
                        OpenFileName.lpstrFilter = L"Device Driver (*.sys)\0*.SYS\0*.inf\0*.INF\0\0";
                        OpenFileName.lpstrInitialDir = psStartDir;
                        OpenFileName.lpstrFileTitle = (WCHAR *)L"Please select a File";
                        OpenFileName.nMaxFile = MAX_PATH;
                        OpenFileName.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

                        if (!GetOpenFileName(&OpenFileName))
                        {
                            lErrMsg = CommDlgExtendedError();
                            if (lErrMsg)
                            {
                                wsprintf(sErrMsg, L"Error %d returned from GetOpenFileName()", lErrMsg);
                                MessageBox(NULL, sErrMsg, L"OpenDialog", MB_ICONERROR);
                            }
                            break;
                        }

                        if (CheckFileEnding((LPCWSTR)psDriverFile, (WCHAR *)L"sys"))
                        {

                            sys_file_present = TRUE;

                            ZeroMemory(&driver_file, sizeof(driver_file));
                            lstrcpyW(driver_file.psDriverFile, psDriverFile);

                            tok = wcstok(psDriverFile, L"\\");
                            do
                            {

                                image = tok;

                                tok = wcstok(NULL, L"\\");

                            } while (tok);

                            if (image)
                            {

                                lstrcpyW(psSysName, image);

                                tok = strchrW(psSysName, '.');

                                tok[0] = '\0';
                            }
                            else
                            {
                                MessageBox(
                                    0, L"Couldn't determine the file name!!!", L"Launch Driver Error", MB_ICONERROR);
                                break;
                            }

                            if (!GetDriverArchitecture((LPCWSTR)driver_file.psDriverFile, &lpBinaryType))
                            {
                                errorcode = GetLastError();
                                wsprintf(errormsg, L"Couldn't determine the file arch type!!!\nError: %d", errorcode);
                                MessageBox(0, errormsg, L"Launch Driver Error", MB_ICONERROR);
                                break;
                            }

                            if (!lpBinaryType)
                            {
                                MessageBox(
                                    0, L"Machine architecture of driver file is not supported!", L"Launch Driver Error",
                                    MB_ICONERROR);
                                break;
                            }

                            driver_file.machine_type = lpBinaryType;
                            lstrcpyW(driver_file.psSysName, psSysName);
                            driver_file.state = INITIALIZED;
                            EnableWindow(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(ID_INSTALL)), TRUE);
                            SetWindowText(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(IDC_DRIVER)), driver_file.psDriverFile);
                        }
                        else if (CheckFileEnding(psDriverFile, L"inf"))
                        {
                            inf_file_present = TRUE;

                            ZeroMemory(inf_file.psInfFile, sizeof(inf_file.psInfFile));
                            lstrcpyW(inf_file.psInfFile, psDriverFile);
                            SetWindowText(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(IDC_DRIVER)), inf_file.psInfFile);
                            EnableWindow(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(ID_INSTALL)), TRUE);
                        }
                        else
                        {
                            MessageBox(
                                0, L"Please select a *.sys or *.inf file!", L"Launch Driver Error", MB_ICONERROR);
                        }

                        break;
                    }
                    case ID_LAUNCH:
                    {
                        INT check_access = 0;

                        SendMessage(GetDlgItem(hdlg, IDC_PROGRESS), PBM_SETPOS, 0, 0);

                        SendDlgItemMessage(hdlg, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELONG(0, MAX_RANGE));

                        switch (driver_file.state)
                        {
                            case STOPED:
                            case INSTALLED:

                                EnableWindow(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(ID_LAUNCH)), FALSE);
                                if (!LaunchDriver())
                                {
                                    EnableWindow(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(ID_LAUNCH)), TRUE);
                                    break;
                                }

                                SetWindowText(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(ID_LAUNCH)), L"Stop");
                                EnableWindow(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(ID_LAUNCH)), TRUE);
                                EnableWindow(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(ID_INSTALL)), FALSE);

                                driver_file.state = STARTED;
                                check_access = (INT)SendMessage(
                                    GetDlgItem(_hdlg, (int)MAKEINTRESOURCE(IDC_CHECK_ACCESS)), BM_GETCHECK, 0, 0);
                                if (check_access)
                                {
                                    if (!CheckAccess())
                                        MessageBox(
                                            0, L"Couldn't access the driver!!!", L"Launch Driver Error", MB_ICONERROR);
                                }

                                break;

                            case STARTED:
                                StopDriver();
                                SetWindowText(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(ID_LAUNCH)), L"Launch");
                                EnableWindow(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(ID_INSTALL)), TRUE);
                                driver_file.state = STOPED;
                                break;

                            default:
                                break;
                        }
                        break;
                    }
                    case ID_INSTALL:

                        // we have an inf file so use the SetupAPI
                        if (inf_file_present == TRUE)
                        {

                            UpdateStatusBar(hdlg, (bar_delta * ++bar_pos));
                            if (!InstallDriverViaSetupApi())
                            {

                                wsprintf(
                                    errormsg, L"Couldn't install kernel driver via INF file!\nErrorcode: 0x%x",
                                    GetLastError());

                                MessageBox(0, errormsg, L"Launch Driver Error", MB_ICONERROR);
                                SendMessage(GetDlgItem(hdlg, IDC_PROGRESS), PBM_SETPOS, 0, 0);
                                break;
                            }

                            UpdateStatusBar(hdlg, (bar_delta * ++bar_pos));
                            SetWindowText(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(ID_INSTALL)), L"&Uninstall");
                            SendMessage(GetDlgItem(hdlg, IDC_PROGRESS), PBM_SETPOS, 0, 0);
                            break;
                        }

                        switch (driver_file.state)
                        {
                            case STARTED:
                                if (!StopDriver())
                                {
                                    MessageBox(0, L"Couldn't stop the driver!!!", L"Launch Driver Error", MB_ICONERROR);
                                    break;
                                }
                                SendMessage(GetDlgItem(hdlg, IDC_PROGRESS), PBM_SETPOS, 0, 0);
                                SendDlgItemMessage(hdlg, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELONG(0, MAX_RANGE));
                                SetWindowText(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(ID_LAUNCH)), L"Launch");

                            case STOPED:
                            case INSTALLED:
                            case INITIALIZED:

                                InitialDriverObj();
                                UpdateStatusBar(hdlg, (bar_delta * ++bar_pos));

                                if (SendMessage(
                                        GetDlgItem(_hdlg, (int)MAKEINTRESOURCE(IDC_KEEP_REG_ENTRYS)), BM_GETCHECK, 0,
                                        0))
                                    if (!CopyDriver())
                                        break;

                                UpdateStatusBar(hdlg, (bar_delta * ++bar_pos));
                                SetRegKeys();
                                UpdateStatusBar(hdlg, (bar_delta * ++bar_pos));

                                driver_file.state = INSTALLED;
                                // SetWindowText(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(ID_INSTALL)), L"Uninstall"));

                                EnableWindow(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(ID_INSTALL)), FALSE);
                                EnableWindow(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(ID_LAUNCH)), TRUE);
                                UpdateStatusBar(hdlg, (bar_delta * ++bar_pos));

                                break;
                            default:
                                break;
                        }

                        break;

                    case IDC_DEPENCES:
                    {
                        if (IsWindowEnabled(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(IDC_ON_SERVICE))))
                            EnableWindow(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(IDC_ON_SERVICE)), FALSE);
                        else
                            EnableWindow(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(IDC_ON_SERVICE)), TRUE);

                        if (IsWindowEnabled(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(IDC_ON_GROUP))))
                            EnableWindow(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(IDC_ON_GROUP)), FALSE);
                        else
                            EnableWindow(GetDlgItem(hdlg, (int)MAKEINTRESOURCE(IDC_ON_GROUP)), TRUE);
                        break;
                    }
                }
                break;
            }
            default:
                return FALSE;
        }
        return TRUE;
    }

    /*
        Upadte the status bar progress
     */
    VOID
    UpdateStatusBar(HWND hdlg, WORD progress)
    {
        if (IsWindow(hdlg))
            SendDlgItemMessage(hdlg, IDC_PROGRESS, PBM_SETPOS, progress, 0);
    }

    /*
        Clean everything up
        FIXME: Registry key deletion
    */
    VOID WINAPI CleanUp(VOID)
    {
        INT iKeepKey = 0;
        if (!driver_file.psDriverFile[0])
            return;

        iKeepKey = (INT)SendMessage(GetDlgItem(_hdlg, (int)MAKEINTRESOURCE(IDC_KEEP_REG_ENTRYS)), BM_GETCHECK, 0, 0);
        if (!iKeepKey)
            RemoveDriver();
    }

    VOID WINAPI InitialDriverObj(VOID)
    {
        LRESULT index = 0;

        index = SendMessage(GetDlgItem(_hdlg, (int)MAKEINTRESOURCE(IDC_DRIVER_TYPE)), CB_GETCURSEL, 0, 0);
        if (index == CB_ERR)
        {
            MessageBox(0, L"Couldn't get the driver type!!!", L"Launch Driver Error", MB_ICONERROR);
            return;
        }

        switch (index)
        {
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

        index = SendMessage(GetDlgItem(_hdlg, (int)MAKEINTRESOURCE(IDC_DRIVER_START)), CB_GETCURSEL, 0, 0);

        if (index == CB_ERR)
        {
            MessageBox(0, L"Couldn't get the start type!!!", L"Launch Driver Error", MB_ICONERROR);
            return;
        }
        driver_file.start = index;

        index = SendMessage(GetDlgItem(_hdlg, (int)MAKEINTRESOURCE(IDC_ERROR_CONTROL)), CB_GETCURSEL, 0, 0);

        if (index == CB_ERR)
        {
            MessageBox(0, L"Couldn't get the error control!!!", L"Launch Driver Error", MB_ICONERROR);
            return;
        }

        driver_file.errorcontrol = index;

        GetWindowText(GetDlgItem(_hdlg, (int)MAKEINTRESOURCE(IDC_DESCRIPTION)), driver_file.description, 255);
        GetWindowText(GetDlgItem(_hdlg, (int)MAKEINTRESOURCE(IDC_DISPLAY_NAME)), driver_file.display_name, 255);
        if (SendMessage(GetDlgItem(_hdlg, (int)MAKEINTRESOURCE(IDC_DEPENCES)), BM_GETCHECK, 0, 0))
        {
            GetWindowText(GetDlgItem(_hdlg, (int)MAKEINTRESOURCE(IDC_ON_SERVICE)), driver_file.depend_service, 255);
            GetWindowText(GetDlgItem(_hdlg, (int)MAKEINTRESOURCE(IDC_ON_GROUP)), driver_file.depend_group, 255);
        }
    }

    /*
    Create the necessary registry keys.
    */
    BOOL WINAPI SetRegKeys(VOID)
    {
        HKEY hkey;
        LONG result = 0;
        DWORD errorcode = 0;
        WCHAR errormsg[256];
        WCHAR psSubKey[MAX_PATH];

        wsprintf(psSubKey, L"%s%s", DRIVER_REGISTRY_PATH, psSysName);

        result = RegCreateKey(HKEY_LOCAL_MACHINE, psSubKey, &hkey);
        if (result != NO_ERROR)
            goto error;

        result = RegSetValueEx(hkey, L"Type", 0, REG_DWORD, (PBYTE)&driver_file.type, sizeof(driver_file.type));
        if (result != NO_ERROR)
            goto error;

        result = RegSetValueEx(hkey, L"Start", 0, REG_DWORD, (PBYTE)&driver_file.start, sizeof(driver_file.start));
        if (result != NO_ERROR)
            goto error;

        result = RegSetValueEx(
            hkey, L"ErrorControl", 0, REG_DWORD, (PBYTE)&driver_file.errorcontrol, sizeof(driver_file.errorcontrol));
        if (result != NO_ERROR)
            goto error;

        if (driver_file.description[0] != '\0')
        {
            result = RegSetValueEx(
                hkey, L"Description", 0, REG_SZ, (PBYTE)&driver_file.description, strlenW(driver_file.description));

            if (result != NO_ERROR)
                goto error;
        }

        if (driver_file.display_name[0] != '\0')
        {
            result = RegSetValueEx(
                hkey, L"DisplayName", 0, REG_SZ, (PBYTE)&driver_file.display_name, strlenW(driver_file.display_name));

            if (result != NO_ERROR)
                goto error;
        }

        if (driver_file.depend_service[0] != '\0')
        {
            result = RegSetValueEx(
                hkey, L"DependOnService", 0, REG_SZ, (PBYTE)&driver_file.depend_service,
                strlenW(driver_file.depend_service));
            if (result != NO_ERROR)
                goto error;
        }

        if (driver_file.depend_group[0] != '\0')
        {
            result = RegSetValueEx(
                hkey, L"DependOnGroup", 0, REG_SZ, (PBYTE)&driver_file.depend_group, strlenW(driver_file.depend_group));
            if (result != NO_ERROR)
                goto error;
        }

        RegFlushKey(hkey);
        RegCloseKey(hkey);
        return (TRUE);

    error:
        errorcode = GetLastError();
        wsprintf(errormsg, L"Couldn't create the registry keys!!!\nErrorcode: %d", errorcode);
        MessageBox(0, errormsg, L"Launch Driver Error", MB_ICONERROR);
        RegCloseKey(hkey);
        return (FALSE);
    }
    /*
        Copy driver file to the specific Windows directory.
    */
    BOOL WINAPI
    CopyDriver()
    {
        WCHAR dstpath[MAX_PATH];
        DWORD errorcode = 0;
        WCHAR errormsg[256];

        wsprintf(dstpath, L"%s\\drivers\\%s.sys", psWinSysDir, driver_file.psSysName);
        if (!strcmpW(driver_file.psDriverFile, dstpath))
            return TRUE;

        if (!CopyFile(driver_file.psDriverFile, dstpath, FALSE))
        {
            errorcode = GetLastError();
            wsprintf(errormsg, L"Couldn't copy driver file to destination %s \nErrorcode: %d", dstpath, errorcode);
            MessageBox(0, errormsg, L"Launch Driver Error", MB_ICONERROR);
            return FALSE;
        }

        driver_file.boot_save = TRUE;
        return TRUE;
    }

    /*
        Create a new entry for this diver in the control manager database.
    */
    BOOL WINAPI
    CreateDriver(SC_HANDLE SchSCManager)
    {
        LONG error;
        SC_HANDLE schService;
        SERVICE_STATUS serviceStatus;
        WCHAR errormsg[128];
        WCHAR DriverPath[MAX_PATH];
        BOOL status;

        status = TRUE;

        if (driver_file.boot_save)
        {
            if (driver_file.machine_type == ARCH_X64)
            {
                wsprintf(DriverPath, L"%s\\SysWOW64\\drivers\\%s.sys", psWinSysDir, driver_file.psSysName);
            }
            else
            {
                wsprintf(DriverPath, L"%s\\drivers\\%s.sys", psWinSysDir, driver_file.psSysName);
            }
        }
        else
            strncpyW(DriverPath, driver_file.psDriverFile, MAX_PATH);

        // first check if the service object is already created
        schService = OpenService(SchSCManager, driver_file.psSysName, SERVICE_ALL_ACCESS);

        // try to clean up
        if (schService)
        {
            DeleteService(schService);
            CloseServiceHandle(schService);
        }

        // open Service Control Manager database
        schService = CreateService(
            SchSCManager,
            driver_file.psSysName, // name of driver
            driver_file.psSysName, // display
            SERVICE_ALL_ACCESS,
            SERVICE_KERNEL_DRIVER, // type of service
            SERVICE_DEMAND_START,  // start type
            SERVICE_ERROR_NORMAL,  // error control type
            DriverPath,            // service's binary
            NULL,                  // no load ordering group
            NULL,                  // no tag identifier
            NULL,                  // no dependencies
            NULL,                  // LocalSystem account
            NULL                   // no password
        );

        if (schService == NULL)
        {
            error = GetLastError();
            if (error == ERROR_SERVICE_MARKED_FOR_DELETE)
            {
                ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus);
            }
            else
            {
                wsprintf(errormsg, L"Couldn't start driver!\nErrorcode: %d", error);
                MessageBox(0, errormsg, L"Launch Driver Error", MB_ICONERROR);
                status = FALSE;
            }
        }

        CloseServiceHandle(schService);
        return status;
    }

    /*
        Checks if the  driver is allready running.
    */
    BOOL WINAPI
    IsDriverRunning(SC_HANDLE SchSCManager)
    {
        SC_HANDLE schService;
        SERVICE_STATUS_PROCESS ServiceStatusProcess;
        DWORD nBufSize;
        DWORD error;
        BOOL status = TRUE;

        schService = OpenService(SchSCManager, driver_file.psSysName, SERVICE_ALL_ACCESS);
        if (schService == NULL)
        {
            error = GetLastError();
            if (error == ERROR_SERVICE_DOES_NOT_EXIST)
            {
                driver_file.errorcontrol = 0;
                //			if (driver_file.state == STARTED)
                //				driver_file.state = INSTALLED;
                return FALSE;
            }
            else
            {
                return FALSE;
            }
        }

        if (!QueryServiceStatusEx(
                schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ServiceStatusProcess, sizeof(ServiceStatusProcess),
                &nBufSize))
        {
            status = FALSE;
            goto err_out;
        }

        switch (ServiceStatusProcess.dwCurrentState)
        {
            case SERVICE_RUNNING:
            case SERVICE_START_PENDING:
                break;
            default:
                status = FALSE;
                break;
        }

    err_out:
        CloseServiceHandle(schService);
        return status;
    }

    /*
        Start the driver.
    */
    BOOL WINAPI
    StartDriver(SC_HANDLE SchSCManager)
    {
        SC_HANDLE schService;
        WCHAR errormsg[128];
        BOOL status;
        DWORD error;

        status = TRUE;

        if (driver_file.state == STARTED)
            return status;

        schService = OpenService(SchSCManager, driver_file.psSysName, SERVICE_ALL_ACCESS);
        if (schService == NULL)
            return FALSE;

        if (!StartService(schService, 0, NULL))
        {
            error = GetLastError();
            switch (error)
            {
                case ERROR_SERVICE_ALREADY_RUNNING:
                    MessageBox(0, L"Driver is already up and running!", L"Launch Driver Error", MB_ICONERROR);
                    CloseServiceHandle(schService);
                    driver_file.state = STARTED;
                    driver_file.last_error = error;
                    return status;

                case ERROR_SERVICE_DISABLED:
                    MessageBox(0, L"Driver is disabled!", L"Launch Driver Error", MB_ICONERROR);
                    status = FALSE;
                    goto error_out;

                default:
                    wsprintf(errormsg, L"Couldn't start driver!\nErrorcode: %d", GetLastError());
                    MessageBox(0, errormsg, L"Launch Driver Error", MB_ICONERROR);
                    status = FALSE;
                    DeleteService(schService);
                    goto error_out;
            }
            CloseServiceHandle(schService);
            return (FALSE);
        }
        driver_file.state = STARTED;

    error_out:
        CloseServiceHandle(schService);
        return status;
    }

    /*
        Stop the driver.
    */
    BOOL WINAPI StopDriver(VOID)
    {
        SC_HANDLE SchSCManager;
        SC_HANDLE schService;
        SERVICE_STATUS serviceStatus;
        WCHAR errormsg[128];
        DWORD error;
        BOOL status;

        if (driver_file.state != STARTED)
            return FALSE;

        SchSCManager = OpenServiceDatabase();
        if (!SchSCManager)
            return FALSE;

        status = TRUE;
        schService = OpenService(SchSCManager, driver_file.psSysName, SERVICE_ALL_ACCESS);
        if (schService == NULL)
        {
            error = GetLastError();
            wsprintf(errormsg, L"ERROR: Couldn't get driver!\nErrorcode: %d", error);
            MessageBox(0, errormsg, L"Launch Driver Error", MB_ICONERROR);
            status = FALSE;
            goto error_out2;
        }

        if (!ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus))
        {
            error = GetLastError();

            // is the kernel driver already stopped?
            if (error != ERROR_SERVICE_NOT_ACTIVE)
            {
                wsprintf(errormsg, L"Couldn't stop driver!\nErrorcode: %d", error);
                MessageBox(0, errormsg, L"Launch Driver Error", MB_ICONERROR);
                status = FALSE;
                goto error_out1;
            }
        }

        DeleteService(schService);
        driver_file.state = STOPED;

    error_out1:
        CloseServiceHandle(schService);

    error_out2:
        CloseServiceHandle(SchSCManager);
        return status;
    }

    /*
        This funtion deletes the service
    */
    BOOL WINAPI RemoveDriver(VOID)
    {
        SC_HANDLE SchSCManager;
        SC_HANDLE schService;
        WCHAR errormsg[128];
        BOOL status;

        if (!StopDriver())
            return FALSE;

        SchSCManager = OpenServiceDatabase();
        if (!SchSCManager)
            return FALSE;

        schService = OpenService(SchSCManager, driver_file.psSysName, SERVICE_ALL_ACCESS);
        if (schService == NULL)
            return FALSE;

        status = TRUE;

        if (!DeleteService(schService))
        {
            switch (GetLastError())
            {
                case ERROR_SERVICE_MARKED_FOR_DELETE:
                    break;
                default:
                    wsprintf(errormsg, L"Couldn't remove driver!\nErrorcode: %d", GetLastError());
                    MessageBox(0, errormsg, L"Launch Driver Error", MB_ICONERROR);
                    status = FALSE;
                    goto error_out;
            }
        }

    error_out:
        CloseServiceHandle(schService);
        CloseServiceHandle(SchSCManager);
        return (status);
    }

    /*
        This funtion trys to access the loaded driver and close the handle.
    */
    BOOL WINAPI CheckAccess(VOID)
    {
        WCHAR DriverName[256];
        HANDLE hDevice;

        if ((GetVersion() & 0xFF) >= 5)
        {
            wsprintf(DriverName, L"\\\\.\\Global\\%s", driver_file.psSysName);
        }
        else
        {
            wsprintf(DriverName, L"\\\\.\\%s", driver_file.psSysName);
        }

        hDevice =
            CreateFile(DriverName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hDevice == ((HANDLE)-1))
            return FALSE;

        CloseHandle(hDevice);
        return TRUE;
    }

    // establish a connection to the service control manager...
    SC_HANDLE WINAPI OpenServiceDatabase(VOID)
    {
        SC_HANDLE SCManagerHandle;
        WCHAR errormsg[128];

        SCManagerHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (!SCManagerHandle)
        {
            wsprintf(errormsg, L"Couldn't open the Service Manager!\nErrorcode: %d", GetLastError());
            MessageBox(0, errormsg, L"Launch Driver Error", MB_ICONERROR);
            return NULL;
        }
        return SCManagerHandle;
    }

    // start the driver
    BOOL WINAPI LaunchDriver(VOID)
    {
        SC_HANDLE SchSCManager;
        BOOL status;

        SchSCManager = OpenServiceDatabase();
        if (!SchSCManager)
            return FALSE;

        if (IsDriverRunning(SchSCManager))
            return TRUE;

        status = CreateDriver(SchSCManager);
        if (!status)
            goto out;

        status = StartDriver(SchSCManager);

    out:
        CloseServiceHandle(SchSCManager);
        return status;
    }

#ifdef __cplusplus
}
#endif
