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

; 타임스탬프 카운터를 읽어서 반환 (RDX <= 32비트 / RAX <= 32비트)
kReadTSC:
	push rdx
	rdtsc
	shl rdx, 32
	or rax, rdx
	pop rdx
	ret

; 태스크 관련 어셈블리 함수
%macro KSAVECONTEXT 0	;파라미터를 받지 않는 매크로 정의
	; RBP 레지스터부터 GS 세그먼트 셀렉터까지 모두 스택에 삽입
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

	; DS와 AX는 push 접근 불가
	mov ax, ds
	push rax
	mov ax, es
	push rax
	push fs
	push gs

%endmacro	; 매크로 끝

; 콘택스트 복원 매크로
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

;; 현재 컨텍스트 저장 및 넥스트 택스트 복구
;; param : current context, next context
kSwitchContext:
	push rbp
	mov rbp, rsp	; rbp: pushed rbp, rbp + 8 : retn addr

	;; current context가 null 이면 컨텍스트 저장 불필요
	pushfq	; cmp 결과로 flag 변경하는것 막기
	cmp rdi, 0
	je LoadContext
	popfq

	; 현재 컨텍스트 저장
	push rax		; 사용할 rax 저장

	;; ss, rsp, rflags, cs, rip 레지스터 순으로 저장
	;; ss 저장
	mov ax, ss
	mov qword[ rdi + (23*8) ], rax
	;; rsp 저장
	;; rsp는 rbp에 저장되어 있는데 함수에 들어오고 retn addr push 및 push rbp 때문에 16이 감소됨. 즉 16을 증가시킴
	mov rax, rbp
	add rax, 16		; mov rbp, rsp 시기에서의 rsp + 16
	mov qword[ rdi + (22*8) ], rax

	;; flags 저장
	pushfq
	pop rax
	mov qword[ rdi + (21*8) ], rax

	;; cs 레지스터 저장
	mov ax, cs
	mov qword[ rdi + (20*8) ], rax

	;; return address 저장을 해야 하는데 저장된 retn 주소가 컨텍스트 복구 후 수행될 위치를 뜻한다.
	mov rax, qword[ rbp + 8 ]
	mov qword[ rdi + (19*8) ], rax

	; 저장한 레지스터를 복구
	pop rax
	pop rbp ; 스택 프레임 형성 전의 rbp 복구

	;들어온 파라미터 안의 레지스터 배열에 저장하기 위해 rsp 변경
	add rdi, (19*8)		; stContext 내부 레지스터 24개 에서 상위 5개를 이미 채움
	mov rsp, rdi
	sub rdi, (19*8)

	; 나머지 레지스터를 모두 CONTEXT 자료구조에 저장
	KSAVECONTEXT

	;; 다음 태스크의 콘텍스트 복원
LoadContext:
	mov rsp, rsi		;rsp에 rsi로 들어온 콘텍스트 자료구조 저장 (NextContext)

	; context 자료구조에서 레지스터를 복원
	KLOADCONTEXT
	iretq				;현 stack에서 나머지 남은 자료는 RIP, CS, RFLAGS, RSP, SS 인데 이는 iretq 에 의해 제자리로 들어간다.

;; 프로세서를 쉬게 함
kHlt:
	hlt
	hlt
	ret

;; 파라미터 : rdi : destination, rsi compare, rdx source
kTestAndSet:
	mov rax, rsi

	;; rax와 [rdi]를 비교하여 같으면 dl값을 rdi(dest)에 저장
	lock cmpxchg byte[ rdi ], dl
	je SUCCESS

NOTSAME:
	mov rax, 0x00
	ret
SUCCESS:
	mov rax, 0x01
	ret

;; FPU 관련 어셈블리 함수

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
	or rax, 0x08 ;; TS bit 1 설정
	mov cr0, rax
	pop rax
	ret

kClearTS:
	clts
	ret

;; 파라미터 : 포트번호
kInPortWord:
	push rdx
	mov rdx, rdi	;; 포트번호 이동
	mov rax, 0		;; rax 레지스터 초기화
	in ax, dx

	pop rdx
	ret
;; 파라미터 : 포트, 데이터
kOutPortWord:
	push rdx
	push rax

	mov rdx, rdi
	mov rax, rsi
	out dx, ax ;; ax -> dx

	pop rax
	pop rdx
	ret
