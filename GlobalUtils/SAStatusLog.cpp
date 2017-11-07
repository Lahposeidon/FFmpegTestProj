/*********************************************************************

   Copyright (C) 1999 Smaller Animals Software, Inc.

   This software is provided 'as-is', without any express or implied
   warranty.  In no event will the authors be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

   3. This notice may not be removed or altered from any source distribution.

   http://www.smalleranimals.com
   smallest@smalleranimals.com

**********************************************************************/
#pragma once

#include "stdafx.h"
#include "SAStatusLog.h"
#include <Shlwapi.h>

/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma warning(disable:4996)

CSAStatusLog::CSAStatusLog()
{
	m_bEnable = FALSE;
   m_bPrintTime = FALSE;
	m_csFileName = "";

   m_csAppName = "";

   // we'll make sure only one call uses the critical stuff at a time
   InitializeCriticalSection(&m_crit);
}

/////////////////////////////////////////////////////////////////////////////

CSAStatusLog::~CSAStatusLog()
{
   DeleteCriticalSection(&m_crit);
}

/////////////////////////////////////////////////////////////////////////////

void CSAStatusLog::Init(LPCTSTR pOutputFilename, BOOL boverWrite)
{
	m_bEnable = TRUE;

	m_csAppName = "";

	m_csFileName = pOutputFilename;

	if(boverWrite && PathFileExists(pOutputFilename))
		DeleteFile(m_csFileName);
}

/////////////////////////////////////////////////////////////////////////////

void CSAStatusLog::Enable(BOOL bEnable)
{
	m_bEnable = bEnable;
}

/////////////////////////////////////////////////////////////////////////////

//BOOL CSAStatusLog::StatusOut(const char* fmt, ...)
BOOL CSAStatusLog::StatusOut(LPCTSTR fmt, ...)
{
	if (m_csFileName.IsEmpty())
		return FALSE;

	if (!m_bEnable)
		return TRUE;

	if (!AfxIsValidString(fmt, -1))
		return FALSE;

   EnterCriticalSection(&m_crit);

   // parse that string format
   try
   {
	   va_list argptr;
	   va_start(argptr, fmt);
	   _vsntprintf(m_tBuf, TBUF_SIZE, fmt, argptr);
	   va_end(argptr);
   }
   catch (...)
   {
      m_tBuf[0] = 0;
   }

   BOOL bOK = FALSE;

   // output 
   FILE *fp = nullptr;
   _tfopen_s(&fp, m_csFileName, _T("a"));
   
	if (fp)
	{
      if (m_bPrintAppName)
      {
         fwprintf(fp,_T("%s : "), m_csAppName);
      }

      if (m_bPrintTime)
      {
		   CTime ct ; 
		   ct = CTime::GetCurrentTime();
		   fwprintf(fp,_T("%s : "),ct.Format("%H:%M:%S"));
      }

      fwprintf(fp, _T("%s\n"), m_tBuf);
		
      fclose(fp);

      bOK = TRUE;
	}

   LeaveCriticalSection(&m_crit);

	return bOK;
}

BOOL CSAStatusLog::StatusOut( TCHAR* lpszMessage )
{
	if (m_csFileName.IsEmpty())
		return FALSE;

	if (!m_bEnable)
		return TRUE;

	EnterCriticalSection(&m_crit);

	BOOL bOK = FALSE;

	// output 
	FILE *fp = nullptr;
	_tfopen_s(&fp, m_csFileName, _T("a"));

	if (fp)
	{
		if (m_bPrintAppName)
		{
			fwprintf(fp,_T("%s : "), m_csAppName);
		}

		if (m_bPrintTime)
		{
			SYSTEMTIME SystemTime;
			GetLocalTime(&SystemTime);
			CString strTime = _T("");
			strTime.Format(_T("%02d:%02d:%02d.%03d"), SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wMilliseconds);
			fwprintf(fp,_T("%s : "), strTime);
		}

		fwprintf(fp, _T("%s"), lpszMessage);

		fclose(fp);

		bOK = TRUE;
	}

	LeaveCriticalSection(&m_crit);

	return bOK;
}

/////////////////////////////////////////////////////////////////////////////

CString	CSAStatusLog::GetBaseName(const CString &path)
{
	CString out = path;

	int iSlashPos = path.ReverseFind('\\');
	if (iSlashPos !=-1) 
	{
		out = out.Mid(iSlashPos+1);
	}
	else
	{
		iSlashPos = path.ReverseFind('/');
		if (iSlashPos !=-1) 
		{
			out = out.Mid(iSlashPos+1);
		}
	}

   int iDotPos = out.ReverseFind('.');
   if (iDotPos>0)
   {
      out = out.Left(iDotPos);
   }

	return out;
}

/////////////////////////////////////////////////////////////////////////////

CString CSAStatusLog::GetBaseDir(const CString & path)
{
	CString out = _T("");
	int iSlashPos = path.ReverseFind('\\');
	if (iSlashPos !=-1) 
	{
		out = path.Left(iSlashPos);
	} 
	else
	{
		iSlashPos = path.ReverseFind('/');
		if (iSlashPos !=-1) 
		{
			out = path.Left(iSlashPos);
		} 
	}

	return out;
}
