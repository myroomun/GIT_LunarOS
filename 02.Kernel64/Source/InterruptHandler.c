/*
 * InterruptHandler.c
 *
 *  Created on: 2015. 9. 22.
 *      Author: user
 */

#include "InterruptHandler.h"
#include "PIC.h"
#include "Keyboard.h"
#include "Console.h"
#include "Utility.h"
#include "Task.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"

void kCommonExceptionHandler( int iVectorNumber, QWORD qwErrorCode )
{
	char vcBuffer[3] = {0,};

	// ���ͷ�Ʈ ���͸� ȭ�� ������ ���� 2�ڸ� ������ ���
	vcBuffer[ 0 ] = '0' + iVectorNumber / 10;
	vcBuffer[ 1 ] = '0' + iVectorNumber % 10;

	kPrintStringXY( 0, 0, "============================================");
	kPrintStringXY( 0, 1, "           Exception Occur~!!!              ");
	kPrintStringXY( 0, 2, "           Vector :                         ");
	kPrintStringXY( 0, 3, "============================================");
	kPrintStringXY( 19,2, vcBuffer);

	while(1);
}

void kCommonInterruptHandler( int iVectorNumber )
{
	char vcBuffer[] = "[INT:  , ]";
	static int g_iCommonInterruptCount = 0;

	vcBuffer[ 5 ] = '0' + iVectorNumber / 10;
	vcBuffer[ 6 ] = '0' + iVectorNumber % 10;
	vcBuffer[ 8 ] = '0' + g_iCommonInterruptCount;
	g_iCommonInterruptCount = ( g_iCommonInterruptCount + 1 ) % 10;
	kPrintStringXY( 70, 0, vcBuffer);

	kSendEOIToPIC( iVectorNumber - PIC_IRQSTARTVECTOR );
}

void kKeyboardHandler( int iVectorNumber )
{
	char vcBuffer[] = "[INT:  , ]";
	static int g_iKeyboardInterruptCount = 0;
	BYTE bTemp;


	vcBuffer[ 5 ] = '0' + iVectorNumber / 10;
	vcBuffer[ 6 ] = '0' + iVectorNumber % 10;
	vcBuffer[ 8 ] = '0' + g_iKeyboardInterruptCount;

	g_iKeyboardInterruptCount = (g_iKeyboardInterruptCount + 1) % 10;
	kPrintStringXY( 0, 0, vcBuffer );

	if( kIsOutputBufferFull() == TRUE )
	{
		bTemp = kGetKeyboardScanCode();
		kConvertScanCodeAndPutQueue( bTemp );
	}
	kSendEOIToPIC( iVectorNumber - PIC_IRQSTARTVECTOR );

}

void kTimerHandler( int iVectorNumber )
{
	char vcBuffer[] = "[INT:  , ]";
	static int g_iTimerInterruptCount = 0;

	vcBuffer[ 5 ] = '0' + iVectorNumber / 10;
	vcBuffer[ 6 ] = '0' + iVectorNumber % 10;
	vcBuffer[ 8 ] = '0' + g_iTimerInterruptCount;
	g_iTimerInterruptCount = ( g_iTimerInterruptCount + 1 ) % 10;
	kPrintStringXY( 70, 0, vcBuffer);

	kSendEOIToPIC( iVectorNumber - PIC_IRQSTARTVECTOR );

	// Ÿ�̸� �߻� Ƚ���� ����
	g_qwTickCount++;

	// �½�ũ�� ����� ���μ����� �ð��� ����
	kDecreaseProcessorTime();
	// ���μ����� ����� �� �ִ� �ð��� �� ��ٸ� �½�ũ ��ȯ
	if( kIsProcessorTimeExpired() == TRUE )
	{
		kScheduleInInterrupt();
	}
}

// �Ǽ� ����� ���� ���� �ҷ� ���� ����ߴ� �½�ũ�� FPU �������͸� ������
void kDeviceNotAvailableHandler( int iVectorNumber )
{
	TCB* pstFPUTask,* pstCurrentTask;
	QWORD qwLastFPUTaskID;

	//////////////////////
	char vcBuffer[] = "[EXC:  , ]";
	static int g_iFPUInterruptCount = 0;

	vcBuffer[ 5 ] = '0' + iVectorNumber / 10;
	vcBuffer[ 6 ] = '0' + iVectorNumber % 10;
	vcBuffer[ 8 ] = '0' + g_iFPUInterruptCount;

	g_iFPUInterruptCount = ( g_iFPUInterruptCount + 1 ) % 10;
	kPrintStringXY( 70, 0, vcBuffer);

	// TS �������� 0
	kClearTS();

	qwLastFPUTaskID = kGetLastFPUUsedTaskID();
	pstCurrentTask = kGetRunningTask();

	// ������ FPU ����Ѱ��� �ڽ��̸� �ƹ��͵� ����
	if( qwLastFPUTaskID == pstCurrentTask->stLink.qwID )
	{
		return;
	}
	// �ڽ��� �ƴ϶��
	else if( qwLastFPUTaskID != TASK_INVALIDID )
	{
		pstFPUTask = kGetTCBInTCBPool( GETTCBOFFSET(qwLastFPUTaskID) );
		if( ( pstFPUTask != NULL ) && (pstFPUTask->stLink.qwID == qwLastFPUTaskID ))
		{
			kSaveFPUContext( pstFPUTask->vqwFPUContext );
		}
	}

	// ���� �� �½�ũ�� FPU�� ����ߴٸ� FPU ���ؽ�Ʈ�� ����
	// �ƴϸ� FPU�� ����ߴٴ� ���¸� ����
	if( pstCurrentTask->bFPUUsed == FALSE )
	{
		kInitializeFPU();
		pstCurrentTask->bFPUUsed = TRUE;
	}
	else
	{
		kLoadFPUContext( pstCurrentTask->vqwFPUContext );
	}

	kSetLastFPUUsedTaskID( pstCurrentTask->stLink.qwID );
}
