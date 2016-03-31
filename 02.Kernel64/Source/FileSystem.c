/*
 * FileSystem.c
 *
 *  Created on: 2015. 10. 22.
 *      Author: user
 */

#include "FileSystem.h"
#include "HardDisk.h"
#include "DynamicMemory.h"
#include "Task.h"
#include "Utility.h"


static FILESYSTEMMANAGER gs_stFileSystemManager;
static BYTE gs_vbTempBuffer[ FILESYSTEM_SECTORSPERCLUSTER * 512 ]; // 한 클러스터의 정보를 담을 수 있을 정도의 크기를 예약

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

	gs_stFileSystemManager.pstHandlePool = (FILE*) kAllocateMemory(FILESYSTEM_HANDLE_MAXCOUNT * sizeof(FILE));

	if( gs_stFileSystemManager.pstHandlePool == NULL )
	{
		gs_stFileSystemManager.bMounted = FALSE;
		return FALSE;
	}

	kMemSet( gs_stFileSystemManager.pstHandlePool , 0 , FILESYSTEM_HANDLE_MAXCOUNT * sizeof( FILE ));
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
	dwMaxClusterCount = dwTotalSectorCount / FILESYSTEM_SECTORSPERCLUSTER;

	// 이 클러스터들을 나타낼 수 있는 링크테이블 수를 포함한 <섹터수> 계산
	// 즉, 전체 클러스터 수에서 한 섹터가 나타낼 수 있는 링크수가 128개이기 때문에 나눈뒤 반올림
	dwClusterLinkSectorCount = (dwMaxClusterCount + 127) / 128;

	// 대략적으로 남은 섹터 카운터는 전체 - MBR - 링크섹터임
	dwRemainSectorCount = dwTotalSectorCount - dwClusterLinkSectorCount - 1;

	// 데이터로 쓸 클러스터는 남은 섹터 / 8
	dwClusterCount = dwRemainSectorCount / FILESYSTEM_SECTORSPERCLUSTER;

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
	for( i = 0 ; i < (dwClusterLinkSectorCount + FILESYSTEM_SECTORSPERCLUSTER ) ; i++ )
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
	return gs_pfReadHDDSector( TRUE, TRUE, ( dwOffset * FILESYSTEM_SECTORSPERCLUSTER) + gs_stFileSystemManager.dwDataAreaStartAddress, FILESYSTEM_SECTORSPERCLUSTER, pbBuffer );
}
BOOL kWriteCluster( DWORD dwOffset, BYTE* pbBuffer )
{
	return gs_pfWriteHDDSector( TRUE, TRUE, ( dwOffset * FILESYSTEM_SECTORSPERCLUSTER) + gs_stFileSystemManager.dwDataAreaStartAddress, FILESYSTEM_SECTORSPERCLUSTER, pbBuffer );
}

static DWORD kFindFreeCluster( void )
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
		// 찾았다면 클러스터 인덱스를 반환
		if( j != dwLinkCountInSector )
		{
			gs_stFileSystemManager.dwLastAllocatedClusterLinkSectorOffset = dwCurrentSectorOffset;
				// 링크 테이블 오프셋 * 128 + j
			return ( dwCurrentSectorOffset * 128 ) + j;
		}
	}



	return FILESYSTEM_LASTCLUSTER;
}

static BOOL kSetClusterLinkData( DWORD dwClusterIndex, DWORD dwData )
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
// 한개의 클러스터는 4k
// 한개의 섹션은 1k
// 한개의 섹션은 Cluster Index 128개를 담을 수 있다.
// 몇번째 섹션에 있는가? 섹션 안에 몇번째에 위치하는가?
BOOL kGetClusterLinkData( DWORD dwClusterIndex, DWORD* pdwData )
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

	//gs_vbTempBuffer에는 1024바이트가 들어감
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

// 고수준 함수

static void* kAllocateFileDirectoryHandle( void )
{
	int i;
	FILE* pstFile;

	pstFile = gs_stFileSystemManager.pstHandlePool;
	for( i = 0 ; i < FILESYSTEM_HANDLE_MAXCOUNT ; i++ )
	{
		if( pstFile->bType == FILESYSTEM_TYPE_FREE )
		{
			pstFile->bType = FILESYSTEM_TYPE_FILE;
			return pstFile;
		}
		pstFile++;
	}

	return NULL;
}

static void kFreeFileDirectoryHandle( FILE* pstFile )
{
	kMemSet( pstFile, 0, sizeof( FILE ) );

	pstFile->bType = FILESYSTEM_TYPE_FREE;
}

static BOOL kCreateFile( const char* pcFileName, DIRECTORYENTRY* pstEntry, int* piDirectoryEntryIndex )
{
	DWORD dwCluster;

	dwCluster = kFindFreeCluster();

	// 빈 클러스터를 찾아 할당된것으로 설정
	if( ( dwCluster == FILESYSTEM_LASTCLUSTER ) || ( kSetClusterLinkData( dwCluster, FILESYSTEM_LASTCLUSTER ) == FALSE ) )
	{
		return FALSE;
	}

	*piDirectoryEntryIndex = kFindFreeDirectoryEntry();
	if( *piDirectoryEntryIndex == -1 )
	{
		kSetClusterLinkData( dwCluster, FILESYSTEM_FREECLUSTER);
		return FALSE;
	}

	kMemCpy( pstEntry->vcFileName, pcFileName, kStrLen( pcFileName ) + 1 );
	pstEntry->dwStartClusterIndex = dwCluster;
	pstEntry->dwFileSize = 0;

	if( kSetDirectoryEntryData( *piDirectoryEntryIndex, pstEntry ) == FALSE )
	{
		kSetClusterLinkData( dwCluster, FILESYSTEM_FREECLUSTER );
		return FALSE;
	}

	return TRUE;
}

static BOOL kFreeClusterUntilEnd( DWORD dwClusterIndex )
{
	DWORD dwCurrentClusterIndex;
	DWORD dwNextClusterIndex;

	dwCurrentClusterIndex = dwClusterIndex;

	while( dwCurrentClusterIndex != FILESYSTEM_LASTCLUSTER )
	{
		if( kGetClusterLinkData( dwCurrentClusterIndex, &dwNextClusterIndex ) == FALSE )
		{
			return FALSE;
		}
		if( kSetClusterLinkData( dwCurrentClusterIndex, FILESYSTEM_FREECLUSTER ) == FALSE )
		{
			return FALSE;
		}

		dwCurrentClusterIndex = dwNextClusterIndex;
	}

	return TRUE;
}

FILE* kOpenFile( const char* pcFileName, const char* pcMode )
{
	DIRECTORYENTRY stEntry;
	int iDirectoryEntryOffset;
	int iFileNameLength;
	DWORD dwSecondCluster;
	FILE* pstFile;

	iFileNameLength = kStrLen( pcFileName );
	if( iFileNameLength > ( sizeof( stEntry.vcFileName ) ) - 1 || ( iFileNameLength == 0 ) )
	{
		return NULL;
	}

	kLock( &(gs_stFileSystemManager.stMutex ) );

	iDirectoryEntryOffset = kFindDirectoryEntry( pcFileName, &stEntry );

	// 파일이 없을때 모드 검사하여 파일 생성 or NULL 리턴
	if( iDirectoryEntryOffset == -1 )
	{
		if( pcMode[ 0 ] == 'r' )
		{
			kUnlock( &(gs_stFileSystemManager.stMutex ) );
			return NULL;
		}

		if( kCreateFile( pcFileName, &stEntry, &iDirectoryEntryOffset ) == FALSE )
		{
			kUnlock( &(gs_stFileSystemManager.stMutex ) );
			return NULL;
		}
	}
	// 비워야 하는감?
	else if( pcMode[ 0 ] == 'w' )
	{
		if( kGetClusterLinkData( stEntry.dwStartClusterIndex, &dwSecondCluster ) == FALSE )
		{
			kUnlock( &(gs_stFileSystemManager.stMutex ) );
			return NULL;
		}

		if( kSetClusterLinkData( stEntry.dwStartClusterIndex, FILESYSTEM_LASTCLUSTER) == FALSE )
		{
			kUnlock( &(gs_stFileSystemManager.stMutex ) );
			return NULL;
		}

		if( kFreeClusterUntilEnd( dwSecondCluster ) == FALSE )
		{
			kUnlock( &(gs_stFileSystemManager.stMutex ) );
			return NULL;
		}

		stEntry.dwFileSize = 0;

		if( kSetDirectoryEntryData( iDirectoryEntryOffset, &stEntry ) == FALSE )
		{
			kUnlock( &(gs_stFileSystemManager.stMutex ) );
			return NULL;
		}
	}

	// 파일 핸들을 할당 받아 데이터 설정 후 반환

	pstFile = kAllocateFileDirectoryHandle();
	if( pstFile == NULL )
	{
		kUnlock( &(gs_stFileSystemManager.stMutex ) );
		return NULL;
	}

	pstFile->bType = FILESYSTEM_TYPE_FILE;
	pstFile->stFileHandle.iDirectoryEntryOffset = iDirectoryEntryOffset;
	pstFile->stFileHandle.dwFileSize = stEntry.dwFileSize;
	pstFile->stFileHandle.dwStartClusterIndex = stEntry.dwStartClusterIndex;
	pstFile->stFileHandle.dwCurrentClusterIndex = stEntry.dwStartClusterIndex;
	pstFile->stFileHandle.dwPreviousClusterIndex = stEntry.dwStartClusterIndex;
	pstFile->stFileHandle.dwCurrentOffset = 0;

	if( pcMode[ 0 ] == 'a' )
	{
		kSeekFile( pstFile, 0, FILESYSTEM_SEEK_END );
	}

	kUnlock( &(gs_stFileSystemManager.stMutex ) );
	return pstFile;

}

DWORD kReadFile( void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile )
{
	DWORD dwTotalCount;
	DWORD dwReadCount;
	DWORD dwOffsetInCluster;
	DWORD dwCopySize;
	FILEHANDLE* pstFileHandle;
	DWORD dwNextClusterIndex;

	if( ( pstFile == NULL ) || ( pstFile->bType != FILESYSTEM_TYPE_FILE ) )
	{
		return 0;
	}
	pstFileHandle = &(pstFile->stFileHandle);

	// 이상하당
	if( (pstFileHandle->dwCurrentOffset == pstFileHandle->dwFileSize) || (pstFileHandle->dwCurrentClusterIndex == FILESYSTEM_LASTCLUSTER) )
	{
		return 0;
	}

	dwTotalCount = MIN( dwSize* dwCount, pstFileHandle->dwFileSize - pstFileHandle->dwCurrentOffset );

	kLock( &( gs_stFileSystemManager.stMutex ) );

	dwReadCount = 0;

	while( dwReadCount != dwTotalCount )
	{
		if( kReadCluster( pstFileHandle->dwCurrentClusterIndex, gs_vbTempBuffer ) == FALSE )
		{
			break;
		}

		dwOffsetInCluster = pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE;
		dwCopySize = MIN(FILESYSTEM_CLUSTERSIZE - dwOffsetInCluster, dwTotalCount - dwReadCount);

		kMemCpy( (char*) pvBuffer + dwReadCount, gs_vbTempBuffer + dwOffsetInCluster, dwCopySize );

		dwReadCount += dwCopySize;
		pstFileHandle->dwCurrentOffset += dwCopySize;

		// 현재 클러스터 다 읽음?
		if( (pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE) == 0 )
		{
			if( kGetClusterLinkData( pstFileHandle->dwCurrentClusterIndex, &dwNextClusterIndex) == FALSE )
			{
				break;
			}

			pstFileHandle->dwPreviousClusterIndex = pstFileHandle->dwCurrentClusterIndex;
			pstFileHandle->dwCurrentClusterIndex = dwNextClusterIndex;
		}
	}
	kUnlock( &(gs_stFileSystemManager.stMutex ) );

	return dwReadCount;
}

static BOOL kUpdateDirectoryEntry(FILEHANDLE* pstFileHandle)
{
	DIRECTORYENTRY stEntry;

	// 핸들로 디렉토리 엔트리 검색
	if( ( pstFileHandle == NULL ) || (kGetDirectoryEntryData(pstFileHandle->iDirectoryEntryOffset, &stEntry) == FALSE ) )
	{
		return FALSE;
	}
	stEntry.dwFileSize = pstFileHandle->dwFileSize;
	stEntry.dwStartClusterIndex = pstFileHandle->dwStartClusterIndex;

	if( kSetDirectoryEntryData( pstFileHandle->iDirectoryEntryOffset, &stEntry ) == FALSE )
	{
		return FALSE;
	}
	return TRUE;
}

DWORD kWriteFile( const void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile )
{
	DWORD dwWriteCount;
	DWORD dwTotalCount;
	DWORD dwOffsetInCluster;
	DWORD dwCopySize;
	DWORD dwAllocatedClusterIndex;
	DWORD dwNextClusterIndex;
	FILEHANDLE* pstFileHandle;

	if( ( pstFile == NULL ) || ( pstFile->bType != FILESYSTEM_TYPE_FILE ) )
	{
		return 0;
	}

	pstFileHandle = &(pstFile->stFileHandle);

	dwTotalCount = dwSize * dwCount;

	kLock( &( gs_stFileSystemManager.stMutex ) );

	dwWriteCount = 0;
	while( dwWriteCount != dwTotalCount )
	{
		if( pstFileHandle->dwCurrentClusterIndex == FILESYSTEM_LASTCLUSTER )
		{
			dwAllocatedClusterIndex = kFindFreeCluster();
			if( dwAllocatedClusterIndex == FILESYSTEM_LASTCLUSTER )
			{
				break;
			}

			if( kSetClusterLinkData( dwAllocatedClusterIndex, FILESYSTEM_LASTCLUSTER) == FALSE)
			{
				break;
			}

			if( kSetClusterLinkData(pstFileHandle->dwPreviousClusterIndex, dwAllocatedClusterIndex) == FALSE )
			{
				kSetClusterLinkData( dwAllocatedClusterIndex, FILESYSTEM_FREECLUSTER );
				break;
			}
			pstFileHandle->dwCurrentClusterIndex = dwAllocatedClusterIndex;

			kMemSet( gs_vbTempBuffer, 0,  FILESYSTEM_LASTCLUSTER);

		}
		// 한 클러스터씩 처리이므로 다 못채우면 임시 클러스터 버퍼로 복사
		else if( ( ( pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE ) != 0 ) || ((dwTotalCount - dwWriteCount) < FILESYSTEM_CLUSTERSIZE ) )
		{
			if( kReadCluster( pstFileHandle->dwCurrentClusterIndex, gs_vbTempBuffer ) == FALSE )
			{
				break;
			}
		}

		// 클러스터 내에서 오프셋 계산
		dwOffsetInCluster = pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE;

		// 왼쪽은 남은 오프셋에서 클러스터 꽉 채울때, 오른쪽은 그냥 남은거 정리할때
		dwCopySize = MIN( FILESYSTEM_CLUSTERSIZE - dwOffsetInCluster, dwTotalCount - dwWriteCount );

		kMemCpy( gs_vbTempBuffer + dwOffsetInCluster, (char*)pvBuffer + dwWriteCount, dwCopySize );

		if( kWriteCluster( pstFileHandle->dwCurrentClusterIndex, gs_vbTempBuffer ) == FALSE)
		{
			break;
		}

		dwWriteCount += dwCopySize;
		pstFileHandle->dwCurrentOffset += dwCopySize;

		if( ( pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE ) == 0 )
		{
			// 현재 클러스터의 링크 데이터로 다음 클러스터를 얻음
			if( kGetClusterLinkData( pstFileHandle->dwCurrentClusterIndex, &dwNextClusterIndex ) == FALSE )
			{
				break;
			}

			pstFileHandle->dwPreviousClusterIndex = pstFileHandle->dwCurrentClusterIndex;
			pstFileHandle->dwCurrentClusterIndex = dwNextClusterIndex;
		}

	}

    if( pstFileHandle->dwFileSize < pstFileHandle->dwCurrentOffset )
    {
        pstFileHandle->dwFileSize = pstFileHandle->dwCurrentOffset;
        kUpdateDirectoryEntry( pstFileHandle );
    }

    // 동기화
    kUnlock( &( gs_stFileSystemManager.stMutex ) );

    // 쓴 레코드 수를 반환
    return ( dwWriteCount / dwSize );
}

BOOL kWriteZero( FILE* pstFile, DWORD dwCount )
{
	BYTE* pbBuffer;
	DWORD dwRemainCount;
	DWORD dwWriteCount;

	if(pstFile == NULL)
	{
		return FALSE;
	}

	pbBuffer = (BYTE*) kAllocateMemory(FILESYSTEM_CLUSTERSIZE);

	if(pbBuffer == NULL)
	{
		return FALSE;
	}

	kMemSet(pbBuffer, 0, FILESYSTEM_CLUSTERSIZE);
	dwRemainCount = dwCount;

	while(dwRemainCount != 0)
	{
		dwWriteCount = MIN(dwRemainCount, FILESYSTEM_CLUSTERSIZE );
		if(kWriteFile(pbBuffer, 1, dwWriteCount, pstFile) != dwWriteCount )
		{
			kFreeMemory(pbBuffer);
			return FALSE;
		}
		dwRemainCount -= dwWriteCount;
	}
	kFreeMemory(pbBuffer);
	return TRUE;
}

int kSeekFile(FILE* pstFile, int iOffset, int iOrigin)
{
	DWORD dwRealOffset;
	DWORD dwClusterOffsetToMove;
	DWORD dwCurrentClusterOffset;
	DWORD dwLastClusterOffset;
	DWORD dwMoveCount;
	DWORD i;
	DWORD dwStartClusterIndex;
	DWORD dwPreviousClusterIndex;
	DWORD dwCurrentClusterIndex;
	FILEHANDLE* pstFileHandle;

	if( ( pstFile == NULL) || (pstFile->bType != FILESYSTEM_TYPE_FILE))
	{
		return 0;
	}
	pstFileHandle = &(pstFile->stFileHandle);

	switch(iOrigin)
	{
	case FILESYSTEM_SEEK_SET:
		if(iOffset <= 0)
		{
			dwRealOffset = 0;
		}
		else
		{
			dwRealOffset = iOffset;
		}
		break;
	case FILESYSTEM_SEEK_CUR:
		if((iOffset < 0) && (pstFileHandle->dwCurrentOffset <= (DWORD) -iOffset ) )
		{
			dwRealOffset = 0;
		}
		else
		{
			dwRealOffset = pstFileHandle->dwCurrentOffset + iOffset;
		}
		break;

	case FILESYSTEM_SEEK_END:
		if( (iOffset < 0 ) && ( pstFileHandle->dwFileSize <= (DWORD) -iOffset ))
		{
			dwRealOffset = 0;
		}
		else
		{
			dwRealOffset = pstFileHandle->dwFileSize + iOffset;
		}
		break;
	}

	// 파일의 마지막 클러스터 오프셋
	dwLastClusterOffset = pstFileHandle->dwFileSize / FILESYSTEM_CLUSTERSIZE;

	// 파일 포인터가 옮겨질 위치의 클러스터 오프셋
	dwClusterOffsetToMove = dwRealOffset / FILESYSTEM_CLUSTERSIZE;

	// 현재 파일 포인터가 있는 클러스터의 오프셋
	dwCurrentClusterOffset = pstFileHandle->dwCurrentOffset / FILESYSTEM_CLUSTERSIZE;

	// 현재 한계 클러스터 오프셋을 넘어가면 write 함수를 이용하여 나머지를 공백으로 채운다.
	if( dwLastClusterOffset < dwClusterOffsetToMove )
	{
		dwMoveCount = dwLastClusterOffset - dwCurrentClusterOffset;
		dwStartClusterIndex = pstFileHandle->dwCurrentClusterIndex;
	}
	else if( dwCurrentClusterOffset <= dwClusterOffsetToMove )
	{
		// 이동후 클러스터 오프셋이 현재 클러스터보다 크면
		// 그만큼 이동하면 된다.
		dwMoveCount = dwClusterOffsetToMove - dwCurrentClusterOffset;
		dwStartClusterIndex = pstFileHandle->dwCurrentClusterIndex;
	}
	else
	{
		// 그 이외에는 처음부터 탐색
		dwMoveCount = dwClusterOffsetToMove;
		dwStartClusterIndex = pstFileHandle->dwStartClusterIndex;
	}

	kLock( &(gs_stFileSystemManager.stMutex) );

	dwCurrentClusterIndex = dwStartClusterIndex;
	for( i = 0 ; i < dwMoveCount ; i++ )
	{
		dwPreviousClusterIndex = dwCurrentClusterIndex;

		// 다음 클러스터의 인덱스를 읽음
		if( kGetClusterLinkData( dwPreviousClusterIndex, &dwCurrentClusterIndex ) == FALSE )
		{
			kUnlock( &gs_stFileSystemManager.stMutex );
			return -1;
		}
	}

	// 이동했으면
	if( dwMoveCount > 0 )
	{
		pstFileHandle->dwPreviousClusterIndex = dwPreviousClusterIndex;
		pstFileHandle->dwCurrentClusterIndex = dwCurrentClusterIndex;
	}
	// 이동 못하고 검색 시작점이 처음이라면
	else if( dwStartClusterIndex == pstFileHandle->dwStartClusterIndex )
	{
		pstFileHandle->dwPreviousClusterIndex = pstFileHandle->dwStartClusterIndex;
		pstFileHandle->dwCurrentClusterIndex = pstFileHandle->dwStartClusterIndex;
	}

	// 파일 크기를 넘어가면 넘은곳을 0으로 채워줌
	if( dwLastClusterOffset < dwClusterOffsetToMove )
	{
		pstFileHandle->dwCurrentOffset = pstFileHandle->dwFileSize;
		kUnlock( &( gs_stFileSystemManager.stMutex ) );

		if( kWriteZero( pstFile, dwRealOffset - pstFileHandle->dwFileSize ) == FALSE)
		{
			return 0;
		}
	}

	pstFileHandle->dwCurrentOffset = dwRealOffset;

	kUnlock( &(gs_stFileSystemManager.stMutex ) );

	return 0;
}

int kCloseFile(FILE* pstFile)
{
	if( ( pstFile == NULL ) || ( pstFile->bType != FILESYSTEM_TYPE_FILE ) )
	{
		return -1;
	}

	kFreeFileDirectoryHandle( pstFile );
	return 0;
}

BOOL kIsFileOpened( const DIRECTORYENTRY* pstEntry )
{
	int i;
	FILE* pstFile;

	pstFile = gs_stFileSystemManager.pstHandlePool;
	for( i = 0; i < FILESYSTEM_HANDLE_MAXCOUNT ; i++ )
	{
		if( ( pstFile[i].bType == FILESYSTEM_TYPE_FILE ) && (pstFile[i].stFileHandle.dwStartClusterIndex == pstEntry->dwStartClusterIndex ) )
		{
			return TRUE;
		}
	}
	return FALSE;
}

int kRemoveFile( const char* pcFileName )
{
	DIRECTORYENTRY stEntry;
	int iDirectoryEntryOffset;
	int iFileNameLength;

	iFileNameLength = kStrLen( pcFileName );
	// 이름 유효성 체크
	if( ( iFileNameLength > ( sizeof( stEntry.vcFileName) - 1 ) ) || ( iFileNameLength == 0 ) )
	{
		return NULL;
	}

	kLock( &( gs_stFileSystemManager.stMutex ) );

	// 파일 이름이 존재하는가?
	iDirectoryEntryOffset = kFindDirectoryEntry( pcFileName, &stEntry );

	if( iDirectoryEntryOffset == -1 )
	{
		kUnlock( &( gs_stFileSystemManager.stMutex ) );
		return -1;
	}

	if( kIsFileOpened( &stEntry ) == TRUE )
	{
		kUnlock( &( gs_stFileSystemManager.stMutex ) );
		return -1;
	}

	if( kFreeClusterUntilEnd( stEntry.dwStartClusterIndex ) == FALSE )
	{
		kUnlock( &( gs_stFileSystemManager.stMutex ) );
		return -1;
	}

	kMemSet( &stEntry, 0 , sizeof(stEntry) );
	// 디렉터리 엔트리 리무브
	if( kSetDirectoryEntryData( iDirectoryEntryOffset, &stEntry ) == FALSE )
	{
		kUnlock( &( gs_stFileSystemManager.stMutex ) );
		return -1;
	}

	kUnlock( &( gs_stFileSystemManager.stMutex ) );
	return 0;
}

DIR* kOpenDirectory( const char* pcDirectoryName )
{
	DIR* pstDirectory;
	DIRECTORYENTRY* pstDirectoryBuffer;

	kLock( &( gs_stFileSystemManager.stMutex ) );

	// 루트디렉터리만 있으므로 바로 할당받음
	pstDirectory = kAllocateFileDirectoryHandle();

	if(pstDirectory == NULL )
	{
		kUnlock( &(gs_stFileSystemManager.stMutex ) );
	}

	pstDirectoryBuffer = (DIRECTORYENTRY*) kAllocateMemory( FILESYSTEM_CLUSTERSIZE );

	if( pstDirectoryBuffer == NULL )
	{
		kFreeFileDirectoryHandle(pstDirectory);
		kUnlock( &(gs_stFileSystemManager.stMutex ) );
		return NULL;
	}

	if( kReadCluster(0, (BYTE*) pstDirectoryBuffer) == FALSE )
	{
		kFreeFileDirectoryHandle( pstDirectory );
		kFreeMemory(pstDirectoryBuffer);

		kUnlock( &(gs_stFileSystemManager.stMutex));
		return NULL;
	}

	pstDirectory->bType = FILESYSTEM_TYPE_DIRECTORY;
	pstDirectory->stDirectoryHandle.iCurrentOffset = 0;
	pstDirectory->stDirectoryHandle.pstDirectoryBuffer = pstDirectoryBuffer;

	kUnlock(&(gs_stFileSystemManager.stMutex));
	return pstDirectory;
}

// 디렉터리 엔트리를 반환하고 다음으로 이동
struct kDirectoryEntryStruct* kReadDirectory(DIR* pstDirectory)
{
	DIRECTORYHANDLE* pstDirectoryHandle;
	DIRECTORYENTRY* pstEntry;

	if( (pstDirectory == NULL) || (pstDirectory->bType != FILESYSTEM_TYPE_DIRECTORY ) )
	{
		return NULL;
	}
	pstDirectoryHandle = &(pstDirectory->stDirectoryHandle);

	if( ( pstDirectoryHandle->iCurrentOffset < 0 ) || ( pstDirectoryHandle->iCurrentOffset >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT ) )
	{
		return NULL;
	}

	kLock( &( gs_stFileSystemManager.stMutex ) );

	pstEntry = pstDirectoryHandle->pstDirectoryBuffer;
	while( pstDirectoryHandle->iCurrentOffset < FILESYSTEM_MAXDIRECTORYENTRYCOUNT )
	{
		if( pstEntry[ pstDirectoryHandle->iCurrentOffset ].dwStartClusterIndex != 0 )
		{
			kUnlock( &(gs_stFileSystemManager.stMutex ) );
			return &( pstEntry[ pstDirectoryHandle->iCurrentOffset++ ] );
		}

		pstDirectoryHandle->iCurrentOffset++;
	}
	//동기화
	kUnlock( &(gs_stFileSystemManager.stMutex));
	return NULL;
}

// 디렉터리 포인터를 디렉터리의 처음으로 이동
void kRewindDirectory( DIR* pstDirectory )
{

	DIRECTORYHANDLE* pstDirectoryHandle;

	if( ( pstDirectory == NULL ) || ( pstDirectory->bType != FILESYSTEM_TYPE_DIRECTORY ) )
	{
		return ;
	}
	pstDirectoryHandle = &(pstDirectory->stDirectoryHandle);

	kLock( &(gs_stFileSystemManager.stMutex ) );

	pstDirectoryHandle->iCurrentOffset = 0;

	kUnlock( &( gs_stFileSystemManager.stMutex ) );

}

// 열린 디렉터리를 닫음
int kCloseDirectory( DIR* pstDirectory )
{
	DIRECTORYHANDLE* pstDirectoryHandle;

	if( ( pstDirectory == NULL ) || ( pstDirectory->bType != FILESYSTEM_TYPE_DIRECTORY ) )
	{
		return -1;
	}
	pstDirectoryHandle = &(pstDirectory->stDirectoryHandle);

	kLock( &(gs_stFileSystemManager.stMutex ) );

	kFreeMemory( pstDirectoryHandle->pstDirectoryBuffer);

	kFreeFileDirectoryHandle( pstDirectory );

	kUnlock( &(gs_stFileSystemManager.stMutex ) );

	return 0;
}
