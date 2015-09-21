/*
 * Main.c
 *
 *  Created on: 2015. 9. 17.
 *      Author: user
 */

#include "Types.h"
#include "Page.h"
#include "ModeSwitch.h"

void kPrintString( int, int, const char*);
BOOL kInitializeKernel64Area(void);
BOOL kIsMemoryEnough( void );
void kCopyKernel64ImageTo2Mbyte(void);

void Main()
{
	DWORD i;
	DWORD dwEAX, dwEBX, dwECX, dwEDX;
	char vcVendorString[13] = {0, };
	kPrintString( 0, 3, "Entering C Language Kernel..................[Pass]" );
	/*
	 * 메모리 최소 용량 체크
	 */
	kPrintString( 0, 4, "Minimum Memory Check........................[    ]" );
	if( !kIsMemoryEnough() )
	{
		kPrintString(45, 4, "Fail");
		kPrintString( 0, 5, "Not Enough Memory! Lunar OS require over 64MBytes");
		while(1);
	}
	else
		kPrintString(45, 4, "Pass");
	/*
	 * 	IA-32e 모드에서 커널이 올라갈 공간 + 자료구조 공간 (1MB ~ 6MB까지 들어있는 값 0으로 초기화)
	 *	Why? 2MB 이후에 커널을 올리는데 그 주변 공간을 자료공간으로 쓸 예정, 초기화 시 쓰레기값 있을까봐 방지
	 */
	kPrintString( 0, 5, "IA-32e Kernel Area Initialization...........[    ]" );
	if(!kInitializeKernel64Area())
		kPrintString(45, 5, "Fail");
	else
		kPrintString(45, 5, "Pass");
	kPrintString( 0, 6, "IA-32e Page Table Initialization............[    ]" );
	kInitializeTables();
	kPrintString(45, 6, "Pass");

	// CPU 제조사 정보 읽기
	kReadCPUID( 0x00, &dwEAX, &dwEBX, &dwECX, &dwEDX );
	*(DWORD*)vcVendorString = dwEBX;
	*( (DWORD*) vcVendorString + 1 ) = dwEDX;
	*( (DWORD*) vcVendorString + 2 ) = dwECX;
	kPrintString( 0, 7, "IA-32e Page Table Initialization............[            ]" );
	kPrintString(45, 7, vcVendorString);

	// 64비트 지원 유무 확인
	kReadCPUID( 0x80000001, &dwEAX, &dwEBX, &dwECX, &dwEDX );
	kPrintString( 0, 8, "64bit Mode Support Check....................[    ]" );
	if( dwEDX & (1 << 29))
	{
		kPrintString(45, 8, "Pass");
	}
	else
	{
		kPrintString(45, 8, "Fail");
		kPrintString(0, 9, "This processor doesn't support 64-bit mode...");
		while(1);
	}
	// OS Image (64bit Kernel) copy to 2MBytes
	kPrintString( 0, 9, "Copy Kernel64 Image to over ................[    ]" );
	kCopyKernel64ImageTo2Mbyte();
	kPrintString(45, 9, "Pass");


	kPrintString( 0, 10,"64bit Mode Switched.........................[    ]" );
	// 64비트 C커널로 스위칭
	kSwitchAndExecute64bitKernel();
	while(1);
}

void kPrintString( int iX, int iY, const char* pcString )
{
	CHARACTER* pstScreen = (CHARACTER* ) 0xB8000;
	int i;

	pstScreen += (iY * 80) + iX;
	for(i = 0 ; pcString[i] != NULL ; i++)
	{
		pstScreen[i].bCharacter = pcString[i];
	}
}
BOOL kInitializeKernel64Area(void)
{
	DWORD* pdwCurrentAddress;

	pdwCurrentAddress = (DWORD*)0x100000; // 1MB 주소 설정

	while(((DWORD)pdwCurrentAddress < 0x600000)) // 6MB 까지
	{
		*pdwCurrentAddress = 0x00;

		// 이 구역에 접근하지 못해서 0x00을 못썻다면,
		if( *pdwCurrentAddress != 0x00 )
		{
			return FALSE;
		}
		pdwCurrentAddress++;
	}
	return TRUE;
}
BOOL kIsMemoryEnough(void)
{
	DWORD* pdwCurrentAddress;

	// 1MB 부터 시작
	pdwCurrentAddress = (DWORD*)0x100000;

	while((DWORD)pdwCurrentAddress < 0x4000000 )
	{
		*pdwCurrentAddress = 0x12345678; // 1MB 단위이므로 첫 32비트값을 검사 (4바이트)

		if( *pdwCurrentAddress != 0x12345678 )
		{
			return FALSE;
		}
		pdwCurrentAddress += (0x100000 / 4); // 1MB 씩 검사
	}
	return TRUE;
}
void kCopyKernel64ImageTo2Mbyte(void)
{
	WORD wKernel32SectorCount, wTotalKernelSectorCount;
	DWORD* pdwSourceAddress, *pdwDestinationAddress;
	int i;

	// 0x7C00 + 149에 총 커널 섹터수, 0x7C00 + 14B에 보호모드 커널 섹터수가 등어있음
	wTotalKernelSectorCount = *((WORD*)(0x7C00 + 0x149));
	wKernel32SectorCount = *((WORD*)(0x7C00 + 0x14B));

	pdwSourceAddress = (DWORD* )(0x10000 + (wKernel32SectorCount * 512) ); // 32비트 커널 끝 계산
	pdwDestinationAddress = (DWORD*)0x200000; // 2MB
	// 전체 64비트 커널 크기를 한번에 32비트 만큼 복사함
	for( i = 0 ; i < 512 * (wTotalKernelSectorCount - wKernel32SectorCount)/4 ; i++ )
	{
		*pdwDestinationAddress = *pdwSourceAddress;
		pdwDestinationAddress++;
		pdwSourceAddress++;
	}

}
