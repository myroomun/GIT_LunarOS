/*
 * HardDisk.c
 *
 *  Created on: 2015. 10. 21.
 *      Author: user
 */

#include "HardDisk.h"

static HDDMANAGER gs_stHDDManager;

BOOL kInitializeHDD( void )
{
	// 뮤텍스 초기화
	kInitializeMutex( &(gs_stHDDManager.stMutex) );

	// 인터럽트 플래그 초기화
	gs_stHDDManager.bPrimaryInterruptOccur = FALSE;
	gs_stHDDManager.bSecondaryInterruptOccur = FALSE;

	// 하드디스크의 컨트롤러 인터럽트 활성화
	kOutPortByte( HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT, 0 );
	kOutPortByte( HDD_PORT_SECONDARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT, 0 );

	// 하드디스크 정보 요청

	if( kReadHDDInformation( TRUE, TRUE, &( gs_stHDDManager.stHDDInformation ) ) == FALSE)
	{
		gs_stHDDManager.bHDDDetected = FALSE;
		gs_stHDDManager.bCanWrite = FALSE;
		return FALSE;
	}

	// 하드디스크가 검색 되어씅면 QEMU 위에서만 설정
	gs_stHDDManager.bHDDDetected = TRUE;
	// QEMU가 처음에 있어서 위치는 0을 받음
	if( kMemCmp( gs_stHDDManager.stHDDInformation.vwModelNumber, "QEMU", 4) == 0  )
	{
		gs_stHDDManager.bCanWrite = TRUE;
	}
	else
	{
		gs_stHDDManager.bCanWrite = FALSE;
	}
	return TRUE;
}

static BYTE kReadHDDStatus( BOOL bPrimary )
{
	if( bPrimary == TRUE )
	{
		return kInPortByte( HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_STATUS );
	}

	return kInPortByte( HDD_PORT_SECONDARYBASE + HDD_PORT_INDEX_STATUS );
}

static BOOL kWaitForHDDNoBusy( BOOL bPrimary )
{
	QWORD qwStartTickCount;
	BYTE bStatus;

	qwStartTickCount = kGetTickCount();

	while( ( kGetTickCount() - qwStartTickCount ) <= HDD_WAITTIME )
	{
		bStatus = kReadHDDStatus( bPrimary );

		// 하드 상태가 busy 상태가 아니면 이미 해지됨
		if( ( bStatus & HDD_STATUS_BUSY ) != HDD_STATUS_BUSY )
		{
			return TRUE;
		}
		kSleep( 1 );
	}

	return FALSE;

}

static BOOL kWaitForHDDReady( BOOL bPrimary )
{
	QWORD qwStartTickCount;
	BYTE bStatus;

	qwStartTickCount = kGetTickCount();

	while( ( kGetTickCount() - qwStartTickCount ) <= HDD_WAITTIME )
	{
		bStatus = kReadHDDStatus( bPrimary );

		if( ( bStatus & HDD_STATUS_READY ) == HDD_STATUS_READY )
		{
			return TRUE;
		}
		kSleep(1);
	}

	return FALSE;
}

void kSetHDDInterruptFlag( BOOL bPrimary, BOOL bFlag )
{
	if( bPrimary == TRUE )
	{
		gs_stHDDManager.bPrimaryInterruptOccur = bFlag;
	}
	else
	{
		gs_stHDDManager.bSecondaryInterruptOccur = bFlag;
	}
}


static BOOL kWaitForHDDInterrupt( BOOL bPrimary )
{
	QWORD qwTickCount;
	qwTickCount = kGetTickCount();

	while( kGetTickCount() - qwTickCount <= HDD_WAITTIME )
	{
		if( ( bPrimary == TRUE  ) && ( gs_stHDDManager.bPrimaryInterruptOccur == TRUE) )
		{
			return TRUE;
		}
		else if( ( bPrimary == FALSE ) && ( gs_stHDDManager.bSecondaryInterruptOccur == TRUE ))
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL kReadHDDInformation( BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation )
{
	WORD wPortBase;
	QWORD qwLastTickCount;
	BYTE bStatus;
	BYTE bDriveFlag;
	int i;
	WORD wTemp;
	BOOL bWaitResult;

	if( bPrimary == TRUE )
	{
		wPortBase = HDD_PORT_PRIMARYBASE;
	}
	else
	{
		wPortBase = HDD_PORT_SECONDARYBASE;
	}

	kLock( &(gs_stHDDManager.stMutex) );

	if( kWaitForHDDNoBusy( bPrimary ) == FALSE )
	{
		kUnlock( &(gs_stHDDManager.stMutex) );
		return FALSE;
	}

	// 마스터면 드라이버 LBA만 설정 아니라면 LBA + 슬레이브 알림
	// 이 경우 읽기 전에는 LBA값을 참고하지 않으므로 설정하지 않아도 괜찮음
	if( bMaster == TRUE )
	{
		bDriveFlag = HDD_DRIVEANDHEAD_LBA;
	}
	else
	{
		bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
	}
	kOutPortByte( wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag );

	// 커맨드 전송 후 인터럽트 대기
	// 커맨드 포트에  받을 준비가 되어있는가?
	if( kWaitForHDDReady( bPrimary ) == FALSE )
	{
		kUnlock( &(gs_stHDDManager.stMutex ) );
		return FALSE;
	}

	// 커맨드 포트에 정보 전송 전 플래그 초기화
	kSetHDDInterruptFlag( bPrimary, FALSE );
	// 커맨드 포트에 정보 전달 요청
	kOutPortByte( wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_IDENTIFY );

	bWaitResult = kWaitForHDDInterrupt( bPrimary );
	bStatus = kReadHDDStatus(bPrimary);

	if( ( bWaitResult == FALSE ) || ( ( bStatus & HDD_STATUS_ERROR ) == HDD_STATUS_ERROR ) )
	{
		kUnlock( &(gs_stHDDManager.stMutex) );
		return FALSE;
	}

	// 인터럽트가 왔으므로 읽어도 된다.
	// 2bytes * 512/2 = 512 bytes per 1 sector
	for( i = 0 ; i < 512 / 2 ; i++ )
	{
		( ( WORD* ) pstHDDInformation )[ i ] = kInPortWord( wPortBase + HDD_PORT_INDEX_DATA );
	}

	// 원형은 kSwapByteInWord(vwModeNumber, 바꿀 갯수)
	kSwapByteInWord( pstHDDInformation->vwModelNumber, sizeof(pstHDDInformation->vwModelNumber)/2);
	kSwapByteInWord( pstHDDInformation->vwSerialNumber, sizeof(pstHDDInformation->vwSerialNumber)/2);

	kUnlock( &(gs_stHDDManager.stMutex ) );
	return TRUE;

}

static void kSwapByteInWord( WORD* pwData, int iWordCount )
{
	int i;
	WORD wTemp;

	for( i = 0 ; i < iWordCount ; i++ )
	{
		wTemp = pwData[i];
		pwData[i] = (wTemp >> 8) | ( wTemp << 8 );
	}
}

// 최대 256개의 섹터(섹터당 512bytes)를 읽을 수 있음 실제로 읽은 섹터수 반환
int kReadHDDSector( BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer )
{
	WORD wPortBase;
	int i, j;
	BYTE bDriveFlag;
	BOOL bStatus;
	long lReadCount = 0;
	BOOL bWaitResult;

	if( ( gs_stHDDManager.bHDDDetected == FALSE ) || ( iSectorCount <= 0 ) || ( 256 < iSectorCount ) || ( ( dwLBA + iSectorCount ) >= gs_stHDDManager.stHDDInformation.dwTotalSectors))
	{
		return 0;
	}

	if( bPrimary == TRUE )
	{
		wPortBase = HDD_PORT_PRIMARYBASE;
	}
	else
	{
		wPortBase = HDD_PORT_SECONDARYBASE;
	}

	kLock( &( gs_stHDDManager.stMutex ) );

	// 수행중인 커맨드가 있나?
	if( kWaitForHDDNoBusy( bPrimary ) == FALSE )
	{
		kUnlock( &(gs_stHDDManager.stMutex ) );
		return FALSE;
	}

	kOutPortByte( wPortBase + HDD_PORT_INDEX_SECTORCOUNT, iSectorCount );
	kOutPortByte( wPortBase + HDD_PORT_INDEX_SECTORNUMBER, dwLBA );
	kOutPortByte( wPortBase + HDD_PORT_INDEX_CYLINDERLSB, dwLBA >> 8 );
	kOutPortByte( wPortBase + HDD_PORT_INDEX_CYLINDERMSB, dwLBA >> 16 );
	// 아래는 Flag와 같이 dwLBA를 설정해야 함
	if( bMaster == TRUE )
	{
		bDriveFlag = HDD_DRIVEANDHEAD_LBA;
	}
	else
	{
		bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
	}
	kOutPortByte( wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag | ( ( dwLBA >> 24 ) & 0x0F ) );

	// 커맨드를 받을 준비가 되어있는가?
	if( kWaitForHDDReady( bPrimary) == FALSE )
	{
		kUnlock( &(gs_stHDDManager.stMutex ) );
		return FALSE;
	}

	// 인터럽트 들어가기전 초기화
	kSetHDDInterruptFlag( bPrimary, FALSE );

	kOutPortByte( wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_READ );

	// 인터럽트 대기후 수신
	for( i = 0 ; i < iSectorCount ; i++ )
	{
		// 섹터마다 에러체크
		bStatus = kReadHDDStatus( bPrimary );
		if( ( bStatus & HDD_STATUS_ERROR ) == HDD_STATUS_ERROR )
		{
			kPrintf("Error Occur\n");
			kUnlock( &(gs_stHDDManager.stMutex ) );
			return i; // 어느 섹터하다가 망가짐? (이건 전에 처리했던것을 뜻한다. 즉 i+1을 할 필요가 없음)
		}

		// 데이터 준비되어 있나요?
		if( ( bStatus & HDD_STATUS_DATAREQUEST ) != HDD_STATUS_DATAREQUEST )
		{
			// 아니라면
			// 인터럽트를 기다립시다.
			// 매 섹터마다 인터럽트가 발생하는것이 아닌, 준비가 되어있을때 딱 한번 발생한다. 즉 처음에..
			bWaitResult = kWaitForHDDInterrupt( bPrimary );
			// 인터럽트를 초기화 해줘야함 썼으니까!
			kSetHDDInterruptFlag( bPrimary, FALSE );

			if( bWaitResult == FALSE )
			{
				kPrintf("Interrupt Not Occur\n");
				kUnlock( &(gs_stHDDManager.stMutex ) );
				return FALSE;
			}
		}
		for( j = 0 ; j < 512 / 2 ; j++ )
		{
			( ( WORD*)pcBuffer )[lReadCount++] = kInPortWord( wPortBase + HDD_PORT_INDEX_DATA );
		}
	}
	kUnlock( &(gs_stHDDManager.stMutex ) );
	return i;
}
int kWriteHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer )
{
	WORD wPortBase;
	WORD wTemp;
	int i, j;
	BYTE bDriveFlag;
	BOOL bStatus;
	long lReadCount = 0;
	BOOL bWaitResult;

	if( ( gs_stHDDManager.bHDDDetected == FALSE ) || ( iSectorCount <= 0 ) || ( 256 < iSectorCount ) || ( ( dwLBA + iSectorCount ) >= gs_stHDDManager.stHDDInformation.dwTotalSectors))
	{
		return 0;
	}

	if( bPrimary == TRUE )
	{
		wPortBase = HDD_PORT_PRIMARYBASE;
	}
	else
	{
		wPortBase = HDD_PORT_SECONDARYBASE;
	}

	kLock( &( gs_stHDDManager.stMutex ) );

	// 수행중인 커맨드가 있나?
	if( kWaitForHDDNoBusy( bPrimary ) == FALSE )
	{
		kUnlock( &(gs_stHDDManager.stMutex ) );
		return FALSE;
	}

	kOutPortByte( wPortBase + HDD_PORT_INDEX_SECTORCOUNT, iSectorCount );
	kOutPortByte( wPortBase + HDD_PORT_INDEX_SECTORNUMBER, dwLBA );
	kOutPortByte( wPortBase + HDD_PORT_INDEX_CYLINDERLSB, dwLBA >> 8 );
	kOutPortByte( wPortBase + HDD_PORT_INDEX_CYLINDERMSB, dwLBA >> 16 );
	// 아래는 Flag와 같이 dwLBA를 설정해야 함
	if( bMaster == TRUE )
	{
		bDriveFlag = HDD_DRIVEANDHEAD_LBA;
	}
	else
	{
		bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
	}
	kOutPortByte( wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag | ( ( dwLBA >> 24 ) & 0x0F ) );

	// 커맨드를 받을 준비가 되어있는가?
	if( kWaitForHDDReady( bPrimary) == FALSE )
	{
		kUnlock( &(gs_stHDDManager.stMutex ) );
		return FALSE;
	}

	// 인터럽트 들어가기전 초기화
	kSetHDDInterruptFlag( bPrimary, FALSE );

	kOutPortByte( wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_WRITE );

	while(1)
	{
		bStatus = kReadHDDStatus( bPrimary );
		// 뜬금포 에러 발생시 종료
		if( ( bStatus & HDD_STATUS_ERROR ) == HDD_STATUS_ERROR )
		{
			kUnlock( &(gs_stHDDManager.stMutex ) );
			return 0;
		}

		// Data request로 Write가능하면 break
		if( ( bStatus & HDD_STATUS_DATAREQUEST ) == HDD_STATUS_DATAREQUEST )
		{
			break;
		}
		kSleep( 1 );
	}

	// 참조 : Data Request 받은 후  Write 1sector
	// 인터럽트 대기후 수신
	for( i = 0 ; i < iSectorCount ; i++ )
	{
		kSetHDDInterruptFlag(bPrimary, FALSE );
		for( j = 0 ; j < 512 / 2 ; j++ )
		{
			kOutPortWord( wPortBase + HDD_PORT_INDEX_DATA, (( WORD* )pcBuffer)[lReadCount++]);
		}
		bStatus = kReadHDDStatus( bPrimary );
		// 뜬금포 에러 발생시 종료
		if( ( bStatus & HDD_STATUS_ERROR ) == HDD_STATUS_ERROR )
		{
			kUnlock( &(gs_stHDDManager.stMutex ) );
			return i; // 이제까지 쓴 섹터
		}
		// 데이터 들어간 후 인터럽트가 발생해야됨
		if( ( ( bStatus & HDD_STATUS_DATAREQUEST ) != HDD_STATUS_DATAREQUEST ) )
		{
			bWaitResult = kWaitForHDDInterrupt( bPrimary );
			kSetHDDInterruptFlag( bPrimary, FALSE );
			if( bWaitResult == FALSE )
			{
				kUnlock( &(gs_stHDDManager.stMutex ) );
				return FALSE;
			}
		}
	}
	kUnlock( &(gs_stHDDManager.stMutex ) );
	return i;
}


