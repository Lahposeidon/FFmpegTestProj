#pragma once

#include "..\GlobalUtils\StopWatch.h"
#include "stdafx.h"

#ifdef _UNICODE
#define _tstring wstring
#else
#define _tstring string
#endif//_UNICODE

#define MAX_IPADDRESS 20
#define MAX_MACADDRESS 50
#define MAX_REGVALUE 1024

typedef enum _PC_NETWORK_TYPE
{
	WIRELESS_LAN = 0,
	PHYSICAL_LAN
} PC_NETWORK_TYPE;

typedef struct _PC_NETWORKINFO
{
	PC_NETWORK_TYPE eNetworkType;
	TCHAR szIPAddress[MAX_IPADDRESS];
	TCHAR szMACAddress[MAX_MACADDRESS];
} PC_NETWORKINFO;

typedef enum _OS_VERSION{
	OS_ERROR,
	OS_WIN98ME,
	OS_WIN2K,
	OS_WINXP,
	OS_WINXP64,
	OS_WINXPSV,
	OS_WINVISTA,
	OS_WIN7,
	OS_WIN8,
	OS_WIN8_1
} OS_VERSION;

typedef enum _ExecuteWaitOption{
	Option_WaitForInputIdle = 0,
	Option_WaitForSingleObject = 1,
	Option_WaitNothing = 2,	
} ExecuteWaitOption;


class AFX_EXT_CLASS GlobalFunc
{
private:
	GlobalFunc(void);
	~GlobalFunc(void);

public:
	// Resource Method
	static void SaveCurrentRunningDir();
	static LPCTSTR GetCurrentRunningDir();

	// Process Execute,Kill Routine
	static bool ExecuteTargetProgram(LPCTSTR lpszTargetProgramPath,
									 LPCTSTR lpszParameter = NULL,
									 bool bRunAdminAuth = false,
									 ExecuteWaitOption WaitUntilProgramEnd = Option_WaitForInputIdle,
									 HWND hWnd = NULL,
									 bool bUseAdminAuthInVista = false);

	static void KillTargetProcess(LPCTSTR lpszProcessName,bool bWaitASecond = false);

	static BOOL CheckTargetSiteIsOpenedOrNot(LPCTSTR lpszCiteName);
	// Current Running Process Name
	static void SetCurrentRunningProcessName(LPCTSTR lpszProcessName);
	static LPCTSTR GetCurrentRunningProcessName();

	// Debug Message Method
	static CString DebugAndLogPrint(bool bSaveFile, LPCTSTR fmt,...);

	// Show Error Message
	static LPCTSTR ShowErrorMessage(DWORD dwErrorCode);

	// Folder Method
	static LPCTSTR GetCommonDataFolder();
	static LPCTSTR GenerateTempFileFullPath();
	static LPCTSTR GetUserTempFolder();
	static LPCTSTR GetAppDataFolder();

	// Registry Method
	static LPCTSTR ReadCryptReg(LPCTSTR lpszKeyName, 
								LPCTSTR lpszValueName,
								HKEY hKey = HKEY_CURRENT_USER,
								bool bCheck64Key = false,
								bool bUse6432Key = false);
	static BOOL	WriteCryptReg(LPCTSTR lpszKeyName, 
							  LPCTSTR lpszValueName, 
							  LPCTSTR lpValue,
							  HKEY hKey = HKEY_CURRENT_USER);

	static LPCTSTR ReadStringValue(LPCTSTR lpszKeyName, 
								   LPCTSTR lpszValueName,
								   LPCTSTR lpszDefaultValue=_T(""),
								   HKEY hKey = HKEY_CURRENT_USER,
								   bool bCheck64Key = false,
								   bool bUse6432Key = false);
	static DWORD ReadDWORDValue(LPCTSTR lpszKeyName, 
								LPCTSTR lpszValueName,
								DWORD dwDefaultValue=0,
								HKEY hKey = HKEY_CURRENT_USER,
								bool bCheck64Key = false,
								bool bUse6432Key = false);

	static BOOL	WriteStringValue(LPCTSTR lpszKeyName, 
								 LPCTSTR lpszValueName, 
								 LPCTSTR lpValue,
								 HKEY hKey = HKEY_CURRENT_USER);
	static BOOL	WriteDWORDValue(LPCTSTR lpszKeyName, 
								LPCTSTR lpszValueName,
								DWORD dwValue,
								HKEY hKey = HKEY_CURRENT_USER);

	static void	DeleteRegKey(LPCTSTR lpszSubKeyName, 
							 LPCTSTR lpszKeyName,
							 HKEY hKey = HKEY_CURRENT_USER);
	static BOOL DeleteRegValue(LPCTSTR lpszKeyName, 
							   LPCTSTR lpszValueName,
							   HKEY hKey = HKEY_CURRENT_USER);

	static void GetSubKeyNames(LPCTSTR lpszKeyName,
							   std::list<CString>& subKeyNames,
							   HKEY hStartKey = HKEY_CURRENT_USER,
							   bool bCheck64Key = false,
							   bool bUse6432Key = false);
	static void GetAllRegValueNames(LPCTSTR lpszKeyName,
									std::list<CString>& ValuesNames,
									HKEY hStartKey = HKEY_CURRENT_USER,
									bool bCheck64Key = false,
									bool bUse6432Key = false);

	// Decode,Encode Method
	static LPSTR Unicode2UTF8 (LPCWSTR szStr,int *cchAnsiLength = NULL);
	static LPWSTR UTF8ToUnicode(LPSTR szStr,int *cchUnicode = NULL);
	static LPCTSTR EncodingToBase64(LPCTSTR lpszTarget);
	static LPCTSTR DecodingFromBase64(LPCTSTR lpszTarget);
	static LONG encryptMD5(CString strInput,BYTE* pbOutput,CString& strOutput);
	static LPSTR ANSIToUTF8(char * pszCode);
	static LPSTR UTF8ToANSI(char *pszCode);
	static LPSTR String2Char(CString str);
	static CString Char2String(char *str);

	// File and Directory Processing Method
	static void DeleteTargetFile(LPCTSTR lpszFilePath);
	static bool CreateDirectoryTree(LPCTSTR lpszFolderPath);
	static void DeleteDirectoryTree(LPCTSTR lpszTargetPath);
	static bool CopyDirectoryTree(LPCTSTR lpszOriginalPath,LPCTSTR lpszTargetPath);
	static void SearchFilesInDirectory(LPCTSTR lpszFolderPath,std::list<CString>& SearchedFilesList);

	static bool FileExist(LPCTSTR lpszFilePath);
	static ULONGLONG FileSize(LPCTSTR lpszFilePath);
	
	static bool CheckTargetProcessIsRunningOrNot(LPCTSTR lpszProcessName);

	// Hardware Info
	static LPCTSTR GetLocalComputerName();
	static LPCTSTR GetMacAddress();
	static LPCTSTR GetIPAddress();
	static LPCTSTR GetOSName();
	static LPCTSTR GetOSLanguage();
	static LPCTSTR GetCPUName();
	static LPCTSTR GetUSBHostName();
	static LPCTSTR GetUSBHostHWID();
	static LPCTSTR GetMotherboardInfo();
	static LPCTSTR GetPhysicalRAMSize();
	static LPCTSTR GetPhysicalDiskSerialNumber();
	static long GetFreeDiskSpace();
	static OS_VERSION GetOSVersion();
	static BOOL IsWinVista();
	static BOOL Is64BitOS();

	static bool CheckCrc(LPCTSTR  lpszFilePath, LPCTSTR lpszCrc);
	static DWORD GetFileCrc(LPCTSTR lpszFileName);
	static void SendUDPSignal(int nPort, LPCTSTR lpszMessage, int ncharset = CP_UTF8);
	static DWORD _tinet_addr(const TCHAR *cp);

private:
	static void InitializeDebugAndLogPrint(LPCTSTR lpszCurrentRunningProcessName);

	static BOOL GetRegValue(HKEY hKey,
							LPCTSTR lpszKeyName, 
							LPCTSTR lpszValueName,
							DWORD& dwValue,
							bool bCheck64Key = false,
							bool bUse6432Key = false);
	static BOOL GetRegValue(HKEY hKey,
							LPCTSTR lpszKeyName, 
							LPCTSTR lpszValueName,
							CString& strValue,
							bool bCheck64Key = false,
							bool bUse6432Key = false);
	static BOOL SetRegValue(HKEY hKey,LPCTSTR lpszKeyName, LPCTSTR lpszValueName,LPCTSTR lpValue);
	static BOOL SetRegValue(HKEY hKey,LPCTSTR lpszKeyName, LPCTSTR lpszValueName,DWORD dwValue);

	static BOOL Base64Encode(const BYTE* pbSrcData, int nSrcLen, _bstr_t& strEncodedData);
	static BOOL Base64Decode(const _bstr_t& strSrcText, LPBYTE* ppbDestData, int* pnDestSize);

	static BOOL LoadNetworkInformation();
	static BOOL LoadNetworkInformation(std::vector<PC_NETWORKINFO> &vecNetworkInfo);
	static BOOL GetWlanList(vector<CString> &vecWlanList);
	static BOOL LoadHardwareInformation();
	static BOOL LoadOSInformation();
	static BOOL LoadCPUName();
	static BOOL LoadUSBController();
	static BOOL LoadOSVersion();
	static BOOL LoadOSLanguage();
};

#define DEBUG_AND_LOG_PRINT GlobalFunc::DebugAndLogPrint