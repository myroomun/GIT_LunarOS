/*
 * PIT.c
 *
 *  Created on: 2015. 9. 24.
 *      Author: user
 */

#include "PIT.h"

void kInitializePIT( WORD wCount, BOOL bPeriodic )
{
	// PIT 컨트롤 레지스터(0x43)에 값을 초기화하여 카운트를 멈춘 뒤에 모드 0에 바이너리 카운터로 설정

	kOutPortByte( PIT_PORT_CONTROL, PIT_COUNTER0_ONCE );

	// 만약 일정한 주기라면
	if( bPeriodic == TRUE )
	{
		// 모드2로 설정
		kOutPortByte( PIT_PORT_CONTROL, PIT_COUNTER0_PERIODIC );
	}
	 kOutPortByte( PIT_PORT_COUNTER0, wCount );
	 kOutPortByte( PIT_PORT_COUNTER0, wCount >> 8 );
}

// 카운터 0의 값을 반환
WORD kReadCounter0( void )
{
	BYTE bHighByte, bLowByte;
	WORD wTemp = 0;

	// 컨트롤 레지스터에 LATCH를 전송시켜 값을 읽을 준비를 함
	kOutPortByte( PIT_PORT_CONTROL, PIT_COUNTER0_LATCH );

	// 8바이트씩 값을 읽음
	bLowByte = kInPortByte( PIT_PORT_COUNTER0 );
	bHighByte = kInPortByte( PIT_PORT_COUNTER0 );

	wTemp = bHighByte;
	wTemp = ( wTemp << 8 ) | bLowByte;
	return wTemp;

}

// 카운터 0을 직접 설정하여 일정 시간 이상 대기
void kWaitUsingDirectPIT( WORD wCount )
{
	WORD wLastCounter0;
	WORD wCurrentCounter0;

	kInitializePIT( 0, TRUE );

	wLastCounter0 = kReadCounter0(); // 초기값 설정
	// 이제 갈수록 감소함
	while(1)
	{
		wCurrentCounter0 = kReadCounter0();
		//kReadCounter0의 리턴값이 16비트임
		/* 이게 왜이러지 */
		if( ( ( wLastCounter0 - wCurrentCounter0 ) & 0xFFFF ) >= wCount )
		{
			break;
		}
	}

}
