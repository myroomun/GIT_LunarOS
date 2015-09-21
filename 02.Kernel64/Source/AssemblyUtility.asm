[BITS 64]

SECTION .text

global kInPortByte, kOutPortByte, kLoadGDTR, kLoadTR, kLoadIDTR

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
