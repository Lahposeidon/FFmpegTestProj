#include "StdAfx.h"
#include "GlobalFunc.h"

#include <atlbase.h>
#include <atlcom.h>
#include <atlenc.h>

#include <setupapi.h>
#include <dbt.h>
#include "Iphlpapi.h"
#include "WQL.h"
#include "DiskInformation.h"
#include "Crc32Dynamic.h"
#include <wlanapi.h>
#include <Tlhelp32.h>
#include "Winsock2.h"
#include <MsHTML.h>
#include <Wincrypt.h>
#include "SAStatusLog.h"

#pragma comment(lib,"Ws2_32.lib")

#ifdef _DEBUG
	#pragma comment(lib,"comsuppwd.lib")
#else
	#pragma comment(lib,"comsuppw.lib")
#endif

#pragma  comment(lib,"setupapi.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "wlanapi.lib")

#pragma warning(disable:4251)
#pragma warning(disable:4996)

/////////////////////////////////////////////////////
#define MAX_REGVALUE 1024
#define IDS_ADAPTDESC_BLUETOOTH	_T("Bluetooth")
#define IDS_ADAPTDESC_VIRTUAL	_T("Virtual")
/////////////////////////////////////////////////////

/////////////////////////////////////////////////////
// Global Variables
CSAStatusLog G_log;
bool G_bCreateLogDirectoryFailed = false;
CString G_strCurrentRunningProcessName = _T("");
CString G_strCurrentRunningDirPath = _T("");

CString G_strIPAddress = _T("");
CString G_strMacAddress = _T("");
CString G_strCPUName = _T("");
CString G_strHardwareID = _T("");
CString G_strUSBHost = _T("");
CString G_strMotherBoard = _T("");
CString G_strOSName = _T("");
CString G_strOSLanguage = _T("");
CString G_strPhysicalRAM = _T("");
CString G_strPhysicalDiskSerialNumber = _T("");
OS_VERSION G_OSVersion = OS_ERROR;
BOOL	G_bIs64BitOS = FALSE;
BOOL G_bIsLoadNetworkInfoFirst = TRUE;
BOOL G_bIsLoadHWInfoFirst = TRUE;
BOOL G_bIsLoadOSInfoFirat = TRUE;
/////////////////////////////////////////////////////
std::map<CString,CString> G_StringResourceMap;

GlobalFunc::GlobalFunc(void)
{
	G_strCurrentRunningProcessName = _T("");
}

GlobalFunc::~GlobalFunc(void)
{
}

void GlobalFunc::SaveCurrentRunningDir() {

	TCHAR szCurrentRunningDirPath[MAX_PATH];
	::ZeroMemory(szCurrentRunningDirPath,sizeof(szCurrentRunningDirPath));
	DWORD dReadSize = GetModuleFileName(NULL, szCurrentRunningDirPath, sizeof(szCurrentRunningDirPath));
	
	G_strCurrentRunningDirPath = szCurrentRunningDirPath;
	
	int index = G_strCurrentRunningDirPath.ReverseFind(_T('\\'));
	G_strCurrentRunningDirPath = G_strCurrentRunningDirPath.Left(index);

	DEBUG_AND_LOG_PRINT(false, _T("==>Current Running Directory is %s"),G_strCurrentRunningDirPath);
}

LPCTSTR GlobalFunc::GetCurrentRunningDir() {
	return G_strCurrentRunningDirPath;
}

bool GlobalFunc::ExecuteTargetProgram(LPCTSTR lpszTargetProgramPath,LPCTSTR lpszParameter,bool bRunAdminAuth,ExecuteWaitOption WaitUntilProgramEnd,HWND hWnd,bool bUseAdminAuthInVista) {

	DEBUG_AND_LOG_PRINT(false, _T("Execute %s program begin!"),lpszTargetProgramPath);
	// Execute Predownloader 
	SHELLEXECUTEINFO sei;
	::ZeroMemory(&sei, sizeof(sei));
	sei.cbSize				= sizeof(sei);
	sei.hwnd				= hWnd;
	sei.lpFile				= lpszTargetProgramPath;
	sei.nShow				= SW_SHOW;
	sei.fMask				= SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;

	if(lpszParameter != NULL) {
		sei.lpParameters = lpszParameter;
	}

	if(bRunAdminAuth) {

		if(GlobalFunc::GetOSVersion() >= OS_WINVISTA) {
			DEBUG_AND_LOG_PRINT(false, _T("Verb is runas"));
			sei.lpVerb	= _T("runas"); 
		}
	}
	if(bUseAdminAuthInVista){
		if(GlobalFunc::IsWinVista() == TRUE){
			DEBUG_AND_LOG_PRINT(false, _T("Verb is runas"));
			sei.lpVerb	= _T("runas");
		}
	}
	if (::ShellExecuteEx(&sei) == FALSE) {
		DWORD dwError = GetLastError();

		DEBUG_AND_LOG_PRINT(false, _T("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"));
		if(dwError == ERROR_CANCELLED) { //권한상승 요청 거절
			DEBUG_AND_LOG_PRINT(false, _T("@@@@ Execute %s program Cancelled! @@@@"),lpszTargetProgramPath);
		}else{
			DEBUG_AND_LOG_PRINT(_T("@@@@ Execute %s program Failed! @@@@"),lpszTargetProgramPath);
		}
		DEBUG_AND_LOG_PRINT(false, _T("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"));
		return false;
	}

	if(WaitUntilProgramEnd == Option_WaitForInputIdle) {
		::WaitForInputIdle(sei.hProcess,INFINITE);
	}else if (WaitUntilProgramEnd == Option_WaitForSingleObject){
		::WaitForSingleObject(sei.hProcess,INFINITE);
	}else{
		//not to do
	}

	CloseHandle (sei.hProcess);

	DEBUG_AND_LOG_PRINT(false, _T("Execute %s program success!"),lpszTargetProgramPath);

	return true;
}

void GlobalFunc::KillTargetProcess(LPCTSTR lpszProcessName,bool bWaitASecond) {
	 //매개변수로 Kill 하려는 프로세스의 이름을 가져옴
	CString strProcessExeName = lpszProcessName;
	if(strProcessExeName.Right(3).CompareNoCase(_T("exe")) != 0) {
		strProcessExeName += _T(".exe");
	}

	DEBUG_AND_LOG_PRINT(false, _T("Target Process FileName to Kill is %s"),strProcessExeName);

	HANDLE hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0);
	if( hProcessSnap == INVALID_HANDLE_VALUE){
		DEBUG_AND_LOG_PRINT(false, _T("CreateToolhelp32Snapshot error!"));
		return;
	}

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	if(!Process32First(hProcessSnap, &pe32)){
		DEBUG_AND_LOG_PRINT(false, _T("Process32First error!"));
		CloseHandle(hProcessSnap);
		return;
	}

	HANDLE hProcess = NULL;
	BOOL bIsKill = FALSE;
 
	do {
		if( strProcessExeName.CompareNoCase(pe32.szExeFile) == 0) {
			hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
			// 사용자가 Kill을 원하는 프로세스의 이름 입력시
			// 같은 정보를 가진 프로세스를 찾았다면
			// OpenProcess를 이용해 핸들을 가져옵니다.
			if(hProcess != NULL){
				TerminateProcess(hProcess, -1);
				//핸들이 정상적이라면..! Kill

				DEBUG_AND_LOG_PRINT(false, _T("%s Process Terminated"),lpszProcessName);
				bIsKill = TRUE;
				CloseHandle(hProcess);

				if(bWaitASecond) {
					::Sleep(3000);
				}
//				break;
			}
		}
	} while( Process32Next(hProcessSnap, &pe32) );

	CloseHandle(hProcessSnap);
	if(!bIsKill) {
		DEBUG_AND_LOG_PRINT(false, _T("There's no other %s process"),lpszProcessName);
	}
}

void GlobalFunc::SetCurrentRunningProcessName(LPCTSTR lpszProcessName) {
	G_strCurrentRunningProcessName = lpszProcessName;

	InitializeDebugAndLogPrint(G_strCurrentRunningProcessName);
}

LPCTSTR GlobalFunc::GetCurrentRunningProcessName() {
	return G_strCurrentRunningProcessName.IsEmpty()?_T("Unknown"):G_strCurrentRunningProcessName;
}

#define BUF_SIZE	1024*24

CString GlobalFunc::DebugAndLogPrint(bool bSaveFile, LPCTSTR fmt,...) {
	TCHAR szBuffer[BUF_SIZE];

	try {

		if(::IsBadStringPtr (G_strCurrentRunningProcessName,_tcslen(G_strCurrentRunningProcessName))) {
			return CString();
		}

		va_list argptr;
		va_start(argptr, fmt);
		_vsntprintf(szBuffer, BUF_SIZE, fmt, argptr);
		va_end(argptr);

		CString strDebugMsg;
		strDebugMsg.Format (_T("(%s) : %s\n"),
			G_strCurrentRunningProcessName.IsEmpty()?_T("Unknown"):G_strCurrentRunningProcessName,
			szBuffer);

		if(!G_bCreateLogDirectoryFailed && bSaveFile == true) {
			G_log.StatusOut(strDebugMsg.GetBuffer());
		}
		OutputDebugString(strDebugMsg);

		return strDebugMsg;
	}catch(...){
		return ShowErrorMessage(GetLastError());;
	}	
}

LPCTSTR GlobalFunc::ShowErrorMessage(DWORD dwErrorCode) {
	
	HLOCAL hLocal = NULL;
	DWORD systemLocale = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

	DWORD dwReturn = ::FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
										NULL,
										dwErrorCode,
										systemLocale,
										(PTSTR)&hLocal,
										0,
										NULL);

	if(dwReturn == 0) {
		HMODULE hDll = ::LoadLibraryEx(_T("netmsg.dll"),NULL,DONT_RESOLVE_DLL_REFERENCES);

		if(hDll != NULL) {
			::FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
							NULL,
							dwErrorCode,
							systemLocale,
							(LPTSTR)&hLocal,
							0,
							NULL);

			::FreeLibrary(hDll);
		}
	}

	static CString strErrorMessage = _T("");
	strErrorMessage = _T("");

	if(hLocal != NULL) {
		
		strErrorMessage = (LPCTSTR)LocalLock(hLocal);
		LocalFree(hLocal);
	}else{
		strErrorMessage = _T("Error Message not fount!");
	}
			
	return strErrorMessage;
}

LPCTSTR GlobalFunc::GetCommonDataFolder() {
	TCHAR szPathBuffer[MAX_PATH];
	::SHGetSpecialFolderPath(NULL,szPathBuffer, CSIDL_COMMON_DOCUMENTS, FALSE);
	
	static CString strCommonDocDir=_T("");
	strCommonDocDir = _T("");

	strCommonDocDir.Format(_T("%s"), szPathBuffer );

	return strCommonDocDir;
}

LPCTSTR GlobalFunc::GenerateTempFileFullPath() {
	UINT uRetVal;

	TCHAR szTempFileFullPath[MAX_PATH];
	
	uRetVal = GetTempFileName(GlobalFunc::GetUserTempFolder(),_T("tmp"),0,szTempFileFullPath);

	static CString strTempFileFullPath = _T("");

	strTempFileFullPath = _T("");
	if(uRetVal == 0) {
		return strTempFileFullPath;
	}

	strTempFileFullPath = szTempFileFullPath;
	return strTempFileFullPath;
}

LPCTSTR GlobalFunc::GetUserTempFolder() {
	DWORD dwRetVal;
	DWORD dwBufSize=MAX_PATH;

	TCHAR lpPathBuffer[MAX_PATH];

	static CString strUserTempFolder = _T("");

	strUserTempFolder = _T("");
	 // Get the temp path.
    dwRetVal = GetTempPath(dwBufSize,     // length of the buffer
                           lpPathBuffer); // buffer for path 
    if (dwRetVal > dwBufSize || (dwRetVal == 0))
    {
		return strUserTempFolder;
    }

	TCHAR szLongPathName[MAX_PATH];
	if(GetLongPathName(lpPathBuffer,szLongPathName,MAX_PATH) != 0) {
		strUserTempFolder = szLongPathName;
	}else{
		strUserTempFolder = lpPathBuffer;
	}

	DEBUG_AND_LOG_PRINT(false, _T("@@@ GetUserTempFolder [%s]"),strUserTempFolder);
	return strUserTempFolder;
}

LPCTSTR GlobalFunc::GetAppDataFolder() {

	TCHAR szPathBuffer[MAX_PATH];
	::SHGetSpecialFolderPath(NULL,szPathBuffer, CSIDL_APPDATA, FALSE);
	
	static CString strAppDataFolder=_T("");
	strAppDataFolder = _T("");

	strAppDataFolder.Format(_T("%s"), szPathBuffer );

	return strAppDataFolder;
}

LPCTSTR GlobalFunc::ReadCryptReg(LPCTSTR lpszKeyName, 
								 LPCTSTR lpszValueName,
								 HKEY hKey,
								 bool bCheck64Key,
								 bool bUse6432Key)
{
	static _tstring strReturnValue = _T("");
	strReturnValue = _T("");

	CString strCryptRegValue = _T("");

	_tstring strKeyName = lpszKeyName;
	_tstring strValueName = lpszValueName;

	GetRegValue(hKey,strKeyName.c_str(), strValueName.c_str(), strCryptRegValue,bCheck64Key,bUse6432Key);

	if(!strCryptRegValue.IsEmpty())
	{
		strReturnValue = GlobalFunc::DecodingFromBase64(strCryptRegValue);
		
	}

	return strReturnValue.c_str();	
}

BOOL GlobalFunc::WriteCryptReg(LPCTSTR lpszKeyName, 
							   LPCTSTR lpszValueName, 
							   LPCTSTR lpValue,
							   HKEY hKey)
{
	
	CString strValue = _T("");
	strValue = lpValue;

	CString strEncryptValue = GlobalFunc::EncodingToBase64(strValue);
	// 레지스트리에서 값 쓰기
	_tstring strKeyName = lpszKeyName;
	_tstring strValueName = lpszValueName;
	BOOL bReturn = SetRegValue(hKey, strKeyName.c_str(), strValueName.c_str(),strEncryptValue);


	return bReturn;
}

LPCTSTR GlobalFunc::ReadStringValue(LPCTSTR lpszKeyName, 
									LPCTSTR lpszValueName,
									LPCTSTR lpszDefaultValue,
									HKEY hKey,
									bool bCheck64Key,
									bool bUse6432Key)
{
	static _tstring strReturnValue = _T("");
	strReturnValue = _T("");

	_tstring strKeyName = lpszKeyName;
	_tstring strValueName = lpszValueName;

	CString strRegValue = _T("");
	if(GetRegValue(hKey,strKeyName.c_str(), strValueName.c_str(),strRegValue,bCheck64Key,bUse6432Key)) {
		strReturnValue = strRegValue.GetString();

		return strReturnValue.c_str();
	}

	strReturnValue = lpszDefaultValue;
	return strReturnValue.c_str();
}


DWORD GlobalFunc::ReadDWORDValue(LPCTSTR lpszKeyName, 
								 LPCTSTR lpszValueName,
								 DWORD dwDefaultValue,
								 HKEY hKey,
								 bool bCheck64Key,
								 bool bUse6432Key) {
	DWORD dwValue = dwDefaultValue;

	_tstring strKeyName = lpszKeyName;
	_tstring strValueName = lpszValueName;
	GetRegValue(hKey,strKeyName.c_str(),strValueName.c_str(),dwValue,bCheck64Key,bUse6432Key);

	return dwValue;
}

BOOL GlobalFunc::WriteStringValue(LPCTSTR lpszKeyName, 
								  LPCTSTR lpszValueName, 
								  LPCTSTR lpValue,
								  HKEY hKey)
{
	_tstring strKeyName = lpszKeyName;
	_tstring strValueName = lpszValueName;
	return SetRegValue(hKey,strKeyName.c_str(), strValueName.c_str(),lpValue);
}

BOOL GlobalFunc::WriteDWORDValue(LPCTSTR lpszKeyName, 
								 LPCTSTR lpszValueName,
								 DWORD dwValue,
								 HKEY hKey) {

	_tstring strKeyName = lpszKeyName;
	_tstring strValueName = lpszValueName;
	return SetRegValue(hKey,strKeyName.c_str(),strValueName.c_str(),dwValue);
}

void GlobalFunc::DeleteRegKey(LPCTSTR lpszSubKeyName, 
							  LPCTSTR lpszKeyName,
							  HKEY hKey)
{
	CRegKey regKey;

	if (ERROR_SUCCESS == regKey.Open(hKey, lpszSubKeyName))
	{
		regKey.RecurseDeleteKey(lpszKeyName);
		regKey.Close();
	}       
}

BOOL GlobalFunc::DeleteRegValue(LPCTSTR lpszKeyName, 
								LPCTSTR lpszValueName,
								HKEY hKey) {
	HKEY hkResult;
	DWORD dwDisp;

	if(CString(lpszKeyName).IsEmpty())
	{
		return FALSE;
	}

	if(RegCreateKeyEx(hKey, lpszKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hkResult, &dwDisp)!= ERROR_SUCCESS)
	{
		return FALSE;
	}

	if(RegDeleteValue(hkResult,lpszValueName) != ERROR_SUCCESS) {

		return FALSE;
	}
	RegCloseKey(hkResult);

	return TRUE;
}

void GlobalFunc::GetSubKeyNames(LPCTSTR lpszKeyName,
								std::list<CString>& subKeyNames,
								HKEY hStartKey,
								bool bCheck64Key,
								bool bUse6432Key) {
	HKEY hKey;
	TCHAR szSubKeyName[MAX_PATH];
	DWORD dwSubKeyLength;
	DWORD dwReturn;

	if(!subKeyNames.empty()) {
		subKeyNames.clear();
	}

	//DEBUG_AND_LOG_PRINT(_T("GetSubKeyNames Start KeyName=%s"),lpszKeyName);
	DWORD dwRegOption = KEY_READ;
	if(bCheck64Key) {
		if(Is64BitOS() == TRUE) {
			if(!bUse6432Key) {
				dwRegOption = KEY_READ | KEY_WOW64_64KEY;
			}else{
				dwRegOption = KEY_READ | KEY_WOW64_32KEY;
			}
		}
	}

	if(lpszKeyName != NULL && _tcslen(lpszKeyName)) {
		dwReturn = RegOpenKeyEx (hStartKey,lpszKeyName,0,dwRegOption,&hKey);

		if(dwReturn == ERROR_SUCCESS) {
			int nIndex = 0;
			while(dwReturn == ERROR_SUCCESS) {
				dwSubKeyLength = MAX_PATH;
				::ZeroMemory(szSubKeyName,sizeof(szSubKeyName));
				dwReturn = RegEnumKeyEx(hKey,nIndex++,szSubKeyName,&dwSubKeyLength,NULL,NULL,NULL,NULL);

				if(dwReturn == ERROR_SUCCESS) {
					subKeyNames.push_back(CString(szSubKeyName));
				}else if(dwReturn == ERROR_NO_MORE_ITEMS) {
					break;
				}
			}

			RegCloseKey(hKey);
		}
	}

	//DEBUG_AND_LOG_PRINT(_T("GetSubKeyNames Start KeyName=%s End"),lpszKeyName);

}

void GlobalFunc::GetAllRegValueNames(LPCTSTR lpszKeyName,
									 std::list<CString>& ValuesNames,
									 HKEY hStartKey,
									 bool bCheck64Key,
									 bool bUse6432Key) {

	HKEY hKey;
	TCHAR szValueName[MAX_PATH];
	DWORD dwValueNameLength;
	DWORD dwReturn;

	if(!ValuesNames.empty()) {
		ValuesNames.clear();
	}

	DEBUG_AND_LOG_PRINT(_T("GetAllRegValueNames Start KeyName=%s"),lpszKeyName);

	DWORD dwRegOption = KEY_READ;
	if(bCheck64Key) {
		if(Is64BitOS() == TRUE) {
			if(!bUse6432Key) {
				dwRegOption = KEY_READ | KEY_WOW64_64KEY;
			}else{
				dwRegOption = KEY_READ | KEY_WOW64_32KEY;
			}
		}
	}

	if(lpszKeyName != NULL && _tcslen(lpszKeyName)) {
		dwReturn = RegOpenKeyEx (hStartKey,lpszKeyName,0,dwRegOption,&hKey);

		if(dwReturn == ERROR_SUCCESS) {
			DWORD dwIndex = 0;
			DWORD dwType = REG_SZ;
			while(dwReturn == ERROR_SUCCESS) {
				dwValueNameLength = MAX_PATH;
				::ZeroMemory(szValueName,sizeof(szValueName));
				dwReturn = RegEnumValue(hKey,dwIndex++,szValueName,&dwValueNameLength,NULL,&dwType,NULL,NULL);
				
				if(dwReturn == ERROR_SUCCESS) {
					ValuesNames.push_back(CString(szValueName));
				}else if(dwReturn == ERROR_NO_MORE_ITEMS) {
					break;
				}
			}

			RegCloseKey(hKey);
		}
	}
}

LPSTR GlobalFunc::Unicode2UTF8 (LPCWSTR szStr,int *cchAnsiLength)
{
	// The unicode encodings are not allowed
	if ( szStr == NULL )
		return NULL ;

	int		cchNeeded ;

	if ( 0 == (cchNeeded = ::WideCharToMultiByte (CP_UTF8, 0, szStr, -1, NULL, 0, NULL, NULL)) )
		return "" ;

	if(cchAnsiLength != NULL) {
		*cchAnsiLength = cchNeeded;
	}

	PSTR		szAnsi = (PSTR) ::malloc (sizeof (CHAR) * cchNeeded) ;
	if ( szAnsi == NULL )
		return "" ;

	if ( 0 == ::WideCharToMultiByte (CP_UTF8, 0, szStr, -1, szAnsi, cchNeeded, NULL, NULL) ) {
		::free (szAnsi);
		return "" ;
	}

	return szAnsi ;
}


LPWSTR GlobalFunc::UTF8ToUnicode(LPSTR szStr,int *cchUnicode) {
	
	if ( szStr == NULL )
		return NULL ;

	int		cchNeeded ;

	if ( 0 == (cchNeeded = ::MultiByteToWideChar (CP_UTF8, 0, szStr, -1, NULL, 0)))
		return L"" ;

	if(cchUnicode != NULL) {
		*cchUnicode = cchNeeded;
	}

	PWSTR		szUnicode = (PWSTR) ::malloc (sizeof (WCHAR) * cchNeeded) ;
	if ( szUnicode == NULL )
		return L"" ;

	if ( 0 == ::MultiByteToWideChar (CP_UTF8, 0, szStr, -1,szUnicode, cchNeeded)) {
		::free (szUnicode);
		return L"" ;
	}

	return szUnicode ;
}

LPCTSTR GlobalFunc::DecodingFromBase64(LPCTSTR lpszTarget) {
	LPBYTE	lpBuf = NULL;	
	DWORD	dwSize = 0;	
	_bstr_t _bstrTarget;

	
	static CString strTarget = _T("");
	strTarget = _T("");
//	strTarget = lpszTarget;

//	if(FALSE == strTarget.IsEmpty())
	if(!CString(lpszTarget).IsEmpty())
	{
		_bstrTarget = lpszTarget;
		if(TRUE == Base64Decode(_bstrTarget, &lpBuf, (int*)&dwSize))
		{
			strTarget.Format(_T("%s"), (LPCTSTR)lpBuf);
			CoTaskMemFree(lpBuf);
			lpBuf = NULL;
		}
	}

	return strTarget;	
}

LPCTSTR GlobalFunc::EncodingToBase64(LPCTSTR lpszTarget) {
	static CString strTarget = _T("");
	strTarget = _T("");

	BOOL bReturn = TRUE;
	int nLength = 0;
	_bstr_t _bstrTarget;

//	strTarget = lpszTarget;

	nLength = CString(lpszTarget).GetLength() * sizeof(TCHAR);
	
	if(Base64Encode((const BYTE *)lpszTarget, nLength, _bstrTarget))
	{
		strTarget = _bstrTarget.copy();
	}

	return strTarget;
}


void GlobalFunc::DeleteTargetFile(LPCTSTR lpszFilePath) {

	try {
		if(!GlobalFunc::FileExist(lpszFilePath)) {
			return;
		}
		::DeleteFile(lpszFilePath);
	}catch(CFileException *e) {

		TCHAR szCause[MAX_PATH];
		e->GetErrorMessage(szCause,MAX_PATH);

		DEBUG_AND_LOG_PRINT(false, _T("File Exception in GlobalFunc::DeleteTargetFile : m_cause=%d, ErrorMessage=%s"),e->m_cause,szCause);

	}
}

bool GlobalFunc::CreateDirectoryTree(LPCTSTR lpszFolderPath ) {
	CString strAllFolderPath = lpszFolderPath;

	DEBUG_AND_LOG_PRINT(_T("CreateDirectoryTree: %s"),strAllFolderPath);
	
	std::vector<CString> vecDirNames;
	CString strToken;
	int iCurPos = 0;
	
	strToken = strAllFolderPath.Tokenize(_T("\\"),iCurPos);
	while(!strToken.IsEmpty()) {
		vecDirNames.push_back(strToken);
		strToken = strAllFolderPath.Tokenize(_T("\\"),iCurPos);
	}

	CString strCurrentFolderPath = _T("");
	std::vector<CString>::iterator iter = vecDirNames.begin();
	while(iter != vecDirNames.end()) {
		strCurrentFolderPath += *iter;

		if(iter != vecDirNames.begin()) {
			if(!GlobalFunc::FileExist(strCurrentFolderPath)) {
				if(!::CreateDirectory(strCurrentFolderPath,NULL)) {
					DEBUG_AND_LOG_PRINT(_T("Create directory %s failed"),strCurrentFolderPath);
					return false;
				}
			}
		}

		strCurrentFolderPath += _T("\\");

		++iter;
	}

	DEBUG_AND_LOG_PRINT(false, _T("Create Directory %s success"),strCurrentFolderPath);
	return true;
}

bool GlobalFunc::FileExist(LPCTSTR lpszFilePath) {
	CFileFind finder;

	CString strFilePath = lpszFilePath;
	if(strFilePath.Right(1).CompareNoCase(_T("\\")) == 0) {
		strFilePath = strFilePath.Left(strFilePath.GetLength()-1);
	}

	DWORD ret = GetFileAttributes( strFilePath );

	if( ret != INVALID_FILE_ATTRIBUTES ) {
	   return true;
	}

	DEBUG_AND_LOG_PRINT(false, _T("### FileExist: %s file is not exist ###"),strFilePath);
	return false;
}

ULONGLONG GlobalFunc::FileSize(LPCTSTR lpszFilePath) {
	CFileFind finder;

	if(finder.FindFile(lpszFilePath)) {
		finder.FindNextFile();

		ULONGLONG ulFileLength = finder.GetLength();
		return ulFileLength;
	}

	return 0;
}

void GlobalFunc::DeleteDirectoryTree(LPCTSTR lpszTargetPath) {
	if(CString(lpszTargetPath).IsEmpty()) {
		DEBUG_AND_LOG_PRINT(false, _T("DeleteDirectoryTree Targetpath null."));
		return;
	}

	if(!GlobalFunc::FileExist(lpszTargetPath)) {
		DEBUG_AND_LOG_PRINT(false, _T("DeleteDirectoryTree Targetpath is not exist. (%s)"),lpszTargetPath);
		return;
	}

	CString strSearchFilePath;
	strSearchFilePath.Format (_T("%s\\*"),lpszTargetPath);

	WIN32_FIND_DATA fd;
	HANDLE hFind;
	if((hFind = ::FindFirstFile (strSearchFilePath,&fd)) != INVALID_HANDLE_VALUE) {

		do {
			if((_tcsicmp(fd.cFileName,_T(".")) != 0) && 
				(_tcsicmp(fd.cFileName,_T("..")) != 0) &&
				((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)) {

					CString strTargetFile;
					strTargetFile.Format(_T("%s\\%s"),lpszTargetPath,fd.cFileName);

					if((fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ||
						(fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)) {
						SetFileAttributes(strTargetFile,FILE_ATTRIBUTE_NORMAL);
					}

					::DeleteFile(strTargetFile);

					DEBUG_AND_LOG_PRINT(false, _T("Delete File Path=%s"),strTargetFile);

			}else if((_tcsicmp(fd.cFileName,_T(".")) != 0) && 
				(_tcsicmp(fd.cFileName,_T("..")) != 0) &&
				(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
				
				CString strTargetDir;
				strTargetDir.Format (_T("%s\\%s"),lpszTargetPath,fd.cFileName);

				GlobalFunc::DeleteDirectoryTree(strTargetDir);

				if((fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ||
					(fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)) {
					SetFileAttributes(strTargetDir,FILE_ATTRIBUTE_NORMAL);
				}
				RemoveDirectory(strTargetDir);
			}
		}while(::FindNextFile(hFind,&fd));
		FindClose(hFind);

		SetFileAttributes(lpszTargetPath,FILE_ATTRIBUTE_NORMAL);
		RemoveDirectory(lpszTargetPath);
	}
}

bool GlobalFunc::CopyDirectoryTree(LPCTSTR lpszOriginalPath,LPCTSTR lpszTargetPath) {

	//해당 모델의 디렉토리가 있을 때 자신이외의 파일이 있으면 모두 삭제(서버는 최신을 내려주기 때문에 자신이외의 파일은 예전 버전의 바이너리)
	if(!GlobalFunc::FileExist(lpszTargetPath)) {
		//디렉토리가 없으면 디렉토리 생성

		if(!GlobalFunc::CreateDirectoryTree(lpszTargetPath)) {
			DEBUG_AND_LOG_PRINT(_T("Create Directory failed: %s"),lpszTargetPath);
			return false;
		}
	}


	CString strSearchFilePath;
	strSearchFilePath.Format (_T("%s\\*"),lpszOriginalPath);

	WIN32_FIND_DATA fd;
	HANDLE hFind;
	if((hFind = ::FindFirstFile (strSearchFilePath,&fd)) != INVALID_HANDLE_VALUE) {

		do {
			if((_tcsicmp(fd.cFileName,_T(".")) != 0) && 
				(_tcsicmp(fd.cFileName,_T("..")) != 0) &&
				((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)) {

					CString strOriginalFile;
					strOriginalFile.Format(_T("%s\\%s"),lpszOriginalPath,fd.cFileName);

					CString strTargetFile;
					strTargetFile.Format(_T("%s\\%s"),lpszTargetPath,fd.cFileName);

					if(!::CopyFile(strOriginalFile,strTargetFile,FALSE)) {
						DEBUG_AND_LOG_PRINT(false, _T("Copy File failed! Original Path=%s,Target Path=%s"),strOriginalFile,strTargetFile);
						return false;
					}

					DEBUG_AND_LOG_PRINT(_T("Copy File Original Path=%s,Target Path=%s"),strOriginalFile,strTargetFile);

			}else if((_tcsicmp(fd.cFileName,_T(".")) != 0) && 
				(_tcsicmp(fd.cFileName,_T("..")) != 0) &&
				(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
				
				CString strOrginalDir;
				strOrginalDir.Format (_T("%s\\%s"),lpszOriginalPath,fd.cFileName);

				CString strTargetDir;
				strTargetDir.Format (_T("%s\\%s"),lpszTargetPath,fd.cFileName);

				if(!GlobalFunc::CopyDirectoryTree(strOrginalDir,strTargetDir)) {
					DEBUG_AND_LOG_PRINT(_T("DirectoryCopy failed! Original Path=%s,Target Path=%s"),strOrginalDir,strTargetDir);
					return false;
				}
			}
		}while(::FindNextFile(hFind,&fd));

		FindClose(hFind);
	}

	return true;
}

void GlobalFunc::SearchFilesInDirectory(LPCTSTR lpszFolderPath,std::list<CString>& SearchedFilesList) {
	
	if(!SearchedFilesList.empty()) {
		SearchedFilesList.clear();
	}

	if(!GlobalFunc::FileExist(lpszFolderPath)) {
		DEBUG_AND_LOG_PRINT(false, _T("SearchFilesInDirectory Target Folder is not exist. (%s)"),lpszFolderPath);
		return;
	}

	CString strSearchFilePath;
	strSearchFilePath.Format (_T("%s\\*"),lpszFolderPath);

	WIN32_FIND_DATA fd;
	HANDLE hFind;
	if((hFind = ::FindFirstFile (strSearchFilePath,&fd)) != INVALID_HANDLE_VALUE) {

		do {
			if((_tcsicmp(fd.cFileName,_T(".")) != 0) && 
				(_tcsicmp(fd.cFileName,_T("..")) != 0) &&
				((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)) {

					CString strTargetFile;
					strTargetFile.Format(_T("%s\\%s"),lpszFolderPath,fd.cFileName);

					SearchedFilesList.push_back(strTargetFile);
	
					DEBUG_AND_LOG_PRINT(false, _T("Searched File is %s"),strTargetFile);

			}else if((_tcsicmp(fd.cFileName,_T(".")) != 0) && 
				(_tcsicmp(fd.cFileName,_T("..")) != 0) &&
				(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
				
				CString strTargetDir;
				strTargetDir.Format (_T("%s\\%s"),lpszFolderPath,fd.cFileName);

				GlobalFunc::SearchFilesInDirectory(strTargetDir,SearchedFilesList);

			}
		}while(::FindNextFile(hFind,&fd));
		FindClose(hFind);
	}
}

// Hardware Info
LPCTSTR GlobalFunc::GetLocalComputerName() {
	TCHAR szComputerName[MAX_PATH];

	::ZeroMemory(szComputerName,sizeof(szComputerName));

	DWORD dwComputerNameLength = sizeof(szComputerName)/sizeof(TCHAR);

	if(!GetComputerName(szComputerName,&dwComputerNameLength)) {
		DEBUG_AND_LOG_PRINT(false, _T("GetLocalComputerName Failed"));

		return _T("");
	}

	static CString strLocalComputerName = _T("");
	strLocalComputerName = szComputerName;

	DEBUG_AND_LOG_PRINT(_T("My Computer Name is %s"),strLocalComputerName);
	return strLocalComputerName;
}

LPCTSTR GlobalFunc::GetMacAddress() {
	
	if(G_bIsLoadNetworkInfoFirst) {
		G_bIsLoadNetworkInfoFirst = FALSE;

		//LoadNetworkInformation();

		vector<PC_NETWORKINFO> vecNetworkInfo;
		vector<PC_NETWORKINFO>::iterator IterNetworkInfo;
	
		LoadNetworkInformation(vecNetworkInfo);
		
		for(IterNetworkInfo = vecNetworkInfo.begin();
			IterNetworkInfo != vecNetworkInfo.end();
			IterNetworkInfo++)
		{
			if(IterNetworkInfo->eNetworkType == PHYSICAL_LAN)
			{	
				G_strIPAddress = IterNetworkInfo->szIPAddress;
				G_strMacAddress = IterNetworkInfo->szMACAddress;

				break;
			}

			G_strIPAddress = IterNetworkInfo->szIPAddress;
			G_strMacAddress = IterNetworkInfo->szMACAddress;
		}
	}

	return G_strMacAddress;
}

LPCTSTR GlobalFunc::GetIPAddress() {

	if(G_bIsLoadNetworkInfoFirst) {
		G_bIsLoadNetworkInfoFirst = FALSE;

		//LoadNetworkInformation();

		vector<PC_NETWORKINFO> vecNetworkInfo;
		vector<PC_NETWORKINFO>::iterator IterNetworkInfo;

		LoadNetworkInformation(vecNetworkInfo);

		for(IterNetworkInfo = vecNetworkInfo.begin();
			IterNetworkInfo != vecNetworkInfo.end();
			IterNetworkInfo++)
		{
			if(IterNetworkInfo->eNetworkType == PHYSICAL_LAN)
			{	
				G_strIPAddress = IterNetworkInfo->szIPAddress;
				G_strMacAddress = IterNetworkInfo->szMACAddress;

				break;
			}

			G_strIPAddress = IterNetworkInfo->szIPAddress;
			G_strMacAddress = IterNetworkInfo->szMACAddress;
		}

	}

	return G_strIPAddress;
}

LPCTSTR GlobalFunc::GetCPUName() {
	if(G_bIsLoadHWInfoFirst) {
		G_bIsLoadHWInfoFirst = FALSE;

		LoadHardwareInformation();
	}

	return G_strCPUName;
}

LPCTSTR GlobalFunc::GetUSBHostName() {
	if(G_bIsLoadHWInfoFirst) {
		G_bIsLoadHWInfoFirst = FALSE;

		LoadHardwareInformation();
	}

	return G_strUSBHost;
}

LPCTSTR GlobalFunc::GetUSBHostHWID() {
	if(G_bIsLoadHWInfoFirst) {
		G_bIsLoadHWInfoFirst = FALSE;

		LoadHardwareInformation();
	}

	return G_strHardwareID;
}

LPCTSTR GlobalFunc::GetOSName() {
	if(G_bIsLoadOSInfoFirat) {
		G_bIsLoadOSInfoFirat = FALSE;

		LoadOSInformation();
	}

	return G_strOSName;
}

OS_VERSION GlobalFunc::GetOSVersion() {
	if(G_bIsLoadOSInfoFirat) {
		G_bIsLoadOSInfoFirat = FALSE;

		LoadOSInformation();
	}

	return G_OSVersion;
}

BOOL GlobalFunc::Is64BitOS() {
	if(G_bIsLoadOSInfoFirat) {
		G_bIsLoadOSInfoFirat = FALSE;

		LoadOSInformation();
	}

	return G_bIs64BitOS;
}

LPCTSTR GlobalFunc::GetOSLanguage() {
	if(G_bIsLoadOSInfoFirat) {
		G_bIsLoadOSInfoFirat = FALSE;

		LoadOSInformation();
	}

	return G_strOSLanguage;
}

LPCTSTR GlobalFunc::GetMotherboardInfo() {
	if(G_bIsLoadHWInfoFirst) {
		G_bIsLoadHWInfoFirst = FALSE;

		LoadHardwareInformation();
	}

	return G_strMotherBoard;
}

LPCTSTR GlobalFunc::GetPhysicalRAMSize() {
	if(G_bIsLoadHWInfoFirst) {
		G_bIsLoadHWInfoFirst = FALSE;

		LoadHardwareInformation();
	}

	return G_strPhysicalRAM;
}

LPCTSTR GlobalFunc::GetPhysicalDiskSerialNumber() {
	if(G_bIsLoadHWInfoFirst) {
		G_bIsLoadHWInfoFirst = FALSE;

		LoadHardwareInformation();
	}

	return G_strPhysicalDiskSerialNumber;
}

long GlobalFunc::GetFreeDiskSpace() {
	ULARGE_INTEGER uliFreeBytesAvailableToCaller;
    ULARGE_INTEGER uliTotalNumberOfBytes;
    ULARGE_INTEGER uliTotalNumberOfFreeBytes;

	CString strDrvName;
	long lFreeDiskSpace = -1;

	// User의 Temp 폴더의 위치를 이용하여 사용하는 디스크의 남은 용량 확인
	if(::GetDiskFreeSpaceEx(GetUserTempFolder(),
                            &uliFreeBytesAvailableToCaller,
                            &uliTotalNumberOfBytes,
							&uliTotalNumberOfFreeBytes) == TRUE) {
      lFreeDiskSpace = static_cast<long>(static_cast<__int64>(uliFreeBytesAvailableToCaller.QuadPart/(1024 * 1024)));
	}

	return lFreeDiskSpace;
}

//////////////////////////////////////////////////////////////////////////////////////
// Private Method
BOOL GlobalFunc::GetRegValue(HKEY hKey, 
							 LPCTSTR lpszKeyName, 
							 LPCTSTR lpszValueName,
							 CString& strValue,
							 bool bCheck64Key,
							 bool bUse6432Key)
{
	HKEY hkResult;
	DWORD dwDisp;

	if(CString(lpszKeyName).IsEmpty())
	{
		return FALSE;
	}

	DWORD dwRegOption = KEY_READ;
	if(bCheck64Key) {
		if(Is64BitOS() == TRUE) {
			if(!bUse6432Key) {
				dwRegOption = KEY_READ | KEY_WOW64_64KEY;
			}else{
				dwRegOption = KEY_READ | KEY_WOW64_32KEY;
			}
		}
	}

	if(RegCreateKeyEx(hKey, lpszKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, dwRegOption, NULL, &hkResult, &dwDisp)!= ERROR_SUCCESS)
	{
		if(RegCreateKeyEx(HKEY_LOCAL_MACHINE, lpszKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, dwRegOption, NULL, &hkResult, &dwDisp)!= ERROR_SUCCESS)
		{
			return FALSE;
		}
	}

	// 12.10.10 : MAX_PATH 260 에서 1024로 늘림. 
	// XP서비스팩2 미리받기 Path 암호화된 길이가 280넘어서 미리받기 기능 안되었음
	DWORD dwType = REG_SZ;
	TCHAR szValue[MAX_REGVALUE];  
	DWORD dwBufferLength = MAX_REGVALUE * sizeof(TCHAR);

	if(RegQueryValueEx(hkResult, lpszValueName, NULL,&dwType,(LPBYTE)szValue,&dwBufferLength) != ERROR_SUCCESS)
	{
		RegCloseKey(hkResult);

		if(RegCreateKeyEx(HKEY_LOCAL_MACHINE, lpszKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, dwRegOption, NULL, &hkResult, &dwDisp)!= ERROR_SUCCESS)
		{
			return FALSE;
		}

		if(RegQueryValueEx(hkResult, lpszValueName, NULL,&dwType,(LPBYTE)szValue,&dwBufferLength) != ERROR_SUCCESS)
		{
			return FALSE;
		}
	}

//	strValue = szValue;
	strValue.Format (_T("%s"),szValue);
	RegCloseKey(hkResult);
	return TRUE;
}


BOOL GlobalFunc::GetRegValue(HKEY hKey,
							 LPCTSTR lpszKeyName, 
							 LPCTSTR lpszValueName,
							 DWORD& dwValue,
							 bool bCheck64Key,
							 bool bUse6432Key) {

	HKEY hkResult;
	DWORD dwDisp;

	if(CString(lpszKeyName).IsEmpty())
	{
		return FALSE;
	}

	DWORD dwRegOption = KEY_READ;
	if(bCheck64Key) {
		if(Is64BitOS() == TRUE) {
			if(!bUse6432Key) {
				dwRegOption = KEY_READ | KEY_WOW64_64KEY;
			}else{
				dwRegOption = KEY_READ | KEY_WOW64_32KEY;
			}
		}
	}
	if(RegCreateKeyEx(hKey, lpszKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, dwRegOption, NULL, &hkResult, &dwDisp)!= ERROR_SUCCESS)
	{
		if(RegCreateKeyEx(HKEY_LOCAL_MACHINE, lpszKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, dwRegOption, NULL, &hkResult, &dwDisp)!= ERROR_SUCCESS)
		{
			return FALSE;
		}
	}

	DWORD dwType = REG_DWORD;
	DWORD dwCount = sizeof(DWORD);
	if(RegQueryValueEx(hkResult, lpszValueName, NULL,&dwType,(LPBYTE)&dwValue,&dwCount) != ERROR_SUCCESS)
	{
		RegCloseKey(hkResult);

		if(RegCreateKeyEx(HKEY_LOCAL_MACHINE, lpszKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, dwRegOption, NULL, &hkResult, &dwDisp)!= ERROR_SUCCESS)
		{
			return FALSE;
		}

		if(RegQueryValueEx(hkResult, lpszValueName, NULL,&dwType,(LPBYTE)&dwValue,&dwCount) != ERROR_SUCCESS)
		{
			return FALSE;
		}
	}

	RegCloseKey(hkResult);
	return TRUE;
}


BOOL GlobalFunc::SetRegValue(HKEY hKey,
							 LPCTSTR lpszKeyName, 
							 LPCTSTR lpszValueName, 
							 LPCTSTR lpValue)
{
	HKEY hkResult;
	DWORD dwDisp;

	if(CString(lpszKeyName).IsEmpty())
	{
		return false;
	}

	if(RegCreateKeyEx(hKey, lpszKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkResult, &dwDisp)!= ERROR_SUCCESS)
	{
		return FALSE;
	}
	if(RegSetValueEx(hkResult, lpszValueName, 0, REG_SZ, (LPBYTE)lpValue, sizeof(TCHAR)*(lstrlen(lpValue)+1)) != ERROR_SUCCESS)
	{
		return FALSE;
	}

	RegCloseKey(hkResult);
	return TRUE;
}

BOOL GlobalFunc::SetRegValue(HKEY hKey,
							 LPCTSTR lpszKeyName, 
							 LPCTSTR lpszValueName,
							 DWORD dwValue) {
	HKEY hkResult;
	DWORD dwDisp;

	if(CString(lpszKeyName).IsEmpty())
	{
		return false;
	}

	if(RegCreateKeyEx(hKey, lpszKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkResult, &dwDisp)!= ERROR_SUCCESS)
	{
		return FALSE;
	}
	if(RegSetValueEx(hkResult, lpszValueName, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD)) != ERROR_SUCCESS)
	{
		return FALSE;
	}

	RegCloseKey(hkResult);
	return TRUE;
}

void GlobalFunc::InitializeDebugAndLogPrint(LPCTSTR lpszCurrentRunningProcessName) {

	CString strLogFileDirectory;
	strLogFileDirectory.Format (_T("%s\\29m"),GlobalFunc::GetCommonDataFolder());

	COleDateTime dtDate = COleDateTime::GetCurrentTime();
	
	CString strLogFileName;
	strLogFileName.Format (_T("%s_Log(%s).txt"),lpszCurrentRunningProcessName, dtDate.Format(_T("%m-%d-%H")));

	if(!GlobalFunc::FileExist(strLogFileDirectory)) {
		
		if(!GlobalFunc::CreateDirectoryTree(strLogFileDirectory)) {
			G_bCreateLogDirectoryFailed = true;
			return ;
		}
	}

	CString strLogFileFullPath;
	strLogFileFullPath.Format (_T("%s\\%s"),strLogFileDirectory,strLogFileName);

	G_log.Init(strLogFileFullPath.GetString());
	G_log.Enable(TRUE);
	G_log.PrintTime(TRUE);
	G_log.PrintAppName(FALSE);
	G_log.SetAppName(strLogFileName.GetString());
}

BOOL GlobalFunc::Base64Encode(const BYTE* pbSrcData, int nSrcLen, _bstr_t& strEncodedData)
{
	char* pBuf = NULL;
	int nDestLen = Base64EncodeGetRequiredLength( nSrcLen, ATL_BASE64_FLAG_NOCRLF ); //(int) ((double) nSrcLen * 1.5);
	BOOL bRet = FALSE;

	nDestLen += 2;

	pBuf = (char*) CoTaskMemAlloc( nDestLen );
	ZeroMemory( pBuf, nDestLen );

	if (!ATL::Base64Encode( pbSrcData, nSrcLen, pBuf, &nDestLen, ATL_BASE64_FLAG_NOCRLF ))
	{
		nDestLen += 2;
		pBuf = (char*) CoTaskMemRealloc( pBuf, nDestLen );
		ZeroMemory( pBuf, nDestLen );

		if (ATL::Base64Encode( pbSrcData, nSrcLen, pBuf, &nDestLen, ATL_BASE64_FLAG_NOCRLF ))
		{
			strEncodedData = pBuf;
			bRet = TRUE;
		}
	}
	else
	{
		strEncodedData = pBuf;
		bRet = TRUE;
	}

	CoTaskMemFree( pBuf );

	return bRet;
}

BOOL GlobalFunc::Base64Decode(const _bstr_t& strSrcText, LPBYTE* ppbDestData, int* pnDestSize)
{
	*pnDestSize = Base64DecodeGetRequiredLength( strSrcText.length() );//strSrcText.length();
	LPBYTE lpbData = (LPBYTE) CoTaskMemAlloc( *pnDestSize );
	BOOL bRet = FALSE;

	ZeroMemory( lpbData, *pnDestSize );

	if (!ATL::Base64Decode( (LPCSTR) strSrcText, strSrcText.length(), lpbData, pnDestSize ))
	{
		lpbData = (LPBYTE) CoTaskMemRealloc( lpbData, *pnDestSize );
		ZeroMemory( lpbData, *pnDestSize );

		if (ATL::Base64Decode( (LPCSTR) strSrcText, strSrcText.length(), lpbData, pnDestSize ))
		{
			bRet = TRUE;
			*ppbDestData = lpbData;
		}
	}
	else
	{
		bRet = TRUE;
		*ppbDestData = lpbData;
	}

	if (!bRet)
	{
		CoTaskMemFree( lpbData );
		*ppbDestData = NULL;
	}

	return bRet;
}


BOOL GlobalFunc::LoadNetworkInformation() {
	DWORD dwError;

	DWORD AdapterInfoSize;
	PIP_ADAPTER_INFO pAdapterInfo,pAdapt;
	PIP_ADDR_STRING pAddrStr;

	dwError = GetAdaptersInfo(NULL, &AdapterInfoSize);
	if (dwError != ERROR_SUCCESS)
	{
		//if Err is not equal to ERROR_BUFFER_OVERFLOW//
		if (dwError != ERROR_BUFFER_OVERFLOW)     
		{
			return FALSE;
		}
	}

	pAdapterInfo = (PIP_ADAPTER_INFO)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,AdapterInfoSize);
	if(pAdapterInfo == NULL) {
		return FALSE;
	}


	// Get actual adapter information
	dwError = GetAdaptersInfo(pAdapterInfo, &AdapterInfoSize);
	if (dwError != ERROR_SUCCESS)
	{
		HeapFree (GetProcessHeap(),0,pAdapterInfo);
		return FALSE;
	}

	CString strDescription = _T("");

	BOOL bReturn = FALSE;
	BOOL bFoundAdapter = FALSE;
	pAdapt = pAdapterInfo;
	while (pAdapt)
	{
		switch (pAdapt->Type)
		{
		case MIB_IF_TYPE_ETHERNET:

			strDescription = pAdapt->Description;

			if(strDescription.CompareNoCase (_T("0.0.0.0")) != 0)
			{
				
				G_strIPAddress = _T("");
				G_strMacAddress = _T("");

				bFoundAdapter = TRUE;
				break;
			}else{

				pAdapt = pAdapt->Next;
				continue;
			}
		case MIB_IF_TYPE_TOKENRING:
		case MIB_IF_TYPE_FDDI:
		case MIB_IF_TYPE_PPP:
		case MIB_IF_TYPE_LOOPBACK:
		case MIB_IF_TYPE_SLIP:
		case MIB_IF_TYPE_OTHER:
		default:
			pAdapt = pAdapt->Next;
			continue;
		}

		if(bFoundAdapter) {

			pAddrStr = &(pAdapt->IpAddressList);

			// IP Address
			G_strIPAddress = pAddrStr->IpAddress.String;
			
			// Mac Address
			G_strMacAddress.Format(_T("%02X:%02X:%02X:%02X:%02X:%02X"),
													pAdapt->Address[0],
													pAdapt->Address[1],
													pAdapt->Address[2],
													pAdapt->Address[3],
													pAdapt->Address[4],
													pAdapt->Address[5]);

			bReturn = TRUE;
			break;
		}

		pAdapt = pAdapt->Next;
	}

	if(pAdapterInfo != NULL) {
		HeapFree (GetProcessHeap(),0,pAdapterInfo);
	}

	DEBUG_AND_LOG_PRINT (false, _T("IP Address: %s"),G_strIPAddress);
	DEBUG_AND_LOG_PRINT (false, _T("Mac Address: %s"),G_strMacAddress);

	return bReturn;
}

BOOL GlobalFunc::LoadNetworkInformation(vector<PC_NETWORKINFO> &vecNetworkInfo) 
{
	BOOL bReturn = FALSE;
	BOOL bWlan = FALSE;
	DWORD dwRetVal = 0;
	LPVOID lpMsgBuf = NULL;
	struct sockaddr_in *pAddr = NULL;
	PIP_ADAPTER_ADDRESSES pAddresses = NULL;
	PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;

	ULONG family;
	ULONG flags;
	ULONG outBufLen;
	CString strGUID;
	CString strDescription;
	CString strIPAddress;
	CString strMACAddress;
	CString strAdapterName;
	PC_NETWORKINFO stNetworkInfo;

	WCHAR GuidString[40] = {0};
	LPWSTR szAdapterName = NULL;

	vector<CString> vecWlanList;
	vector<CString>::iterator IterWlan;
	vecNetworkInfo.clear();

	// Set the flags to pass to GetAdaptersAddresses
	family = AF_INET;
	flags = GAA_FLAG_INCLUDE_PREFIX;
	lpMsgBuf = NULL;
	pAddresses = NULL;
	pCurrAddresses = NULL; 
	outBufLen = sizeof (IP_ADAPTER_ADDRESSES);

	pAddresses = (IP_ADAPTER_ADDRESSES *)malloc(outBufLen);
	if(GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen) == ERROR_BUFFER_OVERFLOW)
	{
		free(pAddresses);
		pAddresses = (IP_ADAPTER_ADDRESSES *)malloc(outBufLen);
	}

	// Wlan 조회
	GetWlanList(vecWlanList);

	bReturn = FALSE;
	dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);
	if (dwRetVal == NO_ERROR) 
	{
		pCurrAddresses = pAddresses;
		while (pCurrAddresses)
		{
			if((IF_TYPE_ETHERNET_CSMACD == pCurrAddresses->IfType) ||
				(IF_TYPE_IEEE80211 == pCurrAddresses->IfType))
			{
				szAdapterName = UTF8ToUnicode(pCurrAddresses->AdapterName);
				strAdapterName.Format(_T("%s"), szAdapterName);
				free(szAdapterName);

				strDescription.Format(_T("%s"), pCurrAddresses->Description);
				if((strDescription.Find(IDS_ADAPTDESC_BLUETOOTH) == -1) &&
					(strDescription.Find(IDS_ADAPTDESC_VIRTUAL) == -1))
				{
					bWlan = FALSE;

					for(IterWlan = vecWlanList.begin();
						IterWlan != vecWlanList.end();
						IterWlan++)
					{
						if(0 == IterWlan->CompareNoCase((LPCTSTR)strAdapterName))
						{
							bWlan = TRUE;	
							break;
						}
					}

					if(TRUE == bWlan)
					{
						stNetworkInfo.eNetworkType = WIRELESS_LAN;
					}
					else
					{
						stNetworkInfo.eNetworkType = PHYSICAL_LAN;
					}

					if(NULL != pCurrAddresses->FirstUnicastAddress)
					{
						pAddr = (struct sockaddr_in*)pCurrAddresses->FirstUnicastAddress->Address.lpSockaddr; 
						strIPAddress = inet_ntoa(pAddr->sin_addr);
						_tcsncpy_s(stNetworkInfo.szIPAddress, _countof(stNetworkInfo.szIPAddress),
							(LPCTSTR)strIPAddress, _TRUNCATE);
					}

					if(0 != pCurrAddresses->PhysicalAddressLength)
					{
						strMACAddress.Format(_T("%02X:%02X:%02X:%02X:%02X:%02X"),
							pCurrAddresses->PhysicalAddress[0],
							pCurrAddresses->PhysicalAddress[1],
							pCurrAddresses->PhysicalAddress[2],
							pCurrAddresses->PhysicalAddress[3],
							pCurrAddresses->PhysicalAddress[4],
							pCurrAddresses->PhysicalAddress[5]);

						_tcsncpy_s(stNetworkInfo.szMACAddress, _countof(stNetworkInfo.szMACAddress),
							(LPCTSTR)strMACAddress, _TRUNCATE);
					}

					if(IfOperStatusDown != pCurrAddresses->OperStatus){
						vecNetworkInfo.push_back(stNetworkInfo);
					}else{
						DEBUG_AND_LOG_PRINT(false,_T("NetInfo OperStatus is Down"));
					}
				}
			}

			pCurrAddresses = pCurrAddresses->Next;
		}
	}

	if(NULL != pAddresses)
	{
		free(pAddresses);
	}

	return bReturn;
}

BOOL GlobalFunc::GetWlanList(vector<CString> &vecWlanList)
{
	CString strWlanGUID;

	BOOL bReturn = FALSE;
	HANDLE hClient = NULL;
	DWORD dwMaxClient = 2;  
	DWORD dwCurVersion = 0;
	DWORD dwResult = 0;
	WCHAR GuidString[40] = {0};
	PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
	PWLAN_INTERFACE_INFO pIfInfo = NULL;

	vecWlanList.clear();

	dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient); 
	if(ERROR_SUCCESS != dwResult)  
	{
		return FALSE;
	}

	bReturn = TRUE;
	dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList); 
	if(ERROR_SUCCESS == dwResult)  
	{
		for (int i = 0; i < (int) pIfList->dwNumberOfItems; i++) 
		{
			pIfInfo = (WLAN_INTERFACE_INFO *)&pIfList->InterfaceInfo[i];

			if(0 != StringFromGUID2(pIfInfo->InterfaceGuid, (LPOLESTR) &GuidString, 39))
			{
				strWlanGUID.Format(_T("%s"), GuidString);
				vecWlanList.push_back(strWlanGUID);
			}
			else 
			{
				bReturn = FALSE;
				break;
			}    
		}
	}
	else 
	{
		bReturn = FALSE;
	}

	if(NULL != pIfList)
	{
		WlanFreeMemory(pIfList);
		pIfList = NULL;
	}	

	return bReturn;
}

typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
typedef BOOL (WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);

BOOL GlobalFunc::LoadHardwareInformation() {
	G_strCPUName = _T("");
	G_strUSBHost = _T("");
	G_strHardwareID = _T("");
	G_strMotherBoard = _T("");
	G_strPhysicalRAM = _T("");

	BOOL bReturn = FALSE;
	
	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Get CPU Name
	if(!LoadCPUName()) {
		bReturn = FALSE;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// USB Controller
	if(!LoadUSBController()) {
		bReturn = FALSE;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Motherboard info
	CWQL myWQL;
	CWQL::RowSet rs;

	if(!myWQL.connect(WMI_LOCALHOST,WMI_DEFAULTNAME,WMI_DEFAULTPW)) {
		bReturn = FALSE;
	}

	if(!myWQL.getClassProperties(L"Win32_BaseBoard",rs)) {
		bReturn = FALSE;
	}

	CString strMB_manufacturer = _T("");
	CString strMB_serialNumber = _T("");
	if(rs.size() > 0)
	{	
		CWQL::Row r =  rs.at(0);		
		strMB_manufacturer.Format(_T("%s"), rs[0][_T("Manufacturer")].c_str());
		strMB_serialNumber.Format(_T("%s"), rs[0][_T("SerialNumber")].c_str());
		
	}
	CString strMainBoard = _T("");
	strMainBoard.Format (_T("[Mainboard Information] %s %s"), strMB_manufacturer, strMB_serialNumber);
	G_strMotherBoard = strMainBoard;
	
	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Physical RAM Size
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof (statex);

	GlobalMemoryStatusEx (&statex);

	G_strPhysicalRAM.Format (_T("%.2f GB"),(float)(statex.ullTotalPhys)/(1024*1024*1024));

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	// Physical Disk Serial Number
	CDiskInformation DiskInformation;
	G_strPhysicalDiskSerialNumber = DiskInformation.GetDiskSerial();
	//////////////////////////////////////////////////////////////////////////////////////////////////////

	DEBUG_AND_LOG_PRINT (true, _T("CPU Name: %s"),G_strCPUName);
	DEBUG_AND_LOG_PRINT (false, _T("USB Host Name: %s"),G_strUSBHost);
	DEBUG_AND_LOG_PRINT (false, _T("USB Host HW ID: %s"),G_strHardwareID);
	DEBUG_AND_LOG_PRINT (false, _T("Motherboard info: %s"),G_strMotherBoard);
	DEBUG_AND_LOG_PRINT (true, _T("Physical RAM Size: %s"),G_strPhysicalRAM);
	DEBUG_AND_LOG_PRINT (false, _T("Physical Disk Serial Number: %s"),G_strPhysicalDiskSerialNumber);
	return bReturn;
}

BOOL GlobalFunc::LoadOSInformation() {
	G_strOSName = _T("");
	G_strOSLanguage = _T("");
	G_OSVersion = OS_ERROR;

	BOOL bReturn = FALSE;
	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Get OS Version
	if(!LoadOSVersion()) {
		bReturn = FALSE;
	}

	if(!LoadOSLanguage()) {
		bReturn = FALSE;
	}

	DEBUG_AND_LOG_PRINT (true, _T("OS Name: %s"),G_strOSName);
	DEBUG_AND_LOG_PRINT (true, _T("OS Language: %s"),G_strOSLanguage);

	return bReturn;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
BOOL GlobalFunc::LoadCPUName() {

	int nReturn;
	HKEY hKey;

	nReturn = RegOpenKeyEx (HKEY_LOCAL_MACHINE,_T("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),0,KEY_QUERY_VALUE,&hKey);
	if(nReturn == ERROR_SUCCESS) {
		DWORD dwType = REG_SZ;
		TCHAR szCPUName[1024];
		DWORD dwcbData = sizeof(szCPUName);

		if(RegQueryValueEx (hKey,_T("ProcessorNameString"),0,&dwType,(LPBYTE)szCPUName,&dwcbData) == ERROR_SUCCESS) {
			G_strCPUName = szCPUName;
			
			return TRUE;
		}
	}

	return FALSE;
}

BOOL GlobalFunc::LoadUSBController() {
	static const GUID USB_CONTROLLER = {0x36FC9E60, 0xC465, 0x11CF, { 0x80, 0x56, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};

	HDEVINFO hDeviceInfo;
	hDeviceInfo = SetupDiGetClassDevs(&USB_CONTROLLER,NULL,NULL,DIGCF_PRESENT);
	if(hDeviceInfo == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	SP_DEVINFO_DATA devInfo;
	devInfo.cbSize = sizeof(devInfo);

	CString strPropertyBuffer = _T("");
	CString strHardwareID = _T("Unknown");

	int nIndex = 0;
	while(SetupDiEnumDeviceInfo(hDeviceInfo,nIndex++,&devInfo)) {
		
		TCHAR szPropertyBuffer[1024];
		::ZeroMemory(szPropertyBuffer,sizeof(szPropertyBuffer));
		if(SetupDiGetDeviceRegistryProperty(hDeviceInfo,
											&devInfo,
											SPDRP_DEVICEDESC,
											NULL,
											(PBYTE)szPropertyBuffer,
											sizeof(szPropertyBuffer),
											NULL)) {
			
			CString strPropertyBufferUpper = szPropertyBuffer;
			strPropertyBufferUpper.MakeUpper();
			if(strPropertyBufferUpper.Find (_T("HOST CONTROLLER")) != -1 &&
				((strPropertyBufferUpper.Find (_T("UNIVERSAL")) != -1) || (strPropertyBufferUpper.Find (_T("ENHANCED")) != -1))) {
			
					TCHAR szInstanceID[1024];
					DWORD dwRequestSize = 0;
					if(SetupDiGetDeviceInstanceId(hDeviceInfo,&devInfo,szInstanceID,sizeof(szInstanceID),&dwRequestSize)) {
						strHardwareID = szInstanceID;
						strPropertyBuffer = szPropertyBuffer;
						break;
					}
			}
		}
	}
	SetupDiDestroyDeviceInfoList(hDeviceInfo);


	if (strHardwareID.CompareNoCase(_T("Unknown")) != 0)
    {
		int iFind = strHardwareID.Find(_T('\\'));
		if(iFind != -1) {
			strHardwareID = strHardwareID.Right (strHardwareID.GetLength() - (iFind+1));
		}

		CString strToken;
		iFind = 0;

		CString strVEN,strDEV;
		strToken = strHardwareID.Tokenize(_T("&"),iFind);
		while(!strToken.IsEmpty()) {
			
			if(strToken.Find (_T("VEN")) != -1) {
				strVEN = strToken;
			}

			if(strToken.Find (_T("DEV")) != -1) {
				strDEV = strToken;
			}

			strToken = strHardwareID.Tokenize(_T("&"),iFind);
		}

		G_strHardwareID.Format (_T("%s|%s"),strVEN,strDEV);

		iFind = strPropertyBuffer.Find(_T('-'));
		if(iFind != -1) {
			strPropertyBuffer = strPropertyBuffer.Right(strPropertyBuffer.GetLength() - (iFind+1));
		}

		iFind = strPropertyBuffer.Find (_T('\0'));
		if(iFind != -1) {
			strPropertyBuffer = strPropertyBuffer.Left(iFind);
		}
		G_strUSBHost = strPropertyBuffer;
    }
	return TRUE;
}

BOOL GlobalFunc::LoadOSVersion() {

	CString strOS;
	OSVERSIONINFOEX osvi;
	SYSTEM_INFO si;
	PGNSI pGNSI;
	PGPI pGPI;
	BOOL bOsVersionInfoEx;
	DWORD dwType;

	G_bIs64BitOS = FALSE;

	ZeroMemory(&si, sizeof(SYSTEM_INFO));
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi)) ) {
		return FALSE;
	}

	// Call GetNativeSystemInfo if supported or GetSystemInfo otherwise.

	pGNSI = (PGNSI) GetProcAddress(
		GetModuleHandle(TEXT("kernel32.dll")), 
		"GetNativeSystemInfo");
	if(NULL != pGNSI)
		pGNSI(&si);
	else GetSystemInfo(&si);

	if ( VER_PLATFORM_WIN32_NT==osvi.dwPlatformId && osvi.dwMajorVersion > 4 )
	{
		strOS.Format(_T("Microsoft "));

		// Test for the specific product.

		if ( osvi.dwMajorVersion == 6 )
		{
			switch (osvi.dwMinorVersion)
			{
			case 1:
				{
					if( osvi.wProductType == VER_NT_WORKSTATION )
						strOS += _T("Windows 7");
					else 
						strOS += _T("Windows Server 2008 R2 ");

					G_OSVersion = OS_WIN7;
				}
				break;
			case 2:
				{
					if( osvi.wProductType == VER_NT_WORKSTATION )
						strOS += _T("Windows 8");
					else 
						strOS += _T("Windows Server 2012 ");

					G_OSVersion = OS_WIN8;
				}
				break;
			case 3:
				{
					if( osvi.wProductType == VER_NT_WORKSTATION )
						strOS += _T("Windows 8.1");
					else 
						strOS += _T("Windows Server 2012 R2");

					G_OSVersion = OS_WIN8_1;
				}
				break;
			default:
				{
					if( osvi.wProductType == VER_NT_WORKSTATION ) {
						strOS += _T("Windows Vista ");
					}else {
						strOS += _T("Windows Server 2008");
					}

					G_OSVersion = OS_WINVISTA;
				}
				break;
			}

			pGPI = (PGPI) GetProcAddress(
				GetModuleHandle(TEXT("kernel32.dll")), 
				"GetProductInfo");

			pGPI( osvi.dwMajorVersion, osvi.dwMinorVersion, 0, 0, &dwType);
		}

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
		{
			if( GetSystemMetrics(SM_SERVERR2) ) {
				strOS += TEXT( "Windows Server 2003 R2, ");

				G_OSVersion = OS_WINXPSV;

			} else if ( osvi.wSuiteMask==VER_SUITE_STORAGE_SERVER ) {
				strOS += TEXT( "Windows Storage Server 2003");

				G_OSVersion = OS_WINXPSV;
			} else if( osvi.wProductType == VER_NT_WORKSTATION &&
				si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
			{
				strOS += TEXT( "Windows XP Professional x64 Edition");

				G_OSVersion = OS_WINXP64;
			}else {
				strOS += TEXT("Windows Server 2003, ");

				G_OSVersion = OS_WINXPSV;
			}
			// Test for the server type.
			if ( osvi.wProductType != VER_NT_WORKSTATION )
			{
				if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_IA64 )
				{
					if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
						strOS += TEXT( "Datacenter Edition for Itanium-based Systems" );
					else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
						strOS += TEXT( "Enterprise Edition for Itanium-based Systems" );
				}

				else if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
				{
					if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
						strOS += TEXT( "Datacenter x64 Edition" );
					else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
						strOS += TEXT( "Enterprise x64 Edition" );
					else 
						strOS += TEXT( "Standard x64 Edition" );
				}

				else
				{
					if ( osvi.wSuiteMask & VER_SUITE_COMPUTE_SERVER )
						strOS += TEXT( "Compute Cluster Edition" );
					else if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
						strOS += TEXT( "Datacenter Edition" );
					else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
						strOS +=  TEXT( "Enterprise Edition" );
					else if ( osvi.wSuiteMask & VER_SUITE_BLADE )
						strOS +=  TEXT( "Web Edition" );
					else 
						strOS += TEXT( "Standard Edition" );
				}
			}
		}

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
		{
			strOS += TEXT("Windows XP ");
			if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
				strOS += TEXT( "Home Edition" );
			else
				strOS += TEXT( "Professional" );

			G_OSVersion = OS_WINXP;
		}

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
		{
			strOS += TEXT("Windows 2000 ");

			if ( osvi.wProductType == VER_NT_WORKSTATION )
			{
				strOS += TEXT( "Professional" );
			}
			else 
			{
				if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
					strOS +=  TEXT( "Datacenter Server" );
				else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
					strOS +=  TEXT( "Advanced Server" );
				else 
					strOS += TEXT( "Server" );
			}

			G_OSVersion = OS_WIN2K;
		}

		// Include service pack (if any) and build number.

		if( _tcslen(osvi.szCSDVersion) > 0 )
		{
			CString temp;
			temp.Format(_T(" %s"),osvi.szCSDVersion);

			strOS += temp;
		}

		CString temp;
		temp.Format(_T(" build %d"),osvi.dwBuildNumber);

		strOS += temp;

		if ( osvi.dwMajorVersion >= 6 )
		{
			if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
				strOS += TEXT( " 64-bit" );
			else if (si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL )
				strOS += TEXT(" 32-bit");
		}

		if (osvi.dwMajorVersion >= 6 || (osvi.dwMajorVersion==5 && osvi.dwMinorVersion >=2))
		{
			if(si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
				si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64) {

				G_bIs64BitOS = TRUE;
			}
		}

		G_strOSName =  strOS; 
	}else if ( VER_PLATFORM_WIN32_WINDOWS==osvi.dwPlatformId ) {
		strOS.Format(_T("Microsoft "));

		if ( osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10 ) {

			strOS += TEXT("Windows 98 ");

			G_OSVersion = OS_WIN98ME;
		}else if ( osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90) {

			strOS += TEXT("Windows ME ");

			G_OSVersion = OS_WIN98ME;
		}else{
			strOS = _T("This sample does not support this version of Windows.\n");

			G_OSVersion = OS_ERROR;
		}

		G_strOSName =  strOS; 
	}else{  
		strOS = _T("This sample does not support this version of Windows.\n");
		G_strOSName =  strOS;

		G_OSVersion = OS_ERROR;

		return FALSE;
	}

	return TRUE;
}

BOOL GlobalFunc::LoadOSLanguage() {

	LCID localID = GetSystemDefaultLCID();

	int nReturn = GetLocaleInfo(localID,LOCALE_SENGLANGUAGE,NULL,0);

	if(nReturn > 0) {
		TCHAR szLCData[256];
		nReturn = GetLocaleInfo(localID,LOCALE_SENGLANGUAGE,szLCData,sizeof(szLCData)/sizeof(TCHAR));

		CString strLCData = szLCData;
		G_strOSLanguage = strLCData.Left(nReturn -1);

		return TRUE;
	}

	return FALSE;
}


LONG GlobalFunc::encryptMD5(CString strInput,BYTE* pbOutput,CString& strOutput) 
{
	HCRYPTPROV hCryptProv; 
	HCRYPTHASH hHash; 
	DWORD dwHashLen= 16; // The MD5 algorithm always returns 16 bytes. 
	DWORD cbInput = strInput.GetLength();
	LONG lResult = 0;
#ifdef UNICODE
	size_t  count=0;
	BYTE* pbInput = (BYTE*) malloc(cbInput+1); // +1 for NULL terminated string.
	wcstombs_s(&count, (char*)pbInput, cbInput+1, strInput.GetBuffer(), cbInput+1);
#else
	BYTE* pbInput= (BYTE*)strInput.GetBuffer(cbInput); 
#endif

	if(CryptAcquireContext(&hCryptProv,NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET)) {
		if(CryptCreateHash(hCryptProv, 
								CALG_MD5, // algorithm identifier definitions see: wincrypt.h
								0, 
								0, 
								&hHash)) 
		{
			if(CryptHashData(hHash, pbInput, cbInput, 0)) {
					if(CryptGetHashParam(hHash, HP_HASHVAL, pbOutput, &dwHashLen, 0)) {
						// Make a string version of the numeric digest value
						CString tmp;
						for (int i = 0; i<16; i++) {
							tmp.Format(_T("%02x"), pbOutput[i]);
							strOutput+=tmp;
						}
				}else{
					DEBUG_AND_LOG_PRINT(false, _T("MD5 - Error getting hash param"));
					lResult = 1;
				}
			}else{
				lResult = 2;
				DEBUG_AND_LOG_PRINT(false, _T("MD5 - Error hashing data"));
			}
		}else{
			lResult = 3;
			DEBUG_AND_LOG_PRINT(false, _T("MD5 - Error creating hash"));
		}
	}else{
		lResult = 4; 
		DEBUG_AND_LOG_PRINT(false, _T("MD5 - Error acquiring context"));
	}

	CryptDestroyHash(hHash); 
	CryptReleaseContext(hCryptProv, 0); 

#ifdef UNICODE
	free(pbInput);
#else
	strInput.ReleaseBuffer();
#endif //UNICODE
	return lResult; 
}

bool GlobalFunc::CheckTargetProcessIsRunningOrNot(LPCTSTR lpszProcessName) {
	CString strTargetProcess = lpszProcessName;
	if(strTargetProcess.Right(3).CompareNoCase(_T("exe")) != 0) {
		strTargetProcess += _T(".exe");
	}

	HANDLE hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0);
	if( hProcessSnap == INVALID_HANDLE_VALUE){
		DEBUG_AND_LOG_PRINT(false, _T("CheckTargetProcessIsRunningOrNot: CreateToolhelp32Snapshot error!"));
		return false;
	}

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	if(!Process32First(hProcessSnap, &pe32)){
		DEBUG_AND_LOG_PRINT(false, _T("CheckTargetProcessIsRunningOrNot: Process32First error!"));
		CloseHandle(hProcessSnap);
		return false;
	}

	bool bFoundTargetProcess = false;
 
	do {
		if( strTargetProcess.CompareNoCase(pe32.szExeFile) == 0) {
			// Found AdminDelegator
			bFoundTargetProcess = true;
			break;
		}
	} while( Process32Next(hProcessSnap, &pe32) );

	CloseHandle(hProcessSnap);

	return bFoundTargetProcess;
}

bool GlobalFunc::CheckCrc(LPCTSTR  lpszFilePath, LPCTSTR lpszCrc) {
	CString strCrcValue;

	try
	{
		DEBUG_AND_LOG_PRINT(false, _T("@@@@@@@@@ %s Check CRC Start @@@@@@@@"), lpszFilePath);
		DWORD	dwCrc32;

		CCrc32Dynamic* pobCrc32Dynamic = new CCrc32Dynamic;

		pobCrc32Dynamic->Init();

		//LPSTR szFileName = GlobalFunc::Unicode2UTF8((LPCWSTR)lpszFileName);


		pobCrc32Dynamic->FileCrc32Win32(lpszFilePath, dwCrc32);

		
		//::free(lpszFileName);

		pobCrc32Dynamic->Free();

		strCrcValue.Format (_T("%lu"),dwCrc32);

		delete pobCrc32Dynamic;

		
		DEBUG_AND_LOG_PRINT(false, _T("@@@@@@ CRC value From file: %s, CRC value from XML: %s @@@@@@"), strCrcValue, lpszCrc);
		if (strCrcValue.CompareNoCase(lpszCrc) != 0) {
			DEBUG_AND_LOG_PRINT(false, _T("@@@@@ Check CRC failed! @@@@@"));
			return false;
		}
//		return true;
	}
	catch (...)
	{
		DWORD dwErrorCode = GetLastError();

		DEBUG_AND_LOG_PRINT(false, _T("CRC ERROR: ErrorCode =%d,ErrorMessage=%s"),dwErrorCode,GlobalFunc::ShowErrorMessage(dwErrorCode));

		return false;
	}
		
	DEBUG_AND_LOG_PRINT(false, _T("@@@@@ Check CRC Success @@@@@"));
	return true;
}

DWORD GlobalFunc::GetFileCrc(LPCTSTR lpszFileName)
{
	CString strCrcValue;

	try
	{
		DEBUG_AND_LOG_PRINT(false, _T("@@@@@@@@@ %s Check CRC Start @@@@@@@@"), lpszFileName);
		DWORD	dwCrc32;

		CCrc32Dynamic* pobCrc32Dynamic = new CCrc32Dynamic;

		pobCrc32Dynamic->Init();

		//LPSTR szFileName = GlobalFunc::Unicode2UTF8((LPCWSTR)lpszFileName);


		pobCrc32Dynamic->FileCrc32Win32(lpszFileName, dwCrc32);

		
		//::free(lpszFileName);

		pobCrc32Dynamic->Free();

		strCrcValue.Format (_T("%lu"),dwCrc32);

		delete pobCrc32Dynamic;

		
		DEBUG_AND_LOG_PRINT(false, _T("@@@@@@ Dll CRC: %s @@@@@@"), strCrcValue);
		DEBUG_AND_LOG_PRINT(false, _T("@@@@@ Check CRC Success @@@@@"));
		return dwCrc32;
	}
	catch (...)
	{
		return -1;
	}

	return -1;
}


BOOL CALLBACK EnumChildProc(HWND hwnd,LPARAM lParam)
{
	CPtrArray* pAry = (CPtrArray*)lParam;
	TCHAR buf[MAX_PATH] = {'\0'};

	if(pAry) 
	{		
		::GetClassName( hwnd, (LPTSTR)&buf, 100 );
		if ( _tcscmp( buf, _T("Internet Explorer_Server") ) == 0 )
			pAry->Add(hwnd);
	}

	return TRUE;
};

BOOL GlobalFunc::CheckTargetSiteIsOpenedOrNot(LPCTSTR lpszCiteName)
{
	CString strURL = _T("");
	BOOL bRet = FALSE;
	HWND hWndChild = NULL;

	HWND hWnd = ::GetDesktopWindow(); 
	if ( hWnd == NULL )
	{
		//	OUTSTR(_T("%s GetDesktopWindow return FALSE"), _T(__FUNCTION__));
		return FALSE;
	}

	HINSTANCE hInst = ::LoadLibrary( _T("OLEACC.DLL") );
	if ( hInst == NULL )
	{
		//	OUTSTR(_T("%s LoadLibrary return FALSE"), _T(__FUNCTION__));
		return FALSE;
	}

#ifndef USE_MSXML_PARSER
	CoInitialize( NULL );
#endif//USE_MSXML_PARSER

	CString strGetLocURL = _T("");
	BSTR bstrURL = NULL;
	HRESULT hr = S_OK;	

	CPtrArray aryWindow;
	CPtrArray arySubWindow;

	aryWindow.RemoveAll();
	arySubWindow.RemoveAll();

	::EnumChildWindows( hWnd, EnumChildProc, (LPARAM)&aryWindow );

	for(int i = 0; i < aryWindow.GetSize(); i++)
	{
		hWndChild = (HWND)aryWindow.GetAt(i);
		if ( hWndChild != NULL)
		{
			LRESULT lRes = 0;

			UINT nMsg = ::RegisterWindowMessage( _T("WM_HTML_GETOBJECT") );
			::SendMessageTimeout( hWndChild, nMsg, 0L, 0L, SMTO_ABORTIFHUNG, 1000, (DWORD*)&lRes );

			CString str = _T("ObjectFromLresult");
			CStringA strA(str);
			LPCSTR ptr = strA;

			LPFNOBJECTFROMLRESULT pfObjectFromLresult =	(LPFNOBJECTFROMLRESULT)::GetProcAddress( hInst, ptr );
			if ( pfObjectFromLresult != NULL )
			{
				CComPtr<IHTMLDocument2> spDoc;
				hr = (*pfObjectFromLresult)( lRes, IID_IHTMLDocument, 0, (void**)&spDoc );
				if ( SUCCEEDED(hr) )
				{
					CComPtr<IDispatch> spDisp = NULL;
					CComQIPtr<IHTMLWindow2> spWin;
					IWebBrowser2 *pSubWB = NULL;
					IServiceProvider *pSP = NULL;
					IDispatch* pDisp=NULL;

					hr = spDoc->get_Script( &spDisp );
					if ( SUCCEEDED(hr) )
					{
						spWin = spDisp;
						hr = spWin->QueryInterface(IID_IServiceProvider, (void**) &pSP);
						if ( SUCCEEDED(hr) )
						{
							hr = pSP->QueryService(IID_IWebBrowserApp, IID_IWebBrowser2, (void**)&pSubWB);
							if ( SUCCEEDED(hr) )
							{
								hr = pSubWB->get_LocationURL(&bstrURL);
								if ( SUCCEEDED(hr) )
								{
									strGetLocURL.Format(_T("%d: %s\r\n"), i+1, bstrURL);
									strURL += strGetLocURL;
								}
								//	else
								//	OUTSTR(_T("%s get_LocationURL return FALSE"), _T(__FUNCTION__));
							}
							//else
							//		OUTSTR(_T("%s IID_IWebBrowserApp return FALSE"), _T(__FUNCTION__));
						}
						//else
						//	OUTSTR(_T("%s IID_IServiceProvider return FALSE"), _T(__FUNCTION__));
					}
					//	else
					//	OUTSTR(_T("%s get_Script return FALSE hr: %x"), _T(__FUNCTION__), hr);

					if ( spDisp )
					{
						spDisp.Release();
						spDisp = NULL;
					}

					if( spWin )
					{
						spWin.Release();
						//spWin = NULL;
					}

					if( pSubWB )
					{
						pSubWB->Release();
						pSubWB = NULL;
					}

					if( pSP )
					{
						pSP->Release();
						pSP = NULL;
					}

					if( pDisp )
					{
						pDisp->Release();
						pDisp = NULL;
					}
				}
				//	else
				//OUTSTR(_T("%s pfObjectFromLresult return FAILED"), _T(__FUNCTION__));

				if( spDoc )
				{
					spDoc.Release();
					spDoc = NULL;
				}
			}
			//else
			//	OUTSTR(_T("%s pfObjectFromLresult is NULL"), _T(__FUNCTION__));
		}
		//	else
		//OUTSTR(_T("%s hWndChild is NULL"), _T(__FUNCTION__));
	}		

	if( hWnd )
		hWnd = NULL;

	if( hWndChild )
		hWndChild = NULL;

	if( hInst )
	{
		::FreeLibrary( hInst );
		hInst = NULL;
	}

#ifndef USE_MSXML_PARSER
		CoUninitialize();
#endif//USE_MSXML_PARSER

	if( strURL.Find(lpszCiteName) > 0 )
		bRet = TRUE;

	return bRet;
}

BOOL GlobalFunc::IsWinVista()
{
	OSVERSIONINFOEX osvi;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if ( VER_PLATFORM_WIN32_NT==osvi.dwPlatformId && osvi.dwMajorVersion > 4 )
		if ( osvi.dwMajorVersion == 6 )
			if( osvi.dwMinorVersion == 0 )
				if( osvi.wProductType == VER_NT_WORKSTATION )
					return TRUE;
	return FALSE;
}

bool G_bInitializeSocket = false;
void GlobalFunc::SendUDPSignal(int nPort,LPCTSTR lpszMessage, int ncharset) {
	if(nPort == 0) {
		DEBUG_AND_LOG_PRINT(false, _T("SendUDPSignal: Port is Zero SendUDPSignal Failed"));
		return;
	}

	CString strMessage;
	strMessage = lpszMessage;

	if(!G_bInitializeSocket) {
		WSADATA wsaData;
		if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
		{
			DEBUG_AND_LOG_PRINT(false, _T("########## WSAStartup() Failure!!! #############"));
		}
		G_bInitializeSocket = true;
	}

	SOCKET sock;
	sock = socket(AF_INET,SOCK_DGRAM,0);
	if(sock == SOCKET_ERROR) {
		DWORD dwError = WSAGetLastError();
		DEBUG_AND_LOG_PRINT(false, _T("SendUDPSignal: socket() failed!!, errorCode=%d,errorMsg=%s"),dwError,GlobalFunc::ShowErrorMessage(dwError));

		return ;
	}

	sockaddr_in stSockAddr;
	memset((char *) &stSockAddr, 0x00, sizeof(stSockAddr));
	stSockAddr.sin_family = AF_INET;
	stSockAddr.sin_port = htons(nPort);
	stSockAddr.sin_addr.s_addr = htonl(_tinet_addr(_T("")));
	
	LPSTR pBuffer = "";
	switch(ncharset)
	{
	case CP_ACP:
		pBuffer = GlobalFunc::String2Char(strMessage);
		break;
	default:
		pBuffer = GlobalFunc::Unicode2UTF8((LPCWSTR)strMessage);
		break;
	}
	
	int nBufferLength = strlen(pBuffer)+1;
	int nResult = sendto(sock,pBuffer,nBufferLength,0,(const struct sockaddr *)&stSockAddr,sizeof(stSockAddr));
	closesocket(sock);

	free(pBuffer);

	if(nResult == SOCKET_ERROR) {
		DWORD dwError = WSAGetLastError();
		DEBUG_AND_LOG_PRINT(true, _T("SendUDPSignal: sendto() failed!!, errorCode=%d,errorMsg=%s"),dwError,GlobalFunc::ShowErrorMessage(dwError));
	}
}

LPSTR GlobalFunc::ANSIToUTF8(char * pszCode)
{
	int		nLength, nLength2;
	BSTR	bstrCode; 
	char	*pszUTFCode = NULL;

	nLength = MultiByteToWideChar(CP_ACP, 0, pszCode, lstrlenA(pszCode), NULL, NULL); 
	bstrCode = SysAllocStringLen(NULL, nLength); 
	MultiByteToWideChar(CP_ACP, 0, pszCode, lstrlenA(pszCode), bstrCode, nLength);


	nLength2 = WideCharToMultiByte(CP_UTF8, 0, bstrCode, -1, pszUTFCode, 0, NULL, NULL); 
	pszUTFCode = (char*)malloc(nLength2+1); 
	WideCharToMultiByte(CP_UTF8, 0, bstrCode, -1, pszUTFCode, nLength2, NULL, NULL); 

	return pszUTFCode;
}

LPSTR GlobalFunc::UTF8ToANSI(char *pszCode)
{
	BSTR    bstrWide;
	char*   pszAnsi;
	int     nLength;

	nLength = MultiByteToWideChar(CP_UTF8, 0, pszCode, lstrlenA(pszCode) + 1, NULL, NULL);
	bstrWide = SysAllocStringLen(NULL, nLength);

	MultiByteToWideChar(CP_UTF8, 0, pszCode, lstrlenA(pszCode) + 1, bstrWide, nLength);

	nLength = WideCharToMultiByte(CP_ACP, 0, bstrWide, -1, NULL, 0, NULL, NULL);
	pszAnsi = new char[nLength];

	WideCharToMultiByte(CP_ACP, 0, bstrWide, -1, pszAnsi, nLength, NULL, NULL);
	SysFreeString(bstrWide);
	return pszAnsi;
}

LPSTR GlobalFunc::String2Char(CString str)
{
	char *szStr = NULL;
#if defined(UNICODE) || defined(_UNICODE)
	int nLen = str.GetLength() + 1;
	TCHAR *tszTemp = NULL;
	tszTemp = new TCHAR[nLen];
	memset(tszTemp, 0x00, nLen*sizeof(TCHAR));
	_tcscpy(tszTemp, str);
	// Get size (실제사용되는바이트사이즈)
	int nSize = WideCharToMultiByte(CP_ACP, 0, tszTemp, -1, NULL, NULL, NULL, NULL);
	szStr = new char[nSize];
	memset(szStr, 0x00, nSize);
	WideCharToMultiByte(CP_ACP, 0, tszTemp, -1, szStr, nSize, NULL, NULL);
	if(tszTemp)
	{
		delete [] tszTemp;
		tszTemp = NULL;
	}
#else
	int nLen = str.GetLength() + 1;
	szStr = new char[nLen];
	memset(szStr, 0x00, nLen);
	strcpy(szStr, str);
#endif
	return szStr;
}

CString GlobalFunc::Char2String(char *str)
{
	CString cStr;
#if defined(UNICODE) || defined(_UNICODE)
	int nLen = strlen(str) + 1;
	TCHAR *tszTemp = NULL;
	tszTemp = new TCHAR[nLen];
	memset(tszTemp, 0x00, nLen*sizeof(TCHAR));
	MultiByteToWideChar(CP_ACP, 0, str, -1, tszTemp, nLen*sizeof(TCHAR));
	cStr.Format(_T("%s"), tszTemp);
	if(tszTemp)
	{
		delete [] tszTemp;
		tszTemp = NULL;
	}
#else
	cStr.Format("%s", str);
#endif
	return cStr;
}

DWORD GlobalFunc::_tinet_addr(const TCHAR *cp)
{
	if(_tcslen(cp) == 0)
		return INADDR_LOOPBACK;
	
#ifdef UNICODE
	char IP[16];
	int Ret = 0;
	Ret = WideCharToMultiByte(CP_ACP, 0, cp, _tcslen(cp), IP, 15, NULL, NULL);
	IP[Ret] = 0;
	return inet_addr(IP);
#endif
#ifndef UNICODE
	return init_addr(cp);
#endif
}
