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

// ���� �� ���ͼ���  8MB / 512bytes
#define RDD_TOTALSECTORCOUNT			( 8 * 1024 * 1024 / 512 )

#pragma pack( push, 1 )

typedef struct kRDDManagerStruct
{
	// �� ��ũ������ �Ҵ���� �޸� ����
	BYTE* pbBuffer;

	// �� ���ͼ�
	DWORD dwTotalSectorCount;

	// ����ȭ ��ü
	MUTEX stMutex;
} RDDMANAGER;

#pragma pack( pop )

BOOL kInitializeRDD( DWORD dwTotalSectorCount );
BOOL kReadRDDInformation( BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation );
int kReadRDDSector( BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer );
int kWriteRDDSector( BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer );


#endif /* RAMDISK_H_ */
