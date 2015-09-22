/*
 * Queue.c
 *
 *  Created on: 2015. 9. 22.
 *      Author: user
 */

#include "Queue.h"

void kInitializeQueue( QUEUE* pstQueue, void* pvQueueBuffer, int iMaxDataCount, int iDataSize )
{
	// �ۿ��� �Ҵ���� ������ ����
	pstQueue->iMaxDataCount = iMaxDataCount;
	pstQueue->iDataSize = iDataSize;
	pstQueue->pvQueueArray = pvQueueBuffer;

	// ť ���� ��ġ�� ���� ��ġ�� �ʱ�ȭ, ����� ��ɾ�� ����
	pstQueue->iPutIndex = 0;
	pstQueue->iGetIndex = 0;

	pstQueue->bLastOperationPut = FALSE; // QUEUE�� ��� ����

}

BOOL kIsQueueFull( const QUEUE* pstQueue )
{
	// ť�� ���� á���� �� ���۷��̼��� ť�� �ִ� ���۷��̼� �� ���� �ε����� ���� �ε����� ������!
	if( ( pstQueue->iGetIndex == pstQueue->iPutIndex) && ( pstQueue->bLastOperationPut == TRUE ))
	{
		return TRUE;
	}
	return FALSE;
}

BOOL kIsQueueEmpty( const QUEUE* pstQueue )
{
	// �� ������ ���۷��̼��� �����̰�, �����ε��� = �����ε���
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

	// �ϳ��� ������Ʈ�� ����
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
	// pvData�� �ڷ� �ű��
	kMemCpy( pvData, ( char* )pstQueue->pvQueueArray + ( pstQueue->iDataSize * pstQueue->iGetIndex), pstQueue->iDataSize );
	// GetIndex�� ���������� �ű�
	pstQueue->iGetIndex = ( pstQueue->iGetIndex + 1 ) % pstQueue->iMaxDataCount;
	pstQueue->bLastOperationPut = FALSE;

	return TRUE;
}
