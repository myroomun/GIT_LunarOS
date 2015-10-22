/*
 * PIC.h
 *
 *  Created on: 2015. 9. 22.
 *      Author: user
 */

#ifndef PIC_H_
#define PIC_H_

#include "Types.h"

//I/O PORT ����

#define PIC_MASTER_PORT1		0x20
#define PIC_MASTER_PORT2		0x21
#define PIC_SLAVE_PORT1			0xA0
#define PIC_SLAVE_PORT2			0xA1

// IDT ���̺��� ���ͷ�Ʈ ���Ͱ� ���۵Ǵ� ��ġ
#define PIC_IRQSTARTVECTOR 		0x20

void kInitializePIC( void );
void kMaskPICInterrupt( WORD wIRQBitmask );
void kSendEOIToPIC( int iIRQNumber );

#endif /* PIC_H_ */
