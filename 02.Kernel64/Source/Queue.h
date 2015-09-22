/*
 * Queue.h
 *
 *  Created on: 2015. 9. 22.
 *      Author: user
 */

#ifndef QUEUE_H_
#define QUEUE_H_

typedef struct kQueueMangerStruct
{
	int iDataSize; // 하나의 데이터 사이즈
	int iMaxDataCount; // 총 들어갈 수 있는 갯수

	// 큐 버퍼 (밖에서 할당해줘야 함)
	void* pvQueueArray;
	int iPutIndex;
	int iGetIndex;

};


#endif /* QUEUE_H_ */
