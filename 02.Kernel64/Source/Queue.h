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
	int iDataSize; // 하나의 데이터 사이즈
	int iMaxDataCount; // 총 들어갈 수 있는 갯수

	// 큐 버퍼 (밖에서 할당해줘야 함)
	void* pvQueueArray;
	int iPutIndex;
	int iGetIndex;

	BOOL bLastOperationPut;	// 마지막 실행한 명령어가 삭제?
} QUEUE;

void kInitializeQueue( QUEUE* pstQueue, void* pvQueueBuffer, int iMaxDataCount, int iDataSize );
BOOL kIsQueueFull( const QUEUE* pstQueue );
BOOL kIsQueueEmpty( const QUEUE* pstQueue );
BOOL kPutQueue( QUEUE* pstQueue, const void* pvData );
BOOL kGetQueue( QUEUE* pstQueue, void* pvData );

#endif /* QUEUE_H_ */
