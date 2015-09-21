#include "Page.h"

void kInitializeTables(void)
{
	PML4TENTRY* pstPML4TEntry;
	PDPTENTRY* pstPDPTEntry;
	PDENTRY* pstPDEntry;
	DWORD dwMappingAddress;
	int i;

	// PML4 ���̺� �ۼ�, ù��° ��Ʈ�� �̿��� �������� ��� �ʱ�ȭ
	pstPML4TEntry = (PML4TENTRY*) 0x100000; // 1MB �� PML4�ۼ� (9��Ʈ�̹Ƿ� 512��)
	kSetPageEntryData(&pstPML4TEntry[0], 0x00, 0x101000, PAGE_FLAGS_DEFAULT, 0); //0x101000 ~ 0x100000�� ũ��� ��Ʈ�� 512�� ���� ����!
	for(i = 1 ; i < PAGE_MAXENTRYCOUNT ; i++)
	{
		kSetPageEntryData( &(pstPML4TEntry[i]), 0, 0, 0, 0 );
	}

	// ������ ���丮 ������ ���̺� ����
	// �̶� ������ ���丮 ������ ��Ʈ���� 64���� �ʿ��� (��Ʈ�� �ϳ��� ���丮 ��Ʈ�� 512�� ��Ʈ�� �ϳ��� 2MB)
	pstPDPTEntry = (PDPTENTRY*) 0x101000;
	for( i = 0 ; i < 64 ; i++ )
	{
		//���� ���������̺�Ʈ�� 512�� ����, �� 0x1000(512��)�� �ø�
		kSetPageEntryData( &(pstPDPTEntry[i]), 0, 0x102000 + (i * PAGE_TABLESIZE), PAGE_FLAGS_DEFAULT, 0 );
	}
	// ������ 512���� 0���� �ʱ�ȭ ����
	for( i = 64 ; i < PAGE_MAXENTRYCOUNT ; i++)
	{
		kSetPageEntryData( &(pstPDPTEntry[i]), 0, 0, 0, 0);
	}
	// �� 64*512���� ��������Ʈ���� ������ 64GB ���� ����
	pstPDEntry = (PDENTRY*) 0x102000;
	dwMappingAddress = 0;

	for( i = 0 ; i < PAGE_MAXENTRYCOUNT * 64 ; i++ )
	{
		// 0���� 2MB �� ��������Ʈ���� ����Ѵ�. �̶� ���� ��Ʈ���� ������ 4GB �̻��ΰ�� �ڸ���. 2MB >> 20 = 2 �ѵ� 2*i �ѵ� 32��Ʈ �Ѿ���� ���� ���ؼ� >>12 �ؼ� �������� �ִ´�.
		kSetPageEntryData( &(pstPDEntry[i]), (i * (PAGE_DEFAULTSIZE >> 20)) >> 12, dwMappingAddress, PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS, 0);
		dwMappingAddress += PAGE_DEFAULTSIZE; // 2MB�� ���Ѵ�.
	}

}
void kSetPageEntryData(PTENTRY* pstEntry, DWORD dwUpperBaseAddress, DWORD dwLowerBaseAddress, DWORD dwLowerFlags, DWORD dwUpperFlags)
{
	pstEntry->dwAttributeAndLowerBaseAddress = dwLowerBaseAddress | dwLowerFlags;
	pstEntry->dwUpperBaseAddressAndEXB = (dwUpperBaseAddress & 0xFF) | dwUpperFlags;
}
