/*
 * Keyboard.c
 *
 *  Created on: 2015. 9. 21.
 *      Author: user
 */
#include "types.h"
#include "AssemblyUtility.h"
#include "Keyboard.h"
#include "Queue.h"
#include "Synchronization.h"

BOOL kIsOutputBufferFull(void) // 출력 버퍼에 내용이 있는가?
{
	if (kInPortByte(0x64) & 0x01) // 0x64 포트를 읽어서 0x01이 셋팅 되어 있으면 출력버퍼 안가져감
	{
		return TRUE;
	}
	return FALSE;
}

BOOL kIsInputBufferFull(void) // 입력 버퍼에 내용이 있는가?
{
	if (kInPortByte(0x64) & 0x02)
	{
		return TRUE;
	}
	return FALSE;
}

BOOL kWaitForACKAndPutOtherScanCode( void )
{
	int i, j;
	BYTE bData;
	BOOL bResult = FALSE;

	// 100번 꺼내본다. (이전에 들어온 키보드 신호때문에)
	for( j = 0 ; j < 100 ; j++ )
	{
		// 올때까지 0xFFFF번 실행
		for( i = 0 ; i < 0xFFFF ; i++ )
		{
			if( kIsOutputBufferFull() ) // outputbuffer에 ACK 신호가 있으면 데이터 읽어내야 함
			{
				break;
			}
		}
		bData = kInPortByte(0x60);

		if( bData == 0xFA )
		{
			bResult = TRUE;
			break;
		}
		else
		{
			kConvertScanCodeAndPutQueue( bData );
		}
	}
	return bResult;
}

BOOL kActivateKeyboard(void)
{
	int i;
	int j;
	BOOL bPreviousInterrupt;
	BOOL bResult;

	// 인터럽트 막아놓고 원래의 상태를 저장
	bPreviousInterrupt = kSetInterruptFlag( FALSE );

	kOutPortByte(0x64, 0xAE); // 0x64(컨트롤(쓰기)레지스터에 0xAE 신호 발송 (키보드 디바이스 활성화) )

	for( i = 0 ; i < 0xFFFF ; i++ )
	{
		if (!kIsInputBufferFull()) // inputbuffer가 비었으면 키보드 활성화(디바이스X) 신호 보낼것
		{
			break;
		}
	}

	kOutPortByte(0x60, 0xF4); // 0x60에 0xF4 전송 (입력버퍼로 키보드 활성화 키 전송)

	// ACK가 올때까지 기다림
	bResult = kWaitForACKAndPutOtherScanCode();

	// 이전 인터럽트 상태 복원
	kSetInterruptFlag(bPreviousInterrupt);

	return bResult;

}

BYTE kGetKeyboardScanCode( void )
{
	while( !kIsOutputBufferFull() )
	{
		// 출력 버퍼에 아무것도 없으면 무한루프
	}
	return kInPortByte(0x60);

}

void kEnableA20Gate( void )
{
	BYTE bOutputPortData;
	int i;

	// 키보드 컨트롤러 정보를 읽거나 쓰려면  미리 D0, D1 커맨드 신호를 보내야 한다.

	kOutPortByte( 0x64, 0xD0 ); // 키보드 컨트롤러의 출력 포트 값을 읽는 함수

	for( i = 0 ; i < 0xFFFF ; i++ )
	{
		if( kIsOutputBufferFull()) // 출력값이 있으면
		{
			break;
		}
	}

	bOutputPortData = kInPortByte(0x60); // 출력값을 읽어냄

	bOutputPortData |= 0x01; // 1인경우 A20Gate, 0인경우 리붓

	for( i = 0 ; i < 0xFFFF ; i++ )
	{
		if ( kIsInputBufferFull() == FALSE ) // 입력 버퍼에 데이터가 비어있으면
		{
			break;
		}
	}

	kOutPortByte(0x64, 0xD1); // 키보드 컨트롤러의 출력 포트 값을 보내는 커맨드
	kOutPortByte(0x60, bOutputPortData); // A20 Gate On
}

BOOL kChangeKeyboardLED( BOOL bCapsLockOn, BOOL bNumLockOn, BOOL bScrollLockOn )
{
	int i, j;
	BOOL bPreviousInterrupt;
	BOOL bResult;
	BYTE bData;

	// 인터럽트 불가
	bPreviousInterrupt = kSetInterruptFlag( FALSE );

	for( i = 0 ; i < 0xFFFF ; i++ )
	{
		if( kIsInputBufferFull() == FALSE ) // 입력버퍼가 빌때까지 기다림
		{
			break;
		}
	}

	// 출력 버퍼에 LED 상태 변경 한다는 커맨드 전송(0xED)
	kOutPortByte(0x60, 0xED);
	for( i = 0 ; i < 0xFFFF ; i++ )
	{
		if(kIsInputBufferFull() == FALSE) // 입력 버퍼가 빌때까지 기다림
			break;
	}

	// ACK를 기다림
	bResult = kWaitForACKAndPutOtherScanCode();

	if( bResult == FALSE )
	{
		kSetInterruptFlag( bPreviousInterrupt );
		return FALSE;
	}
	if ( j>= 100 ) // 일정 시간 이상 ACK가 안옴
		return FALSE;


	// 2비트 , 1비트, 0비트에 따라 온을 시켜줌
	kOutPortByte( 0x60, (bCapsLockOn << 2) | (bNumLockOn << 1) | bScrollLockOn );

	for( i = 0 ; i < 0xFFFF ; i++ )
	{
		if(kIsInputBufferFull() == FALSE)
		{
			break;
		}
	}

	bResult = kWaitForACKAndPutOtherScanCode();
	kSetInterruptFlag(bPreviousInterrupt);

	return bResult;
}

void kReboot( void )
{
	int i;

	for(i = 0 ; i < 0xFFFF ; i++ )
	{
		if( kIsInputBufferFull() == FALSE)
			break;
	}
	kOutPortByte(0x64, 0xD1); // 키보드 버퍼에 컨트롤 입력값을 입력할것이다.

	kOutPortByte(0x60, 0x00);
	while(1)
	{
		; // 재부팅
	}
}

// 스캔 코드를 ASCII 코드로 변환하는 기능
static KEYBOARDMANAGER gs_stKeyboardManager = {0,};

static QUEUE gs_stKeyQueue;
static KEYDATA gs_vstKeyQueueBuffer[ KEY_MAXQUEUECOUNT ];

static KEYMAPPINGENTRY gs_vstKeyMappingTable[ KEY_MAPPINGTABLEMAXCOUNT ] =
{
    /*  0   */  {   KEY_NONE        ,   KEY_NONE        },
    /*  1   */  {   KEY_ESC         ,   KEY_ESC         },
    /*  2   */  {   '1'             ,   '!'             },
    /*  3   */  {   '2'             ,   '@'             },
    /*  4   */  {   '3'             ,   '#'             },
    /*  5   */  {   '4'             ,   '$'             },
    /*  6   */  {   '5'             ,   '%'             },
    /*  7   */  {   '6'             ,   '^'             },
    /*  8   */  {   '7'             ,   '&'             },
    /*  9   */  {   '8'             ,   '*'             },
    /*  10  */  {   '9'             ,   '('             },
    /*  11  */  {   '0'             ,   ')'             },
    /*  12  */  {   '-'             ,   '_'             },
    /*  13  */  {   '='             ,   '+'             },
    /*  14  */  {   KEY_BACKSPACE   ,   KEY_BACKSPACE   },
    /*  15  */  {   KEY_TAB         ,   KEY_TAB         },
    /*  16  */  {   'q'             ,   'Q'             },
    /*  17  */  {   'w'             ,   'W'             },
    /*  18  */  {   'e'             ,   'E'             },
    /*  19  */  {   'r'             ,   'R'             },
    /*  20  */  {   't'             ,   'T'             },
    /*  21  */  {   'y'             ,   'Y'             },
    /*  22  */  {   'u'             ,   'U'             },
    /*  23  */  {   'i'             ,   'I'             },
    /*  24  */  {   'o'             ,   'O'             },
    /*  25  */  {   'p'             ,   'P'             },
    /*  26  */  {   '['             ,   '{'             },
    /*  27  */  {   ']'             ,   '}'             },
    /*  28  */  {   '\n'            ,   '\n'            },
    /*  29  */  {   KEY_CTRL        ,   KEY_CTRL        },
    /*  30  */  {   'a'             ,   'A'             },
    /*  31  */  {   's'             ,   'S'             },
    /*  32  */  {   'd'             ,   'D'             },
    /*  33  */  {   'f'             ,   'F'             },
    /*  34  */  {   'g'             ,   'G'             },
    /*  35  */  {   'h'             ,   'H'             },
    /*  36  */  {   'j'             ,   'J'             },
    /*  37  */  {   'k'             ,   'K'             },
    /*  38  */  {   'l'             ,   'L'             },
    /*  39  */  {   ';'             ,   ':'             },
    /*  40  */  {   '\''            ,   '\"'            },
    /*  41  */  {   '`'             ,   '~'             },
    /*  42  */  {   KEY_LSHIFT      ,   KEY_LSHIFT      },
    /*  43  */  {   '\\'            ,   '|'             },
    /*  44  */  {   'z'             ,   'Z'             },
    /*  45  */  {   'x'             ,   'X'             },
    /*  46  */  {   'c'             ,   'C'             },
    /*  47  */  {   'v'             ,   'V'             },
    /*  48  */  {   'b'             ,   'B'             },
    /*  49  */  {   'n'             ,   'N'             },
    /*  50  */  {   'm'             ,   'M'             },
    /*  51  */  {   ','             ,   '<'             },
    /*  52  */  {   '.'             ,   '>'             },
    /*  53  */  {   '/'             ,   '?'             },
    /*  54  */  {   KEY_RSHIFT      ,   KEY_RSHIFT      },
    /*  55  */  {   '*'             ,   '*'             },
    /*  56  */  {   KEY_LALT        ,   KEY_LALT        },
    /*  57  */  {   ' '             ,   ' '             },
    /*  58  */  {   KEY_CAPSLOCK    ,   KEY_CAPSLOCK    },
    /*  59  */  {   KEY_F1          ,   KEY_F1          },
    /*  60  */  {   KEY_F2          ,   KEY_F2          },
    /*  61  */  {   KEY_F3          ,   KEY_F3          },
    /*  62  */  {   KEY_F4          ,   KEY_F4          },
    /*  63  */  {   KEY_F5          ,   KEY_F5          },
    /*  64  */  {   KEY_F6          ,   KEY_F6          },
    /*  65  */  {   KEY_F7          ,   KEY_F7          },
    /*  66  */  {   KEY_F8          ,   KEY_F8          },
    /*  67  */  {   KEY_F9          ,   KEY_F9          },
    /*  68  */  {   KEY_F10         ,   KEY_F10         },
    /*  69  */  {   KEY_NUMLOCK     ,   KEY_NUMLOCK     },
    /*  70  */  {   KEY_SCROLLLOCK  ,   KEY_SCROLLLOCK  },

    /*  71  */  {   KEY_HOME        ,   '7'             },
    /*  72  */  {   KEY_UP          ,   '8'             },
    /*  73  */  {   KEY_PAGEUP      ,   '9'             },
    /*  74  */  {   '-'             ,   '-'             },
    /*  75  */  {   KEY_LEFT        ,   '4'             },
    /*  76  */  {   KEY_CENTER      ,   '5'             },
    /*  77  */  {   KEY_RIGHT       ,   '6'             },
    /*  78  */  {   '+'             ,   '+'             },
    /*  79  */  {   KEY_END         ,   '1'             },
    /*  80  */  {   KEY_DOWN        ,   '2'             },
    /*  81  */  {   KEY_PAGEDOWN    ,   '3'             },
    /*  82  */  {   KEY_INS         ,   '0'             },
    /*  83  */  {   KEY_DEL         ,   '.'             },
    /*  84  */  {   KEY_NONE        ,   KEY_NONE        },
    /*  85  */  {   KEY_NONE        ,   KEY_NONE        },
    /*  86  */  {   KEY_NONE        ,   KEY_NONE        },
    /*  87  */  {   KEY_F11         ,   KEY_F11         },
    /*  88  */  {   KEY_F12         ,   KEY_F12         }
};

BOOL kIsAlphabetScanCode( BYTE bScanCode )
{
	if ( ('a' <= gs_vstKeyMappingTable[ bScanCode ].bNormalCode) && (gs_vstKeyMappingTable[ bScanCode ].bNormalCode <= 'z'))
	{
		return TRUE;
	}
	return FALSE;
}

BOOL kIsNumberOrSymbolScanCode( BYTE bScanCode )
{
	// 숫자 U 심볼 U 알파벳 - 알파벳
	if( ( ( 2 <= bScanCode ) && (bScanCode <= 53)) && (kIsAlphabetScanCode( bScanCode ) == FALSE) )
	{
		return TRUE;
	}
	return FALSE;
}

BOOL kIsNumberPadScanCode( BYTE bScanCode )
{
	// 숫자 패드는 71 ~ 83 사이에 있음
	if( ( 71 <= bScanCode ) && ( bScanCode <= 83 ))
	{
		return TRUE;
	}
	return FALSE;
}

BOOL kIsUseCombinedCode( BYTE bScanCode )
{
	BYTE bDownScanCode;
	BOOL bUseCombinedKey = FALSE;

	bDownScanCode = bScanCode & 0x7F;

	if( kIsAlphabetScanCode(bDownScanCode) )
	{
		// shift나 capslock이 눌러져 있다면,
		if( gs_stKeyboardManager.bShiftDown ^ gs_stKeyboardManager.bCapsLockOn )
		{
			bUseCombinedKey = TRUE;
		}
		else
		{
			bUseCombinedKey = FALSE;
		}
	}
	//숫자나 기호는 쉬프트의 영향을 받음
	else if( kIsNumberOrSymbolScanCode( bDownScanCode ) )
	{
		if( gs_stKeyboardManager.bShiftDown == TRUE )
		{
			bUseCombinedKey = TRUE;
		}
		else
		{
			bUseCombinedKey = FALSE;
		}
	}
	// 숫자패드 키라면 Num Lock 키의 영향을 받음  즉, 일반 숫자패드 키더라도 NumLock이 되어있으면 일반 숫자키
	else if( ( kIsNumberPadScanCode(bDownScanCode) == TRUE ) && (gs_stKeyboardManager.bExtendedCodeIn == FALSE) )
	{
		if( gs_stKeyboardManager.bNumLockOn == TRUE )
		{
			bUseCombinedKey = TRUE;
		}
		else
		{
			bUseCombinedKey = FALSE;
		}
	}
	return bUseCombinedKey;
}

void UpdateCombinationKeyStatusAndLED( BYTE bScanCode )
{
	BOOL bDown;
	BYTE bDownScanCode;
	BOOL bLEDStatusChanged = FALSE;

	if( bScanCode & 0x80 ) // 만약 스캔코드 최상위 비트가 1이라면
	{
		bDown = FALSE;
		bDownScanCode = bScanCode & 0x7F;
	}
	else
	{
		bDown = TRUE;
		bDownScanCode = bScanCode;
	}

	// 조합키 검색
	// SHIFT가 눌렸다면
	if ( (bDownScanCode == 42 ) || ( bDownScanCode == 54 ) )
	{
		gs_stKeyboardManager.bShiftDown = bDown;
	}
	// CapsLock 키면 CapsLock 상태 갱신 및 LED 상태 변경
	else if ( (bDownScanCode == 58 ) && bDown == TRUE )
	{
		gs_stKeyboardManager.bCapsLockOn ^= TRUE;
		bLEDStatusChanged = TRUE;
	}
	// Num Lock이면
	else if ( (bDownScanCode == 69 ) && bDown == TRUE )
	{
		gs_stKeyboardManager.bNumLockOn ^= TRUE;
		bLEDStatusChanged = TRUE;
	}
	else if ( (bDownScanCode == 70 ) && bDown == TRUE )
	{
		gs_stKeyboardManager.bScrollLockOn ^= TRUE;
		bLEDStatusChanged = TRUE;
	}

	// LED 상태 변경 했으면 실제 변경함
	if ( bLEDStatusChanged == TRUE )
	{
		kChangeKeyboardLED( gs_stKeyboardManager.bCapsLockOn, gs_stKeyboardManager.bNumLockOn, gs_stKeyboardManager.bScrollLockOn );
	}

}

BOOL kConvertScanCodeToASCIICode( BYTE bScanCode, BYTE* pbASCIICode, BOOL* pbFlags )
{
	BOOL bUseCombinedKey;

	// 이전에 PAUSE키가 수신되었다면 PAUSE의 남은 스캔코드 무시
	if ( gs_stKeyboardManager.iSkipCountForPause > 0 )
	{
		gs_stKeyboardManager.iSkipCountForPause--;
		return FALSE;
	}

	// PAUSE키는 특별히 처리
	if (bScanCode == 0xE1)
	{
		*pbASCIICode = KEY_PAUSE;
		*pbFlags = KEY_FLAGS_DOWN;
		gs_stKeyboardManager.iSkipCountForPause = KEY_SKIPCOUNTFORPAUSE;
		return TRUE;
	}

	// 확장 키코드인경우 실제 키는 다음에 들어오므로 플래그 설정만 하고 종료
	if (bScanCode == 0xE0)
	{
		gs_stKeyboardManager.bExtendedCodeIn = TRUE;
		return TRUE;
	}

	// 앞서 어떤 키보드인경우 신호를 주는 루틴은 처리 완료, 앞으로는 키 반환 루틴
	bUseCombinedKey = kIsUseCombinedCode( bScanCode );

	// 조합키인경우
	if ( bUseCombinedKey == TRUE )
	{
		*pbASCIICode = gs_vstKeyMappingTable[ bScanCode & 0x7F ].bCombinedCode;
	}
	else
	{
		*pbASCIICode = gs_vstKeyMappingTable[ bScanCode & 0x7F ].bNormalCode;
	}

	// 확장 키 유무 설정
	if ( gs_stKeyboardManager.bExtendedCodeIn == TRUE )
	{
		*pbFlags = KEY_FLAGS_EXTENDEDKEY;
		gs_stKeyboardManager.bExtendedCodeIn = FALSE;
	}
	else
	{
		*pbFlags = 0;
	}

	if( (bScanCode & 0x80) == 0 )
	{
		*pbFlags |= KEY_FLAGS_DOWN;
	}
	UpdateCombinationKeyStatusAndLED( bScanCode );
	return TRUE;
}

BOOL kInitializeKeyboard( void )
{
	kInitializeQueue( &gs_stKeyQueue, gs_vstKeyQueueBuffer, KEY_MAXQUEUECOUNT, sizeof( KEYDATA ) );

	return kActivateKeyboard();

}

BOOL kConvertScanCodeAndPutQueue( BYTE bScanCode )
{
	KEYDATA stData;
	BOOL bResult;
	BOOL bPreviousFlag;

	stData.bScanCode = bScanCode;

	if( kConvertScanCodeToASCIICode( bScanCode, &( stData.bASCIICode ), &( stData.bFlags ) ) )
	{
		bPreviousFlag = kLockForSystemData();
		bResult = kPutQueue( &gs_stKeyQueue, &stData );
		kUnlockForSystemData( bPreviousFlag );
	}

	return bResult;
}

BOOL kGetKeyFromKeyQueue( KEYDATA* pstData )
{
	BOOL bResult;
	BOOL bPreviousFlag;

	if( kIsQueueEmpty( &gs_stKeyQueue ) == TRUE )
	{
		return FALSE;
	}

	bPreviousFlag = kLockForSystemData();

	bResult = kGetQueue( &gs_stKeyQueue, pstData );

	kUnlockForSystemData( bPreviousFlag );
	return bResult;
}
