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

	// 인터럽트 벡터를 화면 오른쪽 위에 2자리 정수로 출력
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

	// 타이머 발생 횟수를 증가
	g_qwTickCount++;

	// 태스크가 사용한 프로세서의 시간을 줄임
	kDecreaseProcessorTime();
	// 프로세서가 사용할 수 있는 시간을 다 썼다면 태스크 전환
	if( kIsProcessorTimeExpired() == TRUE )
	{
		kScheduleInInterrupt();
	}
}

// 실수 연산시 연산 전에 불려 전에 계산했던 태스크의 FPU 레지스터를 저장함
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

	// TS 레지스터 0
	kClearTS();

	qwLastFPUTaskID = kGetLastFPUUsedTaskID();
	pstCurrentTask = kGetRunningTask();

	// 이전에 FPU 사용한것이 자신이면 아무것도 안함
	if( qwLastFPUTaskID == pstCurrentTask->stLink.qwID )
	{
		return;
	}
	// 자신이 아니라면
	else if( qwLastFPUTaskID != TASK_INVALIDID )
	{
		pstFPUTask = kGetTCBInTCBPool( GETTCBOFFSET(qwLastFPUTaskID) );
		if( ( pstFPUTask != NULL ) && (pstFPUTask->stLink.qwID == qwLastFPUTaskID ))
		{
			kSaveFPUContext( pstFPUTask->vqwFPUContext );
		}
	}

	// 현재 이 태스크가 FPU를 사용했다면 FPU 콘텍스트를 복원
	// 아니면 FPU를 사용했다는 상태를 남김
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
