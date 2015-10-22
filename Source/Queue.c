/*
 * Queue.c
 *
 *  Created on: 2015. 9. 22.
 *      Author: user
 */

#include "Queue.h"

void kInitializeQueue( QUEUE* pstQueue, void* pvQueueBuffer, int iMaxDataCount, int iDataSize )
{
	// 밖에서 할당받은 값들을 넣음
	pstQueue->iMaxDataCount = iMaxDataCount;
	pstQueue->iDataSize = iDataSize;
	pstQueue->pvQueueArray = pvQueueBuffer;

	// 큐 삽입 위치와 제거 위치를 초기화, 수행된 명령어는 제거
	pstQueue->iPutIndex = 0;
	pstQueue->iGetIndex = 0;

	pstQueue->bLastOperationPut = FALSE; // QUEUE가 비어 있음

}

BOOL kIsQueueFull( const QUEUE* pstQueue )
{
	// 큐가 가득 찼으면 전 오퍼레이션은 큐에 넣는 오퍼레이션 및 삭제 인덱스와 삽입 인덱스가 같을것!
	if( ( pstQueue->iGetIndex == pstQueue->iPutIndex) && ( pstQueue->bLastOperationPut == TRUE ))
	{
		return TRUE;
	}
	return FALSE;
}

BOOL kIsQueueEmpty( const QUEUE* pstQueue )
{
	// 맨 마지막 오퍼레이션이 삭제이고, 삽입인덱스 = 삭제인덱스
	if( ( pstQueue->iGetIndex == pstQueue->iPutIndex) && ( pstQueue->bLastOperationPut == FALSE ))
	{
		return TRUE;
	}
	return FALSE;
}

BOOL kPutQueue( QUEUE* pstQueue, const void* pvData )
{
	if( kIsQueueFull( pstQueue ) == TRUE )
	{
		return FALSE;
	}

	// 하나의 오브젝트를 넣음
	kMemCpy( (char* ) pstQueue->pvQueueArray + (pstQueue->iDataSize * pstQueue->iPutIndex ), pvData, pstQueue->iDataSize );
	pstQueue->iPutIndex = ( pstQueue->iPutIndex + 1 ) % pstQueue->iMaxDataCount ;
	pstQueue->bLastOperationPut = TRUE;
	return TRUE;

}

BOOL kGetQueue( QUEUE* pstQueue, void* pvData )
{
	if( kIsQueueEmpty( pstQueue ) == TRUE )
	{
		return FALSE;
	}
	// pvData에 자료 옮기고
	kMemCpy( pvData, ( char* )pstQueue->pvQueueArray + ( pstQueue->iDataSize * pstQueue->iGetIndex), pstQueue->iDataSize );
	// GetIndex를 오른쪽으로 옮김
	pstQueue->iGetIndex = ( pstQueue->iGetIndex + 1 ) % pstQueue->iMaxDataCount;
	pstQueue->bLastOperationPut = FALSE;

	return TRUE;
}
