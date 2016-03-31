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
#include "PIT.h"
#include "RTC.h"
#include "AssemblyUtility.h"
#include "Task.h"
#include "Synchronization.h"
#include "DynamicMemory.h"
#include "HardDisk.h"
#include "FileSystem.h"


SHELLCOMMANDENTRY gs_vstCommandTable[] =
{
		{ "help", "Show Help", kHelp },
		{ "cls", "Clear Screen", kCls },
		{ "totalram", "Show Total RAM Size", kShowTotalRAMSize },
		{ "strtod", "String To Decial/Hex Convert", kStringToDecimalHexTest },
		{ "shutdown", "Shutdown ANd Reboot OS", kShutdown },
		{ "settimer", "Set PIT Controller Counter0, ex)settimer 10(ms) 1(period)", kSetTimer},
		{ "wait", "Wait ms Using PIT, ex) wait 100(ms)", kWaitUsingPIT },
		{ "rdtsc", "Read Time Stamp Counter", kReadTimeStampCounter },
		{ "cpuspeed", "Measure Processor Speed", kMeasureProcessorSpeed },
		{ "date", "Show Date And Time", kShowDateAndTime },
		{ "createtask", "Create Task, ex) createtask 1(type) 10(count)", kCreateTestTask },
		{ "changepriority", "Change Task Priority, ex) changepriority 1(ID) 2(Priority)", kChangeTaskPriority },
		{ "tasklist", "Show Task List", kShowTaskList },
		{ "killtask", "End Task ex) killtask 1(ID) or 0xffffffff(All task)", kKillTask },
		{ "cpuload", "show Processor Load", kCPULoad },
		{ "testmutex", "Test Mutex Function", kTestMutex },
		{ "testthread", "Test Thread And Process Function", kTestThread },
		{ "showmatrix", "Show matrix", kShowMatrix },
		{ "testpie", "Test PIE Calculation", kTestPIE },
		{ "dynamicmeminfo", "Show Dynamic Memory Information", kShowDynamicMemoryInformation},
		{ "testseqalloc", "Test Sequential Allocation & Free", kTestSequentialAllocation},
		{ "testranalloc", "Test Random Allocation & Free", kTestRandomAllocation},
		{ "hddinfo", "Show HDD Information", kShowHDDInformation},
		{ "readsector", "Read HDD Sector, ex) readsector 0(LBA) 10(count)", kReadSector},
		{ "writesector", "Write HDD Sector, ex) writesector 0(LBA) 10(count)", kWriteSector},
        { "mounthdd", "Mount HDD", kMountHDD },
        { "formathdd", "Format HDD", kFormatHDD },
        { "filesysteminfo", "Show File System Information", kShowFileSystemInformation },
        { "createfile", "Create File, ex)createfile a.txt", kCreateFileInRootDirectory },
        { "deletefile", "Delete File, ex)deletefile a.txt", kDeleteFileInRootDirectory },
        { "dir", "Show Directory", kShowRootDirectory },
        { "writefile", "Write data to file, ex) writefile a.txt", kWriteDataToFile },
        { "readfile", "Read data from file, ex) readfile a.txt", kReadDataFromFile },
        { "testfileio", "Test File I/O Function", kTestFileIO },
};

// 쉘의 메인 루프

void kStartConsoleShell( void )
{
	char vcCommandBuffer[ CONSOLESHELL_MAXCOMMANDBUFFERCOUNT ];
	int iCommandBufferIndex = 0;
	BYTE bKey;
	int iCursorX, iCursorY;

	// 프롬프트 출력
	kPrintf( (CONSOLESHELL_PROMPTMESSAGE) );

	while(1)
	{
		// 키 수신 대기
		bKey = kGetCh();
		// 키가 백스페이스인경우
		if( bKey == KEY_BACKSPACE )
		{
			if( iCommandBufferIndex > 0 )
			{
				kGetCursor( &iCursorX, &iCursorY );
				// 한칸 전으로 이동한 뒤, 공백 출력
				kPrintStringXY( iCursorX - 1, iCursorY, " ");
				kSetCursor( iCursorX -1, iCursorY );
				iCommandBufferIndex--;
			}
		}
		// 키가 엔터인경우
		else if( bKey == KEY_ENTER )
		{
			kPrintf( "\n" );

			// 이미 들어있는 글자가 있으면 명령어 실행
			if( iCommandBufferIndex > 0 )
			{
				vcCommandBuffer[ iCommandBufferIndex ] = '\0';
				kExecuteCommand( vcCommandBuffer );
			}
			kPrintf( "%s", CONSOLESHELL_PROMPTMESSAGE );
			kMemSet( vcCommandBuffer, '\0', CONSOLESHELL_MAXCOMMANDBUFFERCOUNT );
			iCommandBufferIndex = 0;
		}
		// Shift, Cpas Lock, Num Lock, Scroll Lock은 무시
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

	// 공백으로 구분된 커맨드를 추출
	iCommandBufferLength = kStrLen( pcCommandBuffer );
	for( iSpaceIndex = 0 ; iSpaceIndex < iCommandBufferLength ; iSpaceIndex++ )
	{
		if( pcCommandBuffer[ iSpaceIndex ] == ' ')
		{
			break;
		}
	}
	// 커맨드가 일치하는가?
	iCount = sizeof( gs_vstCommandTable ) / sizeof( SHELLCOMMANDENTRY ); // 현재 gs_vstCommandTable에 몇개의 명령어가 저장되어있는가?
	for( i = 0 ; i < iCount ; i++ )
	{
		iCommandLength = kStrLen( gs_vstCommandTable[i].pcCommand );
		if( ( iCommandLength == iSpaceIndex) && (kMemCmp( gs_vstCommandTable[i].pcCommand, pcCommandBuffer, iSpaceIndex) == 0))
		{
			// pfFunction에 저장된 함수에다가 스페이스로 이어진 다음 파라미터를 넣는다.
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

	// 더이상 파라미터 없음
	if( pstList->iLength <= pstList->iCurrentPosition )
	{
		return 0;
	}

	// 버퍼의 길이만큼 이동하면서 공백을 검색
	for( i = pstList->iCurrentPosition ; i < pstList->iLength ; i++ )
	{
		if( pstList->pcBuffer[i] == ' ' )
			break;
	}

	// 파라미터 복사 및 길이 반환
	kMemCpy(pcParameter, pstList->pcBuffer + pstList->iCurrentPosition, i );
	iLength = i - pstList->iCurrentPosition;
	pcParameter[ iLength ] = '\0';

	// 파라미터 위치 업데이트
	pstList->iCurrentPosition += iLength + 1;
	return iLength;
}

// 커맨드를 처리하는 코드
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

	// 가장 긴 커맨드의 길이를 계산 ( 헬프들의 Align을 위하여)
	for(i = 0 ; i < iCount ; i++ )
	{
		iLength = kStrLen( gs_vstCommandTable[i].pcCommand );
		if( iLength > iMaxCommandLength )
		{
			iMaxCommandLength = iLength;
		}
	}

	// 도움말 출력
	for( i = 0 ; i < iCount ; i++ )
	{
		kPrintf( "%s", gs_vstCommandTable[i].pcCommand );
		kGetCursor( &iCursorX, &iCursorY );
		kSetCursor( iMaxCommandLength, iCursorY );
		kPrintf( " - %s\n", gs_vstCommandTable[i].pcHelp);

		if( (i != 0) && (( i % 20) == 0) )
		{
			kPrintf("Press any key to continue... ('q' to exit) : ");
			if( kGetCh() == 'q')
			{
				kPrintf("\n");
				break;
			}
			kPrintf("\n");
		}
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

	// 파라미터 초기화
	kInitializeParameter( &stList, pcParameterBuffer );
	while(1)
	{
		// 다음 파라미터를 구함, 파라미터 길이가 0이면 종료
		iLength = kGetNextParameter( &stList, vcParameter );
		if( iLength == 0 )
		{
			break;
		}

		// 파라미터에 대한 정보를 출력하고 16진수인지 10진수인지 판단 변환
		kPrintf("Param %d = %s, Length = %d ",iCount+1, vcParameter, iLength);

		//0x로 시작하면 16진수
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

void kSetTimer( const char* pcParameterBuffer)
{
	char vcParameter[ 100 ];
	PARAMETERLIST stList;
	long lValue;
	BOOL bPeriodic;

	// 파라미터 초기화
	kInitializeParameter( &stList, pcParameterBuffer );

	// millisecond 추출
	if( kGetNextParameter( &stList, vcParameter ) == 0 )
	{
		kPrintf("ex)settimer 10(ms) 1(period<true/false>)\n");
		return;
	}
	lValue = kAToI( vcParameter, 10 );

	if( kGetNextParameter( &stList, vcParameter) == 0 )
	{
		kPrintf("ex)settimer 10(ms) 1(period<true/false>)");
		return;
	}
	bPeriodic = kAToI(vcParameter, 10);

	kInitializePIT(MSTOCOUNT(lValue), bPeriodic );
	kPrintf("Time = %d ms, Period = %d Change Complete\n", lValue, bPeriodic );

}
void kWaitUsingPIT( const char* pcParameterBuffer )
{
	char vcParameter[ 100 ];
	int iLength;
	PARAMETERLIST stList;
	long lMillisecond;
	int i;

	// 파라미터 초기화
	kInitializeParameter( &stList, pcParameterBuffer );
	if( kGetNextParameter( &stList, vcParameter) == 0 )
	{
		kPrintf("ex)wait 100(ms)\n");
		return;
	}

	lMillisecond = kAToI( pcParameterBuffer, 10);
	kPrintf("%d ms Sleep Start...\n",lMillisecond);
	// 인터럽트를 비활성화 하고 PIT 컨트롤러를 통해 직접 시간 측정
	kDisableInterrupt();
	for( i = 0 ; i < lMillisecond/30 ; i++ )
	{
		kWaitUsingDirectPIT(MSTOCOUNT(30));
	}

	kWaitUsingDirectPIT(MSTOCOUNT(lMillisecond % 30));
	kEnableInterrupt();
	kPrintf("%d ms Sleep Complete\n",lMillisecond);

	// 타이머 복원
	kInitializePIT( MSTOCOUNT(1), TRUE );

}

void kReadTimeStampCounter( const char* pcParameterBuffer )
{
	QWORD qwTSC;
	qwTSC = kReadTSC();
	kPrintf("Time Stamp Counter = %q\n",qwTSC );
}

void kMeasureProcessorSpeed( const char* pcParameterBuffer )
{
	int i;
	QWORD qwLastTSC, qwTotalTSC = 0;

	kPrintf("Now Measuring.");

	// 10초동안 변화한 타임스탬프 카운터를 이용하여 프로세서의 속도를 간접적으로 측정
	kDisableInterrupt();
	for( i = 0 ; i < 200 ; i ++ )
	{
		qwLastTSC = kReadTSC();
		kWaitUsingDirectPIT( MSTOCOUNT( 50 ) ); // 50ms * 200 = 10000ms = 10sec
		qwTotalTSC += kReadTSC() - qwLastTSC;

		kPrintf(".");
	}
	kInitializePIT( MSTOCOUNT( 1 ), TRUE );
	kEnableInterrupt();

	kPrintf("\nCPU Speed = %d MHz\n", qwTotalTSC / 10 / 1000 / 1000);
}

void kShowDateAndTime( const char* pcParameterBuffer )
{
	BYTE bSecond, bMinute, bHour;
	BYTE bDayOfWeek, bDayOfMonth, bMonth;
	WORD wYear;

	// RTC 컨트롤러에서 시간 및 일자를 읽음

	kReadRTCTime( &bHour, &bMinute, &bSecond );
	kReadRTCDate( &wYear, &bMonth, &bDayOfMonth, &bDayOfWeek );

	kPrintf("Date: %d/%d/%d %s ",wYear,bMonth,bDayOfMonth,kConvertDayOfWeekToString(bDayOfWeek));
	kPrintf( "Time: %d:%d:%d\n", bHour, bMinute, bSecond );
}


void kTestTask1(void)
{
    BYTE bData;
    int i = 0, iX = 0, iY = 0, iMargin, j;
    CHARACTER* pstScreen = ( CHARACTER* ) CONSOLE_VIDEOMEMORYADDRESS;
    TCB* pstRunningTask;

    // 자신의 ID를 얻어서 화면 오프셋으로 사용
    pstRunningTask = kGetRunningTask();
    iMargin = ( pstRunningTask->stLink.qwID & 0xFFFFFFFF ) % 10;

    // 화면 네 귀퉁이를 돌면서 문자 출력
    for( j = 0 ; j <= 20000 ; j++ )
    {
        switch( i )
        {
        case 0:
            iX++;
            if( iX >= ( CONSOLE_WIDTH - iMargin ) )
            {
                i = 1;
            }
            break;

        case 1:
            iY++;
            if( iY >= ( CONSOLE_HEIGHT - iMargin ) )
            {
                i = 2;
            }
            break;

        case 2:
            iX--;
            if( iX < iMargin )
            {
                i = 3;
            }
            break;

        case 3:
            iY--;
            if( iY < iMargin )
            {
                i = 0;
            }
            break;
        }

        // 문자 및 색깔 지정
        pstScreen[ iY * CONSOLE_WIDTH + iX ].bCharacter = bData;
        pstScreen[ iY * CONSOLE_WIDTH + iX ].bAttribute = bData & 0x0F;
        bData++;

        // 다른 태스크로 전환
        //kSchedule();
    }

    kExitTask();
}
void kTestTask2(void)
{
    int i = 0, iOffset;
    CHARACTER* pstScreen = ( CHARACTER* ) CONSOLE_VIDEOMEMORYADDRESS;
    TCB* pstRunningTask;
    char vcData[ 4 ] = { '-', '\\', '|', '/' };

    // 자신의 ID를 얻어서 화면 오프셋으로 사용
    pstRunningTask = kGetRunningTask();
    iOffset = ( pstRunningTask->stLink.qwID & 0xFFFFFFFF ) * 2;
    iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT -
        ( iOffset % ( CONSOLE_WIDTH * CONSOLE_HEIGHT ) );

    while( 1 )
    {
        // 회전하는 바람개비를 표시
        pstScreen[ iOffset ].bCharacter = vcData[ i % 4 ];
        // 색깔 지정
        pstScreen[ iOffset ].bAttribute = ( iOffset % 15 ) + 1;
        i++;

        // 다른 태스크로 전환
        //kSchedule();
    }
}
void kCreateTestTask( const char* pcParameterBuffer )
{
	PARAMETERLIST stList;

	char vcType[30];
	char vcCount[30];
	int i;

	kInitializeParameter( &stList, pcParameterBuffer );
	kGetNextParameter( &stList, vcType );
	kGetNextParameter( &stList, vcCount );


	switch( kAToI(vcType, 10 ))
	{
	case 1:
		for( i = 0 ; i < kAToI( vcCount, 10) ; i++ )
		{
			if( kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD , 0, 0, (QWORD) kTestTask1 ) == NULL )
			{
				break;
			}
		}
		kPrintf("Task1 %d Created\n",i);
		break;

	case 2:
	default:
		for( i = 0 ; i < kAToI( vcCount, 10) ; i++ )
		{
			if( kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0 , (QWORD) kTestTask2 ) == NULL )
			{
				break;
			}
		}
		kPrintf("Task2 %d Created\n",i);
		break;
	}

}

static void kChangeTaskPriority( const char* pcParameterBuffer )
{
	PARAMETERLIST stList;
	char vcID[ 30 ];
	char vcPriority[ 30 ];
	QWORD qwID;
	BYTE bPriority;

	kInitializeParameter( &stList, pcParameterBuffer );
	kGetNextParameter( &stList, vcID );
	kGetNextParameter( &stList, vcPriority );

	if( kMemCmp( vcID, "0x", 2 ) == 0)
	{
		qwID = kAToI( vcID + 2, 16);
	}
	else
	{
		qwID = kAToI( vcID, 10 );
	}
	bPriority = kAToI( vcPriority, 10 );

	kPrintf( "Change Task Priority ID [0x%q] Priority [%d]",qwID, bPriority );
	if( kChangePriority( qwID, bPriority ) == TRUE )
	{
		kPrintf("Success\n");
	}
	else
	{
		kPrintf("Fail\n");
	}
}

static void kShowTaskList( const char* pcParameterBufffer )
{
	int i;
	TCB* pstTCB;
	int iCount = 0;

	kPrintf("================ Task Total Count [%d] ===============\n", kGetTaskCount());
	for( i = 0 ; i < TASK_MAXCOUNT ; i++ )
	{
		pstTCB = kGetTCBInTCBPool( i );
		// 0이 아니면 사용중인 TCB이다
		if( ( pstTCB->stLink.qwID >> 32 ) != 0 )
		{
			if( (iCount != 0) && ((iCount % 10) == 0 ))
			{
				kPrintf("Press any key to continue... ('q' is to exit)");
				if( kGetCh() == 'q' )
				{
					kPrintf("\n");
					break;
				}
				kPrintf("\n");
			}
			kPrintf("[%d] Task ID[0x%Q], Priority[%d], Flags[0x%Q], Thread[%d]\n", 1+iCount++, pstTCB->stLink.qwID, GETPRIORITY(pstTCB->qwFlags), pstTCB->qwFlags, kGetListCount( &(pstTCB->stChildThreadList) ) );
			kPrintf("    Parent PID[0x%Q], Memory Address[0x%Q], Size[0x%Q]\n", pstTCB->qwParentProcessID, pstTCB->pvMemoryAddress, pstTCB->qwMemorySize );
		}
	}
}

static void kKillTask( const char* pcParameterBuffer )
{
	PARAMETERLIST stList;
	char vcID[ 30 ];
	QWORD qwID;
	TCB* pstTCB;
	int i;

	kInitializeParameter( &stList, pcParameterBuffer );
	kGetNextParameter( &stList, vcID );

	// 태스크를 종료
	if( kMemCmp( vcID, "0x", 2 ) == 0 )
	{
		qwID = kAToI( vcID + 2, 16 );
	}
	else
	{
		qwID = kAToI( vcID, 10 );
	}

	if( qwID != 0xFFFFFFFF )
	{
		pstTCB = kGetTCBInTCBPool( GETTCBOFFSET( qwID ) );
		qwID = pstTCB->stLink.qwID;

		if( ( ( qwID >> 32 ) != 0 ) && ( ( pstTCB->qwFlags & TASK_FLAGS_SYSTEM) == 0x00 ) )
		{
			kPrintf("Kill Task ID [0x%q]", qwID);
			if( kEndTask( qwID ) == TRUE )
			{
				kPrintf("Success\n");
			}
			else
			{
				kPrintf("Fail\n");
			}
		}
		else
		{
			kPrintf("Task does not exist or task is system task\n");
		}
	}
	else
	{
		// i 0은 콘솔 1는 유휴 태스크
		for( i = 0 ; i < TASK_MAXCOUNT; i++ )
		{
			pstTCB = kGetTCBInTCBPool( i );
			qwID = pstTCB->stLink.qwID;
			// 유효하면
			if( ( ( qwID >> 32 ) != 0 ) && ( (  pstTCB->qwFlags & TASK_FLAGS_SYSTEM ) == 0x00 ) )
			{
				kPrintf("Kill Task ID [0x%q] ",qwID);
				if( kEndTask( qwID ) == TRUE )
				{
					kPrintf("Success\n");
				}
				else
				{
					kPrintf("Fail\n");
				}
			}
		}
	}

}

static void kCPULoad( const char* pcParameterBuffer )
{
	kPrintf("Processor Load : %d%%\n", kGetProcessorLoad() );
}

static MUTEX gs_stMutex;
static volatile QWORD gs_qwAdder;

static void kPrintNumberTask( void )
{
	int i;
	int j;
	QWORD qwTickCount;

	qwTickCount = kGetTickCount();
	while( ( kGetTickCount() - qwTickCount < 50 ) )
	{
		kSchedule();
	}
	// 콘솔셀 메시지 출력하기 위한 대기

	for( i = 0 ; i < 5 ; i++ )
	{
		kLock( &(gs_stMutex) );
		kPrintf( "Task ID [0x%Q] value[%d]\n", kGetRunningTask()->stLink.qwID, gs_qwAdder);

		gs_qwAdder += 1;

		kUnlock( &( gs_stMutex) );

		for( j = 0 ; j < 30000 ; j++);
	}

	// 모든 태스크가 종료할때까지 1초정도 대기
	qwTickCount = kGetTickCount();
	while( ( kGetTickCount() - qwTickCount ) < 1000 )
	{
		kSchedule();
	}

	kExitTask();
}

static void kTestMutex( const char* pcParameter)
{
	int i;

	gs_qwAdder = 1;

	kInitializeMutex( &gs_stMutex );

	for( i = 0 ; i < 3 ; i++ )
	{
		kCreateTask( TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kPrintNumberTask );
	}
	kPrintf( "Wait Until %d Task End...\n", i);
	kGetCh();
}
static void kCreateThreadTask( void )
{
	int i;

	for( i = 0 ; i < 3 ; i++ )
	{
		kCreateTask( TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD) kTestTask2 );
	}
	while(1)
	{
		kSleep(1);
	}
}

static void kTestThread( const char* pcParameterBuffer )
{
	TCB* pstProcess;

	pstProcess = kCreateTask( TASK_FLAGS_LOW | TASK_FLAGS_PROCESS, (void*)0xEEEEEEEE, 0x1000, (QWORD) kCreateThreadTask );

	if(pstProcess != NULL )
	{
		kPrintf("Process [0x%Q] create Success\n", pstProcess->stLink.qwID);
	}
	else
	{
		kPrintf("Process Create Fail!\n");
	}
}
static volatile QWORD gs_qwRandomValue = 0;

/**
 *  임의의 난수를 반환
 */
QWORD kRandom( void )
{
    gs_qwRandomValue = ( gs_qwRandomValue * 412153 + 5571031 ) >> 16;
    return gs_qwRandomValue;
}

/**
 *  철자를 흘러내리게 하는 스레드
 */
static void kDropCharactorThread( void )
{
    int iX, iY;
    int i;
    char vcText[ 2 ] = { 0, };

    iX = kRandom() % CONSOLE_WIDTH;

    while( 1 )
    {
        // 잠시 대기함
        kSleep( kRandom() % 20 );

        if( ( kRandom() % 20 ) < 16 )
        {
            vcText[ 0 ] = ' ';
            for( i = 0 ; i < CONSOLE_HEIGHT - 1 ; i++ )
            {
                kPrintStringXY( iX, i , vcText );
                kSleep( 50 );
            }
        }
        else
        {
            for( i = 0 ; i < CONSOLE_HEIGHT - 1 ; i++ )
            {
                vcText[ 0 ] = i + kRandom();
                kPrintStringXY( iX, i, vcText );
                kSleep( 50 );
            }
        }
    }
}

/**
 *  스레드를 생성하여 매트릭스 화면처럼 보여주는 프로세스
 */
static void kMatrixProcess( void )
{
    int i;

    for( i = 0 ; i < 300 ; i++ )
    {
        if( kCreateTask( TASK_FLAGS_THREAD | TASK_FLAGS_LOW, 0, 0,
                         ( QWORD ) kDropCharactorThread ) == NULL )
        {
            break;
        }

        kSleep( kRandom() % 5 + 5 );
    }

    kPrintf( "%d Thread is created\n", i );

    // 키가 입력되면 프로세스 종료
    kGetCh();
}

/**
 *  매트릭스 화면을 보여줌
 */
static void kShowMatrix( const char* pcParameterBuffer )
{
    TCB* pstProcess;

    pstProcess = kCreateTask( TASK_FLAGS_PROCESS | TASK_FLAGS_LOW, ( void* ) 0xE00000, 0xE00000,
                              ( QWORD ) kMatrixProcess );
    if( pstProcess != NULL )
    {
        kPrintf( "Matrix Process [0x%Q] Create Success\n" );

        // 태스크가 종료 될 때까지 대기
        while( ( pstProcess->stLink.qwID >> 32 ) != 0 )
        {
            kSleep( 100 );
        }
    }
    else
    {
        kPrintf( "Matrix Process Create Fail\n" );
    }
}

static void kFPUTestTask( void )
{
	double dValue1;
	double dValue2;
	TCB* pstRunningTask;
	QWORD qwCount;
	QWORD qwRandomValue;
	int i;
	int iOffset;
	char vcData[ 4 ] = {'-', '\\', '|', '/'};
	CHARACTER* pstScreen = (CHARACTER*) CONSOLE_VIDEOMEMORYADDRESS;

	pstRunningTask = kGetRunningTask();

	iOffset = ( pstRunningTask->stLink.qwID & 0xFFFFFFFF) * 2;
	iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT - ( iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT) );

	while(1)
	{
		dValue1 = 1;
		dValue2 = 1;

		for( i = 0 ; i < 10 ; i++ )
		{
			qwRandomValue = kRandom();
			dValue1 *= ( double ) qwRandomValue;
			dValue2 *= ( double ) qwRandomValue;

			kSleep(1);

			qwRandomValue = kRandom();
			dValue1 /= ( double ) qwRandomValue;
			dValue2 /= ( double ) qwRandomValue;
		}

		if( dValue1 != dValue2 )
		{
			kPrintf("Value is not same~!! [%f] != [%f]\n",dValue1, dValue2);
			break;
		}
		qwCount++;

		pstScreen[ iOffset ].bCharacter = vcData[ qwCount % 4 ];
		pstScreen[ iOffset ].bAttribute = (iOffset % 15) + 1;

	}
}

static void kTestPIE( const char* pcParameterBuffer )
{
	double dResult;
	int i;

	kPrintf("PIE Calculation Test\n");
	kPrintf("Result : 355 / 113 = ");
	dResult = (double)355/113;
	kPrintf("%d.%d%d\n", (QWORD)dResult, ( ( QWORD )(dResult*10) % 10 ) , ( ( QWORD ) (dResult * 100) % 10) );

	for( i = 0 ; i < 100 ; i++ )
	{
		kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kFPUTestTask );
	}
}

static void kShowDynamicMemoryInformation( const char* pcParameterBuffer )
{
	QWORD qwStartAddress, qwTotalSize, qwMetaSize, qwUsedSize;

	kGetDynamicMemoryInformation( &qwStartAddress, &qwTotalSize, &qwMetaSize, &qwUsedSize );

	kPrintf("================ Dynamic Memory Information ===============\n");
	kPrintf("Start Address: [0x%Q]\n", qwStartAddress);
	kPrintf("Total Size:    [0x%Q]byte, [%d]MB\n", qwTotalSize, qwTotalSize / 1024 / 1024 );
	kPrintf("Meta Size:     [0x%Q]byte, [%d]KB\n", qwMetaSize, qwMetaSize / 1024);
	kPrintf("Used Size:     [0x%Q]byte, [%d]KB\n", qwUsedSize, qwUsedSize / 1024);

}

static void kTestSequentialAllocation( const char* pcParameterBuffer )
{
	DYNAMICMEMORY* pstMemory;
	long i, j, k;
	QWORD* pqwBuffer;
	kPrintf("================ Dynamic Memory Test ===============\n");
	pstMemory = kGetDynamicMemoryManager();

	for( i = 0 ; i < pstMemory->iMaxLevelCount ; i++ )
	{
		kPrintf("Block List [%d] Test Start\n", i);
		kPrintf("Allocation And Compare: ");

		for(j = 0 ; j < (pstMemory->iBlockCountOfSmallestBlock >> i) ; j++ )
		{
			pqwBuffer = kAllocateMemory(DYNAMICMEMORY_MIN_SIZE << i);
			if(pqwBuffer == NULL)
			{
				kPrintf("\nAllocation Fail\n");
				return;
			}

			for( k = 0 ; k < (DYNAMICMEMORY_MIN_SIZE << i) / 8 ; k++ )
			{
				pqwBuffer[k] = k;
			}

			for( k = 0 ; k < (DYNAMICMEMORY_MIN_SIZE << i) / 8 ; k++ )
			{
				if( pqwBuffer[ k ] != k)
				{
					kPrintf("\nCompare Fail\n");
					return;
				}
			}
			kPrintf(".");
		}
		kPrintf("\nFree: ");
		for( j = 0 ; j < (pstMemory->iBlockCountOfSmallestBlock >> i) ; j++)
		{
			if( kFreeMemory( ( void* ) (pstMemory->qwStartAddress + (DYNAMICMEMORY_MIN_SIZE << i) * j)) == FALSE)
			{
				kPrintf("\nFree Fail\n");
				return;
			}
			kPrintf(".");
		}
		kPrintf("\n");
	}
	kPrintf("Test Completed!\n");
}

static void kRandomAllocationTask( void )
{
    TCB* pstTask;
    QWORD qwMemorySize;
    char vcBuffer[ 200 ];
    BYTE* pbAllocationBuffer;
    int i, j;
    int iY;

    pstTask = kGetRunningTask();
    iY = ( pstTask->stLink.qwID ) % 15 + 9;

    for( j = 0 ; j < 10 ; j++ )
    {
        // 1KB ~ 32M까지 할당하도록 함
        do
        {
            qwMemorySize = ( ( kRandom() % ( 32 * 1024 ) ) + 1 ) * 1024;
            pbAllocationBuffer = kAllocateMemory( qwMemorySize );

            // 만일 버퍼를 할당 받지 못하면 다른 태스크가 메모리를 사용하고
            // 있을 수 있으므로 잠시 대기한 후 다시 시도
            if( pbAllocationBuffer == 0 )
            {
                kSleep( 1 );
            }
        } while( pbAllocationBuffer == 0 );

        kSPrintf( vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Allocation Success",
                  pbAllocationBuffer, qwMemorySize );
        // 자신의 ID를 Y 좌표로 하여 데이터를 출력
        kPrintStringXY( 20, iY, vcBuffer );
        kSleep( 200 );

        // 버퍼를 반으로 나눠서 랜덤한 데이터를 똑같이 채움
        kSPrintf( vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Data Write...     ",
                  pbAllocationBuffer, qwMemorySize );
        kPrintStringXY( 20, iY, vcBuffer );
        for( i = 0 ; i < qwMemorySize / 2 ; i++ )
        {
            pbAllocationBuffer[ i ] = kRandom() & 0xFF;
            pbAllocationBuffer[ i + ( qwMemorySize / 2 ) ] = pbAllocationBuffer[ i ];
        }
        kSleep( 200 );

        // 채운 데이터가 정상적인지 다시 확인
        kSPrintf( vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Data Verify...   ",
                  pbAllocationBuffer, qwMemorySize );
        kPrintStringXY( 20, iY, vcBuffer );
        for( i = 0 ; i < qwMemorySize / 2 ; i++ )
        {
            if( pbAllocationBuffer[ i ] != pbAllocationBuffer[ i + ( qwMemorySize / 2 ) ] )
            {
                kPrintf( "Task ID[0x%Q] Verify Fail\n", pstTask->stLink.qwID );
                kExitTask();
            }
        }
        kFreeMemory( pbAllocationBuffer );
        kSleep( 200 );
    }

    kExitTask();
}

/**
 *  태스크를 여러 개 생성하여 임의의 메모리를 할당하고 해제하는 것을 반복하는 테스트
 */
static void kTestRandomAllocation( const char* pcParameterBuffer )
{
    int i;

    for( i = 0 ; i < 1000 ; i++ )
    {
        kCreateTask( TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD, 0, 0, ( QWORD ) kRandomAllocationTask );
    }
}

static void kShowHDDInformation( const char* pcParameterBuffer )
{
    HDDINFORMATION stHDD;
    char vcBuffer[ 100 ];

    // 하드 디스크의 정보를 읽음
    if( kReadHDDInformation( TRUE, TRUE, &stHDD ) == FALSE )
    {
        kPrintf( "HDD Information Read Fail\n" );
        return ;
    }

    kPrintf( "============ Primary Master HDD Information ============\n" );

    // 모델 번호 출력
    kMemCpy( vcBuffer, stHDD.vwModelNumber, sizeof( stHDD.vwModelNumber ) );
    vcBuffer[ sizeof( stHDD.vwModelNumber ) - 1 ] = '\0';
    kPrintf( "Model Number:\t %s\n", vcBuffer );

    // 시리얼 번호 출력
    kMemCpy( vcBuffer, stHDD.vwSerialNumber, sizeof( stHDD.vwSerialNumber ) );
    vcBuffer[ sizeof( stHDD.vwSerialNumber ) - 1 ] = '\0';
    kPrintf( "Serial Number:\t %s\n", vcBuffer );

    // 헤드, 실린더, 실린더 당 섹터 수를 출력
    kPrintf( "Head Count:\t %d\n", stHDD.wNumberOfHead );
    kPrintf( "Cylinder Count:\t %d\n", stHDD.wNumberOfCylinder );
    kPrintf( "Sector Count:\t %d\n", stHDD.wNumberOfSectorPerCylinder );

    // 총 섹터 수 출력
    kPrintf( "Total Sector:\t %d Sector, %dMB\n", stHDD.dwTotalSectors,
            stHDD.dwTotalSectors / 2 / 1024 );
}

/**
 *  하드 디스크에 파라미터로 넘어온 LBA 어드레스에서 섹터 수 만큼 읽음
 */
static void kReadSector( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcLBA[ 50 ], vcSectorCount[ 50 ];
    DWORD dwLBA;
    int iSectorCount;
    char* pcBuffer;
    int i, j;
    BYTE bData;
    BOOL bExit = FALSE;

    // 파라미터 리스트를 초기화하여 LBA 어드레스와 섹터 수 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    if( ( kGetNextParameter( &stList, vcLBA ) == 0 ) ||
        ( kGetNextParameter( &stList, vcSectorCount ) == 0 ) )
    {
        kPrintf( "ex) readsector 0(LBA) 10(count)\n" );
        return ;
    }
    dwLBA = kAToI( vcLBA, 10 );
    iSectorCount = kAToI( vcSectorCount, 10 );

    // 섹터 수만큼 메모리를 할당 받아 읽기 수행
    pcBuffer = kAllocateMemory( iSectorCount * 512 );
    if( kReadHDDSector( TRUE, TRUE, dwLBA, iSectorCount, pcBuffer ) == iSectorCount )
    {
        kPrintf( "LBA [%d], [%d] Sector Read Success~!!", dwLBA, iSectorCount );
        // 데이터 버퍼의 내용을 출력
        for( j = 0 ; j < iSectorCount ; j++ )
        {
            for( i = 0 ; i < 512 ; i++ )
            {
                if( !( ( j == 0 ) && ( i == 0 ) ) && ( ( i % 256 ) == 0 ) )
                {
                    kPrintf( "\nPress any key to continue... ('q' is exit) : " );
                    if( kGetCh() == 'q' )
                    {
                        bExit = TRUE;
                        break;
                    }
                }

                if( ( i % 16 ) == 0 )
                {
                    kPrintf( "\n[LBA:%d, Offset:%d]\t| ", dwLBA + j, i );
                }

                // 모두 두 자리로 표시하려고 16보다 작은 경우 0을 추가해줌
                bData = pcBuffer[ j * 512 + i ] & 0xFF;
                if( bData < 16 )
                {
                    kPrintf( "0" );
                }
                kPrintf( "%X ", bData );
            }

            if( bExit == TRUE )
            {
                break;
            }
        }
        kPrintf( "\n" );
    }
    else
    {
        kPrintf( "Read Fail\n" );
    }

    kFreeMemory( pcBuffer );
}

/**
 *  하드 디스크에 파라미터로 넘어온 LBA 어드레스에서 섹터 수 만큼 씀
 */
static void kWriteSector( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcLBA[ 50 ], vcSectorCount[ 50 ];
    DWORD dwLBA;
    int iSectorCount;
    char* pcBuffer;
    int i, j;
    BOOL bExit = FALSE;
    BYTE bData;
    static DWORD s_dwWriteCount = 0;

    // 파라미터 리스트를 초기화하여 LBA 어드레스와 섹터 수 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    if( ( kGetNextParameter( &stList, vcLBA ) == 0 ) ||
        ( kGetNextParameter( &stList, vcSectorCount ) == 0 ) )
    {
        kPrintf( "ex) writesector 0(LBA) 10(count)\n" );
        return ;
    }
    dwLBA = kAToI( vcLBA, 10 );
    iSectorCount = kAToI( vcSectorCount, 10 );

    s_dwWriteCount++;

    // 버퍼를 할당 받아 데이터를 채움.
    // 패턴은 4 바이트의 LBA 어드레스와 4 바이트의 쓰기가 수행된 횟수로 생성
    pcBuffer = kAllocateMemory( iSectorCount * 512 );
    for( j = 0 ; j < iSectorCount ; j++ )
    {
        for( i = 0 ; i < 512 ; i += 8 )
        {
            *( DWORD* ) &( pcBuffer[ j * 512 + i ] ) = dwLBA + j;
            *( DWORD* ) &( pcBuffer[ j * 512 + i + 4 ] ) = s_dwWriteCount;
        }
    }

    // 쓰기 수행
    if( kWriteHDDSector( TRUE, TRUE, dwLBA, iSectorCount, pcBuffer ) != iSectorCount )
    {
        kPrintf( "Write Fail\n" );
        return ;
    }
    kPrintf( "LBA [%d], [%d] Sector Write Success~!!", dwLBA, iSectorCount );

    // 데이터 버퍼의 내용을 출력
    for( j = 0 ; j < iSectorCount ; j++ )
    {
        for( i = 0 ; i < 512 ; i++ )
        {
            if( !( ( j == 0 ) && ( i == 0 ) ) && ( ( i % 256 ) == 0 ) )
            {
                kPrintf( "\nPress any key to continue... ('q' is exit) : " );
                if( kGetCh() == 'q' )
                {
                    bExit = TRUE;
                    break;
                }
            }

            if( ( i % 16 ) == 0 )
            {
                kPrintf( "\n[LBA:%d, Offset:%d]\t| ", dwLBA + j, i );
            }

            // 모두 두 자리로 표시하려고 16보다 작은 경우 0을 추가해줌
            bData = pcBuffer[ j * 512 + i ] & 0xFF;
            if( bData < 16 )
            {
                kPrintf( "0" );
            }
            kPrintf( "%X ", bData );
        }

        if( bExit == TRUE )
        {
            break;
        }
    }
    kPrintf( "\n" );
    kFreeMemory( pcBuffer );
}

/**
 *  하드 디스크를 연결
 */
static void kMountHDD( const char* pcParameterBuffer )
{
    if( kMount() == FALSE )
    {
        kPrintf( "HDD Mount Fail\n" );
        return ;
    }
    kPrintf( "HDD Mount Success\n" );
}

/**
 *  하드 디스크에 파일 시스템을 생성(포맷)
 */
static void kFormatHDD( const char* pcParameterBuffer )
{
    if( kFormat() == FALSE )
    {
        kPrintf( "HDD Format Fail\n" );
        return ;
    }
    kPrintf( "HDD Format Success\n" );
}

/**
 *  파일 시스템 정보를 표시
 */
static void kShowFileSystemInformation( const char* pcParameterBuffer )
{
    FILESYSTEMMANAGER stManager;

    kGetFileSystemInformation( &stManager );

    kPrintf( "================== File System Information ==================\n" );
    kPrintf( "Mouted:\t\t\t\t\t %d\n", stManager.bMounted );
    kPrintf( "Reserved Sector Count:\t\t\t %d Sector\n", stManager.dwReservedSectorCount );
    kPrintf( "Cluster Link Table Start Address:\t %d Sector\n",
            stManager.dwClusterLinkAreaStartAddress );
    kPrintf( "Cluster Link Table Size:\t\t %d Sector\n", stManager.dwClusterLinkAreaSize );
    kPrintf( "Data Area Start Address:\t\t %d Sector\n", stManager.dwDataAreaStartAddress );
    kPrintf( "Total Cluster Count:\t\t\t %d Cluster\n", stManager.dwTotalClusterCount );
}

/**
 *  루트 디렉터리에 빈 파일을 생성
 */
static void kCreateFileInRootDirectory( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcFileName[ 50 ];
    int iLength;
    DWORD dwCluster;
    DIRECTORYENTRY stEntry;
    int i;
    FILE* pstFile;

    // 파라미터 리스트를 초기화하여 파일 이름을 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    iLength = kGetNextParameter( &stList, vcFileName );
    vcFileName[ iLength ] = '\0';
    if( ( iLength > ( sizeof( stEntry.vcFileName ) - 1 ) ) || ( iLength == 0 ) )
    {
        kPrintf( "Too Long or Too Short File Name\n" );
        return ;
    }

    pstFile = fopen(vcFileName,"w");
    if( pstFile == NULL )
    {
    	kPrintf("File Create Fail\n");
    	return;
    }
    fclose(pstFile);
    kPrintf( "File Create Success\n" );
}

/**
 *  루트 디렉터리에서 파일을 삭제
 */
static void kDeleteFileInRootDirectory( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcFileName[ 50 ];
    int iLength;

    // 파라미터 리스트를 초기화하여 파일 이름을 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    iLength = kGetNextParameter( &stList, vcFileName );
    vcFileName[ iLength ] = '\0';
    if( ( iLength > ( FILESYSTEM_MAXFILENAMELENGTH - 1 ) ) || ( iLength == 0 ) )
    {
        kPrintf( "Too Long or Too Short File Name\n" );
        return ;
    }

    // 파일 이름으로 디렉터리 엔트리를 검색
    if( remove(vcFileName) != 0 )
    {
    	kPrintf("File Not Found or File Opened");
    	return;
    }

    kPrintf( "File Delete Success\n" );
}

/**
 *  루트 디렉터리의 파일 목록을 표시
 */
static void kShowRootDirectory( const char* pcParameterBuffer )
{
    DIR* pstDirectory;
    int i, iCount, iTotalCount;
    struct dirent* pstEntry;
    char vcBuffer[ 400 ];
    char vcTempValue[ 50 ];
    DWORD dwTotalByte;
    DWORD dwUsedClusterCount;
    FILESYSTEMMANAGER stManager;

    kGetFileSystemInformation( &stManager );

    pstDirectory = opendir("/");
    if(pstDirectory == NULL)
    {
    	kPrintf("Root directory open fail\n");
    	return;
    }

    iTotalCount = 0;
    dwTotalByte = 0;
    dwUsedClusterCount = 0;

    while(1)
    {
    	// 디렉토리에서 엔트리 하나를 읽음
    	pstEntry = readdir(pstDirectory);

    	if(pstEntry == NULL)
    		break;

    	iTotalCount++;

    	dwTotalByte += pstEntry->dwFileSize;

        // 실제로 클러스터 개수를 계산할 때, 파일사이즈가 0이라도 클러스터 한개는 할당되어 있음
        if( pstEntry->dwFileSize == 0 )
        {
        	dwUsedClusterCount++;
        }
        else
        {
        	// 반올림
        	dwUsedClusterCount += (pstEntry->dwFileSize + (FILESYSTEM_CLUSTERSIZE - 1) ) / FILESYSTEM_CLUSTERSIZE;
        }
    }

    // 실제 파일의 내용을 표시하는 루프
    rewinddir( pstDirectory );
    iCount = 0;
    while(1)
    {
    	pstEntry = readdir(pstDirectory);

    	if(pstEntry == NULL)
    	{
    		break;
    	}
    	kMemSet( vcBuffer, ' ', sizeof(vcBuffer) - 1 );
    	vcBuffer[sizeof(vcBuffer) - 1] = '\0';

    	kMemCpy(vcBuffer, pstEntry->d_name, kStrLen(pstEntry->d_name));

    	kSPrintf(vcTempValue, "%d Byte", pstEntry->dwFileSize );
    	kMemCpy( vcBuffer+30, vcTempValue, kStrLen(vcTempValue));

    	kSPrintf(vcTempValue, "0x%x Cluster", pstEntry->dwStartClusterIndex);
    	kMemCpy(vcBuffer + 55, vcTempValue, kStrLen(vcTempValue) + 1);
    	kPrintf("    %s\n", vcBuffer);

    	if( ( iCount != 0 ) && ( ( iCount % 20 ) == 0 ) )
    	{
    		kPrintf("Press any key to continue... ('q' to exit) : ");
    		if( kGetCh() == 'q')
    		{
    			kPrintf("\n");
    			break;
    		}
    	}
    	iCount++;
    }

    kPrintf("\t\tTotal File Count: %d\n", iTotalCount );
    kPrintf("\t\tTotal File Size: %d KByte (%d Cluster)\n", dwTotalByte, dwUsedClusterCount);

    kPrintf("\t\tFree Space: %d KByte (%d Cluster)\n", ( stManager.dwTotalClusterCount - dwUsedClusterCount) * FILESYSTEM_CLUSTERSIZE / 1024, stManager.dwTotalClusterCount - dwUsedClusterCount );

    closedir( pstDirectory );
}


static void kWriteDataToFile( const char* pcParameterBuffer )
{
	PARAMETERLIST stList;
	char vcFileName[ 50 ];
	int iLength;
	FILE* fp;
	int iEnterCount;
	BYTE bKey;

	kInitializeParameter( &stList, pcParameterBuffer );
	iLength = kGetNextParameter( &stList, vcFileName );

	vcFileName[ iLength ] = '\0';

	if( ( iLength > ( FILESYSTEM_MAXFILENAMELENGTH - 1 ) )  || ( iLength == 0 ) )
	{
		kPrintf("Too Long or Too Short File Name\n");
		return;
	}

	fp = fopen( vcFileName, "w" );
	if( fp == NULL )
	{
		kPrintf("%s File Open Failed\n", vcFileName );
		return;
	}

	iEnterCount = 0;

	while(1)
	{
		bKey = kGetCh();

		if( bKey == KEY_ENTER )
		{
			iEnterCount++;
			if( iEnterCount >= 3 )
			{
				break;
			}
		}
		else
		{
			iEnterCount = 0;
		}

		kPrintf("%c", bKey);
		if( fwrite( &bKey, 1 , 1 , fp) != 1 )
		{
			kPrintf("File Write Fail\n");
			break;
		}
	}

	kPrintf("File Create Success\n");
	fclose(fp);
}

// 파일을 열어 데이터를 읽음
static void kReadDataFromFile( const char* pcParameterBuffer )
{
	PARAMETERLIST stList;
	char vcFileName[ 50 ];
	int iLength;
	FILE* fp;
	int iEnterCount;
	BYTE bKey;

	kInitializeParameter( &stList, pcParameterBuffer );
	iLength = kGetNextParameter( &stList, vcFileName );

	vcFileName[ iLength ] = '\0';

	if( ( iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1) ) || (iLength == 0 ) )
	{
		kPrintf("Too Long or Too Short File Name\n");
		return;
	}

	fp = fopen(vcFileName, "r");
	if( fp == NULL )
	{
		kPrintf("%s File Open Fail \n", vcFileName );
		return;
	}

	iEnterCount = 0;

	while(1)
	{
		// 한캐릭터 읽음
		if( fread( &bKey, 1, 1, fp ) != 1 )
		{
			break;
		}
		kPrintf("%c", bKey );

		// 엔터키인경우 20줄마다 물어봄
		if( bKey == KEY_ENTER )
		{
			iEnterCount++;

			if( ( iEnterCount != 0 ) && ( iEnterCount % 20 ) == 0 )
			{
				kPrintf("Press any key to continue... ('q' is exit) : ");
				if( kGetCh() == 'q' )
				{
					kPrintf("\n");
					break;
				}
				kPrintf("\n");
				iEnterCount = 0;
			}
		}
	}
	fclose(fp);
}

static void kTestFileIO( const char* pcParameterBuffer )
{
    FILE* pstFile;
    BYTE* pbBuffer;
    int i;
    int j;
    DWORD dwRandomOffset;
    DWORD dwByteCount;
    BYTE vbTempBuffer[ 1024 ];
    DWORD dwMaxFileSize;

    kPrintf( "================== File I/O Function Test ==================\n" );

    // 4Mbyte의 버퍼 할당
    dwMaxFileSize = 4 * 1024 * 1024;
    pbBuffer = kAllocateMemory( dwMaxFileSize );
    if( pbBuffer == NULL )
    {
        kPrintf( "Memory Allocation Fail\n" );
        return ;
    }
    // 테스트용 파일을 삭제
    remove( "testfileio.bin" );

    //==========================================================================
    // 파일 열기 테스트
    //==========================================================================
    kPrintf( "1. File Open Fail Test..." );
    // r 옵션은 파일을 생성하지 않으므로, 테스트 파일이 없는 경우 NULL이 되어야 함
    pstFile = fopen( "testfileio.bin", "r" );
    if( pstFile == NULL )
    {
        kPrintf( "[Pass]\n" );
    }
    else
    {
        kPrintf( "[Fail]\n" );
        fclose( pstFile );
    }

    //==========================================================================
    // 파일 생성 테스트
    //==========================================================================
    kPrintf( "2. File Create Test..." );
    // w 옵션은 파일을 생성하므로, 정상적으로 핸들이 반환되어야함
    pstFile = fopen( "testfileio.bin", "w" );
    if( pstFile != NULL )
    {
        kPrintf( "[Pass]\n" );
        kPrintf( "    File Handle [0x%Q]\n", pstFile );
    }
    else
    {
        kPrintf( "[Fail]\n" );
    }

    //==========================================================================
    // 순차적인 영역 쓰기 테스트
    //==========================================================================
    kPrintf( "3. Sequential Write Test(Cluster Size)..." );
    // 열린 핸들을 가지고 쓰기 수행
    for( i = 0 ; i < 100 ; i++ )
    {
        kMemSet( pbBuffer, i, FILESYSTEM_CLUSTERSIZE );
        if( fwrite( pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile ) !=
            FILESYSTEM_CLUSTERSIZE )
        {
            kPrintf( "[Fail]\n" );
            kPrintf( "    %d Cluster Error\n", i );
            break;
        }

    }
    if( i >= 100 )
    {
        kPrintf( "[Pass]\n" );
    }

    //==========================================================================
    // 순차적인 영역 읽기 테스트
    //==========================================================================
    kPrintf( "4. Sequential Read And Verify Test(Cluster Size)..." );
    // 파일의 처음으로 이동
    fseek( pstFile, -100 * FILESYSTEM_CLUSTERSIZE, SEEK_END );

    // 열린 핸들을 가지고 읽기 수행 후, 데이터 검증
    for( i = 0 ; i < 100 ; i++ )
    {
        // 파일을 읽음
        if( fread( pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile ) !=
            FILESYSTEM_CLUSTERSIZE )
        {
            kPrintf( "[Fail]\n" );
            return ;
        }

        // 데이터 검사
        for( j = 0 ; j < FILESYSTEM_CLUSTERSIZE ; j++ )
        {
            if( pbBuffer[ j ] != ( BYTE ) i )
            {
                kPrintf( "[Fail]\n" );
                kPrintf( "    %d Cluster Error. [%X] != [%X]\n", i, pbBuffer[ j ],
                         ( BYTE ) i );
                break;
            }
        }
    }
    if( i >= 100 )
    {
        kPrintf( "[Pass]\n" );
    }

    //==========================================================================
    // 임의의 영역 쓰기 테스트
    //==========================================================================
    kPrintf( "5. Random Write Test...\n" );

    // 버퍼를 모두 0으로 채움
    kMemSet( pbBuffer, 0, dwMaxFileSize );
    // 여기 저기에 옮겨다니면서 데이터를 쓰고 검증
    // 파일의 내용을 읽어서 버퍼로 복사
    fseek( pstFile, -100 * FILESYSTEM_CLUSTERSIZE, SEEK_CUR );
    fread( pbBuffer, 1, dwMaxFileSize, pstFile );

    // 임의의 위치로 옮기면서 데이터를 파일과 버퍼에 동시에 씀
    for( i = 0 ; i < 100 ; i++ )
    {
        dwByteCount = ( kRandom() % ( sizeof( vbTempBuffer ) - 1 ) ) + 1;
        dwRandomOffset = kRandom() % ( dwMaxFileSize - dwByteCount );

        kPrintf( "    [%d] Offset [%d] Byte [%d]...", i, dwRandomOffset,
                dwByteCount );

        // 파일 포인터를 이동
        fseek( pstFile, dwRandomOffset, SEEK_SET );
        kMemSet( vbTempBuffer, i, dwByteCount );

        // 데이터를 씀
        if( fwrite( vbTempBuffer, 1, dwByteCount, pstFile ) != dwByteCount )
        {
            kPrintf( "[Fail]\n" );
            break;
        }
        else
        {
            kPrintf( "[Pass]\n" );
        }

        kMemSet( pbBuffer + dwRandomOffset, i, dwByteCount );
    }

    // 맨 마지막으로 이동하여 1바이트를 써서 파일의 크기를 4Mbyte로 만듦
    fseek( pstFile, dwMaxFileSize - 1, SEEK_SET );
    fwrite( &i, 1, 1, pstFile );
    pbBuffer[ dwMaxFileSize - 1 ] = ( BYTE ) i;

    //==========================================================================
    // 임의의 영역 읽기 테스트
    //==========================================================================
    kPrintf( "6. Random Read And Verify Test...\n" );
    // 임의의 위치로 옮기면서 파일에서 데이터를 읽어 버퍼의 내용과 비교
    for( i = 0 ; i < 100 ; i++ )
    {
        dwByteCount = ( kRandom() % ( sizeof( vbTempBuffer ) - 1 ) ) + 1;
        dwRandomOffset = kRandom() % ( ( dwMaxFileSize ) - dwByteCount );

        kPrintf( "    [%d] Offset [%d] Byte [%d]...", i, dwRandomOffset,
                dwByteCount );

        // 파일 포인터를 이동
        fseek( pstFile, dwRandomOffset, SEEK_SET );

        // 데이터 읽음
        if( fread( vbTempBuffer, 1, dwByteCount, pstFile ) != dwByteCount )
        {
            kPrintf( "[Fail]\n" );
            kPrintf( "    Read Fail\n", dwRandomOffset );
            break;
        }

        // 버퍼와 비교
        if( kMemCmp( pbBuffer + dwRandomOffset, vbTempBuffer, dwByteCount )
                != 0 )
        {
            kPrintf( "[Fail]\n" );
            kPrintf( "    Compare Fail\n", dwRandomOffset );
            break;
        }

        kPrintf( "[Pass]\n" );
    }

    //==========================================================================
    // 다시 순차적인 영역 읽기 테스트
    //==========================================================================
    kPrintf( "7. Sequential Write, Read And Verify Test(1024 Byte)...\n" );
    // 파일의 처음으로 이동
    fseek( pstFile, -dwMaxFileSize, SEEK_CUR );

    // 열린 핸들을 가지고 쓰기 수행. 앞부분에서 2Mbyte만 씀
    for( i = 0 ; i < ( 2 * 1024 * 1024 / 1024 ) ; i++ )
    {
        kPrintf( "    [%d] Offset [%d] Byte [%d] Write...", i, i * 1024, 1024 );

        // 1024 바이트씩 파일을 씀
        if( fwrite( pbBuffer + ( i * 1024 ), 1, 1024, pstFile ) != 1024 )
        {
            kPrintf( "[Fail]\n" );
            return ;
        }
        else
        {
            kPrintf( "[Pass]\n" );
        }
    }

    // 파일의 처음으로 이동
    fseek( pstFile, -dwMaxFileSize, SEEK_SET );

    // 열린 핸들을 가지고 읽기 수행 후 데이터 검증. Random Write로 데이터가 잘못
    // 저장될 수 있으므로 검증은 4Mbyte 전체를 대상으로 함
    for( i = 0 ; i < ( dwMaxFileSize / 1024 )  ; i++ )
    {
        // 데이터 검사
        kPrintf( "    [%d] Offset [%d] Byte [%d] Read And Verify...", i,
                i * 1024, 1024 );

        // 1024 바이트씩 파일을 읽음
        if( fread( vbTempBuffer, 1, 1024, pstFile ) != 1024 )
        {
            kPrintf( "[Fail]\n" );
            return ;
        }

        if( kMemCmp( pbBuffer + ( i * 1024 ), vbTempBuffer, 1024 ) != 0 )
        {
            kPrintf( "[Fail]\n" );
            break;
        }
        else
        {
            kPrintf( "[Pass]\n" );
        }
    }

    //==========================================================================
    // 파일 삭제 실패 테스트
    //==========================================================================
    kPrintf( "8. File Delete Fail Test..." );
    // 파일이 열려있는 상태이므로 파일을 지우려고 하면 실패해야 함
    if( remove( "testfileio.bin" ) != 0 )
    {
        kPrintf( "[Pass]\n" );
    }
    else
    {
        kPrintf( "[Fail]\n" );
    }

    //==========================================================================
    // 파일 닫기 테스트
    //==========================================================================
    kPrintf( "9. File Close Test..." );
    // 파일이 정상적으로 닫혀야 함
    if( fclose( pstFile ) == 0 )
    {
        kPrintf( "[Pass]\n" );
    }
    else
    {
        kPrintf( "[Fail]\n" );
    }

    //==========================================================================
    // 파일 삭제 테스트
    //==========================================================================
    kPrintf( "10. File Delete Test..." );
    // 파일이 닫혔으므로 정상적으로 지워져야 함
    if( remove( "testfileio.bin" ) == 0 )
    {
        kPrintf( "[Pass]\n" );
    }
    else
    {
        kPrintf( "[Fail]\n" );
    }

    // 메모리를 해제
    kFreeMemory( pbBuffer );
}

