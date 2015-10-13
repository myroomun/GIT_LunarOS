/*
 * Task.h
 *
 *  Created on: 2015. 10. 1.
 *      Author: user
 */

#ifndef TASK_H_
#define TASK_H_

#include "Types.h"
#include "List.h"

// �������� ���� �� �������� ������ (����Ʈ ����)
#define TASK_REGISTERCOUNT		( 5 + 19 )
#define TASK_REGISTERSIZE		8

// Context �ڷᱸ���� �������� ������
#define TASK_GSOFFSET			0
#define TASK_FSOFFSET			1
#define TASK_ESOFFSET			2
#define TASK_DSOFFSET			3
#define TASK_R15OFFSET			4
#define TASK_R14OFFSET			5
#define TASK_R13OFFSET			6
#define TASK_R12OFFSET			7
#define TASK_R11OFFSET			8
#define TASK_R10OFFSET			9
#define TASK_R9OFFSET			10
#define TASK_R8OFFSET			11
#define TASK_RSIOFFSET			12
#define TASK_RDIOFFSET			13
#define TASK_RDXOFFSET			14
#define TASK_RCXOFFSET			15
#define TASK_RBXOFFSET			16
#define TASK_RAXOFFSET			17
#define TASK_RBPOFFSET			18
#define TASK_RIPOFFSET			19
#define TASK_CSOFFSET			20
#define TASK_RFLAGSOFFSET		21
#define TASK_RSPOFFSET			22
#define TASK_SSOFFSET			23

// �½�ũ Ǯ�� ��巹��

#define TASK_TCBPOOLADDRESS		0x800000 // 8MB
#define TASK_MAXCOUNT			1024

// ���� Ǯ�� ������ ũ��
#define TASK_STACKPOOLADDRESS 	( TASK_TCBPOOLADDRESS + sizeof(TCB) * TASK_MAXCOUNT )
#define TASK_STACKSIZE			8192// ���� kb ���� �ѱ����� 8kb �½�ũ ī��Ʈ 1024

// ��ȣ���� ���� �½�ũ ID
#define TASK_INVALIDID			0xFFFFFFFFFFFFFFFF

// ���� �½�ũ �����ð�(5ms)
#define TASK_PROCESSORTIME		5


// �غ�� ��Ƽť ����
#define TASK_MAXREADYLISTCOUNT	5

// �½�ũ �켱����
#define TASK_FLAGS_HIGHEST		0
#define TASK_FLAGS_HIGH			1
#define TASK_FLAGS_MEDIUM		2
#define TASK_FLAGS_LOW			3
#define TASK_FLAGS_LOWEST		4
#define TASK_FLAGS_WAIT			0xFF

// �½�ũ�� �÷���
#define TASK_FLAGS_ENDTASK		0x8000000000000000
#define TASK_FLAGS_SYSTEM		0x4000000000000000
#define TASK_FLAGS_PROCESS		0x2000000000000000
#define TASK_FLAGS_THREAD		0x1000000000000000
#define TASK_FLAGS_IDLE			0x0800000000000000

// �Լ� ��ũ��
#define GETPRIORITY( x )		( ( x ) & 0xFF )
#define SETPRIORITY( x, priority )	( ( x ) = ( ( x ) & 0xFFFFFFFFFFFFFF00 ) | ( priority ) )
#define GETTCBOFFSET( x )		( ( x ) & 0xFFFFFFFF )

#define GETTCBFROMTHREADLINK( x )	( TCB* ) ( ( QWORD )( x ) - offsetof( TCB, stThreadLink) )

// ����ü
#pragma pack(push, 1)

typedef struct kContextStruct
{
	QWORD vgRegister[ TASK_REGISTERCOUNT ];
} CONTEXT;

typedef struct kTaskControlBlockStruct
{
	LISTLINK stLink;

	QWORD qwFlags;

	// ���μ��� �޸� ������ ���۰� ũ��
	void* pvMemoryAddress;
	QWORD qwMemorySize;

	// �Ʒ��� �ڽ� ������ ����
	LISTLINK stThreadLink;	// ������ ���ḵũ

	// ����Ʈ ���
	LIST stChildThreadList;

	// �θ� ���μ����� ���̵�
	QWORD qwParentProcessID;


	CONTEXT stContext;

	void* pvStackAddress;
	QWORD qwStackSize;
} TCB;

// �½�ũ Ǯ ���� ��Ʈ����
typedef struct kTCBPoolManageStruct
{
	TCB* pstStartAddress;
	// TCB �ְ� ����
	int iMaxCount;
	// ���� ������� ����
	int iUseCount;

	// 1�� �����ϴ� ���밪
	int iAllocatedCount;
} TCBPOOLMANAGER;

// �����췯 ���� ���� �ڷᱸ��
typedef struct kSchedulerStruct
{
	// ���� �������� Task
	TCB* pstRunningTask;

	// ���� �ð�
	int iProcessorTime;

	// ������ �½�ũ�� �غ����� ����Ʈ
	LIST vstReadyList[ TASK_MAXREADYLISTCOUNT ];

	// ������ �½�ũ�� �غ����� ����Ʈ
	LIST stWaitList;

	// �� �켱�������� �½�ũ�� ������ Ƚ���� �����ϴ� �ڷᱸ��
	int viExecuteCount[ TASK_MAXREADYLISTCOUNT ];

	// ���μ��� ���ϸ� ���
	QWORD qwProcessLoad;

	// ���� �½�ũ�� ����� �ð�
	QWORD qwSpendProcessorTimeInIdleTask;

} SCHEDULER;

#pragma pack( pop )

// �Լ�
// �½�ũ Ǯ�� ������ �Լ�
static void kInitializeTCBPool( void );
static TCB* kAllocateTCB( void );
static void kFreeTCB( QWORD qwID );
TCB* kCreateTask( QWORD qwFlags, void* pvMemoryAddress, QWORD qwMemorySize, QWORD qwEntryPointAddress );
static void kSetUpTask( TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize );

// �����췯 ����
void kInitializeScheduler( void );
void kSetRunningTask( TCB* pstTask );
TCB* kGetRunningTask( void );
static TCB* kGetNextTaskToRun( void );
static BOOL kAddTaskToReadyList( TCB* pstTask );
void kSchedule( void );
BOOL kScheduleInInterrupt( void );
void kDecreaseProcessorTime( void );
BOOL kIsProcessorTimeExpired( void );
static TCB* kRemoveTaskFromReadyList( QWORD qwTaskID );
BOOL kChangePriority( QWORD qwID, BYTE bPriority );
BOOL kEndTask( QWORD qwTaskID );
void kExitTask( void );
int kGetReadyTaskCount( void );
int kGetTaskCount( void );
TCB* kGetTCBInTCBPool( int iOffset );
BOOL kIsTaskExist( QWORD qwID );
QWORD kGetProcessorLoad( void );
static TCB* kGetProcessByThread( TCB* pstThread );


// ���� �½�ũ ����
void kIdleTask( void );
void kHaltProcessorByLoad( void );

#endif /* TASK_H_ */
