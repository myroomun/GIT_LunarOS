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
static BYTE gs_vbTempBuffer[ FILESYSTEM_SECTORPERCLUSTER * 512 ]; // �� Ŭ�������� ������ ���� �� ���� ������ ũ�⸦ ����

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

// ���� �ϵ��ũ ������ �о� Mount �õ�, ������ �Ŵ��� ������ ����
BOOL kMount( void )
{
	MBR* pstMBR;

	kLock( &( gs_stFileSystemManager.stMutex ) );

	// MBR �б�
	if( gs_pfReadHDDSector( TRUE, TRUE, 0, 1, gs_vbTempBuffer) == FALSE )
	{
		kUnlock( &(gs_stFileSystemManager.stMutex ) );
		return FALSE;
	}

	// �ñ״�ó Ȯ��
	pstMBR = (MBR*)gs_vbTempBuffer;
	if( pstMBR->dwSignature != FILESYSTEM_SIGNATURE )
	{
		kUnlock( &(gs_stFileSystemManager.stMutex));
		return FALSE;
	}

	gs_stFileSystemManager.bMounted = TRUE;

	// �� ������ ���� LBA ��巹���� ���� ���� ��� �� �Ŵ����� ����
	gs_stFileSystemManager.dwReservedSectorCount = pstMBR->dwReservedSectorCount;
	gs_stFileSystemManager.dwClusterLinkAreaStartAddress = pstMBR->dwReservedSectorCount + 1; // 1�� MBR
	gs_stFileSystemManager.dwClusterLinkAreaSize = pstMBR->dwClusterLinkSectorCount;
	gs_stFileSystemManager.dwDataAreaStartAddress = pstMBR->dwReservedSectorCount + pstMBR->dwClusterLinkSectorCount+ 1; // Base = 0 �̹Ƿ� 0 + MBR(1) Reserved + ClusterLink
	gs_stFileSystemManager.dwTotalClusterCount = pstMBR->dwTotalClusterCount;

	kUnlock( &( gs_stFileSystemManager.stMutex ) );
	return TRUE;

}

// ���Ͻý����� �����ϴ� �Լ�
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

	// ���� �ִ� Ŭ������ �� ���
	dwMaxClusterCount = dwTotalSectorCount / FILESYSTEM_SECTORPERCLUSTER;

	// �� Ŭ�����͵��� ��Ÿ�� �� �ִ� ��ũ���̺� ���� ������ <���ͼ�> ���
	// ��, ��ü Ŭ������ ������ �� ���Ͱ� ��Ÿ�� �� �ִ� ��ũ���� 128���̱� ������ ������ �ݿø�
	dwClusterLinkSectorCount = (dwMaxClusterCount + 127) / 128;

	// �뷫������ ���� ���� ī���ʹ� ��ü - MBR - ��ũ������
	dwRemainSectorCount = dwTotalSectorCount - dwClusterLinkSectorCount - 1;

	// �����ͷ� �� Ŭ�����ʹ� ���� ���� / 8
	dwClusterCount = dwRemainSectorCount / FILESYSTEM_SECTORPERCLUSTER;

	// ���� ��� ������ Ŭ������ ���� ���� ���
	dwClusterLinkSectorCount = ( dwClusterCount + 127 ) / 128;

	// �̷��� �������� �̿��Ͽ� MBR�� ���� �����.
	// MBR �б�
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

    // MBR ������ 1 ���͸� ��
    if( gs_pfWriteHDDSector( TRUE, TRUE, 0, 1, gs_vbTempBuffer ) == FALSE )
    {
        // ����ȭ ó��
        kUnlock( &( gs_stFileSystemManager.stMutex ) );
        return FALSE;
    }
	// MBR���� �����͸�ũ���� �ʱ�ȭ
	kMemSet( gs_vbTempBuffer, 0, 512 );
	// ���� 1��(��ũ�ε��� ����) ���� ��Ʈ ���丮����
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

		// 1���;� ����
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

// ���� �������� ���ڷ� ����
BOOL kReadClusterLinkTable( DWORD dwOffset, BYTE* pbBuffer )
{
	return gs_pfReadHDDSector( TRUE, TRUE, gs_stFileSystemManager.dwClusterLinkAreaStartAddress + dwOffset, 1, pbBuffer );
}

// Ŭ������ ��ũ ���̺� ���� �����¿� �� ���͸� ��
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

	// ������ ã�ƺô� ���� �������� ��´�.
	dwLastSectorOffset = gs_stFileSystemManager.dwLastAllocatedClusterLinkSectorOffset;

	// ��ũ��������

	for( i = 0 ; i < gs_stFileSystemManager.dwClusterLinkAreaSize ; i++ )
	{
		// ã�µ� Ŭ������ �������� �Դٸ�
		if( ( dwLastSectorOffset + i ) == (gs_stFileSystemManager.dwClusterLinkAreaSize - 1 ) )
		{
			// Ŭ�����Ͱ� 128�� ������ 1���Ϳ� ���̴µ� 128�� �̸��̶�� �������� �˻��� ���ͼ��� �ִ´�.
			dwLinkCountInSector = gs_stFileSystemManager.dwTotalClusterCount % 128;
		}
		else
		{
			dwLinkCountInSector = 128;
		}

		// dwClusterLinkAreaSize�� �Ѿ �� �����Ƿ�
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

	// ã�Ҵٸ� Ŭ������ �ε����� ��ȯ
	if( j != dwLinkCountInSector )
	{
		gs_stFileSystemManager.dwLastAllocatedClusterLinkSectorOffset = dwCurrentSectorOffset;

		// ��ũ ���̺� ������ * 128 + j
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

	// ���° ����?
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

// ��Ʈ Ŭ������ �ȿ� �ִ� �� ���͸� �˻�
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

	// ������ ��
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
			// ã���� �� ��Ʈ���� �����ؼ� �ѱ�
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
