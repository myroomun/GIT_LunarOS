/*
 * FileSystem.c
 *
 *  Created on: 2015. 10. 22.
 *      Author: user
 */

#include "FileSystem.h"
#include "HardDisk.h"
#include "DynamicMemory.h"


static FILESYSTEMMANAGER gs_stFileSystemManager;
static BYTE gs_vbTempBuffer[ FILESYSTEM_SECTORPERCLUSTER * 512 ]; // 한 클러스터의 정보를 담을 수 있을 정도의 크기를 예약

fReadHDDInformation gs_pfReadHDDInformation = NULL;
fReadHDDSector gs_pfReadHDDSector = NULL;
fWriteHDDSector gs_pfWriteHDDSector = NULL;

BOOL kInitializeFileSystem( void )
{
	kMemSet( &gs_stFileSystemManager, 0, sizeof(gs_stFileSystemManager));
	kInitializeMutex( &(gs_stFileSystemManager.stMutex ));

	if( kInitializeHDD() == TRUE )
	{
		gs_pfReadHDDInformation = kReadHDDInformation;
		gs_pfReadHDDSector = kReadHDDSector;
		gs_pfWriteHDDSector = kWriteHDDSector;
	}
	else
	{
		return FALSE;
	}

	if( kMount() == FALSE )
	{
		return FALSE;
	}
	return TRUE;
}

// 현재 하드디스크 정보를 읽어 Mount 시도, 성공시 매니저 데이터 세팅
BOOL kMount( void )
{
	MBR* pstMBR;

	kLock( &( gs_stFileSystemManager.stMutex ) );

	// MBR 읽기
	if( gs_pfReadHDDSector( TRUE, TRUE, 0, 1, gs_vbTempBuffer) == FALSE )
	{
		kUnlock( &(gs_stFileSystemManager.stMutex ) );
		return FALSE;
	}

	// 시그니처 확인
	pstMBR = (MBR*)gs_vbTempBuffer;
	if( pstMBR->dwSignature != FILESYSTEM_SIGNATURE )
	{
		kUnlock( &(gs_stFileSystemManager.stMutex));
		return FALSE;
	}

	gs_stFileSystemManager.bMounted = TRUE;

	// 각 영역의 시작 LBA 어드레스와 섹터 수를 계산 후 매니저에 대입
	gs_stFileSystemManager.dwReservedSectorCount = pstMBR->dwReservedSectorCount;
	gs_stFileSystemManager.dwClusterLinkAreaStartAddress = pstMBR->dwReservedSectorCount + 1; // 1은 MBR
	gs_stFileSystemManager.dwClusterLinkAreaSize = pstMBR->dwClusterLinkSectorCount;
	gs_stFileSystemManager.dwDataAreaStartAddress = pstMBR->dwReservedSectorCount + pstMBR->dwClusterLinkSectorCount+ 1; // Base = 0 이므로 0 + MBR(1) Reserved + ClusterLink
	gs_stFileSystemManager.dwTotalClusterCount = pstMBR->dwTotalClusterCount;

	kUnlock( &( gs_stFileSystemManager.stMutex ) );
	return TRUE;

}

// 파일시스템을 생성하는 함수
BOOL kFormat( void )
{
	HDDINFORMATION* pstHDD;
	MBR* pstMBR;
	DWORD dwTotalSectorCount, dwRemainSectorCount;
	DWORD dwMaxClusterCount, dwClusterCount;
	DWORD dwClusterLinkSectorCount;
	DWORD i;

	kLock( &( gs_stFileSystemManager.stMutex ) );

	pstHDD = ( HDDINFORMATION* ) gs_vbTempBuffer;
	if( gs_pfReadHDDInformation( TRUE, TRUE, pstHDD ) == FALSE )
	{
		kUnlock( &(gs_stFileSystemManager.stMutex));
		return FALSE;
	}
	dwTotalSectorCount = pstHDD->dwTotalSectors;

	// 현재 최대 클러스터 수 계산
	dwMaxClusterCount = dwTotalSectorCount / FILESYSTEM_SECTORPERCLUSTER;

	// 이 클러스터들을 나타낼 수 있는 링크테이블 수를 포함한 <섹터수> 계산
	// 즉, 전체 클러스터 수에서 한 섹터가 나타낼 수 있는 링크수가 128개이기 때문에 나눈뒤 반올림
	dwClusterLinkSectorCount = (dwMaxClusterCount + 127) / 128;

	// 대략적으로 남은 섹터 카운터는 전체 - MBR - 링크섹터임
	dwRemainSectorCount = dwTotalSectorCount - dwClusterLinkSectorCount - 1;

	// 데이터로 쓸 클러스터는 남은 섹터 / 8
	dwClusterCount = dwRemainSectorCount / FILESYSTEM_SECTORPERCLUSTER;

	// 실제 사용 가능한 클러스터 수에 맞춰 계산
	dwClusterLinkSectorCount = ( dwClusterCount + 127 ) / 128;

	// 이러한 정보들을 이용하여 MBR에 덮어 씌운다.
	// MBR 읽기
	if( gs_pfReadHDDSector( TRUE, TRUE, 0, 1, gs_vbTempBuffer) == FALSE )
	{
		kUnlock( &( gs_stFileSystemManager.stMutex ) );
		return FALSE;
	}
    pstMBR = ( MBR* ) gs_vbTempBuffer;
    kMemSet( pstMBR->vstPartition, 0, sizeof( pstMBR->vstPartition ) );
    pstMBR->dwSignature = FILESYSTEM_SIGNATURE;
    pstMBR->dwReservedSectorCount = 0;
    pstMBR->dwClusterLinkSectorCount = dwClusterLinkSectorCount;
    pstMBR->dwTotalClusterCount = dwClusterCount;

    // MBR 영역에 1 섹터를 씀
    if( gs_pfWriteHDDSector( TRUE, TRUE, 0, 1, gs_vbTempBuffer ) == FALSE )
    {
        // 동기화 처리
        kUnlock( &( gs_stFileSystemManager.stMutex ) );
        return FALSE;
    }
	// MBR에서 데이터링크부터 초기화
	kMemSet( gs_vbTempBuffer, 0, 512 );
	// 섹터 1번(링크인덱스 시작) 부터 루트 디렉토리까지
	for( i = 0 ; i < (dwClusterLinkSectorCount + FILESYSTEM_SECTORPERCLUSTER ) ; i++ )
	{
		if( i == 0 )
		{
			( ( DWORD* ) ( gs_vbTempBuffer) )[ 0 ] = FILESYSTEM_LASTCLUSTER;
		}
		else
		{
			( ( DWORD* ) ( gs_vbTempBuffer) )[ 0 ] = FILESYSTEM_FREECLUSTER;
		}

		// 1섹터씩 저장
		if( gs_pfWriteHDDSector( TRUE, TRUE, i + 1, 1, gs_vbTempBuffer ) == FALSE )
		{
			kUnlock( &( gs_stFileSystemManager.stMutex ) );
			return FALSE;
		}
	}

	kUnlock( &( gs_stFileSystemManager.stMutex ) );
	return TRUE;
}

BOOL kGetHDDInformation( HDDINFORMATION* pstInformation )
{
	BOOL bResult;

	kLock( &( gs_stFileSystemManager.stMutex ) );

	bResult = gs_pfReadHDDInformation( TRUE, TRUE, pstInformation );

	kUnlock( &( gs_stFileSystemManager.stMutex ) );

	return bResult;
}

// 섹터 오프셋을 인자로 받음
BOOL kReadClusterLinkTable( DWORD dwOffset, BYTE* pbBuffer )
{
	return gs_pfReadHDDSector( TRUE, TRUE, gs_stFileSystemManager.dwClusterLinkAreaStartAddress + dwOffset, 1, pbBuffer );
}

// 클러스터 링크 테이블 내의 오프셋에 한 섹터를 씀
BOOL kWriteClusterLinkTable( DWORD dwOffset, BYTE* pbBuffer )
{
	return gs_pfWriteHDDSector( TRUE, TRUE, gs_stFileSystemManager.dwClusterLinkAreaStartAddress + dwOffset, 1, pbBuffer );
}

BOOL kReadCluster( DWORD dwOffset, BYTE* pbBuffer )
{
	return gs_pfReadHDDSector( TRUE, TRUE, ( dwOffset * FILESYSTEM_SECTORPERCLUSTER) + gs_stFileSystemManager.dwDataAreaStartAddress, FILESYSTEM_SECTORPERCLUSTER, pbBuffer );
}
BOOL kWriteCluster( DWORD dwOffset, BYTE* pbBuffer )
{
	return gs_pfWriteHDDSector( TRUE, TRUE, ( dwOffset * FILESYSTEM_SECTORPERCLUSTER) + gs_stFileSystemManager.dwDataAreaStartAddress, FILESYSTEM_SECTORPERCLUSTER, pbBuffer );
}

DWORD kFindFreeCluster( void )
{
	DWORD dwLinkCountInSector;
	DWORD dwLastSectorOffset, dwCurrentSectorOffset;
	DWORD i, j;

	if( gs_stFileSystemManager.bMounted == FALSE )
	{
		return FILESYSTEM_LASTCLUSTER;
	}

	// 마지막 찾아봤던 섹터 오프셋을 얻는다.
	dwLastSectorOffset = gs_stFileSystemManager.dwLastAllocatedClusterLinkSectorOffset;

	// 링크데이터의

	for( i = 0 ; i < gs_stFileSystemManager.dwClusterLinkAreaSize ; i++ )
	{
		// 찾는데 클러스터 마지막에 왔다면
		if( ( dwLastSectorOffset + i ) == (gs_stFileSystemManager.dwClusterLinkAreaSize - 1 ) )
		{
			// 클러스터가 128개 단위로 1섹터에 쓰이는데 128개 미만이라면 나머지를 검색할 섹터수에 넣는다.
			dwLinkCountInSector = gs_stFileSystemManager.dwTotalClusterCount % 128;
		}
		else
		{
			dwLinkCountInSector = 128;
		}

		// dwClusterLinkAreaSize를 넘어갈 수 있으므로
		dwCurrentSectorOffset = ( dwLastSectorOffset + i ) % gs_stFileSystemManager.dwClusterLinkAreaSize;

		if( kReadClusterLinkTable( dwCurrentSectorOffset, gs_vbTempBuffer ) == FALSE )
		{
			return FILESYSTEM_LASTCLUSTER;
		}

		for( j = 0 ; j < dwLinkCountInSector ; j++ )
		{
			if( ( ( DWORD* ) gs_vbTempBuffer )[ j ] == FILESYSTEM_FREECLUSTER )
			{
				break;
			}
		}
	}

	// 찾았다면 클러스터 인덱스를 반환
	if( j != dwLinkCountInSector )
	{
		gs_stFileSystemManager.dwLastAllocatedClusterLinkSectorOffset = dwCurrentSectorOffset;

		// 링크 테이블 오프셋 * 128 + j
		return ( dwCurrentSectorOffset * 128 ) + j;
	}

	return FILESYSTEM_LASTCLUSTER;
}

BOOL kSetClusterLinkData( DWORD dwClusterIndex, DWORD dwData )
{
	DWORD dwSectorOffset;

	if( gs_stFileSystemManager.bMounted == FALSE )
	{
		return FALSE;
	}

	// 몇번째 섹터?
	dwSectorOffset = dwClusterIndex / 128;

	if( kReadClusterLinkTable( dwSectorOffset, gs_vbTempBuffer ) == FALSE )
	{
		return FALSE;
	}

	( ( DWORD* ) gs_vbTempBuffer )[ dwClusterIndex % 128 ] = dwData;

	if( kWriteClusterLinkTable( dwSectorOffset, gs_vbTempBuffer ) == FALSE )
	{
		return FALSE;
	}
	return TRUE;
}

BOOL kGetClutserLinkData( DWORD dwClusterIndex, DWORD* pdwData )
{
	DWORD dwSectorOffset;

	if( gs_stFileSystemManager.bMounted == FALSE )
	{
		return FALSE;
	}

	dwSectorOffset = dwClusterIndex / 128;

	if( dwSectorOffset > gs_stFileSystemManager.dwClusterLinkAreaSize )
	{
		return FALSE;
	}

	if( kReadClusterLinkTable( dwSectorOffset, gs_vbTempBuffer ) == FALSE )
	{
		return FALSE;
	}

	*pdwData = ( ( DWORD* ) gs_vbTempBuffer )[ dwClusterIndex % 128 ];

	return TRUE;
}

// 루트 클러스터 안에 있는 빈 디렉터리 검색
int kFindFreeDirectoryEntry( void )
{
	DIRECTORYENTRY* pstEntry;
	int i;

	if( gs_stFileSystemManager.bMounted == FALSE )
	{
		return -1;
	}

	if( kReadCluster(0, gs_vbTempBuffer) == FALSE )
	{
		return -1;
	}

	pstEntry = (DIRECTORYENTRY*)gs_vbTempBuffer;

	for( i = 0 ; i < FILESYSTEM_MAXDIRECTORYENTRYCOUNT ; i++ )
	{
		if( pstEntry[ i ].dwStartClusterIndex == 0 )
		{
			return i;
		}
	}
	return -1;
}

BOOL kSetDirectoryEntryData( int iIndex, DIRECTORYENTRY* pstEntry )
{
	DIRECTORYENTRY* pstRootEntry;

	if( ( gs_stFileSystemManager.bMounted == FALSE) || ( iIndex < 0 ) || ( iIndex >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT ) )
	{
		return FALSE;
	}

	if( kReadCluster(0, gs_vbTempBuffer ) == FALSE )
	{
		return FALSE;
	}

	pstRootEntry = (DIRECTORYENTRY*) gs_vbTempBuffer;

	// 정보를 씀
	kMemCpy(pstRootEntry + iIndex, pstEntry, sizeof( DIRECTORYENTRY ) );

	if( kWriteCluster( 0, gs_vbTempBuffer ) == FALSE )
	{
		return FALSE;
	}
	return TRUE;
}

BOOL kGetDirectoryEntryData( int iIndex, DIRECTORYENTRY* pstEntry )
{
	DIRECTORYENTRY* pstRootEntry;

	if( ( gs_stFileSystemManager.bMounted == FALSE) || ( iIndex < 0 ) || ( iIndex >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT ) )
	{
		return FALSE;
	}

	if( kReadCluster( 0, gs_vbTempBuffer ) == FALSE )
	{
		return FALSE;
	}

	pstRootEntry = (DIRECTORYENTRY*)gs_vbTempBuffer;
	kMemCpy( pstEntry, pstRootEntry + iIndex, sizeof( DIRECTORYENTRY ) );
	return TRUE;
}

int kFindDirectoryEntry( const char* pcFileName, DIRECTORYENTRY* pstEntry )
{
	DIRECTORYENTRY* pstRootEntry;
	int i;
	int iLength;

	if( gs_stFileSystemManager.bMounted == FALSE )
	{
		return -1;
	}

	if( kReadCluster( 0, gs_vbTempBuffer ) == FALSE )
	{
		return -1;
	}

	iLength = kStrLen( pcFileName );

	pstRootEntry = ( DIRECTORYENTRY* )gs_vbTempBuffer;

	for( i = 0 ; i < FILESYSTEM_MAXDIRECTORYENTRYCOUNT ; i++ )
	{
		if( kMemCmp( pstRootEntry[ i ].vcFileName, pcFileName, iLength ) == 0 )
		{
			// 찾으면 그 엔트리를 복사해서 넘김
			kMemCpy( pstEntry, pstRootEntry + i, sizeof(DIRECTORYENTRY));
			return i;
		}
	}
	return -1;
}

void kGetFileSystemInformation( FILESYSTEMMANAGER* pstManager )
{
	kMemCpy( pstManager, &gs_stFileSystemManager, sizeof( gs_stFileSystemManager ) );
}
