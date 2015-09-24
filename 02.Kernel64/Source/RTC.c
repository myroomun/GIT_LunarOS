/*
 * RTc.c
 *
 *  Created on: 2015. 9. 24.
 *      Author: user
 */


#include "RTC.h"

void kReadRTCTime( BYTE* pbHour, BYTE* pbMinute, BYTE* pbSecond )
{
	BYTE bData;
	// CMOS 메모리 어드레스 레지스터(포트 0x70)에 시간을 저장하는 레지스터 지정
	kOutPortByte( RTC_CMOSADDRESS, RTC_ADDRESS_HOUR );
	// 데이터 레지스터에서 시간 얻어옴
	bData = kInPortByte( RTC_CMOSDATA );
	*pbHour = RTC_BCDTOBINARY( bData );

	kOutPortByte( RTC_CMOSADDRESS, RTC_ADDRESS_MINUTE );
	bData = kInPortByte( RTC_CMOSDATA );
	*pbMinute = RTC_BCDTOBINARY( bData );

	kOutPortByte( RTC_CMOSADDRESS, RTC_ADDRESS_SECOND );
	bData = kInPortByte( RTC_CMOSDATA );
	*pbSecond = RTC_BCDTOBINARY( bData );

}

void kReadRTCDate( WORD* pwYear, BYTE* pbMonth, BYTE* pbDayOfMonth, BYTE* pbDayOfWeek )
{
	BYTE bData;

	kOutPortByte( RTC_CMOSADDRESS, RTC_ADDRESS_YEAR );
	bData = kInPortByte( RTC_CMOSDATA );
	*pwYear = RTC_BCDTOBINARY( bData ) + 2000;



	kOutPortByte( RTC_CMOSADDRESS, RTC_ADDRESS_MONTH );
	bData = kInPortByte( RTC_CMOSDATA );
	*pbMonth = RTC_BCDTOBINARY( bData );

	kOutPortByte( RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFMONTH );
	bData = kInPortByte( RTC_CMOSDATA );
	*pbDayOfMonth = RTC_BCDTOBINARY( bData );

	kOutPortByte( RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFWEEK );
	bData = kInPortByte( RTC_CMOSDATA );
	*pbDayOfWeek = RTC_BCDTOBINARY( bData );

}

char* kConvertDayOfWeekToString( BYTE bDayOfWeek )
{
	static char* vpcDayOfWeekString[8] = { "Error", "Sunday", "Monday", "Tuesday", "Wednsday", "Thursday", "Friday", "Saturday" };

	if( bDayOfWeek >= 8 )
	{
		return vpcDayOfWeekString[0];
	}
	return vpcDayOfWeekString[ bDayOfWeek ];
}
