/*
 * Page.h
 *
 *  Created on: 2015. 9. 18.
 *      Author: user
 */

#ifndef PAGE_H_
#define PAGE_H_

#include "Types.h"

#define PAGE_FLAGS_P	0x00000001	//present
#define PAGE_FLAGS_RW	0x00000002	//RW
#define PAGE_FLAGS_US	0x00000004	//User,Supervisor
#define PAGE_FLAGS_PWT	0x00000008	//Page Level Write-through
#define PAGE_FLAGS_PCD	0x00000010	//Page Level Cache Disable
#define PAGE_FLAGS_A	0x00000020	//Accessed
#define PAGE_FLAGS_D	0x00000040	//Dirty
#define PAGE_FLAGS_PS	0x00000080	//Page Size
#define PAGE_FLAGS_PAT	0x00000100	//Page Attribute Table Index
#define PAGE_FLAGS_EBX	0x80000000	//Execute Disable
#define PAGE_FLAGS_DEFAULT	(PAGE_FLAGS_P | PAGE_FLAGS_RW)
#define	PAGE_TABLESIZE	0x1000		// 256
#define PAGE_MAXENTRYCOUNT	512
#define PAGE_DEFAULTSIZE	0x200000	//2MB

#pragma pack(push,1)
typedef struct pageTableEntryStruct
{
	DWORD dwAttributeAndLowerBaseAddress;
	DWORD dwUpperBaseAddressAndEXB;
} PML4TENTRY, PDPTENTRY, PDENTRY, PTENTRY;
#pragma pack(pop)

void kInitializePageTables(void);
void kSetPageEntryData(PTENTRY* pstEntry, DWORD dwUpperBaseAddress, DWORD dwLowerBaseAddress, DWORD dwLowerFlags, DWORD dwUpperFlags);



#endif /* PAGE_H_ */
