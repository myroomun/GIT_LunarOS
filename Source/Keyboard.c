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

BOOL kIsOutputBufferFull(void) // ��� ���ۿ� ������ �ִ°�?
{
	if (kInPortByte(0x64) & 0x01) // 0x64 ��Ʈ�� �о 0x01�� ���� �Ǿ� ������ ��¹��� �Ȱ�����
	{
		return TRUE;
	}
	return FALSE;
}

BOOL kIsInputBufferFull(void) // �Է� ���ۿ� ������ �ִ°�?
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

	// 100�� ��������. (������ ���� Ű���� ��ȣ������)
	for( j = 0 ; j < 100 ; j++ )
	{
		// �ö����� 0xFFFF�� ����
		for( i = 0 ; i < 0xFFFF ; i++ )
		{
			if( kIsOutputBufferFull() ) // outputbuffer�� ACK ��ȣ�� ������ ������ �о�� ��
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

	// ���ͷ�Ʈ ���Ƴ��� ������ ���¸� ����
	bPreviousInterrupt = kSetInterruptFlag( FALSE );

	kOutPortByte(0x64, 0xAE); // 0x64(��Ʈ��(����)�������Ϳ� 0xAE ��ȣ �߼� (Ű���� ����̽� Ȱ��ȭ) )

	for( i = 0 ; i < 0xFFFF ; i++ )
	{
		if (!kIsInputBufferFull()) // inputbuffer�� ������� Ű���� Ȱ��ȭ(����̽�X) ��ȣ ������
		{
			break;
		}
	}

	kOutPortByte(0x60, 0xF4); // 0x60�� 0xF4 ���� (�Է¹��۷� Ű���� Ȱ��ȭ Ű ����)

	// ACK�� �ö����� ��ٸ�
	bResult = kWaitForACKAndPutOtherScanCode();

	// ���� ���ͷ�Ʈ ���� ����
	kSetInterruptFlag(bPreviousInterrupt);

	return bResult;

}

BYTE kGetKeyboardScanCode( void )
{
	while( !kIsOutputBufferFull() )
	{
		// ��� ���ۿ� �ƹ��͵� ������ ���ѷ���
	}
	return kInPortByte(0x60);

}

void kEnableA20Gate( void )
{
	BYTE bOutputPortData;
	int i;

	// Ű���� ��Ʈ�ѷ� ������ �аų� ������  �̸� D0, D1 Ŀ�ǵ� ��ȣ�� ������ �Ѵ�.

	kOutPortByte( 0x64, 0xD0 ); // Ű���� ��Ʈ�ѷ��� ��� ��Ʈ ���� �д� �Լ�

	for( i = 0 ; i < 0xFFFF ; i++ )
	{
		if( kIsOutputBufferFull()) // ��°��� ������
		{
			break;
		}
	}

	bOutputPortData = kInPortByte(0x60); // ��°��� �о

	bOutputPortData |= 0x01; // 1�ΰ�� A20Gate, 0�ΰ�� ����

	for( i = 0 ; i < 0xFFFF ; i++ )
	{
		if ( kIsInputBufferFull() == FALSE ) // �Է� ���ۿ� �����Ͱ� ���������
		{
			break;
		}
	}

	kOutPortByte(0x64, 0xD1); // Ű���� ��Ʈ�ѷ��� ��� ��Ʈ ���� ������ Ŀ�ǵ�
	kOutPortByte(0x60, bOutputPortData); // A20 Gate On
}

BOOL kChangeKeyboardLED( BOOL bCapsLockOn, BOOL bNumLockOn, BOOL bScrollLockOn )
{
	int i, j;
	BOOL bPreviousInterrupt;
	BOOL bResult;
	BYTE bData;

	// ���ͷ�Ʈ �Ұ�
	bPreviousInterrupt = kSetInterruptFlag( FALSE );

	for( i = 0 ; i < 0xFFFF ; i++ )
	{
		if( kIsInputBufferFull() == FALSE ) // �Է¹��۰� �������� ��ٸ�
		{
			break;
		}
	}

	// ��� ���ۿ� LED ���� ���� �Ѵٴ� Ŀ�ǵ� ����(0xED)
	kOutPortByte(0x60, 0xED);
	for( i = 0 ; i < 0xFFFF ; i++ )
	{
		if(kIsInputBufferFull() == FALSE) // �Է� ���۰� �������� ��ٸ�
			break;
	}

	// ACK�� ��ٸ�
	bResult = kWaitForACKAndPutOtherScanCode();

	if( bResult == FALSE )
	{
		kSetInterruptFlag( bPreviousInterrupt );
		return FALSE;
	}
	if ( j>= 100 ) // ���� �ð� �̻� ACK�� �ȿ�
		return FALSE;


	// 2��Ʈ , 1��Ʈ, 0��Ʈ�� ���� ���� ������
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
	kOutPortByte(0x64, 0xD1); // Ű���� ���ۿ� ��Ʈ�� �Է°��� �Է��Ұ��̴�.

	kOutPortByte(0x60, 0x00);
	while(1)
	{
		; // �����
	}
}

// ��ĵ �ڵ带 ASCII �ڵ�� ��ȯ�ϴ� ���
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
	// ���� U �ɺ� U ���ĺ� - ���ĺ�
	if( ( ( 2 <= bScanCode ) && (bScanCode <= 53)) && (kIsAlphabetScanCode( bScanCode ) == FALSE) )
	{
		return TRUE;
	}
	return FALSE;
}

BOOL kIsNumberPadScanCode( BYTE bScanCode )
{
	// ���� �е�� 71 ~ 83 ���̿� ����
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
		// shift�� capslock�� ������ �ִٸ�,
		if( gs_stKeyboardManager.bShiftDown ^ gs_stKeyboardManager.bCapsLockOn )
		{
			bUseCombinedKey = TRUE;
		}
		else
		{
			bUseCombinedKey = FALSE;
		}
	}
	//���ڳ� ��ȣ�� ����Ʈ�� ������ ����
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
	// �����е� Ű��� Num Lock Ű�� ������ ����  ��, �Ϲ� �����е� Ű���� NumLock�� �Ǿ������� �Ϲ� ����Ű
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

	if( bScanCode & 0x80 ) // ���� ��ĵ�ڵ� �ֻ��� ��Ʈ�� 1�̶��
	{
		bDown = FALSE;
		bDownScanCode = bScanCode & 0x7F;
	}
	else
	{
		bDown = TRUE;
		bDownScanCode = bScanCode;
	}

	// ����Ű �˻�
	// SHIFT�� ���ȴٸ�
	if ( (bDownScanCode == 42 ) || ( bDownScanCode == 54 ) )
	{
		gs_stKeyboardManager.bShiftDown = bDown;
	}
	// CapsLock Ű�� CapsLock ���� ���� �� LED ���� ����
	else if ( (bDownScanCode == 58 ) && bDown == TRUE )
	{
		gs_stKeyboardManager.bCapsLockOn ^= TRUE;
		bLEDStatusChanged = TRUE;
	}
	// Num Lock�̸�
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

	// LED ���� ���� ������ ���� ������
	if ( bLEDStatusChanged == TRUE )
	{
		kChangeKeyboardLED( gs_stKeyboardManager.bCapsLockOn, gs_stKeyboardManager.bNumLockOn, gs_stKeyboardManager.bScrollLockOn );
	}

}

BOOL kConvertScanCodeToASCIICode( BYTE bScanCode, BYTE* pbASCIICode, BOOL* pbFlags )
{
	BOOL bUseCombinedKey;

	// ������ PAUSEŰ�� ���ŵǾ��ٸ� PAUSE�� ���� ��ĵ�ڵ� ����
	if ( gs_stKeyboardManager.iSkipCountForPause > 0 )
	{
		gs_stKeyboardManager.iSkipCountForPause--;
		return FALSE;
	}

	// PAUSEŰ�� Ư���� ó��
	if (bScanCode == 0xE1)
	{
		*pbASCIICode = KEY_PAUSE;
		*pbFlags = KEY_FLAGS_DOWN;
		gs_stKeyboardManager.iSkipCountForPause = KEY_SKIPCOUNTFORPAUSE;
		return TRUE;
	}

	// Ȯ�� Ű�ڵ��ΰ�� ���� Ű�� ������ �����Ƿ� �÷��� ������ �ϰ� ����
	if (bScanCode == 0xE0)
	{
		gs_stKeyboardManager.bExtendedCodeIn = TRUE;
		return TRUE;
	}

	// �ռ� � Ű�����ΰ�� ��ȣ�� �ִ� ��ƾ�� ó�� �Ϸ�, �����δ� Ű ��ȯ ��ƾ
	bUseCombinedKey = kIsUseCombinedCode( bScanCode );

	// ����Ű�ΰ��
	if ( bUseCombinedKey == TRUE )
	{
		*pbASCIICode = gs_vstKeyMappingTable[ bScanCode & 0x7F ].bCombinedCode;
	}
	else
	{
		*pbASCIICode = gs_vstKeyMappingTable[ bScanCode & 0x7F ].bNormalCode;
	}

	// Ȯ�� Ű ���� ����
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
