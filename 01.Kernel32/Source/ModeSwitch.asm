[BITS 32]

; C �Լ����� �˾Ƹ԰� �۷ι��� �Լ� �̸��� ���� (EXPORT)
global kReadCPUID, kSwitchAndExecute64bitKernel

SECTION .text		;text ���׸�Ʈ

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
	; EAX �������� ������ CPUID ��ɾ� ����
	mov eax, dword[ ebp + 8 ]
	cpuid

	; ��ȯ�� ���� �Ķ���Ϳ� ����
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
	; cr4 �� PAE �� 1�� ����
	mov eax, cr4
	or eax, 0x20
	mov cr4, eax

	; cr3�� PML4 �ֱ�
	mov eax, 0x100000 ; PML ��ġ�� eax�� ����
	mov cr3, eax

	; IA32_EFER.LME(Ȯ�� ��������)�� LME�� 1�� �����Ͽ� 64��Ʈ Ȱ��ȭ
	mov ecx, 0xC0000080	;msr address(������ specific �ּ�)
	rdmsr	; msr read

	or eax, 0x100	; LME bit 1�� ��
	wrmsr	; msr write

	; CR0 �������� :: ĳ�� ��ɰ� ����¡ ��� Ȱ��ȭ
	; NW(Non-writethrough:29bit): disable(0) CD(Cache Disable:30bit) : disable(0) PG(Paging Enable:31bit) : enable(1)
	mov eax, cr0
	or eax, 0x80000000
	xor eax, 0x60000000
	mov cr0, eax

	jmp 0x08:0x200000	; 2MB  ��巹���� �̵�

	jmp $
