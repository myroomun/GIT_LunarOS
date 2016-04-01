/*
 * RAMDisk.h
 *
 *  Created on: 2016. 4. 1.
 *      Author: user
 */

#ifndef RAMDISK_H_
#define RAMDISK_H_

#include "Types.h"
#include "Synchronization.h"
#include "HardDisk.h"

// 램의 총 섹터수는  8MB / 512bytes
#define RDD_TOTALSECTORCOUNT			( 8 * 1024 * 1024 / 512 )

#pragma pack( push, 1 )

typedef struct kRDDManagerStruct
{
	// 램 디스크용으로 할당받은 메모리 영역
	BYTE* pbBuffer;

	// 총 섹터수
	DWORD dwTotalSectorCount;

	// 동기화 객체
	MUTEX stMutex;
} RDDMANAGER;

#pragma pack( pop )

BOOL kInitializeRDD( DWORD dwTotalSectorCount );
BOOL kReadRDDInformation( BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation );
int kReadRDDSector( BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer );
int kWriteRDDSector( BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer );


#endif /* RAMDISK_H_ */
