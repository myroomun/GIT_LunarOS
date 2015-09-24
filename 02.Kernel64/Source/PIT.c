/*
 * PIT.c
 *
 *  Created on: 2015. 9. 24.
 *      Author: user
 */

#include "PIT.h"

void kInitializePIT( WORD wCount, BOOL bPeriodic )
{
	// PIT ��Ʈ�� ��������(0x43)�� ���� �ʱ�ȭ�Ͽ� ī��Ʈ�� ���� �ڿ� ��� 0�� ���̳ʸ� ī���ͷ� ����

	kOutPortByte( PIT_PORT_CONTROL, PIT_COUNTER0_ONCE );

	// ���� ������ �ֱ���
	if( bPeriodic == TRUE )
	{
		// ���2�� ����
		kOutPortByte( PIT_PORT_CONTROL, PIT_COUNTER0_PERIODIC );
	}
	 kOutPortByte( PIT_PORT_COUNTER0, wCount );
	 kOutPortByte( PIT_PORT_COUNTER0, wCount >> 8 );
}

// ī���� 0�� ���� ��ȯ
WORD kReadCounter0( void )
{
	BYTE bHighByte, bLowByte;
	WORD wTemp = 0;

	// ��Ʈ�� �������Ϳ� LATCH�� ���۽��� ���� ���� �غ� ��
	kOutPortByte( PIT_PORT_CONTROL, PIT_COUNTER0_LATCH );

	// 8����Ʈ�� ���� ����
	bLowByte = kInPortByte( PIT_PORT_COUNTER0 );
	bHighByte = kInPortByte( PIT_PORT_COUNTER0 );

	wTemp = bHighByte;
	wTemp = ( wTemp << 8 ) | bLowByte;
	return wTemp;

}

// ī���� 0�� ���� �����Ͽ� ���� �ð� �̻� ���
void kWaitUsingDirectPIT( WORD wCount )
{
	WORD wLastCounter0;
	WORD wCurrentCounter0;

	kInitializePIT( 0, TRUE );

	wLastCounter0 = kReadCounter0(); // �ʱⰪ ����
	// ���� ������ ������
	while(1)
	{
		wCurrentCounter0 = kReadCounter0();
		//kReadCounter0�� ���ϰ��� 16��Ʈ��
		/* �̰� ���̷��� */
		if( ( ( wLastCounter0 - wCurrentCounter0 ) & 0xFFFF ) >= wCount )
		{
			break;
		}
	}

}
