/*
 * Utility.h
 *
 *  Created on: 2015. 9. 21.
 *      Author: user
 */

#ifndef UTILITY_H_
#define UTILITY_H_

#include <stdarg.h>
#include "Types.h"

void kMemSet( void* pvDestination, BYTE bData, int iSize );
int kMemCpy( void* pvDestination, const void* pvSource, int iSize );
int kMemCmp( const void* pvDestination, const void* pvSource, int iSize );
BOOL kSetInterruptFlag( BOOL bEnableInterrupt );
void kCheckTotalRAMSize( void );
QWORD kGetTotalRAMSize( void );
void kReverseString( char* pcBuffer );
long kAToI( const char* pcBuffer, int iRadix );
QWORD kHexStringToQword( const char* pcBuffer );
long kDecimalStringToLong( const char* pcBuffer );
int kIToA( long lValue, char* pcBuffer, int iRadix );
int kHexToString( QWORD qwValue, char* pcBuffer );
int kDecimalToString( long lValue, char* pcBuffer );
int kSPrintf( char* pcBuffer, const char* pcFormatString, ... );
int kVSPrintf( char* pcBuffer, const char* pcFormatString, va_list ap );
QWORD kGetTickCount( void );
void kSleep( QWORD qwMillisecond );

// ��Ÿ
extern volatile QWORD g_qwTickCount;

#endif /* UTILITY_H_ */
