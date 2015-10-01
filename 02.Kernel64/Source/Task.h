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

// 레지스터 갯수 및 레지스터 사이즈 (바이트 단위)
#define TASK_REGISTERCOUNT		( 5 + 19 )
#define TASK_REGISTERSIZE		8

// Context 자료구조의 레지스터 오프셋
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

// 태스크 풀의 어드레스

#define TASK_TCBPOOLADDRESS		0x800000 // 8MB
#define TASK_MAXCOUNT			1024

// 스택 풀과 스택의 크기
#define TASK_STACKPOOLADDRESS 	( TASK_TCBPOOLADDRESS + sizeof(TCB) * TASK_MAXCOUNT )
#define TASK_STACKSIZE			8192// 단위 kb 스택 한구간당 8kb 태스크 카운트 1024

// 유호하지 않은 태스크 ID
#define TASK_INVALIDID			0xFFFFFFFFFFFFFFFF

// 허용된 태스크 가동시간(5ms)
#define TASK_PROCESSORTIME		5

// 구조체
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

// 태스크 풀 관리 스트럭쳐
typedef struct kTCBPoolManageStruct
{
	TCB* pstStartAddress;
	// TCB 최고 갯수
	int iMaxCount;
	// 현재 사용중인 갯수
	int iUseCount;

	// 1씩 증가하는 절대값
	int iAllocatedCount;
} TCBPOOLMANAGER;

// 스케쥴러 상태 관리 자료구조
typedef struct kSchedulerStruct
{
	// 현재 수행중인 Task
	TCB* pstRunningTask;

	// 가용 시간
	int iProcessorTime;

	// 실행할 태스크가 준비중인 리스트
	LIST stReadyList;
} SCHEDULER;

#pragma pack( pop )

// 함수
// 태스크 풀과 관련한 함수
void kInitializeTCBPool( void );
TCB* kAllocateTCB( void );
void kFreeTCB( QWORD qwID );
TCB* kCreateTask( QWORD qwFlags, QWORD qwEntryPointAddress );
void kSetUpTask( TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize );

// 스케쥴러 관련
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
