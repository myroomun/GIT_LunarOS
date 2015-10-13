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


// 준비된 멀티큐 갯수
#define TASK_MAXREADYLISTCOUNT	5

// 태스크 우선순위
#define TASK_FLAGS_HIGHEST		0
#define TASK_FLAGS_HIGH			1
#define TASK_FLAGS_MEDIUM		2
#define TASK_FLAGS_LOW			3
#define TASK_FLAGS_LOWEST		4
#define TASK_FLAGS_WAIT			0xFF

// 태스크의 플래그
#define TASK_FLAGS_ENDTASK		0x8000000000000000
#define TASK_FLAGS_SYSTEM		0x4000000000000000
#define TASK_FLAGS_PROCESS		0x2000000000000000
#define TASK_FLAGS_THREAD		0x1000000000000000
#define TASK_FLAGS_IDLE			0x0800000000000000

// 함수 매크로
#define GETPRIORITY( x )		( ( x ) & 0xFF )
#define SETPRIORITY( x, priority )	( ( x ) = ( ( x ) & 0xFFFFFFFFFFFFFF00 ) | ( priority ) )
#define GETTCBOFFSET( x )		( ( x ) & 0xFFFFFFFF )

#define GETTCBFROMTHREADLINK( x )	( TCB* ) ( ( QWORD )( x ) - offsetof( TCB, stThreadLink) )

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

	// 프로세스 메모리 영역의 시작과 크기
	void* pvMemoryAddress;
	QWORD qwMemorySize;

	// 아래는 자식 스레드 정보
	LISTLINK stThreadLink;	// 스레드 연결링크

	// 리스트 헤더
	LIST stChildThreadList;

	// 부모 프로세스의 아이디
	QWORD qwParentProcessID;


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
	LIST vstReadyList[ TASK_MAXREADYLISTCOUNT ];

	// 실행할 태스크가 준비중인 리스트
	LIST stWaitList;

	// 각 우선순위별로 태스크를 실행한 횟수를 저장하는 자료구조
	int viExecuteCount[ TASK_MAXREADYLISTCOUNT ];

	// 프로세서 부하를 계산
	QWORD qwProcessLoad;

	// 유휴 태스크가 사용한 시간
	QWORD qwSpendProcessorTimeInIdleTask;

} SCHEDULER;

#pragma pack( pop )

// 함수
// 태스크 풀과 관련한 함수
static void kInitializeTCBPool( void );
static TCB* kAllocateTCB( void );
static void kFreeTCB( QWORD qwID );
TCB* kCreateTask( QWORD qwFlags, void* pvMemoryAddress, QWORD qwMemorySize, QWORD qwEntryPointAddress );
static void kSetUpTask( TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize );

// 스케쥴러 관련
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


// 유휴 태스크 관련
void kIdleTask( void );
void kHaltProcessorByLoad( void );

#endif /* TASK_H_ */
