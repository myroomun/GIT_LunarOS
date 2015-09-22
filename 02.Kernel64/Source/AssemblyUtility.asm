[BITS 64]

SECTION .text

global kInPortByte, kOutPortByte, kLoadGDTR, kLoadTR, kLoadIDTR
global kEnableInterrupt, kDisableInterrupt, kReadRFLAGS

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

; GDTR레지스터에 gdt 로드
; param : gdtr 자료구조 어드레스(edi)
kLoadGDTR:
	lgdt	[rdi]
	ret

; TR 레지스터에 TSS 세그먼트 디스크립터 설정
; PARAM : TSS 세그먼트 디스크립터의 오프셋
kLoadTR:
	ltr		di
	ret

; IDTR 레지스터에 IDT 테이블을 설정
; PARAM : IDT 테이블의 정보를 저장하는 자료구조 어드레스
kLoadIDTR:
	lidt	[rdi]
	ret

; 인터럽트 활성화
; PARAM : NONE
kEnableInterrupt:
	sti
	ret

; 인터럽트 비활성화
; PARAM : NONE
kDisableInterrupt:
	cli
	ret

; RFLAGS 레지스터 리드
kReadRFLAGS:
	pushfq
	pop rax
	ret
