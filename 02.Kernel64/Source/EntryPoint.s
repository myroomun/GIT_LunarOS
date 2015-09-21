[BITS 64]

SECTION .text

extern Main

START:
	; segment 초기화 (0x10 = 64bit data descriptor)
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	; Stack 위치 0x600000 ~ 0x6FFFFF 1MB Size로 생성
	mov rsp, 0x6FFFF8
	mov rbp, 0x6FFFF8

	Call Main

	jmp $
