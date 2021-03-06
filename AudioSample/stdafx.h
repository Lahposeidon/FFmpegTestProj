
// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions


#include <afxdisp.h>        // MFC Automation classes



#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // MFC support for ribbons and control bars









#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

///> Library Link On Windows System
#pragma comment( lib, "avformat-56.lib" )
#pragma comment( lib, "avutil-54.lib" )
#pragma comment( lib, "avcodec-56.lib" )
#pragma comment( lib, "swresample-1.lib" )

#pragma warning(disable : 4005)

inline LPCTSTR ShowErrorMessage(DWORD dwErrorCode) {

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

#define BUF_SIZE	1024*24

inline void DebugAndLogPrint(LPCTSTR fmt,...) {
	TCHAR szBuffer[BUF_SIZE];

	try {
		va_list argptr;
		va_start(argptr, fmt);
		_vsntprintf_s(szBuffer, BUF_SIZE, fmt, argptr);
		va_end(argptr);

		CString strDebugMsg;
		strDebugMsg.Format(_T("(AudioSample) : %s\n"), szBuffer);

		OutputDebugString(strDebugMsg);
	}catch(...){
		OutputDebugString(ShowErrorMessage(GetLastError()));
	}
}