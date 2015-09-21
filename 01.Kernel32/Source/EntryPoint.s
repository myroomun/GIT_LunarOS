[ORG 0x00]
[BITS 16]

SECTION	.text

START:
	mov ax, 0x1000		; Current code is at 0x1000
	mov ds, ax
	mov es, ax
	;;A20 게이트 활성화 시도
	mov ax, 0x2401
	int 0x15			; BIOS call
	jc A20GATEERROR
	jmp A20GATESUCCESS
A20GATEERROR:
	in al, 0x92			; read 0x92 port (system control port)
	or al, 0x02			; 2번째 비트 1로 활성화
	and al, 0xFE		; 1번째 비트는 무조건 0 : 리붓 방지 * QEMU에서는 빠른 리붓 기능이 비활성화 되어있음
	out 0x92, al
A20GATESUCCESS:
	;;////////////////
	cli					; interrupt disable until IA-32 interrupt handler is set
	lgdt [GDTR]			; 32비트 크기의 구조체를 로드함 (GDT size = 2^16 그리고 하나의 디스크립터의 주소사이즈는 8(64bit % 8bit) => 8192개, GDT Address)
	mov eax, 0x4000003B	; 프로텍티드 모드, 캐쉬 비설정, 페이징 비설정, FPU 사용가능
	mov cr0, eax
	;; 16bits => 32bits 될때 파이프라이닝 플러쉬 필요 없나?
	jmp dword 0x18:(PROTECTEDMODE - $$ + 0x10000 )	; GDP + 0x18 : Protected Code Descriptor Setting in CS

; 보호모드 진입

[BITS 32]

PROTECTEDMODE:
	mov ax, 0x20	; GDP + 0x10 : Protected Data Descriptor
	mov ds, ax
	mov es, ax
	mov fs, ax		; fs, gs register는 사용용도가 안정해진 레지스터이다. (범용 디스크립터 저장)
	mov gs, ax

	mov ss, ax		; stack descriptor 범위는 0 ~ 4GB
	mov esp, 0xFFFE
	mov ebp, 0xFFFE

	push ( SWITCHSUCCESSMESSAGE - $$ + 0x10000 )
	push 2
	push 0
	call PRINTMESSAGE
	add esp, 12

	jmp dword 0x18: 0x10200 ; jump to C kernel at 0x10200

; 함수

PRINTMESSAGE:
	push ebp
	mov ebp, esp
	; ebp : esp    ebp + 4 : ret addr ebp + 8 :x ebp + 12 : y ebp + 16 string
	push eax
	push esi
	push edi
	push ecx

	mov eax, dword[ebp + 12]	;y 좌표를 꺼내 160을 곱하여 y좌표 설정
	mov esi, 160
	mul esi
	mov edi, eax				;edi는 0xb8000 + edi 로써 들어갈 오프셋

	mov eax, dword[ebp + 8]		;x 좌표를 꺼내 2를 곱해서 x좌표 설정
	mov esi, 2
	mul esi
	add edi, eax

	mov esi, [ebp + 16]

MESSAGELOOP:
	mov cl, byte[esi]
	cmp cl, 0
	je MESSAGEEND

	mov byte[edi + 0xB8000], cl

	add edi,2
	add esi,1
	jmp MESSAGELOOP

MESSAGEEND:
	pop ecx
	pop edi
	pop esi
	pop eax
	pop ebp
	ret

;DATA
align 8, db 0
dw 0x0000 ; 4bytes?
GDTR:
	dw GDTEND - GDT - 1
	dd (GDT - $$ + 0x10000)
GDT:
	NULLDescriptor: ; 0x00
		dw 0x0000
		dw 0x0000
		db 0x00
		db 0x00
		db 0x00
		db 0x00
	IA_32eCODEDESCRIPTOR: ;0x08
		dw 0xFFFF	;Limit
		dw 0x0000	;Base
		db 0x00		;Base
		db 0x9A		;P = 1(valid?) DPL = 0(요구권한), Code Segment(in Type), Excecute/Read
		db 0xAF		;G = 1(Limit * 4kb?), D = 0(32비트 세그먼트 Disable), L = 1 (64bit),Limit ( D = 1 L = 1 은 이미 예약중 )
		db 0x00
	IA_32eDATADESCRIPTOR: ; 0x10
		dw 0xFFFF	;Limit
		dw 0x0000	;Base
		db 0x00		;Base
		db 0x92		;P = 1(valid?) DPL = 0(요구권한), DATA Segment(in Type), Read/Write
		db 0xAF		;G = 1(Limit * 4kb?), D = 0(32비트 세그먼트 Disable), L = 1 (64bit),Limit ( D = 1 L = 1 은 이미 예약중 )
		db 0x00
	CODEDESCRIPTOR: ;0x18
		dw 0xFFFF	;Limit
		dw 0x0000	;Base
		db 0x00		;Base
		db 0x9A		;P = 1(valid?) DPL = 0(요구권한), Code Segment(in Type), Excecute/Read
		db 0xCF		;G = 1(Limit * 4kb?), D = 1(32비트 세그먼트임), L = 0 (No 64bit),Limit
		db 0x00
	DATADESCRIPTOR:
		dw 0xFFFF
		dw 0x0000
		db 0x00
		db 0x92
		db 0xCF
		db 0x00
GDTEND:

SWITCHSUCCESSMESSAGE:	db		'Switch To Protected Mode Success',0

times	512 - ($ - $$) db 0x00
