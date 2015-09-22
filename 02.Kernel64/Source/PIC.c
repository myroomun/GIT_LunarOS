/*
 * PIC.c
 *
 *  Created on: 2015. 9. 22.
 *      Author: user
 */

#include "PIC.h"

void kInitializePIC(void)
{
	// ������ PIC �ʱ�ȭ
	// ICW1 ����
	kOutPortByte( PIC_MASTER_PORT1, 0x11 ); // 0001 0001
	// ICW2 ����
	kOutPortByte( PIC_MASTER_PORT2, PIC_IRQSTARTVECTOR ); // ������ PIC�� �Ҵ�� ��ȣ : 32~39 : 0x20 ~ 0x27
	// ICW3 ����
	kOutPortByte( PIC_MASTER_PORT2, 0x04 ); // ��� ��ġ�� CASCODED�� PIC�ΰ�
	// ICW4
	kOutPortByte( PIC_MASTER_PORT2, 0x01 ); // Only UPM bit 1 (compatible with x8086)

	// �����̺� PIC ��Ʈ�ѷ� �ʱ�ȭ
	// ICW1 ����
	kOutPortByte( PIC_SLAVE_PORT1, 0x11 );
	// ICW2 ����
	kOutPortByte( PIC_SLAVE_PORT2, PIC_IRQSTARTVECTOR + 8 ); // IRQSTARTVECTOR : 40 ~ 47 �̹Ƿ�..
	// ICW3 ����
	kOutPortByte( PIC_SLAVE_PORT2, 0x02 ); // ������ �����̺� 2���ɿ� �����̺� PIC ���� ����
	// ICW4 ����
	kOutPortByte( PIC_SLAVE_PORT2, 0x01 );

}

void kMaskPICInterrupt( WORD wIRQBitmask )
{
	// OCW1(PORT 21)�� IRQ 0 ~ IRQ 7 (bit register ���� in IMR using OCW1)
	kOutPortByte( PIC_MASTER_PORT2, (BYTE) wIRQBitmask );

	// �����̺� PIC ��Ʈ�ѷ��� IMR ����
	kOutPortByte( PIC_SLAVE_PORT2, (BYTE) (wIRQBitmask >> 8) );
}

void kSendEOIToPIC( int iIRQNumber )
{
	// EOI ��Ʈ  1 (Non-specific EOI)
	kOutPortByte( PIC_MASTER_PORT1, 0x20 );

	if( iIRQNumber >= 8 )
	{
		// �����̺꿡 EOI ���� �ʿ� (EOI ��Ʈ  )
		kOutPortByte( PIC_SLAVE_PORT1, 0x20 );
	}
}

