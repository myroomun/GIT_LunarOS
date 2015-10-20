/*
 * DynamicMemory.c
 *
 *  Created on: 2015. 10. 20.
 *      Author: user
 */

#include "DynamicMemory.h"
#include "Utility.h"
#include "Task.h"

static DYNAMICMEMORY gs_stDynamicMemory;

void kInitializeDynamicMemory( void )
{
	QWORD qwDynamicMemorySize;
	int i, j;
	BYTE* pbCurrentBitmapPosition;
	int iBlockCountOfLevel, iMetaBlockCount;

	qwDynamicMemorySize = kCalculateDynamicMemorySize();

	iMetaBlockCount = kCalculateMetaBlockCount( qwDynamicMemorySize );

	gs_stDynamicMemory.iBlockCountOfSmallestBlock = (qwDynamicMemorySize / DYNAMICMEMORY_MIN_SIZE) - iMetaBlockCount;

	for(i = 0 ; (gs_stDynamicMemory.iBlockCountOfSmallestBlock >> i) > 0 ; i++)
	{
		;// Nothing
	}
	gs_stDynamicMemory.iMaxLevelCount = i;
	// 할당된 메모리가 속한 블록 리스트의 인덱스를 저장하는 영역을 초기화
	gs_stDynamicMemory.pbAllocatedBlockListIndex = (BYTE*) DYNAMICMEMORY_START_ADDRESS;

	for( i = 0 ; i < gs_stDynamicMemory.iBlockCountOfSmallestBlock ; i++)
	{
		gs_stDynamicMemory.pbAllocatedBlockListIndex[i] = 0xff;
	}

	gs_stDynamicMemory.pstBitmapOfLevel = (BITMAP*) (DYNAMICMEMORY_START_ADDRESS + (sizeof(BYTE)* gs_stDynamicMemory.iBlockCountOfSmallestBlock));
	pbCurrentBitmapPosition = ( ( BYTE* ) gs_stDynamicMemory.pstBitmapOfLevel) + (sizeof( BITMAP ) * gs_stDynamicMemory.iBlockCountOfSmallestBlock);

	for( j = 0 ; j < gs_stDynamicMemory.iMaxLevelCount ; j++ )
	{
		gs_stDynamicMemory.pstBitmapOfLevel[ j ].pbBitmap = pbCurrentBitmapPosition;
		gs_stDynamicMemory.pstBitmapOfLevel[ j ].qwExistBitCount = 0;
		iBlockCountOfLevel = gs_stDynamicMemory.iBlockCountOfSmallestBlock >> j;

		// 아래는 pbBitmap 에 대한 초기화

		for( i = 0 ; i < iBlockCountOfLevel / 8 ; i++ )
		{
			*pbCurrentBitmapPosition = 0x00;
			pbCurrentBitmapPosition++;
		}
		if( ( iBlockCountOfLevel % 8 )  != 0 )
		{
			*pbCurrentBitmapPosition = 0x00;
			i = iBlockCountOfLevel % 8;
			if( ( i % 2 )  == 1 )
			{
				*pbCurrentBitmapPosition |= ( DYNAMICMEMORY_EXIST << (i - 1));
				gs_stDynamicMemory.pstBitmapOfLevel[ j ].qwExistBitCount = 1;
			}
			pbCurrentBitmapPosition++;
		}

	}
	gs_stDynamicMemory.qwStartAddress = DYNAMICMEMORY_START_ADDRESS + iMetaBlockCount * DYNAMICMEMORY_MIN_SIZE;
	gs_stDynamicMemory.qwEndAddress = kCalculateDynamicMemorySize() + DYNAMICMEMORY_START_ADDRESS;
	gs_stDynamicMemory.qwUsedSize = 0;

}

static QWORD kCalculateDynamicMemorySize( void )
{
	QWORD qwRAMSize;

	qwRAMSize = ( kGetTotalRAMSize() * 1024 * 1024 ); //MB 단위이므로
	if(qwRAMSize > (QWORD) 3 * 1024 * 1024 * 1024 )
	{
		qwRAMSize = (QWORD) 3 * 1024 * 1024 * 1024;
	}
	return qwRAMSize - DYNAMICMEMORY_START_ADDRESS;
}

// 내 정보를 포함한 메타블록을 생성한다.
static int kCalculateMetaBlockCount( QWORD qwDynamicRAMSize )
{
	long lBlockCountOfSmallestBlock;
	DWORD dwSizeOfAllocatedBlockListIndex;
	DWORD dwSizeOfBitmap;
	long i;

	lBlockCountOfSmallestBlock = qwDynamicRAMSize / DYNAMICMEMORY_MIN_SIZE;
	// n번째 블럭은 몇번째 리스트에 속해있는가? 에 대한 공간
	dwSizeOfAllocatedBlockListIndex = lBlockCountOfSmallestBlock * sizeof(BYTE);

	dwSizeOfBitmap = 0;

	for( i = 0 ; (lBlockCountOfSmallestBlock >> i) > 0 ; i++ )
	{
		dwSizeOfBitmap += sizeof(BITMAP);
		dwSizeOfBitmap += ( ( lBlockCountOfSmallestBlock >> i) + 7 ) / 8;
	}

	return ( dwSizeOfAllocatedBlockListIndex + dwSizeOfBitmap + DYNAMICMEMORY_MIN_SIZE - 1) / DYNAMICMEMORY_MIN_SIZE;
}

void* kAllocateMemory( QWORD qwSize )
{
	QWORD qwAlignedSize;
	QWORD qwRelativeAddress;
	long lOffset;
	int iSizeArrayOffset;
	int iIndexOfBlockList;

	qwAlignedSize = kGetBuddyBlockSize( qwSize );

	if( qwAlignedSize == 0)
	{
		return NULL;
	}

	if( gs_stDynamicMemory.qwStartAddress + gs_stDynamicMemory.qwUsedSize + qwAlignedSize > gs_stDynamicMemory.qwEndAddress )
	{
		return NULL;
	}

	lOffset = kAllocationBuddyBlock( qwAlignedSize );
	if( lOffset == -1 )
	{
		return NULL;
	}

	iIndexOfBlockList = kGetBlockListIndexOfMatchSize( qwAlignedSize );

	qwRelativeAddress = qwAlignedSize * lOffset;
	iSizeArrayOffset = qwRelativeAddress / DYNAMICMEMORY_MIN_SIZE;

	gs_stDynamicMemory.pbAllocatedBlockListIndex[ iSizeArrayOffset ] = (BYTE) iIndexOfBlockList;
	gs_stDynamicMemory.qwUsedSize += qwAlignedSize;

	return (void*) (qwRelativeAddress + gs_stDynamicMemory.qwStartAddress);
}

static QWORD kGetBuddyBlockSize( QWORD qwSize )
{
	long i;

	for( i = 0 ; i < gs_stDynamicMemory.iMaxLevelCount ; i++ )
	{
		if( qwSize <= ( DYNAMICMEMORY_MIN_SIZE << i))
		{
			return DYNAMICMEMORY_MIN_SIZE << i;
		}
	}
	return 0;
}

static int kAllocationBuddyBlock( QWORD qwAlignedSize )
{
	int iBlockListIndex, iFreeOffset;
	int i;
	BOOL bPreviousInterruptFlag;

	iBlockListIndex = kGetBlockListIndexOfMatchSize( qwAlignedSize );
	if(iBlockListIndex == -1)
	{
		return -1;
	}

	bPreviousInterruptFlag = kLockForSystemData();

	for( i = iBlockListIndex ; i<gs_stDynamicMemory.iMaxLevelCount ; i++)
	{
		iFreeOffset = kFindFreeBlockInBitmap(i);
		if( iFreeOffset != -1 )
		{
			break;
		}
	}

	if(iFreeOffset == -1)
	{
		kUnlockForSystemData( bPreviousInterruptFlag);
		return -1;
	}

	kSetFlagInBitmap( i, iFreeOffset, DYNAMICMEMORY_EMPTY );

	if( i > iBlockListIndex )
	{
		for( i = i - 1 ; i>= iBlockListIndex ; i--)
		{
			kSetFlagInBitmap( i, iFreeOffset*2, DYNAMICMEMORY_EMPTY);
			kSetFlagInBitmap( i, iFreeOffset*2 + 1, DYNAMICMEMORY_EMPTY);
			iFreeOffset = iFreeOffset * 2;
		}
	}
	kUnlockForSystemData( bPreviousInterruptFlag );

	return iFreeOffset;
}

static int kGetBlockListIndexOfMatchSize( QWORD qwAlignedSize )
{
	int i;
	for( i = 0 ; i < gs_stDynamicMemory.iMaxLevelCount ; i++ )
	{
		if( qwAlignedSize <= (DYNAMICMEMORY_MIN_SIZE << i))
		{
			return i;
		}
	}
	 return -1;
}

static int kFindFreeBlockInBitmap( int iBlockListIndex )
{
	int i, iMaxCount;
	BYTE* pbBitmap;
	QWORD* pqwBitmap;

	if( gs_stDynamicMemory.pstBitmapOfLevel[ iBlockListIndex ].qwExistBitCount == 0)
	{
		return -1;
	}

	iMaxCount = gs_stDynamicMemory.iBlockCountOfSmallestBlock >> iBlockListIndex;
	pbBitmap = gs_stDynamicMemory.pstBitmapOfLevel[ iBlockListIndex ].pbBitmap;
	for(i = 0 ; i < iMaxCount ; )
	{
		if( ( ( iMaxCount - i ) / 64) > 0)
		{
			pqwBitmap = (QWORD*) &( pbBitmap[i / 8] );
			if( *pqwBitmap == 0)
			{
				i += 64;
				continue;
			}
		}
		if( ( pbBitmap[ i / 8 ] & (DYNAMICMEMORY_EXIST << (i % 8))) != 0 )
		{
			return i;
		}
		i++;
	}
	return -1;
}

static void kSetFlagInBitmap( int iBlockListIndex, int iOffset, BYTE bFlag )
{
	BYTE* pbBitmap;

	pbBitmap = gs_stDynamicMemory.pstBitmapOfLevel[ iBlockListIndex ].pbBitmap;

	if(bFlag == DYNAMICMEMORY_EXIST)
	{
		//데이터 비어있다면 갯수 증가
		if( (pbBitmap[ iOffset / 8 ] & (0x01 << ( iOffset % 8))) == 0)
		{
			gs_stDynamicMemory.pstBitmapOfLevel[ iBlockListIndex ].qwExistBitCount++;
		}
		pbBitmap[ iOffset / 8] |= ( 0x01 << (iOffset % 8) );
	}
	else
	{
		//데이터 가 존재한다면
		if( (pbBitmap[ iOffset / 8 ] & (0x01 << ( iOffset % 8))) != 0)
		{
			gs_stDynamicMemory.pstBitmapOfLevel[ iBlockListIndex ].qwExistBitCount--;
		}
		pbBitmap[ iOffset / 8] &= ~( 0x01 << (iOffset % 8) );
	}
}

BOOL kFreeMemory( void* pvAddress )
{
	QWORD qwRelativeAddress;
	int iSizeArrayOffset;
	QWORD qwBlockSize;
	int iBlockListIndex;
	int iBitmapOffset;

	if( pvAddress == NULL )
	{
		return FALSE;
	}

	qwRelativeAddress = ( ( QWORD ) pvAddress ) - gs_stDynamicMemory.qwStartAddress;
	iSizeArrayOffset = qwRelativeAddress / DYNAMICMEMORY_MIN_SIZE;

	// 할당되어 있지 않으면?
	if( gs_stDynamicMemory.pbAllocatedBlockListIndex[ iSizeArrayOffset ] == 0xFF )
	{
		return FALSE;
	}

	// 할당된 블록 인덱스 초기화
	// 비트맵 초기화
	iBlockListIndex = (int)gs_stDynamicMemory.pbAllocatedBlockListIndex[ iSizeArrayOffset ];
	gs_stDynamicMemory.pbAllocatedBlockListIndex[ iSizeArrayOffset ] = 0xFF;
	qwBlockSize = DYNAMICMEMORY_MIN_SIZE << iBlockListIndex;

	// 메모리 공간은 쭉 나열되어 있고 그 안에 분류가 되어있으므로..
	iBitmapOffset = qwRelativeAddress / qwBlockSize;
	// qwRelativeAddress = qwAlignedSize * lOffset; 로 구했었음
	if( kFreeBuddyBlock( iBlockListIndex, iBitmapOffset) == TRUE )
	{
		gs_stDynamicMemory.qwUsedSize -= qwBlockSize;
		return TRUE;
	}
	return FALSE;
}

static BOOL kFreeBuddyBlock( int iBlockListIndex, int iBlockOffset )
{
	int iBuddyBlockOffset;
	int i;
	BOOL bFlag;
	BOOL bPreviousInterruptFlag;

	bPreviousInterruptFlag = kLockForSystemData();

	for( i = iBlockListIndex ; i < gs_stDynamicMemory.iMaxLevelCount ; i++)
	{
		kSetFlagInBitmap( i, iBlockOffset, DYNAMICMEMORY_EXIST);
		if( ( iBlockOffset % 2 ) == 0 )
		{
			iBuddyBlockOffset = iBlockOffset + 1;
		}
		else
		{
			iBuddyBlockOffset = iBlockOffset - 1;
		}
		bFlag = kGetFlagInBitmap( i ,iBuddyBlockOffset );

		// 옆 블락에 정보가 존재
		if( bFlag == DYNAMICMEMORY_EMPTY )
		{
			break;
		}

		// 여기까지 왔다면 블럭을 합쳐줘야 함
		// 블럭을 올리기 때문에 이웃한 블럭은 EMPTY로 없어진것을 뜻하고 위로 갔을때 EXIST로 넣는다.
		kSetFlagInBitmap( i, iBuddyBlockOffset, DYNAMICMEMORY_EMPTY );
		kSetFlagInBitmap( i, iBlockOffset, DYNAMICMEMORY_EMPTY );

		iBlockOffset = iBlockOffset / 2;
	}

	kUnlockForSystemData( bPreviousInterruptFlag );
	return TRUE;
}

static BYTE kGetFlagInBitmap( int iBlockListIndex, int iOffset )
{
	BYTE* pbBitmap;

	pbBitmap = gs_stDynamicMemory.pstBitmapOfLevel[ iBlockListIndex ].pbBitmap;
	// 블럭이 존재한다면
	if( ( pbBitmap[ iOffset / 8 ] & (0x01 << (iOffset % 8) )) != 0x00 )
	{
		return DYNAMICMEMORY_EXIST;
	}
	return DYNAMICMEMORY_EMPTY;
}

void kGetDynamicMemoryInformation( QWORD* pqwDynamicMemoryStartAddress, QWORD* pqwDynamicMemoryTotalSize, QWORD* pqwMetaDataSize, QWORD* pqwUsedMemorySize)
{
	*pqwDynamicMemoryStartAddress = DYNAMICMEMORY_START_ADDRESS;
	*pqwDynamicMemoryTotalSize = kCalculateDynamicMemorySize();
	*pqwMetaDataSize = kCalculateMetaBlockCount( *pqwDynamicMemoryTotalSize ) * DYNAMICMEMORY_MIN_SIZE;
	*pqwUsedMemorySize = gs_stDynamicMemory.qwUsedSize;
}

DYNAMICMEMORY* kGetDynamicMemoryManager( void )
{
	return &gs_stDynamicMemory;
}
