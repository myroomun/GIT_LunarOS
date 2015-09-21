[BITS 32]

; C 함수에서 알아먹게 글로벌로 함수 이름을 설정 (EXPORT)
global kReadCPUID, kSwitchAndExecute64bitKernel

SECTION .text		;text 세그먼트

; kReadCPUID(DWORD eax, DWORD* eax,ebx,ecx,edx)
kReadCPUID:
	push ebp
	mov ebp, esp		; ebp : ebp, ebp + 4 : ret ebp + 8 : eax ...
	; register backup
	push eax
	push esi
	push ebx
	push ecx
	push edx
	; EAX 레지스터 값으로 CPUID 명령어 실행
	mov eax, dword[ ebp + 8 ]
	cpuid

	; 반환된 값을 파라미터에 저장
	; *eax
	mov esi, dword[ ebp + 12 ]
	mov dword[esi], eax

	; *ebx
	mov esi, dword[ ebp + 16 ]
	mov dword[esi], ebx

	; *ecx
	mov esi, dword[ ebp + 20 ]
	mov dword[esi], ecx

	; *edx
	mov esi, dword[ ebp + 24 ]
	mov dword[esi],edx

	;pop backup
	pop edx
	pop ecx
	pop ebx
	pop esi
	pop eax
	pop ebp
	ret


; kSwitchExecute64bitKernel(void)
kSwitchAndExecute64bitKernel:
	; cr4 의 PAE 를 1로 설정
	mov eax, cr4
	or eax, 0x20
	mov cr4, eax

	; cr3에 PML4 넣기
	mov eax, 0x100000 ; PML 위치를 eax에 저장
	mov cr3, eax

	; IA32_EFER.LME(확장 레지스터)의 LME를 1로 설정하여 64비트 활성화
	mov ecx, 0xC0000080	;msr address(제조사 specific 주소)
	rdmsr	; msr read

	or eax, 0x100	; LME bit 1로 셋
	wrmsr	; msr write

	; CR0 레지스터 :: 캐시 기능과 페이징 기능 활성화
	; NW(Non-writethrough:29bit): disable(0) CD(Cache Disable:30bit) : disable(0) PG(Paging Enable:31bit) : enable(1)
	mov eax, cr0
	or eax, 0x80000000
	xor eax, 0x60000000
	mov cr0, eax

	jmp 0x08:0x200000	; 2MB  어드레스로 이동

	jmp $
