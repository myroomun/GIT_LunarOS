/*
 * InterruptHandler.c
 *
 *  Created on: 2015. 9. 22.
 *      Author: user
 */

#include "InterruptHandler.h"
#include "PIC.h"

void kCommonExceptionHandler( int iVectorNumber, QWORD qwErrorCode )
{
	char vcBuffer[3] = {0,};

	// ���ͷ�Ʈ ���͸� ȭ�� ������ ���� 2�ڸ� ������ ���
	vcBuffer[ 0 ] = '0' + iVectorNumber / 10;
	vcBuffer[ 1 ] = '0' + iVectorNumber % 10;

	kPrintString( 0, 0, "============================================");
	kPrintString( 0, 1, "           Exception Occur~!!!              ");
	kPrintString( 0, 2, "============================================");
	kPrintString( 27, 2, vcBuffer );

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
	kPrintString( 70, 0, vcBuffer);

	kSendEOIToPIC( iVectorNumber - PIC_IRQSTARTVECTOR );
}

void kKeyboardHandler( int iVectorNumber )
{
	char vcBuffer[] = "[INT:  , ]";
	static int g_iKeyboardInterruptCount = 0;

	vcBuffer[ 5 ] = '0' + iVectorNumber / 10;
	vcBuffer[ 6 ] = '0' + iVectorNumber % 10;
	vcBuffer[ 8 ] = '0' + g_iKeyboardInterruptCount;

	g_iKeyboardInterruptCount = (g_iKeyboardInterruptCount + 1) % 10;
	kPrintString( 0, 0, vcBuffer );

	kSendEOIToPIC( iVectorNumber - PIC_IRQSTARTVECTOR );
}