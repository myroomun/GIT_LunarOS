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
	LIST stReadyList;
} SCHEDULER;

#pragma pack( pop )

// �Լ�
// �½�ũ Ǯ�� ������ �Լ�
void kInitializeTCBPool( void );
TCB* kAllocateTCB( void );
void kFreeTCB( QWORD qwID );
TCB* kCreateTask( QWORD qwFlags, QWORD qwEntryPointAddress );
void kSetUpTask( TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize );

// �����췯 ����
void kInitializeScheduler( void );
void kSetRunningTask( TCB* pstTask );
TCB* kGetRunningTask( void );
TCB* kGetNextTaskToRun( void );
void kAddTaskToReadyList( TCB* pstTask );
void kSchedule( void );
BOOL kScheduleInInterrupt( void );
void kDecreaseProcessorTime( void );
BOOL kIsProcessorTimeExpired( void );


#endif /* TASK_H_ */