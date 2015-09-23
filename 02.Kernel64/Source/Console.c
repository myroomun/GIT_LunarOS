/*
 * Console.c
 *
 *  Created on: 2015. 9. 23.
 *      Author: user
 */


#include <stdarg.h>
#include "Console.h"
#include "Keyboard.h"

CONSOLEMANAGER gs_stConsoleManager = { 0, };

void kInitializeConsole( int iX, int iY )
{
	kMemSet( &gs_stConsoleManager, 0, sizeof( gs_stConsoleManager ));
	kSetCursor( iX, iY );
}

void kSetCursor( int iX, int iY )
{
	int iLinearValue;

	iLinearValue = iY * CONSOLE_WIDTH + iX;

	// CRTC 컨트롤 어드레스 레지스터에 0x0E를 전송하여 상위 커서위치 레지스터 선택
	kOutPortByte( VGA_PORT_INDEX, VGA_INDEX_UPPERCURSOR );
	// CRTC 컨트롤 데이터 레지스터에 커서의 상위 바이트 출력
	kOutPortByte( VGA_PORT_DATA, iLinearValue >> 8 );

	// CRTC 컨트롤 어드레스 레지스터에 0x0F를 전송하여 하위 커서 위치 레지스터 선택
	kOutPortByte( VGA_PORT_INDEX, VGA_INDEX_LOWERCURSOR );
	// CRTC 컨트롤 데이터 레지스터에 커서의 하위 바이트 출력
	kOutPortByte( VGA_PORT_DATA, iLinearValue & 0xFF );

	gs_stConsoleManager.iCurrentPrintOffset = iLinearValue;

}

void kGetCursor( int* piX, int* piY )
{
	*piX = gs_stConsoleManager.iCurrentPrintOffset % CONSOLE_WIDTH;
	*piY = gs_stConsoleManager.iCurrentPrintOffset / CONSOLE_WIDTH;
}

void kPrintf( const char* pcFormatString, ... )
{
	va_list ap;
	char vcBuffer[ 1024 ];
	int iNextPrintOffset;

	// 가변인자 처리
	va_start( ap, pcFormatString );
	kVSPrintf( vcBuffer, pcFormatString, ap );
	va_end( ap );

	iNextPrintOffset = kConsolePrintString( vcBuffer );

	kSetCursor( iNextPrintOffset % CONSOLE_WIDTH, iNextPrintOffset / CONSOLE_WIDTH );
}

int kConsolePrintString( const char* pcBuffer )
{
	CHARACTER* pstScreen = (CHARACTER*) CONSOLE_VIDEOMEMORYADDRESS;
	int i, j;
	int iLength;
	int iPrintOffset;

	iPrintOffset = gs_stConsoleManager.iCurrentPrintOffset;

	iLength = kStrLen( pcBuffer );
	for( i = 0 ; i < iLength ; i++ )
	{
		if( pcBuffer[i] == '\n' )
		{
			// 현재 위치에서 끝까지의 오프셋을 구한 뒤, 더해준다. (새줄)
			iPrintOffset += ( CONSOLE_WIDTH - ( iPrintOffset % CONSOLE_WIDTH ) );
		}
		else if( pcBuffer[i] == '\t' )
		{
			iPrintOffset += ( 8 - (iPrintOffset % 8 ) );
		}
		else
		{
			// 화면 출력처리
			pstScreen[ iPrintOffset ].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
			pstScreen[ iPrintOffset ].bCharacter = pcBuffer[ i ];
			iPrintOffset++;
		}

		// 출력 범위를 벗어났으면
		if( iPrintOffset >= CONSOLE_HEIGHT * CONSOLE_WIDTH )
		{
			// 맨 위 한줄을 제외한 나머지를 한줄씩 위로 복사
			kMemCpy( CONSOLE_VIDEOMEMORYADDRESS, CONSOLE_VIDEOMEMORYADDRESS + CONSOLE_WIDTH * sizeof(CHARACTER), (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * sizeof( CHARACTER ));
			// 맨 마지막 줄은 공백처리
			for( j = (CONSOLE_HEIGHT - 1) * (CONSOLE_WIDTH) ; j < ( CONSOLE_HEIGHT * CONSOLE_WIDTH ) ; j++ )
			{
				pstScreen[ j ].bCharacter = ' ';
				pstScreen[ j ].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
			}

			// 출력 위치를 가장 아래쪽 라인의 처음으로 설정
			iPrintOffset = ( CONSOLE_HEIGHT -1 ) * CONSOLE_WIDTH;
		}
	}
	return iPrintOffset;
}

void kClearScreen( void )
{
	CHARACTER* pstScreen = ( CHARACTER* ) CONSOLE_VIDEOMEMORYADDRESS;
	int i;

	for( i = 0 ; i < CONSOLE_WIDTH * CONSOLE_HEIGHT ; i++ )
	{
		pstScreen[ i ].bCharacter = ' ';
		pstScreen[ i ].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
	}

	kSetCursor( 0, 0 );
}


BYTE kGetCh( void )
{
	KEYDATA stData;

	while(1)
	{
		while( kGetKeyFromKeyQueue( &stData ) == FALSE )
		{
			;
		}

		if( stData.bFlags & KEY_FLAGS_DOWN )
		{
			return stData.bASCIICode;
		}
	}
}

void kPrintStringXY( int iX, int iY, const char* pcString )
{
	CHARACTER* pstScreen = (CHARACTER*) CONSOLE_VIDEOMEMORYADDRESS;
	int i;

	pstScreen += (iY * 80) + iX;

	for( i = 0 ; pcString[i] != 0 ; i++ )
	{
		pstScreen[ i ].bCharacter = pcString[i];
		pstScreen[ i ].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
	}
}
