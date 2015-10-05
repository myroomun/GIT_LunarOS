/*
 * Task.c
 *
 *  Created on: 2015. 10. 1.
 *      Author: user
 */

#include "Task.h"
#include "Descriptor.h"
#include "Utility.h"

// �����췯 ���� �ڷᱸ��
static SCHEDULER gs_stScheduler;
static TCBPOOLMANAGER gs_stTCBPoolManager;

void kSetUpTask( TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize )
{
	// ���ؽ�Ʈ �ʱ�ȭ
	kMemSet( pstTCB->stContext.vgRegister, 0, sizeof(pstTCB->stContext.vgRegister ));

	// ���ÿ� ���õ� RSP, RBP �������� ����
	// �̶� �������ʹ� ����� �������� �����̹Ƿ� �ֻ�ܿ� �÷��ش�.
	pstTCB->stContext.vgRegister[ TASK_RSPOFFSET ] = (QWORD) pvStackAddress + qwStackSize;
	pstTCB->stContext.vgRegister[ TASK_RBPOFFSET ] = (QWORD) pvStackAddress + qwStackSize;

	// ���׸�Ʈ ������ ���� (���� Ŀ�γ����� ����Ǵ� �ڵ��̴�.)
	pstTCB->stContext.vgRegister[ TASK_CSOFFSET ] = GDT_KERNELCODESEGMENT;
	pstTCB->stContext.vgRegister[ TASK_DSOFFSET ] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vgRegister[ TASK_ESOFFSET ] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vgRegister[ TASK_FSOFFSET ] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vgRegister[ TASK_GSOFFSET ] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vgRegister[ TASK_SSOFFSET ] = GDT_KERNELDATASEGMENT;

	// RIP�� �ʱ� �ּ� ��巹��
	pstTCB->stContext.vgRegister[ TASK_RIPOFFSET ] = qwEntryPointAddress;

	// RGLAGS �������� IF ��Ʈ(��Ʈ9)�� 1�� ����
	pstTCB->stContext.vgRegister[ TASK_RFLAGSOFFSET ] |= 0x200;

	// ID, ����, ������, �÷��� ����
	//pstTCB->qwID = qwID; (qwID�� TCB allocation���� �Ѿ)
	pstTCB->pvStackAddress = pvStackAddress;
	pstTCB->qwStackSize = qwStackSize;
	pstTCB->qwFlags = qwFlags;
}

// �½�ũ Ǯ�� �½�ũ ���õ� �Լ���
void kInitializeTCBPool( void )
{
	int i;

	kMemSet( &(gs_stTCBPoolManager), 0, sizeof(gs_stTCBPoolManager) );

	// �½�ũ Ǯ�� ��巹���� �����ϰ� �ʱ�ȭ
	gs_stTCBPoolManager.pstStartAddress = (TCB*)TASK_TCBPOOLADDRESS;
	kMemSet(TASK_TCBPOOLADDRESS, 0, sizeof( TCB ) * TASK_MAXCOUNT );

	// TCB�� LIST ID �Ҵ�
	for( i = 0 ; i < TASK_MAXCOUNT ; i++ )
	{
		gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;
	}

	// TCB�� �ִ� ������ �Ҵ�� Ƚ���� �ʱ�ȭ
	gs_stTCBPoolManager.iMaxCount = TASK_MAXCOUNT;
	gs_stTCBPoolManager.iAllocatedCount = 1;
}

// TCB�� ����޴´�.
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
		// allocated�� �ѹ� Ȯ�� (���� 32��Ʈ qwid�ѹ��� �׻� ������� �����Ѵ�.)
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

	// ���� 32��Ʈ�� �ε��� ����
	i = GETTCBOFFSET( qwID );

	kMemSet( &(gs_stTCBPoolManager.pstStartAddress[i].stContext), 0 , sizeof(CONTEXT) );
	// ������ ���� ��Ʈ�� �����
	gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;

	gs_stTCBPoolManager.iUseCount--;
}

// �½�ũ ����
TCB* kCreateTask( QWORD qwFlags, QWORD qwEntryPointAddress )
{
	TCB* pstTask;
	void* pvStackAddress;

	pstTask = kAllocateTCB();
	if( pstTask == NULL )
	{
		return NULL;
	}

	// ���� �ε��� ��Ʈ * ���û����� + ����Ǯ���̽�
	pvStackAddress = ( void* ) (TASK_STACKPOOLADDRESS + ( TASK_STACKSIZE * GETTCBOFFSET(pstTask->stLink.qwID) ) );


	kSetUpTask( pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE );
	kAddTaskToReadyList( pstTask );

	return pstTask;
}

// �����췯 ����

// TCB �ʱ�ȭ �� �����췯 �Ŵ����� �ʱ�ȭ
void kInitializeScheduler( void )
{
	int i;

	kInitializeTCBPool();

	for( i = 0 ; i < TASK_MAXREADYLISTCOUNT ; i++ )
	{
		kInitializeList( &(gs_stScheduler.vstReadyList[ i ]));
		gs_stScheduler.viExecuteCount[ i ] = 0;
	}

	// TCB�� �Ҵ�޾� �������� �½�ũ�� ����, ������ ������ �½�ũ�� ������ TCB�� �غ�
	gs_stScheduler.pstRunningTask = kAllocateTCB();
	gs_stScheduler.pstRunningTask->qwFlags = TASK_FLAGS_HIGHEST; // ���� �켱���� ����

	//���μ��� ���� ��� ���� �ʱ�ȭ
	gs_stScheduler.qwSpendProcessorTimeInIdleTask = 0;
	gs_stScheduler.qwProcessLoad = 0;
}

// ���� �������� �½�ũ�� ����
void kSetRunningTask( TCB* pstTask )
{
	gs_stScheduler.pstRunningTask = pstTask;
}

TCB* kGetRunningTask( void )
{
	return gs_stScheduler.pstRunningTask;
}

// �½�ũ ����Ʈ���� �������� ������ �½�ũ�� ����
TCB* kGetNextTaskToRun( void )
{
	TCB* pstTarget = NULL;
	int iTaskCount, i, j;

	for( j = 0 ;  j < 2 ; j++ )
	{
		for( i = 0 ; i < TASK_MAXREADYLISTCOUNT ; i++ )
		{
			iTaskCount = kGetListCount( &(gs_stScheduler.vstReadyList[ i ] ));

			// ���� ť�� �ִ� �½�ũ������ŭ ������ ��������
			if( gs_stScheduler.viExecuteCount[ i ] < iTaskCount )
			{
				pstTarget = (TCB*)kRemoveListFromHeader(&(gs_stScheduler.vstReadyList[i]));
				gs_stScheduler.viExecuteCount[i]++;
				break;
			}
			// �� ������ �纸
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

// �½�ũ�� �����ٷ��� �غ� ����Ʈ�� ����
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

	// ���� �������� �½�ũ�̸� �켱������ ����
	// ���߿� �½�ũ ����Ī�� �Ͼ�� �½�ũ�� ����ť�� ������ �з��ȴ�.
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
			// �½�ũ ���̵�� ���� ã��
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

// �ٸ� �½�ũ�� ã�Ƽ� ��ȯ (���ܹ߻��ΰ��� ȣ�� ���� ) => ����ó���� �ٸ��� �ؾ���
void kSchedule( void )
{
	TCB* pstRunningTask, * pstNextTask;
	BOOL bPreviousFlag;

	if( kGetReadyTaskCount() < 1 )
	{
		return;
	}

	// ��ȯ�ϴ� ���� ���ͷ�Ʈ�� �߻��Ͽ� �½�ũ ��ȯ�� �Ͼ�� ���!
	bPreviousFlag = kSetInterruptFlag( FALSE );

	// ������ ���� �½�ũ�� ����
	pstNextTask = kGetNextTaskToRun();
	if( pstNextTask == NULL )
	{
		kSetInterruptFlag( bPreviousFlag );
		return;
	}

	// ���� �������� �½�ũ�� ������ ������ �� ���ؽ�Ʈ ��ȯ
	pstRunningTask = gs_stScheduler.pstRunningTask;
	gs_stScheduler.pstRunningTask = pstNextTask;

	// �� �½�ũ�� �����½�ũ������ ���μ��� �ð��� ������Ŵ
    if( ( pstRunningTask->qwFlags & TASK_FLAGS_IDLE ) == TASK_FLAGS_IDLE )
    {
        gs_stScheduler.qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME - gs_stScheduler.iProcessorTime;
    }

    // �½�ũ �����÷��װ� ������ ��� ���ؽ�Ʈ ���� X, ��⸮��Ʈ�� ����
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

// ���ͷ�Ʈ�� �߻��� �� �ٸ� �½�ũ ã��
BOOL kScheduleInInterrupt( void )
{
	TCB* pstRunningTask, * pstNextTask;
	char* pcContextAddress;

	// ��ȯ�� �½�ũ�� ������ ����
	pstNextTask = kGetNextTaskToRun();
	if( pstNextTask == NULL )
	{
		return FALSE;
	}

	// �½�ũ ��ȯ ó��
	pcContextAddress = (char*) IST_STARTADDRESS + IST_SIZE - sizeof( CONTEXT );
	// IST_STARTADDRESS + IST_SIZE�� ���ͷ�Ʈ stack top �� �̹� ���ؽ�Ʈ ����Ǿ�����
	// �̶� Context ������ �ȿ� �����ڵ�� �������� ����

	// ���� �½�ũ�� �� IST�� �ִ� ���ؽ�Ʈ�� �����ϰ�, ���� �½�ũ�� �غ� ����Ʈ�� �ű�
	pstRunningTask = gs_stScheduler.pstRunningTask;
	gs_stScheduler.pstRunningTask = pstNextTask;

	// ���� �½�ũ���� ��ȯ�Ǿ��ٸ� ����� ���μ��� �ð� ����
	if( (pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE )
	{
		gs_stScheduler.qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME;
	}

	// �½�ũ ���� �÷��װ� ������ ��� ���ؽ�Ʈ �������� �ʰ� ��⸮��Ʈ�� ����
	if( pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK )
	{
		kAddListToTail(&(gs_stScheduler.stWaitList), pstRunningTask );
	}
	else
	{
		// �� �½�ũ�� ���ؽ�Ʈ�� ���� �½�ũ ���ؽ�Ʈ�� �ű�� ���𸮽�Ʈ�� ����
		kMemCpy( &( pstRunningTask->stContext), pcContextAddress, sizeof(CONTEXT) );
		kAddTaskToReadyList(pstRunningTask);
	}

	// �½�ũ ��ȯ
	kMemCpy( pcContextAddress, &(pstNextTask->stContext), sizeof(CONTEXT) );

	// ���μ��� ��� �ð��� ������Ʈ
	gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;
	return TRUE; // rax �������ͷ� �Ѿ�� ���û� �������� ����.
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

		// �Ʒ��κ��� ���� ������� ����
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

// Wait + ready + ������
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
	// SpendTickInIdleTask << ������ ƽī��Ʈ MeasrueTickCount << ƽī��Ʈ ���
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
			// ��?
			gs_stScheduler.qwProcessLoad = 100 - ( qwCurrentSpendTickInIdleTask - qwLastSpendTickInIdleTask )*100/(qwCurrentMeasureTickCount - qwLastMeasureTickCount);
		}
		// ���� ���¸� ���� ���¿� ����
		qwLastMeasureTickCount = qwCurrentMeasureTickCount;
		qwLastSpendTickInIdleTask = qwCurrentSpendTickInIdleTask;

		kHaltProcessorByLoad();

		if( kGetListCount( & gs_stScheduler.stWaitList ) >= 0 )
		{
			while(1)
			{
				// ���� �÷��� ���μ��� ����
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
