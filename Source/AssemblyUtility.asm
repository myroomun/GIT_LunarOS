[BITS 64]

SECTION .text

global kInPortByte, kOutPortByte, kInPortWord, kOutPortWord
global kLoadGDTR, kLoadTR, kLoadIDTR
global kEnableInterrupt, kDisableInterrupt, kReadRFLAGS
global kReadTSC
global kSwitchContext, kHlt, kTestAndSet
global kInitializeFPU, kSaveFPUContext, kLoadFPUContext, kSetTS, kClearTS

; param : port
; ret : data (in rax)
kInPortByte:
	push rdx
	mov rdx, rdi
	mov rax, 0
	in al, dx

	pop rdx
	ret

; param port, data
; non ret
; Left -> Right : rdi, rsi, rdx, rcx ...
kOutPortByte:
	push rdx
	push rax

	mov rdx, rdi
	mov rax, rsi
	out dx, al

	pop rax
	pop rdx

	ret

; GDTR�������Ϳ� gdt �ε�
; param : gdtr �ڷᱸ�� ��巹��(edi)
kLoadGDTR:
	lgdt	[rdi]
	ret

; TR �������Ϳ� TSS ���׸�Ʈ ��ũ���� ����
; PARAM : TSS ���׸�Ʈ ��ũ������ ������
kLoadTR:
	ltr		di
	ret

; IDTR �������Ϳ� IDT ���̺��� ����
; PARAM : IDT ���̺��� ������ �����ϴ� �ڷᱸ�� ��巹��
kLoadIDTR:
	lidt	[rdi]
	ret

; ���ͷ�Ʈ Ȱ��ȭ
; PARAM : NONE
kEnableInterrupt:
	sti
	ret

; ���ͷ�Ʈ ��Ȱ��ȭ
; PARAM : NONE
kDisableInterrupt:
	cli
	ret

; RFLAGS �������� ����
kReadRFLAGS:
	pushfq
	pop rax
	ret

; Ÿ�ӽ����� ī���͸� �о ��ȯ (RDX <= 32��Ʈ / RAX <= 32��Ʈ)
kReadTSC:
	push rdx
	rdtsc
	shl rdx, 32
	or rax, rdx
	pop rdx
	ret

; �½�ũ ���� ����� �Լ�
%macro KSAVECONTEXT 0	;�Ķ���͸� ���� �ʴ� ��ũ�� ����
	; RBP �������ͺ��� GS ���׸�Ʈ �����ͱ��� ��� ���ÿ� ����
	push rbp
	push rax
	push rbx
	push rcx
	push rdx
	push rdi
	push rsi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	; DS�� AX�� push ���� �Ұ�
	mov ax, ds
	push rax
	mov ax, es
	push rax
	push fs
	push gs

%endmacro	; ��ũ�� ��

; ���ý�Ʈ ���� ��ũ��
%macro KLOADCONTEXT 0
	pop gs
	pop fs
	pop rax
	mov es, ax
	pop rax
	mov ds, ax

	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rsi
	pop rdi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	pop rbp
%endmacro

;; ���� ���ؽ�Ʈ ���� �� �ؽ�Ʈ �ý�Ʈ ����
;; param : current context, next context
kSwitchContext:
	push rbp
	mov rbp, rsp	; rbp: pushed rbp, rbp + 8 : retn addr

	;; current context�� null �̸� ���ؽ�Ʈ ���� ���ʿ�
	pushfq	; cmp ����� flag �����ϴ°� ����
	cmp rdi, 0
	je LoadContext
	popfq

	; ���� ���ؽ�Ʈ ����
	push rax		; ����� rax ����

	;; ss, rsp, rflags, cs, rip �������� ������ ����
	;; ss ����
	mov ax, ss
	mov qword[ rdi + (23*8) ], rax
	;; rsp ����
	;; rsp�� rbp�� ����Ǿ� �ִµ� �Լ��� ������ retn addr push �� push rbp ������ 16�� ���ҵ�. �� 16�� ������Ŵ
	mov rax, rbp
	add rax, 16		; mov rbp, rsp �ñ⿡���� rsp + 16
	mov qword[ rdi + (22*8) ], rax

	;; flags ����
	pushfq
	pop rax
	mov qword[ rdi + (21*8) ], rax

	;; cs �������� ����
	mov ax, cs
	mov qword[ rdi + (20*8) ], rax

	;; return address ������ �ؾ� �ϴµ� ����� retn �ּҰ� ���ؽ�Ʈ ���� �� ����� ��ġ�� ���Ѵ�.
	mov rax, qword[ rbp + 8 ]
	mov qword[ rdi + (19*8) ], rax

	; ������ �������͸� ����
	pop rax
	pop rbp ; ���� ������ ���� ���� rbp ����

	;���� �Ķ���� ���� �������� �迭�� �����ϱ� ���� rsp ����
	add rdi, (19*8)		; stContext ���� �������� 24�� ���� ���� 5���� �̹� ä��
	mov rsp, rdi
	sub rdi, (19*8)

	; ������ �������͸� ��� CONTEXT �ڷᱸ���� ����
	KSAVECONTEXT

	;; ���� �½�ũ�� ���ؽ�Ʈ ����
LoadContext:
	mov rsp, rsi		;rsp�� rsi�� ���� ���ؽ�Ʈ �ڷᱸ�� ���� (NextContext)

	; context �ڷᱸ������ �������͸� ����
	KLOADCONTEXT
	iretq				;�� stack���� ������ ���� �ڷ�� RIP, CS, RFLAGS, RSP, SS �ε� �̴� iretq �� ���� ���ڸ��� ����.

;; ���μ����� ���� ��
kHlt:
	hlt
	hlt
	ret

;; �Ķ���� : rdi : destination, rsi compare, rdx source
kTestAndSet:
	mov rax, rsi

	;; rax�� [rdi]�� ���Ͽ� ������ dl���� rdi(dest)�� ����
	lock cmpxchg byte[ rdi ], dl
	je SUCCESS

NOTSAME:
	mov rax, 0x00
	ret
SUCCESS:
	mov rax, 0x01
	ret

;; FPU ���� ����� �Լ�

kInitializeFPU:
	finit
	ret

kSaveFPUContext:
	fxsave [rdi]
	ret

kLoadFPUContext:
	fxrstor [rdi]
	ret

kSetTS:
	push rax
	mov rax, cr0
	or rax, 0x08 ;; TS bit 1 ����
	mov cr0, rax
	pop rax
	ret

kClearTS:
	clts
	ret

;; �Ķ���� : ��Ʈ��ȣ
kInPortWord:
	push rdx
	mov rdx, rdi	;; ��Ʈ��ȣ �̵�
	mov rax, 0		;; rax �������� �ʱ�ȭ
	in ax, dx

	pop rdx
	ret
;; �Ķ���� : ��Ʈ, ������
kOutPortWord:
	push rdx
	push rax

	mov rdx, rdi
	mov rax, rsi
	out dx, ax ;; ax -> dx

	pop rax
	pop rdx
	ret
