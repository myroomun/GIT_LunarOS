/*
 * PIC.c
 *
 *  Created on: 2015. 9. 22.
 *      Author: user
 */

#include "PIC.h"

void kInitializePIC(void)
{
	// 마스터 PIC 초기화
	// ICW1 설정
	kOutPortByte( PIC_MASTER_PORT1, 0x11 ); // 0001 0001
	// ICW2 설정
	kOutPortByte( PIC_MASTER_PORT2, PIC_IRQSTARTVECTOR ); // 마스터 PIC에 할당된 번호 : 32~39 : 0x20 ~ 0x27
	// ICW3 설정
	kOutPortByte( PIC_MASTER_PORT2, 0x04 ); // 어디 위치가 CASCODED된 PIC인가
	// ICW4
	kOutPortByte( PIC_MASTER_PORT2, 0x01 ); // Only UPM bit 1 (compatible with x8086)

	// 슬레이브 PIC 컨트롤러 초기화
	// ICW1 설정
	kOutPortByte( PIC_SLAVE_PORT1, 0x11 );
	// ICW2 설정
	kOutPortByte( PIC_SLAVE_PORT2, PIC_IRQSTARTVECTOR + 8 ); // IRQSTARTVECTOR : 40 ~ 47 이므로..
	// ICW3 설정
	kOutPortByte( PIC_SLAVE_PORT2, 0x02 ); // 마스터 슬레이브 2번핀에 슬레이브 PIC 꽃혀 있음
	// ICW4 설정
	kOutPortByte( PIC_SLAVE_PORT2, 0x01 );

}

void kMaskPICInterrupt( WORD wIRQBitmask )
{
	// OCW1(PORT 21)에 IRQ 0 ~ IRQ 7 (bit register 변경 in IMR using OCW1)
	kOutPortByte( PIC_MASTER_PORT2, (BYTE) wIRQBitmask );

	// 슬레이브 PIC 컨트롤러에 IMR 설정
	kOutPortByte( PIC_SLAVE_PORT2, (BYTE) (wIRQBitmask >> 8) );
}

void kSendEOIToPIC( int iIRQNumber )
{
	// EOI 비트  1 (Non-specific EOI)
	kOutPortByte( PIC_MASTER_PORT1, 0x20 );

	if( iIRQNumber >= 8 )
	{
		// 슬레이브에 EOI 전송 필요 (EOI 비트  )
		kOutPortByte( PIC_SLAVE_PORT1, 0x20 );
	}
}

