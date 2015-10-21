/*
 * HardDisk.h
 *
 *  Created on: 2015. 10. 21.
 *      Author: user
 */

#ifndef HARDDISK_H_
#define HARDDISK_H_

#include "Types.h"
#include "Synchronization.h"

#define HDD_PORT_PRIMARYBASE			0x1F0
#define HDD_PORT_SECONDARYBASE			0x170

// ��Ʈ �ε���
#define HDD_PORT_INDEX_DATA				0x00
#define	HDD_PORT_INDEX_SECTORCOUNT		0x02
#define HDD_PORT_INDEX_SECTORNUMBER		0x03
#define HDD_PORT_INDEX_CYLINDERLSB		0x04
#define HDD_PORT_INDEX_CYLINDERMSB		0x05
#define HDD_PORT_INDEX_DRIVEANDHEAD		0x06
#define	HDD_PORT_INDEX_STATUS			0x07
#define HDD_PORT_INDEX_COMMAND			0x07
#define HDD_PORT_INDEX_DIGITALOUTPUT	0x206

// Ŀ�ǵ� ��������
#define HDD_COMMAND_READ				0x20
#define HDD_COMMAND_WRITE				0x30
#define HDD_COMMAND_IDENTIFY			0xEC

// ���� �������Ϳ� ���õ� ��ũ��
#define HDD_STATUS_ERROR				0x01
#define HDD_STATUS_INDEX				0x02
#define HDD_STATUS_CORREDTEDDATA		0x04
#define HDD_STATUS_DATAREQUEST			0x08
#define HDD_STATUS_SEEKCOMPLETE			0x10
#define HDD_STATUS_WRITEFAULT			0x20
#define HDD_STATUS_READY				0x40
#define HDD_STATUS_BUSY					0x80

// ����̺�/��� �������Ϳ� ���õ� ��ũ��
#define HDD_DRIVEANDHEAD_LBA			0xE0
#define HDD_DRIVEANDHEAD_SLAVE			0x10

// ������ ��� �������Ϳ� ���õ� ��ũ��
#define HDD_DIGITALOUTPUT_RESET				0x04
#define HDD_DIGITALOUTPUT_DISABLEINTERRUPT	0x01

// �ϵ��ũ�� ������ ����ϴ� �ð�
#define HDD_WAITTIME					500
#define HDD_MAXBULKSECTORCOUNT			256

#pragma pack(push, 1)

typedef struct kHDDInformationStruct
{
	// ������
	WORD wConfiguration;

	// �Ǹ�����
	WORD wNumberOfCylinder;
	WORD wReserved1;

	// ��� ��
	WORD wNumberOfHead;
	WORD wUnformattedBytesPerTrack;
	WORD wUnformattedBytesPerSector;

	// �Ǹ����� ���ͼ�
	WORD wNumberOfSectorPerCylinder;
	WORD wInterSectorGap;
	WORD wBytesInPhaseLock;
	WORD wNumberOfVendorUniqueStatusWord;

	// �ϵ� ��ũ�� �ø��� �ѹ�
	WORD vwSerialNumber[ 10 ];
	WORD wControllerType;
	WORD wBufferSize;
	WORD wNumberOfECCBytes;
	WORD vwFirmwareRevision[ 4 ];

	WORD vwModelNumber[ 20 ];
	WORD vmReversed2[ 13 ];

	DWORD dwTotalSectors;
	WORD vwReserved3[ 196 ];
} HDDINFORMATION;

#pragma pack(pop)

typedef struct kHDDManagerStruct
{
	BOOL bHDDDetected;
	BOOL bCanWrite;

	volatile BOOL bPrimaryInterruptOccur;
	volatile BOOL bSecondaryInterruptOccur;

	MUTEX stMutex;

	HDDINFORMATION stHDDInformation;
} HDDMANAGER;


// �Լ�
BOOL kInitializeHDD( void );
BOOL kReadHDDInformation( BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation);
int kReadHDDSector( BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer );
int kWriteHDDSector( BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer );
void kSetHDDInterruptFlag( BOOL bPrimary, BOOL bFlag );

static void kSwapByteInWord( WORD* pwData, int iWordCount );
static BYTE kReadHDDStatus( BOOL bPrimary );
static BOOL kIsHDDBusy( BOOL bPrimary );
static BOOL kIsHDDReady( BOOL bPrimary );
static BOOL kWaitForHDDNoBusy( BOOL bPrimary );
static BOOL kWaitForHDDReady( BOOL bPrimary );
static BOOL kWaitForHDDInterrupt( BOOL bPrimary );

#endif /* HARDDISK_H_ */
