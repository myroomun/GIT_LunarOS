/*
 * Descriptor.c
 *
 *  Created on: 2015. 9. 21.
 *      Author: user
 */

#include "Descriptor.h"

void kInitializeGDTTableAndTSS( void )
{
	GDTR* pstGDTR;
	GDTENTRY8* pstEntry;
	TSSSEGMENT* pstTSS;
	int i;

	pstGDTR = ( GDTR* ) GDTR_STARTADDRESS; // 1MB ��巹������ 264KB�� ������ ���̺� �������� ���
	pstEntry = (GDTENTRY8*)(GDTR_STARTADDRESS + sizeof(GDTR));
	pstGDTR->wLimit = GDT_TABLESIZE - 1; // NULL, Ŀ���ڵ�, Ŀ�ε����� ��ũ����  // �½�ũ ���׸�Ʈ ��ũ����
	pstGDTR->qwBaseAddress = (QWORD)pstEntry;
	// TSS ���� ����
	pstTSS = ( TSSSEGMENT* )( (QWORD) pstEntry + GDT_TABLESIZE );

	kSetGDTEntry8( &(pstEntry[ 0 ]) , 0, 0, 0, 0, 0); // NULL segment
	kSetGDTEntry8( &(pstEntry[ 1 ]) , 0, 0xFFFF, GDT_FLAGS_UPPER_CODE, GDT_FLAGS_LOWER_KERNELCODE, GDT_TYPE_CODE ); // Kernel Code Segment (Ŀ�� �ڵ�� 0���� 4GB���� ���Ѵ�.)
	kSetGDTEntry8( &(pstEntry[ 2 ]) , 0, 0xFFFF, GDT_FLAGS_UPPER_DATA, GDT_FLAGS_LOWER_KERNELDATA, GDT_TYPE_DATA ); // Kernel Code Segment (Ŀ�� �ڵ�� 0���� 4GB���� ���Ѵ�.)
	kSetGDTEntry16( (GDTENTRY16*)&(pstEntry[3]), (QWORD)pstTSS, sizeof(TSSSEGMENT) - 1, GDT_FLAGS_UPPER_TSS, GDT_FLAGS_LOWER_TSS, GDT_TYPE_TSS );
	// Base Address == 0x142000 + GDTR(8) + TSS(16) + GDT(8)*3 = 0x142030 = pstTSS: Limit�� ����� �ƴ�. 0~15 �� size-1�� ���־�� Limit�� ����

	kInitializeTSSSegment( pstTSS );
}

void kSetGDTEntry8( GDTENTRY8* pstEntry, DWORD dwBaseAddress, DWORD dwLimit, BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType )
{
	pstEntry->wLowerLimit = dwLimit & 0xFFFF;
	pstEntry->wLowerBaseAddress = dwBaseAddress & 0xFFFF;
	pstEntry->bUpperBaseAddress1 = ( dwBaseAddress >> 16 ) & 0xFF;
	pstEntry->bTypeAndLowerFlag = bLowerFlags | bType;
	pstEntry->bUpperLimitAndUpperFlag = ( ( ( dwLimit >> 16 ) & 0xFF ) | bUpperFlags );
	pstEntry->bUpperBaseAddress2 = ( dwBaseAddress >> 24 ) & 0xFF;
}

void kSetGDTEntry16( GDTENTRY16* pstEntry, QWORD qwBaseAddress, DWORD dwLimit, BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType )
{
	pstEntry->wLowerLimit = dwLimit & 0xFFFF;
	pstEntry->wLowerBaseAddress = qwBaseAddress & 0xFFFF;
	pstEntry->bMiddleBaseAddress1 = ( qwBaseAddress >> 16 ) & 0xFF;
	pstEntry->bTypeAndLowerFlag = bLowerFlags | bType;
	pstEntry->bUpperLimitAndUpperFlag = ( ( dwLimit >> 16 )  & 0xFF ) | bUpperFlags;
	pstEntry->bMiddleBaseAddress2 = ( qwBaseAddress >> 24 ) & 0xFF;
	pstEntry->dwUpperBaseAddress = qwBaseAddress >> 32;
	pstEntry->dwReserved = 0;
}
void kInitializeTSSSegment( TSSSEGMENT* pstTSS)
{
	kMemSet(pstTSS, 0, sizeof(TSSSEGMENT) );
	pstTSS->qwIST[0] = IST_STARTADDRESS + IST_SIZE; // 8MB���� �����ּҷ� Ȯ���ϴ� ���� �����ּ�
	pstTSS->wIOMapBaseAddress = 0xFFFF; // �� ���� TSS������ �������� �����Ͱ� ������ ���Ѵ�. 0xFFFF >= sizeof(TSSSEGMENT) - 1
}

void kSetIDTEntry( IDTENTRY* pstEntry, void* pvHandler, WORD wSelector, BYTE bIST, BYTE bFlags, BYTE bType )
{
	pstEntry->wLowerBaseAddress = ( QWORD )pvHandler & 0xFFFF;
	pstEntry->wSegmentSelector = wSelector;
	pstEntry->bIST = bIST & 0x3;
	pstEntry->bTypeAndFlags = bType | bFlags;
	pstEntry->wMiddleBaseAddress = ( (QWORD)pvHandler >> 16 ) & 0xFFFF;
	pstEntry->dwUpperBaseAddress = ( QWORD )pvHandler >> 32;
	pstEntry->dwReserved = 0;
}
void kInitializeIDTTables( void )
{
	IDTR* pstIDTR;
	IDTENTRY* pstEntry;
	int i;

	pstIDTR = (IDTR*)IDTR_STARTADDRESS; // 1M + 264k + GDTR + GDT*3 + TSS + TSSD(104) + 8(IDTEntry) => IDTHandler�� �����ּ�
	pstEntry = (IDTENTRY*) (IDTR_STARTADDRESS + sizeof(IDTR) );
	pstIDTR->qwBaseAddress = (QWORD)pstEntry;
	pstIDTR->wLimit = IDT_TABLESIZE - 1; // ���ͷ�Ʈ�� ���ܴ� �ִ� 100������..

	for( i = 0 ; i < IDT_MAXENTRYCOUNT ; i++ )
	{
		kSetIDTEntry(&pstEntry[i], kDummyHandler, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	}
}

void kDummyHandler(void)
{
	kPrintString( 0, 0, "================================================");
	kPrintString( 0, 1, "          Dummy Interrupt Handler Called        ");
	kPrintString( 0, 2, "================================================");
	while(1);
}
