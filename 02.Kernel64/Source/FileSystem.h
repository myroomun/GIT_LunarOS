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
#define FILESYSTEM_SECTORSPERCLUSTER		8
#define FILESYSTEM_LASTCLUSTER				0xFFFFFFFF
#define FILESYSTEM_FREECLUSTER				0x00
// ���丮 �ϳ��� �ִ� ��Ʈ�� ���� ( ���丮 ������ / ���丮 ��Ʈ�� )
#define FILESYSTEM_MAXDIRECTORYENTRYCOUNT	( ( FILESYSTEM_SECTORSPERCLUSTER * 512 ) / sizeof( DIRECTORYENTRY ) )

// Ŭ������ ������ (Ŭ�����ʹ� ���� 8�� �� ���ʹ� 512����Ʈ)
#define FILESYSTEM_CLUSTERSIZE				( FILESYSTEM_SECTORSPERCLUSTER * 512 )

// ��� �� �ڵ� ��
#define FILESYSTEM_HANDLE_MAXCOUNT			( TASK_MAXCOUNT * 3)

#define FILESYSTEM_MAXFILENAMELENGTH		24

// �ڵ��� Ÿ��
#define FILESYSTEM_TYPE_FREE				0
#define FILESYSTEM_TYPE_FILE				1
#define FILESYSTEM_TYPE_DIRECTORY			2

// seek �ɼ� ����
#define FILESYSTEM_SEEK_SET					0
#define FILESYSTEM_SEEK_CUR					1
#define FILESYSTEM_SEEK_END					2

// �ϵ��ũ ���� �Լ� ������
typedef BOOL (* fReadHDDInformation ) ( BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation );
typedef int (* fReadHDDSector ) (BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer );
typedef int (* fWriteHDDSector ) (BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer );

// Ŀ�� �Լ��� ǥ�� ����� �Լ��� ������
#define fopen								kOpenFile
#define fread								kReadFile
#define fwrite								kWriteFile
#define fseek								kSeekFile
#define fclose								kCloseFile
#define remove								kRemoveFile
#define opendir								kOpenDirectory
#define readdir								kReadDirectory
#define rewinddir							kRewindDirectory
#define closedir							kCloseDirectory

// ���Ͻý��� ǥ�� ����� ��ũ��
#define SEEK_SET							FILESYSTEM_SEEK_SET
#define SEEK_CUR							FILESYSTEM_SEEK_CUR
#define SEEK_END							FILESYSTEM_SEEK_END

// ���� ǥ�� ����� ������ Ÿ��
#define size_t								DWORD
#define dirent								kDirectoryEntryStruct
#define d_name								vcFileName


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

typedef struct kFileHandleStruct
{
	// ������ �����ϴ� ���͸� ��Ʈ���� ������
	int iDirectoryEntryOffset;
	// ����ũ��
	DWORD dwFileSize;
	// ������ ���� Ŭ������
	DWORD dwStartClusterIndex;
	// ���� I/O�� �������� Ŭ�������� �ε��� (�̸� ���� �б� ���� ���� ����)
	DWORD dwCurrentClusterIndex;
	// ���� Ŭ������ �ٷ� ���� Ŭ������ �ε���
	DWORD dwPreviousClusterIndex;
	// ������������ ���� ��ġ
	DWORD dwCurrentOffset;
} FILEHANDLE;

typedef struct kDirectoryHandleStruct
{
	// ��Ʈ���丮�� ���� ������ ��� �����ص� ���� (���丮 ��Ʈ����)
	DIRECTORYENTRY* pstDirectoryBuffer;
	// ���͸� ������
	int iCurrentOffset;
} DIRECTORYHANDLE;

// ���ϰ� ���丮�� ���ÿ� ������ �ڷᱸ��
typedef struct kFileDirectoryHandleStruct
{
	// �����? ����? ���丮?
	BYTE bType;

	union
	{
		FILEHANDLE stFileHandle;
		DIRECTORYHANDLE	stDirectoryHandle;
	};
} FILE, DIR;


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

	FILE* pstHandlePool;
} FILESYSTEMMANAGER;

#pragma pack( pop )



BOOL kInitializeFileSystem( void );
BOOL kFormat( void );
BOOL kMount( void );
BOOL kGetHDDInformation( HDDINFORMATION* pstInformation);

// ������ �Լ��� �ܺο� ������ �ʰ� �Ѵ�.
static BOOL kReadClusterLinkTable( DWORD dwOffset, BYTE* pbBuffer );
static BOOL kWriteClusterLinkTable( DWORD dwOffset, BYTE* pbBuffer );
static BOOL kReadCluster( DWORD dwOffset, BYTE* pbBuffer );
static BOOL kWriteCluster( DWORD dwOffset, BYTE* pbBuffer );
static DWORD kFindFreeCluster( void );
static BOOL kSetClusterLinkData( DWORD dwClusterIndex, DWORD dwData );
static BOOL kGetClusterLinkData( DWORD dwClusterIndex, DWORD* pdwData );
static int kFindFreeDirectoryEntry( void );
static BOOL kSetDirectoryEntryData( int iIndex, DIRECTORYENTRY* pstEntry );
static BOOL kGetDirectoryEntryData( int iIndex, DIRECTORYENTRY* pstEntry );
static int kFindDirectoryEntry( const char* pcFileName, DIRECTORYENTRY* pstEntry );
void kGetFileSystemInformation( FILESYSTEMMANAGER* pstManager );

// ����� �Լ�
FILE* kOpenFile( const char* pcFileName, const char* pcMode );
DWORD kReadFile( void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile );
DWORD kWriteFile( const void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile );
int kSeekFile( FILE* pstFile, int iOffset, int iOrigin );
int kCloseFile( FILE* pstFile );
int kRemoveFile( const char* pcFileName );
DIR* kOpenDirectory( const char* pcDirectoryName );
struct kDirectoryEntryStruct* kReadDirectory( DIR* pstDirectory );
void kRewindDirectory( DIR* pstDirectory );
int kCloseDirectory( DIR* pstDirectory );
BOOL kWriteZero( FILE* pstFile, DWORD dwCount );
BOOL kIsFileOpened( const DIRECTORYENTRY* pstEntry );

// ����� ���� �Լ�
static void* kAllocateFileDirectoryHandle( void );
static void kFreeFileDirectoryHandle( FILE* pstFile );
static BOOL kCreateFile( const char* pcFileName, DIRECTORYENTRY* pstEntry, int* piDirectoryEntryIndex );
static BOOL kFreeClusterUntilEnd( DWORD dwClusterIndex );
static BOOL kUpdateDirectoryEntry( FILEHANDLE* pstFileHandle );

#endif /* FILESYSTEM_H_ */
