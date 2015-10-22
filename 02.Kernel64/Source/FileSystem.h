/*
 * FileSystem.h
 *
 *  Created on: 2015. 10. 22.
 *      Author: user
 */

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include "Types.h"
#include "Synchronization.h"
#include "HardDisk.h"

#define FILESYSTEM_SIGNATURE				0x7E38CF10
#define FILESYSTEM_SECTORPERCLUSTER			8
#define FILESYSTEM_LASTCLUSTER				0xFFFFFFFF
#define FILESYSTEM_FREECLUSTER				0x00
// ���丮 �ϳ��� �ִ� ��Ʈ�� ���� ( ���丮 ������ / ���丮 ��Ʈ�� )
#define FILESYSTEM_MAXDIRECTORYENTRYCOUNT	( ( FILESYSTEM_SECTORPERCLUSTER * 512 ) / sizeof( DIRECTORYENTRY ) )

// Ŭ������ ������ (Ŭ�����ʹ� ���� 8�� �� ���ʹ� 512����Ʈ)
#define FILESYSTEM_CLUSTERSIZE				( FILESYSTEM_SECTORSPERCLUSTER * 512 )

#define FILESYSTEM_MAXFILENAMELENGTH		24

// �ϵ��ũ ���� �Լ� ������
typedef BOOL (* fReadHDDInformation ) ( BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation );
typedef int (* fReadHDDSector ) (BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer );
typedef int (* fWriteHDDSector ) (BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer );

// ����ü
#pragma pack( push, 1 )

// ��Ƽ�� ���� ����ü
typedef struct kPartitionStruct
{
	BYTE bBootableFlag;
	BYTE vbStartingCHSAddress[ 3 ];
	BYTE bPartitionType;
	BYTE vbEndingCHSAddress[ 3 ];
	DWORD dwStartingLBAAddress;
	DWORD dwSizeInSector;
} PARTITION;

// MBR �ڷᱸ���� 1���͸� �����ϸ�(1k) ���� �� �뷮�� 446 ����Ʈ�̴�.
typedef struct kMBRStruct
{
	BYTE vbBootCode[ 430 ];

	DWORD dwSignature;
	DWORD dwReservedSectorCount;
	DWORD dwClusterLinkSectorCount;
	DWORD dwTotalClusterCount;

	PARTITION vstPartition[ 4 ];

	// 0x55, 0xAA
	BYTE vbBootLoaderSignature[ 2 ];
} MBR;

typedef struct kDirectoryEntryStruct
{
	char vcFileName[ FILESYSTEM_MAXFILENAMELENGTH ];
	DWORD dwFileSize;
	DWORD dwStartClusterIndex;
} DIRECTORYENTRY;

#pragma pack( pop )

typedef struct kFileSystemManagerStruct
{
	BOOL bMounted;

	DWORD dwReservedSectorCount;
	DWORD dwClusterLinkAreaStartAddress;
	DWORD dwClusterLinkAreaSize;
	DWORD dwDataAreaStartAddress;

	DWORD dwTotalClusterCount;

	DWORD dwLastAllocatedClusterLinkSectorOffset;
	MUTEX stMutex;
} FILESYSTEMMANAGER;

BOOL kInitializeFileSystem( void );
BOOL kFormat( void );
BOOL kMount( void );
BOOL kGetHDDInformation( HDDINFORMATION* pstInformation);

BOOL kReadClusterLinkTable( DWORD dwOffset, BYTE* pbBuffer );
BOOL kWriteClusterLinkTable( DWORD dwOffset, BYTE* pbBuffer );
BOOL kReadCluster( DWORD dwOffset, BYTE* pbBuffer );
BOOL kWriteCluster( DWORD dwOffset, BYTE* pbBuffer );
DWORD kFindFreeCluster( void );
BOOL kSetClusterLinkData( DWORD dwClusterIndex, DWORD dwData );
BOOL kGetClusterLinkData( DWORD dwClusterIndex, DWORD* pdwData );
int kFindFreeDirectoryEntry( void );
BOOL kSetDirectoryEntryData( int iIndex, DIRECTORYENTRY* pstEntry );
BOOL kGetDirectoryEntryData( int iIndex, DIRECTORYENTRY* pstEntry );
int kFindDirectoryEntry( const char* pcFileName, DIRECTORYENTRY* pstEntry );
void kGetFileSystemInformation( FILESYSTEMMANAGER* pstManager );


#endif /* FILESYSTEM_H_ */
