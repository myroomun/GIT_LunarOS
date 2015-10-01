/*
 * Task.c
 *
 *  Created on: 2015. 10. 1.
 *      Author: user
 */

#include "Task.h"
#include "Descriptor.h"

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

	i = qwID & 0xFFFFFFFF; // ���� ��Ʈ�� �ε��� ��Ʈ ���� ��Ʈ�� �Ҵ�Ƚ�� ��Ʈ
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
	pvStackAddress = ( void* ) (TASK_STACKPOOLADDRESS + ( TASK_STACKSIZE * (pstTask->stLink.qwID & 0xFFFFFFFF ) ) );


	kSetUpTask( pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE );
	kAddTaskToReadyList( pstTask );

	return pstTask;
}

// �����췯 ����

// TCB �ʱ�ȭ �� �����췯 �Ŵ����� �ʱ�ȭ
void kInitializeScheduler( void )
{
	kInitializeTCBPool();

	kInitializeList( &(gs_stScheduler.stReadyList ) );

	// TCB�� �Ҵ�޾� �������� �½�ũ�� ����, ������ ������ �½�ũ�� ������ TCB�� �غ�
	gs_stScheduler.pstRunningTask = kAllocateTCB();

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
	if( kGetListCount( &( gs_stScheduler.stReadyList ) ) == 0 )
	{
		return NULL;
	}

	return (TCB*)kRemoveListFromHeader( &( gs_stScheduler.stReadyList ) );
}

// �½�ũ�� �����ٷ��� �غ� ����Ʈ�� ����
void kAddTaskToReadyList( TCB* pstTask )
{
	kAddListToTail( &(gs_stScheduler.stReadyList), pstTask );
}

// �ٸ� �½�ũ�� ã�Ƽ� ��ȯ (���ܹ߻��ΰ��� ȣ�� ���� ) => ����ó���� �ٸ��� �ؾ���
void kSchedule( void )
{
	TCB* pstRunningTask, * pstNextTask;
	BOOL bPreviousFlag;

	if( kGetListCount( &( gs_stScheduler.stReadyList) ) == 0 )
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

	pstRunningTask = gs_stScheduler.pstRunningTask;
	kAddTaskToReadyList(pstRunningTask);

	gs_stScheduler.pstRunningTask = pstNextTask;
	gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

	// �� �Լ��� ���� �� ���ؽ�Ʈ ���� �Ǿ����� �� �������� �����. �� �����½�ũ�� ���ؽ�Ʈ�� ���� �� �Լ��� ȣ���� �½�ũ�� �� �������� �����
	kSwitchContext( &(pstRunningTask->stContext), &(pstNextTask->stContext));

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
	kMemCpy( &( pstRunningTask->stContext), pcContextAddress, sizeof(CONTEXT) );
	kAddTaskToReadyList(pstRunningTask);

	// ��ȯ�Ͽ� ������ �½�ũ�� running task�� ����, ���ؽ�Ʈ�� IST�� ����
	gs_stScheduler.pstRunningTask = pstNextTask;
	kMemCpy( pcContextAddress, &(pstNextTask->stContext), sizeof(CONTEXT) );

	// ���μ��� ��� �ð��� ������Ʈ
	gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;
	return TRUE; // rax �������ͷ� �Ѿ�� ���û� �������� ����.
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
