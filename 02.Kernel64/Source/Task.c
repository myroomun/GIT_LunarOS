/*
 * Task.c
 *
 *  Created on: 2015. 10. 1.
 *      Author: user
 */

#include "Task.h"
#include "Descriptor.h"

// 스케쥴러 관련 자료구조
static SCHEDULER gs_stScheduler;
static TCBPOOLMANAGER gs_stTCBPoolManager;

void kSetUpTask( TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize )
{
	// 콘텍스트 초기화
	kMemSet( pstTCB->stContext.vgRegister, 0, sizeof(pstTCB->stContext.vgRegister ));

	// 스택에 관련된 RSP, RBP 레지스터 설정
	// 이때 레지스터는 사용전 레지스터 스택이므로 최상단에 올려준다.
	pstTCB->stContext.vgRegister[ TASK_RSPOFFSET ] = (QWORD) pvStackAddress + qwStackSize;
	pstTCB->stContext.vgRegister[ TASK_RBPOFFSET ] = (QWORD) pvStackAddress + qwStackSize;

	// 세그먼트 셀렉터 설정 (현재 커널내에서 실행되는 코드이다.)
	pstTCB->stContext.vgRegister[ TASK_CSOFFSET ] = GDT_KERNELCODESEGMENT;
	pstTCB->stContext.vgRegister[ TASK_DSOFFSET ] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vgRegister[ TASK_ESOFFSET ] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vgRegister[ TASK_FSOFFSET ] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vgRegister[ TASK_GSOFFSET ] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vgRegister[ TASK_SSOFFSET ] = GDT_KERNELDATASEGMENT;

	// RIP는 초기 주소 어드레스
	pstTCB->stContext.vgRegister[ TASK_RIPOFFSET ] = qwEntryPointAddress;

	// RGLAGS 레지스터 IF 비트(비트9)를 1로 설정
	pstTCB->stContext.vgRegister[ TASK_RFLAGSOFFSET ] |= 0x200;

	// ID, 스택, 사이즈, 플래그 저장
	//pstTCB->qwID = qwID; (qwID가 TCB allocation으로 넘어감)
	pstTCB->pvStackAddress = pvStackAddress;
	pstTCB->qwStackSize = qwStackSize;
	pstTCB->qwFlags = qwFlags;
}

// 태스크 풀과 태스크 관련된 함수들
void kInitializeTCBPool( void )
{
	int i;

	kMemSet( &(gs_stTCBPoolManager), 0, sizeof(gs_stTCBPoolManager) );

	// 태스크 풀의 어드레스를 지정하고 초기화
	gs_stTCBPoolManager.pstStartAddress = (TCB*)TASK_TCBPOOLADDRESS;
	kMemSet(TASK_TCBPOOLADDRESS, 0, sizeof( TCB ) * TASK_MAXCOUNT );

	// TCB에 LIST ID 할당
	for( i = 0 ; i < TASK_MAXCOUNT ; i++ )
	{
		gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;
	}

	// TCB에 최대 개수와 할당된 횟수를 초기화
	gs_stTCBPoolManager.iMaxCount = TASK_MAXCOUNT;
	gs_stTCBPoolManager.iAllocatedCount = 1;
}

// TCB를 핟당받는다.
TCB* kAllocateTCB( void )
{
	TCB* pstEmptyTCB;
	int i;

	if( gs_stTCBPoolManager.iUseCount == gs_stTCBPoolManager.iMaxCount )
	{
		return NULL;
	}

	for( i = 0 ; i < gs_stTCBPoolManager.iMaxCount ; i++ )
	{
		// allocated된 넘버 확인 (하위 32비트 qwid넘버는 항상 순서대로 존재한다.)
		if( ( gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID >> 32 ) == 0 )
		{
			pstEmptyTCB = &( gs_stTCBPoolManager.pstStartAddress[i] );
			break;
		}
	}

	pstEmptyTCB->stLink.qwID = ( ( QWORD ) gs_stTCBPoolManager.iAllocatedCount << 32 ) | i;
	gs_stTCBPoolManager.iUseCount++;
	gs_stTCBPoolManager.iAllocatedCount++;

	if( gs_stTCBPoolManager.iAllocatedCount == 0 )
	{
		gs_stTCBPoolManager.iAllocatedCount = 1;
	}
	return pstEmptyTCB;
}

void kFreeTCB( QWORD qwID )
{
	int i;

	i = qwID & 0xFFFFFFFF; // 하위 비트가 인덱스 비트 상위 비트가 할당횟수 비트
	kMemSet( &(gs_stTCBPoolManager.pstStartAddress[i].stContext), 0 , sizeof(CONTEXT) );
	// 스택은 하위 비트로 계산함
	gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;

	gs_stTCBPoolManager.iUseCount--;
}

// 태스크 생성
TCB* kCreateTask( QWORD qwFlags, QWORD qwEntryPointAddress )
{
	TCB* pstTask;
	void* pvStackAddress;

	pstTask = kAllocateTCB();
	if( pstTask == NULL )
	{
		return NULL;
	}

	// 하위 인덱스 비트 * 스택사이즈 + 스택풀베이스
	pvStackAddress = ( void* ) (TASK_STACKPOOLADDRESS + ( TASK_STACKSIZE * (pstTask->stLink.qwID & 0xFFFFFFFF ) ) );


	kSetUpTask( pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE );
	kAddTaskToReadyList( pstTask );

	return pstTask;
}

// 스케쥴러 관련

// TCB 초기화 및 스케쥴러 매니저도 초기화
void kInitializeScheduler( void )
{
	kInitializeTCBPool();

	kInitializeList( &(gs_stScheduler.stReadyList ) );

	// TCB를 할당받아 실행중인 태스크로 설정, 부팅을 수행한 태스크를 저장할 TCB를 준비
	gs_stScheduler.pstRunningTask = kAllocateTCB();

}

// 현재 수행중인 태스크를 설정
void kSetRunningTask( TCB* pstTask )
{
	gs_stScheduler.pstRunningTask = pstTask;
}

TCB* kGetRunningTask( void )
{
	return gs_stScheduler.pstRunningTask;
}

// 태스크 리스트에서 다음으로 실행할 태스크를 얻음
TCB* kGetNextTaskToRun( void )
{
	if( kGetListCount( &( gs_stScheduler.stReadyList ) ) == 0 )
	{
		return NULL;
	}

	return (TCB*)kRemoveListFromHeader( &( gs_stScheduler.stReadyList ) );
}

// 태스크를 스케줄러의 준비 리스트에 삽입
void kAddTaskToReadyList( TCB* pstTask )
{
	kAddListToTail( &(gs_stScheduler.stReadyList), pstTask );
}

// 다른 태스크를 찾아서 전환 (예외발생인경우는 호출 금지 ) => 스택처리를 다르게 해야함
void kSchedule( void )
{
	TCB* pstRunningTask, * pstNextTask;
	BOOL bPreviousFlag;

	if( kGetListCount( &( gs_stScheduler.stReadyList) ) == 0 )
	{
		return;
	}

	// 전환하는 도충 인터럽트가 발생하여 태스크 전환이 일어나면 곤란!
	bPreviousFlag = kSetInterruptFlag( FALSE );

	// 실행할 다음 태스크를 얻음
	pstNextTask = kGetNextTaskToRun();
	if( pstNextTask == NULL )
	{
		kSetInterruptFlag( bPreviousFlag );
		return;
	}

	pstRunningTask = gs_stScheduler.pstRunningTask;
	kAddTaskToReadyList(pstRunningTask);

	gs_stScheduler.pstRunningTask = pstNextTask;
	gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

	// 이 함수는 실행 후 컨텍스트 복원 되었을때 이 다음줄이 실행됨. 즉 러닝태스크의 컨텍스트는 지금 이 함수를 호출한 태스크와 이 다음줄이 저장됨
	kSwitchContext( &(pstRunningTask->stContext), &(pstNextTask->stContext));

	kSetInterruptFlag( bPreviousFlag );
}

// 인터럽트가 발생할 시 다른 태스크 찾기
BOOL kScheduleInInterrupt( void )
{
	TCB* pstRunningTask, * pstNextTask;
	char* pcContextAddress;

	// 전환할 태스크가 없으면 종료
	pstNextTask = kGetNextTaskToRun();
	if( pstNextTask == NULL )
	{
		return FALSE;
	}

	// 태스크 전환 처리
	pcContextAddress = (char*) IST_STARTADDRESS + IST_SIZE - sizeof( CONTEXT );
	// IST_STARTADDRESS + IST_SIZE가 인터럽트 stack top 임 이미 컨텍스트 저장되어있음
	// 이때 Context 사이즈 안에 에러코드는 존재하지 않음

	// 현재 태스크를 얻어서 IST에 있는 콘텍스트를 복사하고, 현재 태스크를 준비 리스트로 옮김
	pstRunningTask = gs_stScheduler.pstRunningTask;
	kMemCpy( &( pstRunningTask->stContext), pcContextAddress, sizeof(CONTEXT) );
	kAddTaskToReadyList(pstRunningTask);

	// 전환하여 실행할 태스크를 running task로 설정, 콘텍스트를 IST에 복사
	gs_stScheduler.pstRunningTask = pstNextTask;
	kMemCpy( pcContextAddress, &(pstNextTask->stContext), sizeof(CONTEXT) );

	// 프로세스 사용 시간을 업데이트
	gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;
	return TRUE; // rax 레지스터로 넘어가서 스택상 불이익이 없다.
}

void kDecreaseProcessorTime( void )
{
	if( gs_stScheduler.iProcessorTime > 0 )
	{
		gs_stScheduler.iProcessorTime--;
	}
}

BOOL kIsProcessorTimeExpired( void )
{
	if( gs_stScheduler.iProcessorTime <= 0 )
	{
		return TRUE;
	}
	return FALSE;
}
