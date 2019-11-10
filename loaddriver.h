/*
loaddriver.h
*/

#define DRIVER_REGISTRY_PATH	TEXT("SYSTEM\\CurrentControlSet\\Services\\")
#define KDMF_VERSION_PATH		TEXT("SYSTEM\\CurrentControlSet\\Control\\Wdf\\Kmdf\\KmdfLibrary\\Versions\\")
#define DLL_NEW_DEV				TEXT("newdev.dll")

#ifdef UNICDE
#define  DIINSTALLDRIVER		TEXT("DiInstallDriverW")
#else
#define  DIINSTALLDRIVER		TEXT("DiInstallDriverA")
#endif

// supported machine types
#define ARCH_X64				0x64
#define ARCH_I386				0x32

typedef struct _DRIVER_FILE {
	UINT	start;
	UINT	type;
	UINT	size;
	UINT	state;
	UINT	errorcontrol;
	DWORD	last_error;
	DWORD	version;
	DWORD	flags;
	DWORD	ftype;
	DWORD	fos;
	DWORD	fsubtype;
	TCHAR	description[256];
	TCHAR	display_name[256];
	TCHAR	depend_service[256];
	TCHAR	depend_group[256];
	TCHAR	psDriverFile[MAX_PATH];
	TCHAR	psSysName[256];
	BOOL	boot_save;
	DWORD	machine_type;
}DRIVER_FILE,*LP_DRIVER_FILE;

#define DRIVER_FILE_SIZE sizeof(DRIVER_FILE)

typedef struct _INF_FILE {
	UINT	state;
	TCHAR	psInfFile[MAX_PATH];
}INF_FILE, *PINF_FILE;


typedef struct _API_HELPER {
	HMODULE	hDll;
	FARPROC	Function;
} API_HELPER, *PAPI_HELPER;

typedef struct _SYSTEM_INFORMATION {
	DWORD	WindowsVersion;
	DWORD	KdmfVersion;
	DWORD	WdfVersion;
	DWORD	ArchType;
	DWORD	NoneDriverSigning; 
	DWORD	DriverSigning;
}SYSTEM_INFORMATION, *LPSYSTEM_INFORMATION;

#define SYSTEM_INFORMATION_SIZE	sizeof(SYSTEM_INFORMATION)

static enum _DRIVER_STATES
{
	INITIALIZED	  =1,	
	INSTALLED, 
	STARTED,
	STOPED,
	UNINSTALLED
}DRIVER_STATES;

CONST LPCTSTR DRIVER_TYPE[] = {
	TEXT("Kernel Mode Driver"),
	TEXT("File System Driver"),
	TEXT("Adapter"),
	NULL
};

CONST LPCTSTR DRIVER_START[] = {
	TEXT("Boot Start"),
	TEXT("System Start"),
	TEXT("Auto Start"),
	TEXT("Demand Start"),
	TEXT("Disabled"),
	NULL
};

CONST LPCTSTR ERROR_CONTROL[] = {
	TEXT("SERVICE_ERROR_IGNORE"),
	TEXT("SERVICE_ERROR_NORMAL"),
	TEXT("SERVICE_ERROR_SEVERE"),
	TEXT("SERVICE_ERROR_CRITICAL"),
	NULL
};
