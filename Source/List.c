/*
 * List.c
 *
 *  Created on: 2015. 10. 1.
 *      Author: user
 */

#include "List.h"

void kInitializeList( LIST* pstList )
{
	pstList->iItemCount = 0;
	pstList->pvHeader = NULL;
	pstList->pvTail = NULL;
}

int kGetListCount( const LIST* pstList )
{
	return pstList->iItemCount;
}

void kAddListToTail( LIST* pstList, void* pvItem )
{
	LISTLINK* pstLink;
	// 항상 아이템의 첫부분에는 LISTLINK 구조체가 존재함. 즉, pstLink에 LISTLINK의 주소를 박아넣음
	pstLink = ( LISTLINK* )pvItem;
	pstLink->pvNext = NULL;

	if( pstList->pvHeader == NULL )
	{
		pstList->pvHeader = pvItem;
		pstList->pvTail = pvItem;
		pstList->iItemCount = 1;
		return;
	}
	// 원래 header가 null 이 아닌경우에는 tail에 있는것을 박아넣음
	pstLink = ( LISTLINK* )pstList->pvTail;
	pstLink->pvNext = pvItem;

	pstList->pvTail = pvItem;
	pstList->iItemCount++;

}

// 리스트 첫부분에 추가한다
void kAddListToHeader( LIST* pstList, void* pvItem )
{
	LISTLINK* pstLink;

	pstLink = (LISTLINK*)pvItem;
	pstLink->pvNext = pstList->pvHeader;

	if( pstList->pvHeader == NULL )
	{
		pstList->pvHeader = pvItem;
		pstList->pvTail = pvItem;
		pstList->iItemCount = 1;

		return;
	}

	pstList->pvHeader = pvItem;
	pstList->iItemCount++;

}

// 리스트에서 데이터를 제거 후 데이터 포인터를 반환
void* kRemoveList( LIST* pstList, QWORD qwID )
{
	LISTLINK* pstLink;
	LISTLINK* pstPreviousLink;

	pstPreviousLink = (LISTLINK*)pstList->pvHeader;
	for( pstLink = pstPreviousLink ; pstLink != NULL ; pstLink = pstLink->pvNext )
	{
		if( pstLink->qwID == qwID )
		{
			// 현재 데이터가 하나밖에 없으면
			if( (pstLink == pstList->pvHeader) && (pstLink == pstList->pvTail) )
			{
				pstList->pvHeader = NULL;
				pstList->pvTail = NULL;
			}
			// 만약 첫번째의 데이터면
			else if( pstLink == pstList->pvHeader )
			{
				pstList->pvHeader = pstLink->pvNext;
			}
			// 만약 마지막 데이터면
			else if( pstLink == pstList->pvTail )
			{
				pstList->pvTail = pstPreviousLink;
			}
			// 만약 중간데이터면 중간 삭제하고 사이를 연결해줌
			else
			{
				pstPreviousLink->pvNext = pstLink->pvNext;
			}

			pstList->iItemCount--;
			return pstLink;
		}
		pstPreviousLink = pstLink;
	}

	return NULL;
}

// 리스트의 첫 번째 데이터를 제거하여 반환
void* kRemoveListFromHeader( LIST* pstList )
{
	LISTLINK* pstLink;

	if( pstList->iItemCount == 0 )
	{
		return NULL;
	}

	pstLink = (LISTLINK*)pstList->pvHeader;
	return kRemoveList(pstList, pstLink->qwID);
}

// 리스트의 마지막 데이터를 제거하여 반환

void* kRemoveListFromTail( LIST* pstList )
{
	LISTLINK* pstLink;

	if( pstList->iItemCount == 0 )
	{
		return NULL;
	}
	pstLink = (LISTLINK*)pstList->pvTail;
	return kRemoveList(pstList, pstLink->qwID);
}
