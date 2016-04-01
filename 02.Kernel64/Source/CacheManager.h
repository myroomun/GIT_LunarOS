/*
 * CacheManager.h
 *
 *  Created on: 2016. 4. 1.
 *      Author: user
 */

#ifndef __CACHEMANAGER_H__
#define __CACHEMANAGER_H__

#include "Types.h"

// ��ũ���̺� ĳ�� ���� �� 16
#define CACHE_MAXCLUSTERLINKTABLEAREACOUNT		16
// ������ ĳ�� ���� �� 32
#define	CACHE_MAXDATAAREACOUNT					32
// ��ȿ���� ���� �±�
#define CACHE_INVALIDTAG						0xFFFFFFFF

// CACHETABLE ���� ���� 2��
#define CACHE_MAXCACHETABLEINDEX				2

// ��ũ ���̺� �����: 0
#define CACHE_CLUSTERLINKTABLEAREA				0
// ������ �����: 1
#define CACHE_DATAAREA							1

// ���� �ý��� ĳ�� ����ü
typedef struct kCacheBufferStruct
{
	// �ϵ忡 �ش��ϴ� �ε����� �±׷� Ȱ��
	DWORD dwTag;

	// ĳ�� ���ۿ� ������ �ð�
	DWORD dwAccessTime;

	// ������ ����?
	BOOL bChanged;

	// ������ ����
	BYTE* pbBuffer;
} CACHEBUFFER;

// ĳ�� �Ŵ���
typedef struct kCacheManagerStruct
{
	// ��ũ, �����Ϳ� �ش��ϴ� ����� �ð�
	DWORD vdwAccessTime[ CACHE_MAXCACHETABLEINDEX];

	// �Ҵ���� ���۵�
	BYTE* vpbBuffer[ CACHE_MAXCACHETABLEINDEX ];

	// ĳ�� ���ֵ�
	CACHEBUFFER vvstCacheBuffer[ CACHE_MAXCACHETABLEINDEX ][ CACHE_MAXDATAAREACOUNT ];

	// ĳ�� ������ �ִ� ����
	DWORD vdwMaxCount[ CACHE_MAXCACHETABLEINDEX ];
} CACHEMANAGER;

// �Լ�
BOOL kInitializeCacheManager( void );
CACHEBUFFER* kAllocateCacheBuffer( int iCacheTableIndex );
CACHEBUFFER* kFindCacheBuffer( int iCacheTableIndex, DWORD dwTag );
CACHEBUFFER* kGetVictimInCacheBuffer( int iCacheTableIndex );
void kDiscardAllCacheBuffer( int iCacheTableIndex );
BOOL kGetCacheBufferAndCount( int iCacheTableIndex, CACHEBUFFER** pstCacheBuffer, int* piMaxCount );

static void kCutDownAccessTime( int iCacheTableIndex );

#endif
