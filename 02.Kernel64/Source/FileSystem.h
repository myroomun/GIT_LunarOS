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
// 디렉토리 하나당 최대 엔트리 개수 ( 디렉토리 사이즈 / 디렉토리 엔트리 )
#define FILESYSTEM_MAXDIRECTORYENTRYCOUNT	( ( FILESYSTEM_SECTORSPERCLUSTER * 512 ) / sizeof( DIRECTORYENTRY ) )

// 클러스터 사이즈 (클러스터는 섹터 8개 각 섹터는 512바이트)
#define FILESYSTEM_CLUSTERSIZE				( FILESYSTEM_SECTORSPERCLUSTER * 512 )

// 허락 된 핸들 수
#define FILESYSTEM_HANDLE_MAXCOUNT			( TASK_MAXCOUNT * 3)

#define FILESYSTEM_MAXFILENAMELENGTH		24

// 핸들의 타입
#define FILESYSTEM_TYPE_FREE				0
#define FILESYSTEM_TYPE_FILE				1
#define FILESYSTEM_TYPE_DIRECTORY			2

// seek 옵션 정의
#define FILESYSTEM_SEEK_SET					0
#define FILESYSTEM_SEEK_CUR					1
#define FILESYSTEM_SEEK_END					2

// 하드디스크 관련 함수 포인터
typedef BOOL (* fReadHDDInformation ) ( BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation );
typedef int (* fReadHDDSector ) (BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer );
typedef int (* fWriteHDDSector ) (BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer );

// 커널 함수를 표준 입출력 함수로 재정의
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

// 파일시스템 표준 입출력 매크로
#define SEEK_SET							FILESYSTEM_SEEK_SET
#define SEEK_CUR							FILESYSTEM_SEEK_CUR
#define SEEK_END							FILESYSTEM_SEEK_END

// 파일 표준 입출력 데이터 타입
#define size_t								DWORD
#define dirent								kDirectoryEntryStruct
#define d_name								vcFileName


// 구조체
#pragma pack( push, 1 )

// 파티션 관련 구조체
typedef struct kPartitionStruct
{
	BYTE bBootableFlag;
	BYTE vbStartingCHSAddress[ 3 ];
	BYTE bPartitionType;
	BYTE vbEndingCHSAddress[ 3 ];
	DWORD dwStartingLBAAddress;
	DWORD dwSizeInSector;
} PARTITION;

// MBR 자료구조는 1섹터를 차지하며(1k) 현재 이 용량은 446 바이트이다.
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
	// 파일이 존재하는 디렉터리 엔트리의 오프셋
	int iDirectoryEntryOffset;
	// 파일크기
	DWORD dwFileSize;
	// 파일의 시작 클러스터
	DWORD dwStartClusterIndex;
	// 현재 I/O가 수행중인 클러스터의 인덱스 (이를 통해 읽기 쓰기 통제 가능)
	DWORD dwCurrentClusterIndex;
	// 현재 클러스터 바로 이전 클러스터 인덱스
	DWORD dwPreviousClusterIndex;
	// 파일포인터의 현재 위치
	DWORD dwCurrentOffset;
} FILEHANDLE;

typedef struct kDirectoryHandleStruct
{
	// 루트디렉토리에 관한 내용을 모두 저장해둔 버퍼 (디렉토리 엔트리들)
	DIRECTORYENTRY* pstDirectoryBuffer;
	// 디렉터리 포인터
	int iCurrentOffset;
} DIRECTORYHANDLE;

// 파일과 디렉토리를 동시에 관리할 자료구조
typedef struct kFileDirectoryHandleStruct
{
	// 비었나? 파일? 디렉토리?
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

// 저수준 함수는 외부에 보이지 않게 한다.
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

// 고수준 함수
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

// 고수준 내부 함수
static void* kAllocateFileDirectoryHandle( void );
static void kFreeFileDirectoryHandle( FILE* pstFile );
static BOOL kCreateFile( const char* pcFileName, DIRECTORYENTRY* pstEntry, int* piDirectoryEntryIndex );
static BOOL kFreeClusterUntilEnd( DWORD dwClusterIndex );
static BOOL kUpdateDirectoryEntry( FILEHANDLE* pstFileHandle );

#endif /* FILESYSTEM_H_ */
