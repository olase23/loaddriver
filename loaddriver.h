/*
loaddriver.h
*/

#define DRIVER_REGISTRY_PATH	TEXT("SYSTEM\\CurrentControlSet\\Services\\")

typedef struct _DRIVER_FILE {
	UINT	start;
	UINT	type;
	UINT	size;
	UINT	state;
	UINT	errorcontrol;
	TCHAR	description[256];
	TCHAR	display_name[256];
	TCHAR	depend_service[256];
	TCHAR	depend_group[256];
	TCHAR	psDriverFile[MAX_PATH];
	TCHAR	psSysName[256];
}DRIVER_FILE,*LP_DRIVER_FILE;

#define DRIVER_FILE_SIZE sizeof(DRIVER_FILE)

enum _DRIVER_STATES
{
	INITIALIZED	  =1,	
	INSTALLED, 
	STARTED,
	STOPED
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
