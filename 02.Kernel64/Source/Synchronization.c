/*
 * Synchronization.c
 *
 *  Created on: 2015. 10. 7.
 *      Author: user
 */

#include "Synchronization.h"
#include "Utility.h"
#include "Task.h"

BOOL kLockForSystemData( void )
{
	return kSetInterruptFlag( FALSE );
}

void kUnlockForSystemData( BOOL bInterruptFlag )
{
	kSetInterruptFlag( bInterruptFlag );
}

void kInitializeMutex( MUTEX* pstMutex )
{
	pstMutex->bLockFlag = FALSE;
	pstMutex->dwLockCount = 0;
	pstMutex->qwTaskID = TASK_INVALIDID;
}

void kLock( MUTEX* pstMutex )
{
	if( kTestAndSet(&(pstMutex->bLockFlag), 0, 1) == FALSE ) // pstMutex->bLockFlag == TRUE? (if not, pstMutex->bLockFlag = TRUE;
	{
		// 자신이 잠근경우
		if( pstMutex->qwTaskID == kGetRunningTask()->stLink.qwID )
		{
			pstMutex->dwLockCount++;
			return;
		}

		// 자신이 잠근게 아닌경우
		while( kTestAndSet(&(pstMutex->bLockFlag), 0, 1) == FALSE )
		{
			kSchedule();
		}
	}
    pstMutex->dwLockCount = 1;
    pstMutex->qwTaskID = kGetRunningTask()->stLink.qwID;
}

void kUnlock( MUTEX* pstMutex )
{
	// 안잠겨 있거나, 잠근 주인이 아닌경우
	if( (pstMutex->bLockFlag == FALSE) || (pstMutex->qwTaskID != kGetRunningTask()->stLink.qwID))
	{
		return;
	}

	if( pstMutex->dwLockCount > 1 )
	{
		pstMutex->dwLockCount--;
		return;
	}

	pstMutex->qwTaskID = TASK_INVALIDID;
	pstMutex->dwLockCount = 0;
	pstMutex->bLockFlag = FALSE;
}
