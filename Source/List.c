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
	// �׻� �������� ù�κп��� LISTLINK ����ü�� ������. ��, pstLink�� LISTLINK�� �ּҸ� �ھƳ���
	pstLink = ( LISTLINK* )pvItem;
	pstLink->pvNext = NULL;

	if( pstList->pvHeader == NULL )
	{
		pstList->pvHeader = pvItem;
		pstList->pvTail = pvItem;
		pstList->iItemCount = 1;
		return;
	}
	// ���� header�� null �� �ƴѰ�쿡�� tail�� �ִ°��� �ھƳ���
	pstLink = ( LISTLINK* )pstList->pvTail;
	pstLink->pvNext = pvItem;

	pstList->pvTail = pvItem;
	pstList->iItemCount++;

}

// ����Ʈ ù�κп� �߰��Ѵ�
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

// ����Ʈ���� �����͸� ���� �� ������ �����͸� ��ȯ
void* kRemoveList( LIST* pstList, QWORD qwID )
{
	LISTLINK* pstLink;
	LISTLINK* pstPreviousLink;

	pstPreviousLink = (LISTLINK*)pstList->pvHeader;
	for( pstLink = pstPreviousLink ; pstLink != NULL ; pstLink = pstLink->pvNext )
	{
		if( pstLink->qwID == qwID )
		{
			// ���� �����Ͱ� �ϳ��ۿ� ������
			if( (pstLink == pstList->pvHeader) && (pstLink == pstList->pvTail) )
			{
				pstList->pvHeader = NULL;
				pstList->pvTail = NULL;
			}
			// ���� ù��°�� �����͸�
			else if( pstLink == pstList->pvHeader )
			{
				pstList->pvHeader = pstLink->pvNext;
			}
			// ���� ������ �����͸�
			else if( pstLink == pstList->pvTail )
			{
				pstList->pvTail = pstPreviousLink;
			}
			// ���� �߰������͸� �߰� �����ϰ� ���̸� ��������
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

// ����Ʈ�� ù ��° �����͸� �����Ͽ� ��ȯ
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

// ����Ʈ�� ������ �����͸� �����Ͽ� ��ȯ

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
