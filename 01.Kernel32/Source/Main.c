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
	 * �޸� �ּ� �뷮 üũ
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
	 * 	IA-32e ��忡�� Ŀ���� �ö� ���� + �ڷᱸ�� ���� (1MB ~ 6MB���� ����ִ� �� 0���� �ʱ�ȭ)
	 *	Why? 2MB ���Ŀ� Ŀ���� �ø��µ� �� �ֺ� ������ �ڷ�������� �� ����, �ʱ�ȭ �� �����Ⱚ ������� ����
	 */
	kPrintString( 0, 5, "IA-32e Kernel Area Initialization...........[    ]" );
	if(!kInitializeKernel64Area())
		kPrintString(45, 5, "Fail");
	else
		kPrintString(45, 5, "Pass");
	kPrintString( 0, 6, "IA-32e Page Table Initialization............[    ]" );
	kInitializeTables();
	kPrintString(45, 6, "Pass");

	// CPU ������ ���� �б�
	kReadCPUID( 0x00, &dwEAX, &dwEBX, &dwECX, &dwEDX );
	*(DWORD*)vcVendorString = dwEBX;
	*( (DWORD*) vcVendorString + 1 ) = dwEDX;
	*( (DWORD*) vcVendorString + 2 ) = dwECX;
	kPrintString( 0, 7, "IA-32e Page Table Initialization............[            ]" );
	kPrintString(45, 7, vcVendorString);

	// 64��Ʈ ���� ���� Ȯ��
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
	// 64��Ʈ CĿ�η� ����Ī
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

	pdwCurrentAddress = (DWORD*)0x100000; // 1MB �ּ� ����

	while(((DWORD)pdwCurrentAddress < 0x600000)) // 6MB ����
	{
		*pdwCurrentAddress = 0x00;

		// �� ������ �������� ���ؼ� 0x00�� �����ٸ�,
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

	// 1MB ���� ����
	pdwCurrentAddress = (DWORD*)0x100000;

	while((DWORD)pdwCurrentAddress < 0x4000000 )
	{
		*pdwCurrentAddress = 0x12345678; // 1MB �����̹Ƿ� ù 32��Ʈ���� �˻� (4����Ʈ)

		if( *pdwCurrentAddress != 0x12345678 )
		{
			return FALSE;
		}
		pdwCurrentAddress += (0x100000 / 4); // 1MB �� �˻�
	}
	return TRUE;
}
void kCopyKernel64ImageTo2Mbyte(void)
{
	WORD wKernel32SectorCount, wTotalKernelSectorCount;
	DWORD* pdwSourceAddress, *pdwDestinationAddress;
	int i;

	// 0x7C00 + 149�� �� Ŀ�� ���ͼ�, 0x7C00 + 14B�� ��ȣ��� Ŀ�� ���ͼ��� �������
	wTotalKernelSectorCount = *((WORD*)(0x7C00 + 0x149));
	wKernel32SectorCount = *((WORD*)(0x7C00 + 0x14B));

	pdwSourceAddress = (DWORD* )(0x10000 + (wKernel32SectorCount * 512) ); // 32��Ʈ Ŀ�� �� ���
	pdwDestinationAddress = (DWORD*)0x200000; // 2MB
	// ��ü 64��Ʈ Ŀ�� ũ�⸦ �ѹ��� 32��Ʈ ��ŭ ������
	for( i = 0 ; i < 512 * (wTotalKernelSectorCount - wKernel32SectorCount)/4 ; i++ )
	{
		*pdwDestinationAddress = *pdwSourceAddress;
		pdwDestinationAddress++;
		pdwSourceAddress++;
	}

}
