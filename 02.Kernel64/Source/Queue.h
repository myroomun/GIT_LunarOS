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
	int iDataSize; // �ϳ��� ������ ������
	int iMaxDataCount; // �� �� �� �ִ� ����

	// ť ���� (�ۿ��� �Ҵ������ ��)
	void* pvQueueArray;
	int iPutIndex;
	int iGetIndex;

};


#endif /* QUEUE_H_ */
