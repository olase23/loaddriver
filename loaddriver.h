/*
loaddriver.h
*/

#define DRIVER_REGISTRY_PATH L"SYSTEM\\CurrentControlSet\\Services\\"
#define DLL_NEW_DEV L"newdev.dll"

#ifdef UNICDE
#define DIINSTALLDRIVER L"DiInstallDriverW"
#else
#define DIINSTALLDRIVER L"DiInstallDriverA"
#endif

// supported machine types
#define ARCH_X64 0x64
#define ARCH_I386 0x32

enum
{
    INITIALIZED = 1,
    INSTALLED,
    STARTED,
    STOPED,
    UNINSTALLED
};

typedef struct _DRIVER_FILE
{
    UINT start;
    UINT type;
    UINT size;
    UINT state;
    UINT errorcontrol;
    DWORD last_error;
    WCHAR description[256];
    WCHAR display_name[256];
    WCHAR depend_service[256];
    WCHAR depend_group[256];
    WCHAR psDriverFile[MAX_PATH];
    WCHAR psSysName[256];
    BOOL boot_save;
    DWORD machine_type;
} DRIVER_FILE, *LP_DRIVER_FILE;

typedef struct _INF_FILE
{
    UINT state;
    WCHAR psInfFile[MAX_PATH];
} INF_FILE, *PINF_FILE;

typedef struct _API_HELPER
{
    HMODULE hDll;
    FARPROC Function;
} API_HELPER, *PAPI_HELPER;

#define DRIVER_FILE_SIZE sizeof(DRIVER_FILE)

#ifdef MAX_PATH
#define MAX_PATH 260
#endif

static CONST LPCTSTR DRIVER_TYPE[] = {TEXT("Kernel Mode Driver"), TEXT("File System Driver"), TEXT("Adapter"), NULL};

static CONST LPCTSTR DRIVER_START[] = {TEXT("Boot Start"),   TEXT("System Start"), TEXT("Auto Start"),
                                       TEXT("Demand Start"), TEXT("Disabled"),     NULL};

static CONST LPCTSTR ERROR_CONTROL[] = {
    TEXT("SERVICE_ERROR_IGNORE"), TEXT("SERVICE_ERROR_NORMAL"), TEXT("SERVICE_ERROR_SEVERE"),
    TEXT("SERVICE_ERROR_CRITICAL"), NULL};
