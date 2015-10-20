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

static void kSetUpTask( TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize )
{
	// ���ؽ�Ʈ �ʱ�ȭ
	kMemSet( pstTCB->stContext.vgRegister, 0, sizeof(pstTCB->stContext.vgRegister ));

	// ���ÿ� ���õ� RSP, RBP �������� ����
	// �̶� �������ʹ� ����� �������� �����̹Ƿ� �ֻ�ܿ� �÷��ش�.
	pstTCB->stContext.vgRegister[ TASK_RSPOFFSET ] = (QWORD) pvStackAddress + qwStackSize - 8;
	pstTCB->stContext.vgRegister[ TASK_RBPOFFSET ] = (QWORD) pvStackAddress + qwStackSize - 8;

	* (QWORD *)( ( QWORD ) pvStackAddress + qwStackSize - 8 ) = (QWORD)kExitTask;

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
static void kInitializeTCBPool( void )
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
static TCB* kAllocateTCB( void )
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

static void kFreeTCB( QWORD qwID )
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
TCB* kCreateTask( QWORD qwFlags, void* pvMemoryAddress, QWORD qwMemorySize, QWORD qwEntryPointAddress )
{
	TCB* pstTask, *pstProcess;
	void* pvStackAddress;
	BOOL bPreviousFlag;

	// �Ʒ��κ��� TCB�� �Ҵ�޴� �κ�����, Main console shell�� ������ ȣ����� �� ����! (�� ������ ������ �ƴѵ�)
	bPreviousFlag = kLockForSystemData();
	pstTask = kAllocateTCB();
	if( pstTask == NULL )
	{
		kUnlockForSystemData( bPreviousFlag );
		return NULL;
	}


	pstProcess = kGetProcessByThread( kGetRunningTask() );

	if( pstProcess == NULL )
	{
		kFreeTCB(pstTask->stLink.qwID);
		kUnlockForSystemData( bPreviousFlag);
		return NULL;
	}

	if( qwFlags & TASK_FLAGS_THREAD )
	{
		pstTask->qwParentProcessID = pstProcess->stLink.qwID;
		pstTask->pvMemoryAddress = pstProcess->pvMemoryAddress;
		pstTask->qwMemorySize = pstProcess->qwMemorySize;

		kAddListToTail( &( pstProcess->stChildThreadList ), &( pstTask->stThreadLink));
	}
	else
	{
		pstTask->qwParentProcessID = pstProcess->stLink.qwID;
		pstTask->pvMemoryAddress = pvMemoryAddress;
		pstTask->qwMemorySize = qwMemorySize;
	}

	// ���� �ε��� ��Ʈ * ���û����� + ����Ǯ���̽�
	pstTask->stThreadLink.qwID = pstTask->stLink.qwID;
	kUnlockForSystemData( bPreviousFlag );


	pvStackAddress = ( void* ) (TASK_STACKPOOLADDRESS + ( TASK_STACKSIZE * GETTCBOFFSET(pstTask->stLink.qwID) ) );

	// setup task�� �̹� �� �θ� �Լ��� �������̴�.
	kSetUpTask( pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE );

	kInitializeList( &( pstTask->stChildThreadList ) );

	pstTask->bFPUUsed = FALSE;

	bPreviousFlag = kLockForSystemData();
	kAddTaskToReadyList( pstTask );
	kUnlockForSystemData();

	return pstTask;
}

// �����췯 ����

// TCB �ʱ�ȭ �� �����췯 �Ŵ����� �ʱ�ȭ
void kInitializeScheduler( void )
{
	int i;
	TCB* pstTask;

	kInitializeTCBPool();

	for( i = 0 ; i < TASK_MAXREADYLISTCOUNT ; i++ )
	{
		kInitializeList( &(gs_stScheduler.vstReadyList[ i ]));
		gs_stScheduler.viExecuteCount[ i ] = 0;
	}
	kInitializeList( &(gs_stScheduler.stWaitList ) );

	pstTask = kAllocateTCB();

	// TCB�� �Ҵ�޾� �������� �½�ũ�� ����, ������ ������ �½�ũ�� ������ TCB�� �غ�
	gs_stScheduler.pstRunningTask = pstTask;
	pstTask->qwFlags = TASK_FLAGS_HIGHEST | TASK_FLAGS_PROCESS | TASK_FLAGS_SYSTEM; // ���� �켱���� ����
	pstTask->qwParentProcessID = pstTask->stLink.qwID;
	pstTask->pvMemoryAddress = (void*) 0x100000;
	pstTask->qwMemorySize = 0x500000;
	pstTask->pvStackAddress = (void*) 0x600000;
	pstTask->qwStackSize = 0x100000;

	//���μ��� ���� ��� ���� �ʱ�ȭ
	gs_stScheduler.qwSpendProcessorTimeInIdleTask = 0;
	gs_stScheduler.qwProcessLoad = 0;
	// FPU�� �ֱٿ� ����� �½�ũ ���̵� Invalid �ʱ�ȭ
	gs_stScheduler.qwLastFPUUsedTaskID = TASK_INVALIDID;
}

// ���� �������� �½�ũ�� ����
void kSetRunningTask( TCB* pstTask )
{
	BOOL bPreviousFlag;

	bPreviousFlag = kLockForSystemData();
	gs_stScheduler.pstRunningTask = pstTask;
	kUnlockForSystemData( bPreviousFlag );
}

TCB* kGetRunningTask( void )
{
	BOOL bPreviousFlag;
	TCB* pstRunningTask;

	bPreviousFlag = kLockForSystemData();
	pstRunningTask = gs_stScheduler.pstRunningTask;
	kUnlockForSystemData(bPreviousFlag);

	return pstRunningTask;
}

// �½�ũ ����Ʈ���� �������� ������ �½�ũ�� ����
static TCB* kGetNextTaskToRun( void )
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
static BOOL kAddTaskToReadyList( TCB* pstTask )
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

static TCB* kRemoveTaskFromReadyList(QWORD qwTaskID)
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
	BOOL bPreviousFlag;

	if( bPriority > TASK_MAXREADYLISTCOUNT )
	{
		return FALSE;
	}

	bPreviousFlag = kLockForSystemData();
	// ���ͷ�Ʈ �����층 �ڵ鷯�� ������ �ڷᱸ���� ���� ����Ѵ�.
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
	kUnlockForSystemData( bPreviousFlag );
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
	bPreviousFlag = kLockForSystemData();

	// ������ ���� �½�ũ�� ����
	pstNextTask = kGetNextTaskToRun();
	if( pstNextTask == NULL )
	{
		kUnlockForSystemData(bPreviousFlag);
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

    // ������ ������ �½�ũ�� FPU�� �� �½�ũ�� �ƴ϶�� TS��Ʈ ���� (7�� ���ͷ�Ʈ�� �߻��� �� ����)
    if( gs_stScheduler.qwLastFPUUsedTaskID != pstNextTask->stLink.qwID )
    {
    	kSetTS();
    }
    else
    {
    	kClearTS();
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

	kUnlockForSystemData(bPreviousFlag);
}

// ���ͷ�Ʈ�� �߻��� �� �ٸ� �½�ũ ã��
BOOL kScheduleInInterrupt( void )
{
	TCB* pstRunningTask, * pstNextTask;
	char* pcContextAddress;
	BOOL bPreviousFlag;

	bPreviousFlag = kLockForSystemData();

	// ��ȯ�� �½�ũ�� ������ ����
	pstNextTask = kGetNextTaskToRun();
	if( pstNextTask == NULL )
	{
		kUnlockForSystemData(bPreviousFlag);
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
	kUnlockForSystemData(bPreviousFlag);

    // ������ ������ �½�ũ�� FPU�� �� �½�ũ�� �ƴ϶�� TS��Ʈ ���� (7�� ���ͷ�Ʈ�� �߻��� �� ����)
    if( gs_stScheduler.qwLastFPUUsedTaskID != pstNextTask->stLink.qwID )
    {
    	kSetTS();
    }
    else
    {
    	kClearTS();
    }

	//�Ʒ����ʹ� �����ڿ��� �ƴ�

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
	BOOL bPreviousFlag;

	bPreviousFlag = kLockForSystemData();

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
				kUnlockForSystemData(bPreviousFlag);
				return TRUE;
			}
			kUnlockForSystemData(bPreviousFlag);
			return FALSE;
		}
		pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
		SETPRIORITY( pstTarget->qwFlags, TASK_FLAGS_WAIT );
		kAddListToTail( &(gs_stScheduler.stWaitList), pstTarget );
	}
	kUnlockForSystemData(bPreviousFlag);
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
	BOOL bPreviousFlag;

	bPreviousFlag = kLockForSystemData();

	for( i = 0 ; i < TASK_MAXREADYLISTCOUNT ; i++ )
	{
		iTotalCount += kGetListCount( &(gs_stScheduler.vstReadyList[ i ] ) );
	}

	kUnlockForSystemData( bPreviousFlag );
	return iTotalCount;
}

// Wait + ready + ������
int kGetTaskCount( void )
{
	int iTotalCount;
	BOOL bPreviousFlag;

	iTotalCount = kGetReadyTaskCount();

	bPreviousFlag = kLockForSystemData();
	iTotalCount += kGetListCount( &(gs_stScheduler.stWaitList) ) + 1;
	kUnlockForSystemData(bPreviousFlag);

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
	TCB* pstTask, * pstChildThread, * pstProcess;
	// SpendTickInIdleTask << ������ ƽī��Ʈ MeasrueTickCount << ƽī��Ʈ ���
	QWORD qwLastMeasureTickCount, qwLastSpendTickInIdleTask;
	QWORD qwCurrentMeasureTickCount, qwCurrentSpendTickInIdleTask;
	BOOL bPreviousFlag;
	QWORD qwTaskID;
	int i, iCount;
	void* pstThreadLink;

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
				bPreviousFlag = kLockForSystemData();
				// ���� �÷��� ���μ��� ����
				pstTask = kRemoveListFromHeader( &(gs_stScheduler.stWaitList) );
				if( pstTask == NULL )
				{
					kUnlockForSystemData( bPreviousFlag );
					break;
				}

				if( pstTask->qwFlags & TASK_FLAGS_PROCESS)
				{
					iCount = kGetListCount( &( pstTask->stChildThreadList ) );
					for( i = 0 ; i < iCount ; i++ )
					{
						// ������ ����Ʈ �ȿ� �ִ� �����带 �����Ű�� ���� �� �����
						pstThreadLink = (TCB*)kRemoveListFromHeader(&(pstTask->stChildThreadList));
						if( pstThreadLink == NULL )
						{
							break;
						}
						pstChildThread = GETTCBFROMTHREADLINK( pstThreadLink );
						kAddListToTail( &(pstTask->stChildThreadList), &(pstChildThread->stThreadLink));
						// �½�ũ ��������
						kEndTask( pstChildThread->stLink.qwID );
					}

					// ���� �ڽ� �����尡 �����ִ°�?
					if( kGetListCount( &( pstTask->stChildThreadList) ) > 0 )
					{
						// ���� �ڽ� �����尡 �ٸ� �ھ�� ����Ǿ� �ٷ� ���ᰡ �Ұ����� ��� ������ �� �������� �ٽ� �˻��Ѵ�.
						kAddListToTail( &(gs_stScheduler.stWaitList), pstTask);
						kUnlockForSystemData( bPreviousFlag );
						continue;
					}
					else
					{
						// �Ҵ� �޸� ����
						// TODO
					}
				}
				else if( pstTask->qwFlags & TASK_FLAGS_THREAD )
				{
					pstProcess = kGetProcessByThread( pstTask );
					if( pstProcess != NULL )
					{
						kRemoveList( &(pstProcess->stChildThreadList), pstTask->stLink.qwID);
					}
				}
				qwTaskID = pstTask->stLink.qwID;
				kFreeTCB(qwTaskID);
				kUnlockForSystemData( bPreviousFlag );

				kPrintf("IDLE : Task ID[0x%q] is completely ended.\n",pstTask->stLink.qwID);
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

static TCB* kGetProcessByThread( TCB* pstThread )
{
	TCB* pstProcess;

	if( pstThread->qwFlags & TASK_FLAGS_PROCESS )
	{
		return pstThread;
	}

	// ���� ��������
	pstProcess = kGetTCBInTCBPool( GETTCBOFFSET(pstThread->qwParentProcessID));

	if( ( pstProcess == NULL ) || ( pstProcess->stLink.qwID != pstThread->qwParentProcessID ) )
	{
		return NULL;
	}

	return pstProcess;
}

QWORD kGetLastFPUUsedTaskID( void )
{
	return gs_stScheduler.qwLastFPUUsedTaskID;
}

void kSetLastFPUUsedTaskID( QWORD qwTaskID )
{
	gs_stScheduler.qwLastFPUUsedTaskID = qwTaskID;
}
