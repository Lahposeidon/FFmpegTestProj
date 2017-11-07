#include "StdAfx.h"
#include "DiskInformation.h"

//-----------------------------------------------------------------------------
// Define global buffers.
//-----------------------------------------------------------------------------
BYTE IdOutCmd[sizeof (SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1];


CDiskInformation::CDiskInformation(void)
{
}

CDiskInformation::~CDiskInformation(void)
{
}

//-----------------------------------------------------------------------------
// DoIDENTIFY
//-----------------------------------------------------------------------------
BOOL CDiskInformation::DoIDENTIFY(HANDLE hPhysicalDriveIOCTL, PSENDCMDINPARAMS pSCIP, 
								  PSENDCMDOUTPARAMS pSCOP, BYTE bIDCmd, BYTE bDriveNum, 
								  PDWORD lpcbBytesReturned)
{
   // Set up data structures for IDENTIFY command.
   pSCIP->cBufferSize = IDENTIFY_BUFFER_SIZE;
   pSCIP->irDriveRegs.bFeaturesReg = 0;
   pSCIP->irDriveRegs.bSectorCountReg = 1;
   pSCIP->irDriveRegs.bSectorNumberReg = 1;
   pSCIP->irDriveRegs.bCylLowReg = 0;
   pSCIP->irDriveRegs.bCylHighReg = 0;

   // Compute the drive number.
   pSCIP->irDriveRegs.bDriveHeadReg = 0xA0 | ((bDriveNum & 1) << 4);

   // The command can either be IDE identify or ATAPI identify.
   pSCIP->irDriveRegs.bCommandReg = bIDCmd;
   pSCIP->bDriveNumber = bDriveNum;
   pSCIP->cBufferSize = IDENTIFY_BUFFER_SIZE;

   return(DeviceIoControl(hPhysicalDriveIOCTL, 
	                      DFP_RECEIVE_DRIVE_DATA,
                          (LPVOID)pSCIP,
                          sizeof(SENDCMDINPARAMS) - 1,
                          (LPVOID)pSCOP,
                          sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1,
                          lpcbBytesReturned, NULL));
}

//-----------------------------------------------------------------------------
// ReadPhysicalDriveInNT
//-----------------------------------------------------------------------------
BOOL CDiskInformation::ReadPhysicalDriveInNT(DWORD diskdata [256])
{
	BOOL bDone = FALSE;
	int nDrive = 0;

	for(nDrive = 0; nDrive < MAX_IDE_DRIVES; nDrive++)
	{
		HANDLE hPhysicalDriveIOCTL = 0;
		TCHAR szdriveName[MAX_DRIVENAME_SIZE];

//		_stprintf_s(szdriveName, _countof(szdriveName), _T("\\\\.\\PhysicalDrive%d"), nDrive);
		_stprintf_s(szdriveName, _countof(szdriveName), _T("\\\\.\\%c:"),'C'+nDrive);

		// Windows NT, Windows 2000, must have admin rights
		hPhysicalDriveIOCTL = CreateFile(szdriveName,
										 GENERIC_READ | GENERIC_WRITE, 
									     FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
										 OPEN_EXISTING, 0, NULL);
		if(hPhysicalDriveIOCTL != INVALID_HANDLE_VALUE)
		{
			DWORD cbBytesReturned = 0;
			GETVERSIONOUTPARAMS VersionParams;
			
			memset ((void*) &VersionParams, 0, sizeof(VersionParams));

			DeviceIoControl(hPhysicalDriveIOCTL, DFP_GET_VERSION, NULL, 0,
							&VersionParams, sizeof(VersionParams), &cbBytesReturned, NULL);

			// If there is a IDE device at number "i" issue commands to the device
			// SSD 사용하는 경우도 정상 동작하도록 IDE 체크 제거 by changhee@mobileleader.com 2011/11/29
			// if(VersionParams.bIDEDeviceMap > 0)
			{
				BYTE bIDCmd = 0;   // IDE or ATAPI IDENTIFY cmd
				SENDCMDINPARAMS  scip;

				// Now, get the ID sector for all IDE devices in the system.
				// If the device is ATAPI use the IDE_ATAPI_IDENTIFY command,
				// otherwise use the IDE_ATA_IDENTIFY command
				bIDCmd = (VersionParams.bIDEDeviceMap >> nDrive & 0x10) ? IDE_ATAPI_IDENTIFY : IDE_ATA_IDENTIFY;
				memset(&scip, 0, sizeof(scip));
				memset(IdOutCmd, 0, sizeof(IdOutCmd));

				if(DoIDENTIFY(hPhysicalDriveIOCTL, &scip, (PSENDCMDOUTPARAMS)&IdOutCmd, 
                              (BYTE)bIDCmd, (BYTE)nDrive, &cbBytesReturned))
				{
					USHORT *pIdSector = (USHORT *)((PSENDCMDOUTPARAMS)IdOutCmd)->bBuffer;
					for (int ijk = 0; ijk < 256; ijk++)
					{
						diskdata[ijk] = pIdSector[ijk];
					}

					bDone = TRUE;
				}
			 }

			CloseHandle(hPhysicalDriveIOCTL);
			if(TRUE == bDone)
			{
				break;
			}
		}
	}
	
	return bDone;
}

//-----------------------------------------------------------------------------
// ConvertToString
//-----------------------------------------------------------------------------
void CDiskInformation::ConvertToString(TCHAR retval[256], DWORD diskdata[256],
					                   int firstIndex, int lastIndex)
{
	int position = 0;

	//  each integer has two characters stored in it backwards
	for(int index = firstIndex; index <= lastIndex; index++)
	{
		//  get high byte for 1st character
		retval[position] = (char)(diskdata[index] / 256);
		if(isspace (retval[position]) == 0)
			position++;

		//  get low byte for 2nd character
		retval[position] = (char)(diskdata[index] % 256);
		if(isspace (retval[position]) == 0)
			position++;
	}	

	retval[position] = 0;
	for(int index = position - 1; index > 0 && 32 == retval[index]; index--)
		retval[index] = 0;
}

CString CDiskInformation::GetDiskSerial()
{
	CString strDiskSerial;
	DWORD DiskInfo[MAX_DISKINFO_SIZE];
	TCHAR szDiskSerial[MAX_DISKSERIAL_SIZE];

	ZeroMemory(DiskInfo, sizeof(DiskInfo));
	ZeroMemory(szDiskSerial, sizeof(szDiskSerial));
	
	if(TRUE == ReadPhysicalDriveInNT(DiskInfo))
	{
		ConvertToString(szDiskSerial, DiskInfo, DISKSERIAL_FIRSTINDEX, DISKSERIAL_LASTINDEX);
		strDiskSerial = szDiskSerial;
	}
	
	return strDiskSerial;
}