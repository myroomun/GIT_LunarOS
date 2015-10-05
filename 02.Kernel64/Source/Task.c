/*
 * Task.c
 *
 *  Created on: 2015. 10. 1.
 *      Author: user
 */

#include "Task.h"
#include "Descriptor.h"
#include "Utility.h"

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

	// 하위 32비트가 인덱스 역할
	i = GETTCBOFFSET( qwID );

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
	pvStackAddress = ( void* ) (TASK_STACKPOOLADDRESS + ( TASK_STACKSIZE * GETTCBOFFSET(pstTask->stLink.qwID) ) );


	kSetUpTask( pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE );
	kAddTaskToReadyList( pstTask );

	return pstTask;
}

// 스케쥴러 관련

// TCB 초기화 및 스케쥴러 매니저도 초기화
void kInitializeScheduler( void )
{
	int i;

	kInitializeTCBPool();

	for( i = 0 ; i < TASK_MAXREADYLISTCOUNT ; i++ )
	{
		kInitializeList( &(gs_stScheduler.vstReadyList[ i ]));
		gs_stScheduler.viExecuteCount[ i ] = 0;
	}

	// TCB를 할당받아 실행중인 태스크로 설정, 부팅을 수행한 태스크를 저장할 TCB를 준비
	gs_stScheduler.pstRunningTask = kAllocateTCB();
	gs_stScheduler.pstRunningTask->qwFlags = TASK_FLAGS_HIGHEST; // 쉘의 우선도를 높임

	//프로세스 사용률 계산 구조 초기화
	gs_stScheduler.qwSpendProcessorTimeInIdleTask = 0;
	gs_stScheduler.qwProcessLoad = 0;
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
	TCB* pstTarget = NULL;
	int iTaskCount, i, j;

	for( j = 0 ;  j < 2 ; j++ )
	{
		for( i = 0 ; i < TASK_MAXREADYLISTCOUNT ; i++ )
		{
			iTaskCount = kGetListCount( &(gs_stScheduler.vstReadyList[ i ] ));

			// 만약 큐에 있는 태스크갯수만큼 실행을 안했으면
			if( gs_stScheduler.viExecuteCount[ i ] < iTaskCount )
			{
				pstTarget = (TCB*)kRemoveListFromHeader(&(gs_stScheduler.vstReadyList[i]));
				gs_stScheduler.viExecuteCount[i]++;
				break;
			}
			// 더 많으면 양보
			else
			{
				gs_stScheduler.viExecuteCount[ i ] = 0;
			}
		}

		if( pstTarget != NULL )
		{
			break;
		}
	}

	return pstTarget;
}

// 태스크를 스케줄러의 준비 리스트에 삽입
BOOL kAddTaskToReadyList( TCB* pstTask )
{
	BYTE bPriority;

	bPriority = GETPRIORITY(pstTask->qwFlags);
	if( bPriority >= TASK_MAXREADYLISTCOUNT )
	{
		return FALSE;
	}
	kAddListToTail( &(gs_stScheduler.vstReadyList[bPriority]), pstTask );
	return TRUE;
}

TCB* kRemoveTaskFromReadyList(QWORD qwTaskID)
{
	TCB* pstTarget;
	BYTE bPriority;

	if( GETTCBOFFSET( qwTaskID ) >= TASK_MAXCOUNT )
	{
		return NULL;
	}

	pstTarget = &(gs_stTCBPoolManager.pstStartAddress[ GETTCBOFFSET( qwTaskID )] );
	if( pstTarget->stLink.qwID != qwTaskID )
	{
		return NULL;
	}
	bPriority = GETPRIORITY( pstTarget->qwFlags );

	pstTarget = kRemoveList( &(gs_stScheduler.vstReadyList[bPriority]), qwTaskID );
	return pstTarget;
}

BOOL kChangePriority( QWORD qwTaskID, BYTE bPriority )
{
	TCB* pstTarget;

	if( bPriority > TASK_MAXREADYLISTCOUNT )
	{
		return FALSE;
	}

	// 현재 실행중인 태스크이면 우선순위만 변경
	// 나중에 태스크 스위칭이 일어나면 태스크를 레디큐에 넣을때 분류된다.
	pstTarget = gs_stScheduler.pstRunningTask;
	if( pstTarget->stLink.qwID == qwTaskID )
	{

		SETPRIORITY( pstTarget->qwFlags, bPriority );
	}
	else
	{
		pstTarget = kRemoveTaskFromReadyList( qwTaskID );
		if( pstTarget == NULL )
		{
			// 태스크 아이디로 직접 찾기
			pstTarget = kGetTCBInTCBPool( GETTCBOFFSET(qwTaskID) );
			if( pstTarget != NULL )
			{
				SETPRIORITY( pstTarget->qwFlags, bPriority );
			}
		}
		else
		{
			SETPRIORITY( pstTarget->qwFlags, bPriority );
			kAddTaskToReadyList( pstTarget );
		}

	}
	return TRUE;

}

// 다른 태스크를 찾아서 전환 (예외발생인경우는 호출 금지 ) => 스택처리를 다르게 해야함
void kSchedule( void )
{
	TCB* pstRunningTask, * pstNextTask;
	BOOL bPreviousFlag;

	if( kGetReadyTaskCount() < 1 )
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

	// 현재 수행중인 태스크의 정보를 수정한 뒤 콘텍스트 전환
	pstRunningTask = gs_stScheduler.pstRunningTask;
	gs_stScheduler.pstRunningTask = pstNextTask;

	// 전 태스크가 유휴태스크였으면 프로세서 시간을 증가시킴
    if( ( pstRunningTask->qwFlags & TASK_FLAGS_IDLE ) == TASK_FLAGS_IDLE )
    {
        gs_stScheduler.qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME - gs_stScheduler.iProcessorTime;
    }

    // 태스크 종료플래그가 설정된 경우 콘텍스트 저장 X, 대기리스트에 삽입
    if( pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK )
    {
    	kAddListToTail( &( gs_stScheduler.stWaitList ), pstRunningTask );
    	kSwitchContext(NULL, &( pstNextTask->stContext ));
    }
    else
    {
    	kAddTaskToReadyList(pstRunningTask);
    	kSwitchContext( &( pstRunningTask->stContext ), &( pstNextTask->stContext ));
    }

	gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

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
	gs_stScheduler.pstRunningTask = pstNextTask;

	// 유휴 태스크에서 전환되었다면 사용한 프로세서 시간 증가
	if( (pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE )
	{
		gs_stScheduler.qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME;
	}

	// 태스크 종료 플래그가 설정된 경우 콘텍스트 저장하지 않고 대기리스트에 삽입
	if( pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK )
	{
		kAddListToTail(&(gs_stScheduler.stWaitList), pstRunningTask );
	}
	else
	{
		// 전 태스크의 컨텍스트를 이전 태스크 컨텍스트에 옮기고 레디리스트에 삽입
		kMemCpy( &( pstRunningTask->stContext), pcContextAddress, sizeof(CONTEXT) );
		kAddTaskToReadyList(pstRunningTask);
	}

	// 태스크 전환
	kMemCpy( pcContextAddress, &(pstNextTask->stContext), sizeof(CONTEXT) );

	// 프로세스 사용 시간을 업데이트
	gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;
	return TRUE; // rax 레지스터로 넘어가서 스택상 불이익이 없다.
}
BOOL kEndTask( QWORD qwTaskID )
{
	TCB* pstTarget;
	BYTE bPriority;
	\
	pstTarget = gs_stScheduler.pstRunningTask;

	if( pstTarget->stLink.qwID == qwTaskID )
	{
		pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
		SETPRIORITY( pstTarget->qwFlags, TASK_FLAGS_WAIT );

		kSchedule();

		// 아랫부분은 절대 실행되지 않음
		while(1);
	}
	else
	{
		pstTarget = kRemoveTaskFromReadyList( qwTaskID );
		if( pstTarget == NULL )
		{
			pstTarget = kGetTCBInTCBPool( GETTCBOFFSET( qwTaskID ) );
			if( pstTarget != NULL )
			{
				pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
				SETPRIORITY( pstTarget->qwFlags, TASK_FLAGS_WAIT );
			}
			return FALSE;
		}
		pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
		SETPRIORITY( pstTarget->qwFlags, TASK_FLAGS_WAIT );
		kAddListToTail( &(gs_stScheduler.stWaitList), pstTarget );
	}
	return TRUE;
}

void kExitTask( void )
{
	kEndTask( gs_stScheduler.pstRunningTask->stLink.qwID);
}

int kGetReadyTaskCount(void)
{
	int iTotalCount = 0;
	int i;

	for( i = 0 ; i < TASK_MAXREADYLISTCOUNT ; i++ )
	{
		iTotalCount += kGetListCount( &(gs_stScheduler.vstReadyList[ i ] ) );
	}
	return iTotalCount;
}

// Wait + ready + 실행중
int kGetTaskCount( void )
{
	int iTotalCount;

	iTotalCount = kGetReadyTaskCount();
	iTotalCount += kGetListCount( &(gs_stScheduler.stWaitList) ) + 1;

	return iTotalCount;
}

TCB* kGetTCBInTCBPool( int iOffset )
{
	if( (iOffset < -1) || (iOffset > TASK_MAXCOUNT) )
	{
		return NULL;
	}

	return &(gs_stTCBPoolManager.pstStartAddress[iOffset]);
}

BOOL kIsTaskExist( QWORD qwID )
{
	TCB* pstTCB;

	pstTCB = kGetTCBInTCBPool( GETTCBOFFSET( qwID ) );

	if( (pstTCB == NULL) || (pstTCB->stLink.qwID != qwID ))
	{
		return FALSE;
	}
	return TRUE;
}

QWORD kGetProcessorLoad( void )
{
	return gs_stScheduler.qwProcessLoad;
}

void kIdleTask( void )
{
	TCB* pstTask;
	// SpendTickInIdleTask << 누적된 틱카운트 MeasrueTickCount << 틱카운트 재기
	QWORD qwLastMeasureTickCount, qwLastSpendTickInIdleTask;
	QWORD qwCurrentMeasureTickCount, qwCurrentSpendTickInIdleTask;

	qwLastSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;
	qwLastMeasureTickCount = kGetTickCount();

	while(1)
	{
		qwCurrentMeasureTickCount = kGetTickCount();
		qwCurrentSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;

		if( qwCurrentMeasureTickCount - qwLastMeasureTickCount == 0 )
		{
			gs_stScheduler.qwProcessLoad = 0;
		}
		else
		{
			// 왜?
			gs_stScheduler.qwProcessLoad = 100 - ( qwCurrentSpendTickInIdleTask - qwLastSpendTickInIdleTask )*100/(qwCurrentMeasureTickCount - qwLastMeasureTickCount);
		}
		// 현재 상태를 이전 상태에 보관
		qwLastMeasureTickCount = qwCurrentMeasureTickCount;
		qwLastSpendTickInIdleTask = qwCurrentSpendTickInIdleTask;

		kHaltProcessorByLoad();

		if( kGetListCount( & gs_stScheduler.stWaitList ) >= 0 )
		{
			while(1)
			{
				// 종료 플래그 프로세스 정리
				pstTask = kRemoveListFromHeader( &(gs_stScheduler.stWaitList) );
				if( pstTask == NULL )
				{
					break;
				}
				kPrintf("IDLE : Task ID[0x%q] is completely ended.\n",pstTask->stLink.qwID);
				kFreeTCB(pstTask->stLink.qwID);
			}
		}
		kSchedule();
	}

}

void kHaltProcessorByLoad( void )
{
	if( gs_stScheduler.qwProcessLoad < 40 )
	{
		kHlt();
		kHlt();
		kHlt();
	}
	else if( gs_stScheduler.qwProcessLoad < 80 )
	{
		kHlt();
		kHlt();
	}
	else if( gs_stScheduler.qwProcessLoad < 95 )
	{
		kHlt();
	}
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
