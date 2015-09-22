/*
 * Utility.c
 *
 *  Created on: 2015. 9. 21.
 *      Author: user
 */

#include "Utility.h"

void kMemSet( void* pvDestination, BYTE bData, int iSize )
{
	int i;

	for( i = 0 ; i < iSize ; i++ )
	{
		( (char*) pvDestination )[i] = bData;
	}
}

int kMemCpy( void* pvDestination, const void* pvSource, int iSize )
{
	int i;

	for( i = 0 ; i < iSize ; i++ )
	{
		( (char*) pvDestination )[i] = ( (char*) pvSource )[i];
	}
	return iSize;
}

int kMemCmp(const void* pvDestination, const void* pvSource, int iSize )
{
	int i;
	char cTemp;

	for( i = 0 ; i < iSize ; i++ )
	{
		cTemp = ( (char*) pvDestination )[i] - ( (char*) pvSource )[i];
		if( cTemp != 0 )
		{
			return ( int ) cTemp;
		}
	}
	return 0;
}

BOOL kSetInterruptFlag( BOOL bEnableInterrupt )
{
	QWORD qwRFLAGS;

	qwRFLAGS = kReadRFLAGS();
	if( bEnableInterrupt == TRUE )
	{
		kEnableInterrupt();
	}
	else
	{
		kDisableInterrupt();
	}
	// 9비트가 인터럽트 비트
	if( qwRFLAGS & 0x200 )
	{
		return TRUE;
	}
	return FALSE;
}
