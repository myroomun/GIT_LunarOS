/*
 * Main.c
 *
 *  Created on: 2015. 9. 18.
 *      Author: user
 */

#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "PIC.h"
#include "Console.h"
#include "ConsoleShell.h"
#include "PIT.h"
#include "Task.h"
#include "DynamicMemory.h"
#include "FileSystem.h"

void kPrintString( int iX, int iY, const char* pcString );

void Main( void )
{
	int iCursorX, iCursorY;
	kPrintString( 45, 10,"Pass" );
	kInitializeConsole( 0, 11);
	kPrintf("Success to enter 64-bit C Kernel\n");
	kPrintf("Console Initialization......................[Pass]\n");
	// 부팅상황
	kGetCursor( &iCursorX, &iCursorY );
	kPrintf("GDT Initialize And Switch For 64bit Mode....[    ]");
	kInitializeGDTTableAndTSS();
	kLoadGDTR( GDTR_STARTADDRESS );
	kSetCursor( 45, iCursorY++);
	kPrintf("Pass\n");

	kPrintf("TSS Segment Load............................[    ]" );
	kLoadTR( GDT_TSSSEGMENT );
	kSetCursor( 45, iCursorY++);
	kPrintf("Pass\n");

	kPrintf("IDT Initialize..............................[    ]" );
	kInitializeIDTTables();
	kLoadIDTR( IDTR_STARTADDRESS );
	kSetCursor( 45, iCursorY++);
	kPrintf("Pass\n");


	kPrintf("Total RAM Size Check........................[    ]" );
	kCheckTotalRAMSize();
	kSetCursor( 45, iCursorY++);
	kPrintf("Pass], Size = %d MB\n", kGetTotalRAMSize());

	kPrintf("TCB Pool And Scheduler Initialize...........[Pass]\n" );
	iCursorY++;
	kInitializeScheduler();

	kPrintf("Dynamic Memory Initialize...................[Pass]\n");
	iCursorY++;
	kInitializeDynamicMemory();

	// 1ms 당 인터럽트 발생
	kInitializePIT( MSTOCOUNT(1), 1 );

	kPrintf("Keyboard Activation.........................[    ]" );
	if( kInitializeKeyboard() == TRUE )
	{
		kSetCursor( 45, iCursorY++);
		kPrintf("Pass\n");
		kChangeKeyboardLED(FALSE, FALSE, FALSE);
	}
	else
	{
		kSetCursor( 45, iCursorY++);
		kPrintf("Fail\n");
		while(1)
		{
			;
		}
	}

	kPrintf("PIC Controller And Interrupt Initialize.....[    ]" );
	kInitializePIC();
	kMaskPICInterrupt( 0 );
	kEnableInterrupt();
	kSetCursor( 45, iCursorY++);
	kPrintf("Pass\n");

	kPrintf("HDD Initialize..............................[    ]" );
	if(kInitializeHDD() == TRUE)
	{
		kSetCursor( 45, iCursorY++);
		kPrintf("Pass\n");
	}
	else
	{
		kSetCursor( 45, iCursorY++);
		kPrintf("Fail\n");
	}
	kPrintf("File System Initialize......................[    ]" );
	if(kInitializeFileSystem() == TRUE)
	{
		kSetCursor( 45, iCursorY++);
		kPrintf("Pass\n");
	}
	else
	{
		kSetCursor( 45, iCursorY++);
		kPrintf("Fail\n");
	}


	kCreateTask( TASK_FLAGS_LOWEST | TASK_FLAGS_IDLE | TASK_FLAGS_SYSTEM | TASK_FLAGS_THREAD , 0, 0, (QWORD) kIdleTask );
	kStartConsoleShell();
}
void kPrintString( int iX, int iY, const char* pcString )
{
	CHARACTER* pstScreen = (CHARACTER* ) 0xB8000;
	int i;

	pstScreen += (iY * 80) + iX;
	for(i = 0 ; pcString[i] != NULL ; i++)
	{
		pstScreen[i].bCharacter = pcString[i];
	}
}
