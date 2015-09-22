/*
 * Descriptor.c
 *
 *  Created on: 2015. 9. 21.
 *      Author: user
 */

#include "Descriptor.h"
#include "Utility.h"
#include "ISR.h"

void kInitializeGDTTableAndTSS( void )
{
	GDTR* pstGDTR;
	GDTENTRY8* pstEntry;
	TSSSEGMENT* pstTSS;
	int i;

	pstGDTR = ( GDTR* ) GDTR_STARTADDRESS; // 1MB 어드레스에서 264KB를 페이지 테이블 영역으로 사용
	pstEntry = (GDTENTRY8*)(GDTR_STARTADDRESS + sizeof(GDTR));
	pstGDTR->wLimit = GDT_TABLESIZE - 1; // NULL, 커널코드, 커널데이터 디스크립터  // 태스크 세그먼트 디스크립터
	pstGDTR->qwBaseAddress = (QWORD)pstEntry;
	// TSS 영역 설정
	pstTSS = ( TSSSEGMENT* )( (QWORD) pstEntry + GDT_TABLESIZE );

	kSetGDTEntry8( &(pstEntry[ 0 ]) , 0, 0, 0, 0, 0); // NULL segment
	kSetGDTEntry8( &(pstEntry[ 1 ]) , 0, 0xFFFF, GDT_FLAGS_UPPER_CODE, GDT_FLAGS_LOWER_KERNELCODE, GDT_TYPE_CODE ); // Kernel Code Segment (커널 코드는 0부터 4GB까지 정한다.)
	kSetGDTEntry8( &(pstEntry[ 2 ]) , 0, 0xFFFF, GDT_FLAGS_UPPER_DATA, GDT_FLAGS_LOWER_KERNELDATA, GDT_TYPE_DATA ); // Kernel Code Segment (커널 코드는 0부터 4GB까지 정한다.)
	kSetGDTEntry16( (GDTENTRY16*)&(pstEntry[3]), (QWORD)pstTSS, sizeof(TSSSEGMENT) - 1, GDT_FLAGS_UPPER_TSS, GDT_FLAGS_LOWER_TSS, GDT_TYPE_TSS );
	// Base Address == 0x142000 + GDTR(8) + TSS(16) + GDT(8)*3 = 0x142030 = pstTSS: Limit은 사이즈가 아님. 0~15 등 size-1을 해주어야 Limit이 나옴

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
	pstTSS->qwIST[0] = IST_STARTADDRESS + IST_SIZE; // 8MB에서 하위주소로 확장하는 스택 시작주소
	pstTSS->wIOMapBaseAddress = 0xFFFF; // 이 말은 TSS데이터 구조내에 데이터가 없음을 뜻한다. 0xFFFF >= sizeof(TSSSEGMENT) - 1
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

	pstIDTR = (IDTR*)IDTR_STARTADDRESS; // 1M + 264k + GDTR + GDT*3 + TSS + TSSD(104) + 8(IDTEntry) => IDTHandler의 시작주소
	pstEntry = (IDTENTRY*) (IDTR_STARTADDRESS + sizeof(IDTR) );
	pstIDTR->qwBaseAddress = (QWORD)pstEntry;
	pstIDTR->wLimit = IDT_TABLESIZE - 1; // 인터럽트와 예외는 최대 100개까지..

	/*
	 * 예외 ISR 등록 (순서 : 핸들러 접근 -> 컨텍트 저장 -> 인터럽트 핸들러 호출 -> 컨텍스트 복구 -> 인터럽트 리턴
	 */
	kSetIDTEntry(&pstEntry[0], kISRDivideError, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[1], kISRDebug, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[2], kISRNMI, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[3], kISRBreakPoint, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[4], kISROverflow, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[5], kISRBoundRangeExceeded, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[6], kISRInvalidOpcode, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[7], kISRDeviceNotAvailable, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[8], kISRDoubleFault, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[9], kISRCoprocessorSegmentOverrun, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[10], kISRInvalidTSS, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[11], kISRSegmentNotPresent, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[12], kISRStackSegmentFault, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[13], kISRGeneralProtection, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[14], kISRPageFault, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[15], kISR15, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[16], kISRFPUError, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[17], kISRAlignmentCheck, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[18], kISRMachineCheck, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[19], kISRSIMDError, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[20], kISRETCException, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );

	for( i = 21 ; i < 32 ; i++ )
	{
		kSetIDTEntry(&pstEntry[i], kISRETCException, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	}


	/*
	 * 인터럽트 ISR 등록
	 */
	kSetIDTEntry(&pstEntry[32], kISRTimer, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[33], kISRKeyboard, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[34], kISRSlavePIC, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[35], kISRSerial2, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[36], kISRSerial1, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[37], kISRParallel2, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[38], kISRFloppy, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[39], kISRParallel1, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[40], kISRRTC, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[41], kISRReserved, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[42], kISRNotUsed1, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[43], kISRNotUsed2, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[44], kISRMouse, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[45], kISRCoprocessor, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[46], kISRHDD1, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry(&pstEntry[47], kISRHDD2, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );

	for( i = 48 ; i < IDT_MAXENTRYCOUNT ; i++ )
	{
		kSetIDTEntry(&pstEntry[i], kISRETCInterrupt, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	}
}

void kDummyHandler(void)
{
	kPrintString( 0, 0, "================================================");
	kPrintString( 0, 1, "          Dummy Interrupt Handler Called        ");
	kPrintString( 0, 2, "================================================");
	while(1);
}
