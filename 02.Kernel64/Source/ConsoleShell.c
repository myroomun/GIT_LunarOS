/*
 * ConsoleShell.c
 *
 *  Created on: 2015. 9. 23.
 *      Author: user
 */

#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"


SHELLCOMMANDENTRY gs_vstCommandTable[] =
{
		{ "help", "Show Help", kHelp },
		{ "cls", "Clear Screen", kCls },
		{ "totalram", "Show Total RAM Size", kShowTotalRAMSize },
		{ "strtod", "String To Decial/Hex Convert", kStringToDecimalHexTest },
		{ "shutdown", "Shutdown ANd Reboot OS", kShutdown },

};

// ���� ���� ����

void kStartConsoleShell( void )
{
	char vcCommandBuffer[ CONSOLESHELL_MAXCOMMANDBUFFERCOUNT ];
	int iCommandBufferIndex = 0;
	BYTE bKey;
	int iCursorX, iCursorY;

	// ������Ʈ ���
	kPrintf( (CONSOLESHELL_PROMPTMESSAGE) );

	while(1)
	{
		// Ű ���� ���
		bKey = kGetCh();
		// Ű�� �齺���̽��ΰ��
		if( bKey == KEY_BACKSPACE )
		{
			if( iCommandBufferIndex > 0 )
			{
				kGetCursor( &iCursorX, &iCursorY );
				// ��ĭ ������ �̵��� ��, ���� ���
				kPrintStringXY( iCursorX - 1, iCursorY, " ");
				kSetCursor( iCursorX -1, iCursorY );
				iCommandBufferIndex--;
			}
		}
		// Ű�� �����ΰ��
		else if( bKey == KEY_ENTER )
		{
			kPrintf( "\n" );

			// �̹� ����ִ� ���ڰ� ������ ��ɾ� ����
			if( iCommandBufferIndex > 0 )
			{
				vcCommandBuffer[ iCommandBufferIndex ] = '\0';
				kExecuteCommand( vcCommandBuffer );
			}
			kPrintf( "%s", CONSOLESHELL_PROMPTMESSAGE );
			kMemSet( vcCommandBuffer, '\0', CONSOLESHELL_MAXCOMMANDBUFFERCOUNT );
			iCommandBufferIndex = 0;
		}
		// Shift, Cpas Lock, Num Lock, Scroll Lock�� ����
		else if( (bKey == KEY_LSHIFT) || (bKey == KEY_RSHIFT) || (bKey == KEY_CAPSLOCK) || ( bKey == KEY_NUMLOCK ) || ( bKey == KEY_SCROLLLOCK ))
		{
			;
		}
		else
		{
			if( bKey == KEY_TAB)
			{
				bKey = ' ';
			}

			if( iCommandBufferIndex < CONSOLESHELL_MAXCOMMANDBUFFERCOUNT )
			{
				vcCommandBuffer[ iCommandBufferIndex++ ] = bKey;
				kPrintf("%c", bKey );
			}
		}
	}
}

void kExecuteCommand( const char* pcCommandBuffer )
{
	int i, iSpaceIndex;
	int iCommandBufferLength, iCommandLength;
	int iCount;

	// �������� ���е� Ŀ�ǵ带 ����
	iCommandBufferLength = kStrLen( pcCommandBuffer );
	for( iSpaceIndex = 0 ; iSpaceIndex < iCommandBufferLength ; iSpaceIndex++ )
	{
		if( pcCommandBuffer[ iSpaceIndex ] == ' ')
		{
			break;
		}
	}
	// Ŀ�ǵ尡 ��ġ�ϴ°�?
	iCount = sizeof( gs_vstCommandTable ) / sizeof( SHELLCOMMANDENTRY ); // ���� gs_vstCommandTable�� ��� ��ɾ ����Ǿ��ִ°�?
	for( i = 0 ; i < iCount ; i++ )
	{
		iCommandLength = kStrLen( gs_vstCommandTable[i].pcCommand );
		if( ( iCommandLength == iSpaceIndex) && (kMemCmp( gs_vstCommandTable[i].pcCommand, pcCommandBuffer, iSpaceIndex) == 0))
		{
			// pfFunction�� ����� �Լ����ٰ� �����̽��� �̾��� ���� �Ķ���͸� �ִ´�.
			gs_vstCommandTable[i].pfFunction( pcCommandBuffer + iSpaceIndex + 1 );
			break;
		}
	}

	if( i >= iCount )
	{
		kPrintf( "'%s' is not found.\n", pcCommandBuffer );
	}
}

void kInitializeParameter( PARAMETERLIST* pstList, const char* pcParameter)
{
	pstList->pcBuffer = pcParameter;
	pstList->iLength = kStrLen( pcParameter );
	pstList->iCurrentPosition = 0;
}
int kGetNextParameter( PARAMETERLIST* pstList, char* pcParameter )
{
	int i;
	int iLength;

	// ���̻� �Ķ���� ����
	if( pstList->iLength <= pstList->iCurrentPosition )
	{
		return 0;
	}

	// ������ ���̸�ŭ �̵��ϸ鼭 ������ �˻�
	for( i = pstList->iCurrentPosition ; i < pstList->iLength ; i++ )
	{
		if( pstList->pcBuffer[i] == ' ' )
			break;
	}

	// �Ķ���� ���� �� ���� ��ȯ
	kMemCpy(pcParameter, pstList->pcBuffer + pstList->iCurrentPosition, i );
	iLength = i - pstList->iCurrentPosition;
	pcParameter[ iLength ] = '\0';

	// �Ķ���� ��ġ ������Ʈ
	pstList->iCurrentPosition += iLength + 1;
	return iLength;
}

// Ŀ�ǵ带 ó���ϴ� �ڵ�
void kHelp( const char* pcCommandBuffer )
{
	int i;
	int iCount;
	int iCursorX, iCursorY;
	int iLength, iMaxCommandLength = 0;

	kPrintf( "================================================================================" );
	kPrintf( "                             LUNAR OS Shell Help                                " );
	kPrintf( "================================================================================" );

	iCount = sizeof( gs_vstCommandTable ) / sizeof( SHELLCOMMANDENTRY);

	// ���� �� Ŀ�ǵ��� ���̸� ��� ( �������� Align�� ���Ͽ�)
	for(i = 0 ; i < iCount ; i++ )
	{
		iLength = kStrLen( gs_vstCommandTable[i].pcCommand );
		if( iLength > iMaxCommandLength )
		{
			iMaxCommandLength = iLength;
		}
	}

	// ���� ���
	for( i = 0 ; i < iCount ; i++ )
	{
		kPrintf( "%s", gs_vstCommandTable[i].pcCommand );
		kGetCursor( &iCursorX, &iCursorY );
		kSetCursor( iMaxCommandLength, iCursorY );
		kPrintf( " - %s\n", gs_vstCommandTable[i].pcHelp);
	}
}

void kCls( const char* pcParameterBuffer )
{
	kClearScreen();
	kSetCursor( 0, 1 );
}

void kShowTotalRAMSize( const char* pcParameterBuffer )
{
	kPrintf("Total Ram Size= %d MB\n", kGetTotalRAMSize());
}

void kStringToDecimalHexTest( const char* pcParameterBuffer )
{
	char vcParameter[ 100 ];
	int iLength;
	PARAMETERLIST stList;
	int iCount = 0;
	long lValue;

	// �Ķ���� �ʱ�ȭ
	kInitializeParameter( &stList, pcParameterBuffer );
	while(1)
	{
		// ���� �Ķ���͸� ����, �Ķ���� ���̰� 0�̸� ����
		iLength = kGetNextParameter( &stList, vcParameter );
		if( iLength == 0 )
		{
			break;
		}

		// �Ķ���Ϳ� ���� ������ ����ϰ� 16�������� 10�������� �Ǵ� ��ȯ
		kPrintf("Param %d = %s, Length = %d ",iCount+1, vcParameter, iLength);

		//0x�� �����ϸ� 16����
		if( kMemCmp( vcParameter, "0x", 2) == 0 )
		{
			lValue = kAToI( vcParameter + 2, 16);
			kPrintf( "HEX Value = %q\n", lValue);
		}
		else
		{
			lValue = kAToI( vcParameter, 10 );
			kPrintf( "Decimal Value = %d\n", lValue );
		}

		iCount++;
	}
}

void kShutdown( const char* pcParameterBuffer )
{
	kPrintf("Press any key to reboot pc...");
	kGetCh();
	kReboot();
}
