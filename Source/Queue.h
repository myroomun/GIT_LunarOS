/*
 * Queue.h
 *
 *  Created on: 2015. 9. 22.
 *      Author: user
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#include "Types.h"

typedef struct kQueueMangerStruct
{
	int iDataSize; // �ϳ��� ������ ������
	int iMaxDataCount; // �� �� �� �ִ� ����

	// ť ���� (�ۿ��� �Ҵ������ ��)
	void* pvQueueArray;
	int iPutIndex;
	int iGetIndex;

	BOOL bLastOperationPut;	// ������ ������ ��ɾ ����?
} QUEUE;

void kInitializeQueue( QUEUE* pstQueue, void* pvQueueBuffer, int iMaxDataCount, int iDataSize );
BOOL kIsQueueFull( const QUEUE* pstQueue );
BOOL kIsQueueEmpty( const QUEUE* pstQueue );
BOOL kPutQueue( QUEUE* pstQueue, const void* pvData );
BOOL kGetQueue( QUEUE* pstQueue, void* pvData );

#endif /* QUEUE_H_ */
