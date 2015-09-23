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

// ���ڿ� �����͸� �Ķ���ͷ� �޴� �Լ� ������ Ÿ�� ����
typedef void ( *CommandFunction ) ( const char* pcParameter );

#pragma pack( push, 1 )

// ���� Ŀ�ǵ� ����
typedef struct kShellCommandEntryStruct
{
	char* pcCommand;
	char* pcHelp;
	CommandFunction pfFunction;
} SHELLCOMMANDENTRY;

//�Ķ���͸� ó���ϱ� ���� ������ ����
typedef struct kParameterListStruct
{
	const char* pcBuffer;
	// �Ķ������ ����
	int iLength;
	// ���� ó���� �Ķ���� �ε���
	int iCurrentPosition;
} PARAMETERLIST;

#pragma pack ( pop )

// ���� �Լ� �ڵ�
void kStartConsoleShell( void );
void kExecuteCommand( const char* pcCommandBuffer );
void kInitializeParameter( PARAMETERLIST* pstList, const char* pcParameter );
int kGetNextParameter( PARAMETERLIST* pstList, char* pcParameter );

// Ŀ�ǵ带 ó���ϴ� �Լ�
void kHelp( const char* pcParameterBuffer );
void kCls( const char* pcParameterBuffer );
void kShowTotalRAMSize( const char* pcParameterBuffer );
void kStringToDecimalHexTest( const char* pcParameterBuffer );
void kShutdown( const char* pcParameterBuffer );

#endif /* CONSOLESHELL_H_ */
