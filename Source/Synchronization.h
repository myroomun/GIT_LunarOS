/*
 * Synchronization.h
 *
 *  Created on: 2015. 10. 7.
 *      Author: user
 */

#ifndef SYNCHRONIZATION_H_
#define SYNCHRONIZATION_H_

#include "Types.h"


#pragma pack( push, 1 )

typedef struct kMutexStruct
{
	// volatile 이유는 이 변수가 레지스터에 저장되는것이 아닌 메모리에 저장되어 공유가 될 수 있게 하는 것이다.
	volatile QWORD qwTaskID;
	volatile DWORD dwLockCount;

	volatile BOOL bLockFlag;

	BYTE vbPadding[3];
} MUTEX;

#pragma pack( pop )

BOOL kLockForSystemData( void );
void kUnlockForSystemData( BOOL bInterruptFlag );

void kInitializeMutex( MUTEX* pstMutex );
void kLock( MUTEX* pstMutex );
void kUnlock( MUTEX* pstMutex );


#endif /* SYNCHRONIZATION_H_ */
