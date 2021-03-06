/*
 * RTC.h
 *
 *  Created on: 2015. 9. 24.
 *      Author: user
 */

#ifndef RTC_H_
#define RTC_H_

#include "Types.h"

// I/O 포트
#define RTC_CMOSADDRESS				0x70
#define RTC_CMOSDATA				0x71

// CMOS 메모리 어드레스
#define RTC_ADDRESS_SECOND			0x00
#define RTC_ADDRESS_MINUTE			0x02
#define RTC_ADDRESS_HOUR			0x04
#define RTC_ADDRESS_DAYOFWEEK		0x06
#define RTC_ADDRESS_DAYOFMONTH		0x07
#define RTC_ADDRESS_MONTH			0x08
#define RTC_ADDRESS_YEAR			0x09

// BCT to Binary for 1byte
#define RTC_BCDTOBINARY(x)			( ( ( ( x ) >> 4 ) * 10 ) + ( ( x ) & 0x0F ) )

// 함수
void kReadRTCTime( BYTE* pbHour, BYTE* pbMinute, BYTE* pbSecond );
void kReadRTCDate( WORD* pwYear, BYTE* pbMonth, BYTE* pdDayOfMonth, BYTE* pbDayOfWeek );
char* kConvertDayOfWeekToString( BYTE bDayOfWeek );

#endif /* RTC_H_ */
