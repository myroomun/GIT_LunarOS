/*
 * List.h
 *
 *  Created on: 2015. 10. 1.
 *      Author: user
 */

#ifndef LIST_H_
#define LIST_H_

#include "Types.h"

#pragma pack( push, 1 )

typedef struct kListLinkStruct
{
	void* pvNext;
	QWORD qwID;
}LISTLINK;

/*예제
struct kListItemExampleStruct
{
	// 리스트로 연결하는 자료구조
	LISTLINK stLink;

	// 데이터들
	int iData1;
	char cData2;
};
*/

typedef struct kListManagerStruct
{
	// 리스트의 데이터 수
	int iItemCount;
	void* pvHeader;
	void* pvTail;
} LIST;

#pragma pack( pop )

// 함수
void kInitializeList( LIST* pstList );
int kGetListCount( const LIST* pstLIst );
void kAddListToTail( LIST* pstList, void* pvItem );
void kAddListToHeader( LIST* pstList, void* pvItem );
void* kRemoveList( LIST* pstList, QWORD qwID );
void* kRemoveListFromHeader( LIST* pstList );
void* kRemoveListFromTail( LIST* pstList );
void* kFindList( const LIST* pstList, QWORD qwID );
void* kGetHeaderFromList( const LIST* pstList );
void* kGetTailFromList( const LIST* pstList );
void* kGetNextFromList( const LIST* pstList, void* pstCurrent );


#endif /* LIST_H_ */
