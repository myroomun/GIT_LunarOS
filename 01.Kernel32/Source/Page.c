#include "Page.h"

void kInitializeTables(void)
{
	PML4TENTRY* pstPML4TEntry;
	PDPTENTRY* pstPDPTEntry;
	PDENTRY* pstPDEntry;
	DWORD dwMappingAddress;
	int i;

	// PML4 테이블 작성, 첫번째 엔트리 이외의 나머지는 모두 초기화
	pstPML4TEntry = (PML4TENTRY*) 0x100000; // 1MB 상에 PML4작성 (9비트이므로 512개)
	kSetPageEntryData(&pstPML4TEntry[0], 0x00, 0x101000, PAGE_FLAGS_DEFAULT, 0); //0x101000 ~ 0x100000의 크기는 엔트리 512개 저장 가능!
	for(i = 1 ; i < PAGE_MAXENTRYCOUNT ; i++)
	{
		kSetPageEntryData( &(pstPML4TEntry[i]), 0, 0, 0, 0 );
	}

	// 페이지 디렉토리 포인터 테이블 생성
	// 이때 페이지 디렉토리 포인터 엔트리는 64개가 필요함 (엔트리 하나당 디렉토리 엔트리 512개 엔트리 하나당 2MB)
	pstPDPTEntry = (PDPTENTRY*) 0x101000;
	for( i = 0 ; i < 64 ; i++ )
	{
		//개당 페이지테이블엔트리 512개 가능, 즉 0x1000(512개)씩 올림
		kSetPageEntryData( &(pstPDPTEntry[i]), 0, 0x102000 + (i * PAGE_TABLESIZE), PAGE_FLAGS_DEFAULT, 0 );
	}
	// 나머지 512개는 0으로 초기화 해줌
	for( i = 64 ; i < PAGE_MAXENTRYCOUNT ; i++)
	{
		kSetPageEntryData( &(pstPDPTEntry[i]), 0, 0, 0, 0);
	}
	// 총 64*512개의 페이지엔트리를 만들어야 64GB 수용 가능
	pstPDEntry = (PDENTRY*) 0x102000;
	dwMappingAddress = 0;

	for( i = 0 ; i < PAGE_MAXENTRYCOUNT * 64 ; i++ )
	{
		// 0부터 2MB 씩 페이지엔트리로 배분한다. 이때 상위 엔트리의 구조는 4GB 이상인경우 자른다. 2MB >> 20 = 2 한뒤 2*i 한뒤 32비트 넘어가는지 보기 위해서 >>12 해서 나머지를 넣는다.
		kSetPageEntryData( &(pstPDEntry[i]), (i * (PAGE_DEFAULTSIZE >> 20)) >> 12, dwMappingAddress, PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS, 0);
		dwMappingAddress += PAGE_DEFAULTSIZE; // 2MB씩 더한다.
	}

}
void kSetPageEntryData(PTENTRY* pstEntry, DWORD dwUpperBaseAddress, DWORD dwLowerBaseAddress, DWORD dwLowerFlags, DWORD dwUpperFlags)
{
	pstEntry->dwAttributeAndLowerBaseAddress = dwLowerBaseAddress | dwLowerFlags;
	pstEntry->dwUpperBaseAddressAndEXB = (dwUpperBaseAddress & 0xFF) | dwUpperFlags;
}
