/*
 * ConsoleShell.h
 *
 *  Created on: 2015. 9. 23.
 *      Author: user
 */

#ifndef CONSOLESHELL_H_
#define CONSOLESHELL_H_


#include "Types.h"

#define CONSOLESHELL_MAXCOMMANDBUFFERCOUNT	300
#define CONSOLESHELL_PROMPTMESSAGE			"LUNAR OS>"

// 문자열 포인터를 파라미터로 받는 함수 포인터 타입 정의
typedef void ( *CommandFunction ) ( const char* pcParameter );

#pragma pack( push, 1 )

// 쉘의 커맨드 저장
typedef struct kShellCommandEntryStruct
{
	char* pcCommand;
	char* pcHelp;
	CommandFunction pfFunction;
} SHELLCOMMANDENTRY;

//파라미터를 처리하기 위해 정보를 저장
typedef struct kParameterListStruct
{
	const char* pcBuffer;
	// 파라미터의 길이
	int iLength;
	// 현재 처리할 파라미터 인덱스
	int iCurrentPosition;
} PARAMETERLIST;

#pragma pack ( pop )

// 실제 함수 코드
void kStartConsoleShell( void );
void kExecuteCommand( const char* pcCommandBuffer );
void kInitializeParameter( PARAMETERLIST* pstList, const char* pcParameter );
int kGetNextParameter( PARAMETERLIST* pstList, char* pcParameter );

// 커맨드를 처리하는 함수
void kHelp( const char* pcParameterBuffer );
void kCls( const char* pcParameterBuffer );
void kShowTotalRAMSize( const char* pcParameterBuffer );
void kStringToDecimalHexTest( const char* pcParameterBuffer );
void kShutdown( const char* pcParameterBuffer );
void kSetTimer( const char* pcParameterBuffer );
void kWaitUsingPIT( const char* pcParameterBuffer );
void kReadTimeStampCounter( const char* pcParameterBuffer );
void kMeasureProcessorSpeed( const char* pcParameterBuffer );
void kShowDateAndTime( const char* pcParameterBuffer );

#endif /* CONSOLESHELL_H_ */
