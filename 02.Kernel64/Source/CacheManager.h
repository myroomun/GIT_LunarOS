/*
 * CacheManager.h
 *
 *  Created on: 2016. 4. 1.
 *      Author: user
 */

#ifndef __CACHEMANAGER_H__
#define __CACHEMANAGER_H__

#include "Types.h"

// 링크테이블 캐시 버퍼 수 16
#define CACHE_MAXCLUSTERLINKTABLEAREACOUNT		16
// 데이터 캐시 버퍼 수 32
#define	CACHE_MAXDATAAREACOUNT					32
// 유효하지 않은 태그
#define CACHE_INVALIDTAG						0xFFFFFFFF

// CACHETABLE 종류 갯수 2개
#define CACHE_MAXCACHETABLEINDEX				2

// 링크 테이블 에어리어: 0
#define CACHE_CLUSTERLINKTABLEAREA				0
// 데이터 에어리어: 1
#define CACHE_DATAAREA							1

// 파일 시스템 캐시 구조체
typedef struct kCacheBufferStruct
{
	// 하드에 해당하는 인덱스를 태그로 활용
	DWORD dwTag;

	// 캐시 버퍼에 접근한 시간
	DWORD dwAccessTime;

	// 데이터 변경?
	BOOL bChanged;

	// 데이터 버퍼
	BYTE* pbBuffer;
} CACHEBUFFER;

// 캐시 매니저
typedef struct kCacheManagerStruct
{
	// 링크, 데이터에 해당하는 상대적 시간
	DWORD vdwAccessTime[ CACHE_MAXCACHETABLEINDEX];

	// 할당받은 버퍼들
	BYTE* vpbBuffer[ CACHE_MAXCACHETABLEINDEX ];

	// 캐시 유닛들
	CACHEBUFFER vvstCacheBuffer[ CACHE_MAXCACHETABLEINDEX ][ CACHE_MAXDATAAREACOUNT ];

	// 캐시 버퍼의 최대 갯수
	DWORD vdwMaxCount[ CACHE_MAXCACHETABLEINDEX ];
} CACHEMANAGER;

// 함수
BOOL kInitializeCacheManager( void );
CACHEBUFFER* kAllocateCacheBuffer( int iCacheTableIndex );
CACHEBUFFER* kFindCacheBuffer( int iCacheTableIndex, DWORD dwTag );
CACHEBUFFER* kGetVictimInCacheBuffer( int iCacheTableIndex );
void kDiscardAllCacheBuffer( int iCacheTableIndex );
BOOL kGetCacheBufferAndCount( int iCacheTableIndex, CACHEBUFFER** pstCacheBuffer, int* piMaxCount );

static void kCutDownAccessTime( int iCacheTableIndex );

#endif
