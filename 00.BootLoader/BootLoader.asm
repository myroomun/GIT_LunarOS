[ORG 0x00]	;�ڵ��� ���� ��巹�� : 0x00 ���� (�� �ڵ� ��ü�� ������ ���� ��巹��)
[BITS 16]	;�Ʒ��� �ڵ�� 16��Ʈ ȣȯ �ڵ�

Section	.text	;text segment ����

jmp 0x07C0:START
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	�ڵ� ���� (DS ����, ���� �޸� ����, ���� ���� ����)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
START:
	mov	ax, 0x07C0	;DS Segment is in 0x7C0
	mov	ds,	ax

	mov	ax, 0xB800	;Print Memroy (Directed mapped)
	mov	es, ax
	mov	ax, 0x0000
	mov	ss, ax		;StackSegment start addr : 0
	mov	sp, 0xFFFE	;after 0x10000 OS image will be loaded
	mov	bp, 0xFFFE

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	ȭ���� �����, �Ӽ����� ������� ����
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov	si, 0
SCREENCLEARLOOP:	;Clear screen from 0 to 80*25*2
					;All character consists of 2 bytes Low bit is for character(ascii), High bit is for color
	mov byte[es:si], 0
	mov byte[es:si+1], 0x0A
	add si,2
	cmp si, 80*25*2
	jl	SCREENCLEARLOOP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	ȭ�� ��ܿ� �޽��� ��� PRINTMESSAGE(iX,iY,String)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	push MESSAGE1
	push 0
	push 0
	call PRINTMESSAGE ; __cdcel protocol
	add	sp, 6
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	ȭ�� ��ܿ� OS�ε� �޽���
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	push IMAGELOADINGMESSAGE
	push 1
	push 0
	call PRINTMESSAGE ; __cdcel protocol
	add	sp, 6

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	��ũ���� OS�̹����� �ε�
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	��ũ�� �б��� ���� �ʿ�
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
RESETDISK:
	; Service number : 0, Drive Number : 0
	mov ax, 0
	mov dl, 0
	int 0x13
	jc HANDLEDISKERROR	; if error
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	��ũ�� ����
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; setting for destination address after disk reading!
	mov	si,	0x1000
	mov es, si
	mov bx, 0x0
	mov di, word[TOTALSECTORCOUNT]

READDATA:
	cmp	di, 0
	je	READEND		;if di(totalsectorcount is 0, all sectors are read)
	sub di, 1
	;Calling BIOS Function
	mov	ah, 0x02	;BIOS service number 2 => read sector
	mov	al, 0x1		;# of sector to read => 1
	mov	ch, byte[TRACKNUMBER]	;# of track to read
	mov	cl, byte[SECTORNUMBER]	;# of sector to read
	mov dh, byte[HEADNUMBER]	;# of head to read
	mov	dl, 0x00				;where to read (0 => Floppy)
	int 0x13					;BIOS call
	jc	HANDLEDISKERROR			;if error, jmp to HANDLEDISKERROR
	;After read, adjust the numbers of track, sector, head
	add	si, 0x20				;modify address after 512 bytes (for 1 sector => 512 bytes)
	mov	es, si

	;; The final sector is 18 and change head to 0->1
	mov	al, byte[SECTORNUMBER]
	add	al, 0x01
	mov	byte[SECTORNUMBER], al
	cmp	al, 19
	jl	READDATA

	;; change head (sector 18 was read)
	xor	byte[HEADNUMBER], 0x01	;Toggle by xor 0=>1=>0=>1
	mov	byte[SECTORNUMBER], 0x01	;EXCEPT MBR, SECTORNUMBER should start from 1

	cmp	byte[HEADNUMBER], 0x00	; if after changing head
	jne	READDATA

	; increase track 1
	add	byte[TRACKNUMBER], 0x01
	jmp	READDATA
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	OS �Ϸ� �޽��� ���
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
READEND:
	push LOADINGCOMPLETEMESSAGE
	push 1
	push 20
	call PRINTMESSAGE
	add sp,6

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	�ε��� OS �̹��� ����
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	jmp 0x1000:0x0000

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	�Լ�����
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
HANDLEDISKERROR:
	push DISKERRORMESSAGE
	push 1
	push 20
	call PRINTMESSAGE
	add sp, 6
	jmp $	;infinite loop
; PRINTMESSAGE(ix,iy,string)
PRINTMESSAGE:
	push bp
	mov bp, sp
	; bp : sp, bp+2 : ret adr, bp+4 : ix bp+6 : iy bp+8 : string
	;; Backup register region
	push ax
	push es
	push si
	push di
	push cx
	mov ax, 0xB800
	mov es, ax
	; Calculate video buffer address
	; First, y (one line is composed of 160 bytes)
	mov ax, word[bp + 6]
	mov si, 160
	mul si
	mov di, ax
	; Second, x (one char is composed of 2 bytes)
	mov ax, word[bp + 4]
	mov si, 2
	mul si
	add di, ax
	mov si, word[bp + 8] ; save pointer of string
MESSAGELOOP:
	mov cl, byte[si]
	cmp cl, 0
	je MESSAGEEND
	mov byte[es:di], cl
	add di, 2
	add si, 1

	jmp MESSAGELOOP
MESSAGEEND:
	pop cx
	pop di
	pop si
	pop es
	pop ax
	pop bp
	ret
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	DATA Region
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
MESSAGE1:			db			'Lunar OS Boot Loader Start!', 0	;string + null
DISKERRORMESSAGE:	db			'DISK Error!',0
IMAGELOADINGMESSAGE:db			'OS Image Loading...', 0
LOADINGCOMPLETEMESSAGE:db		'Complete!',0

TOTALSECTORCOUNT:	dw			0x2	    ;MINTOS Image size(sector count * 512 bytes) except bootloader // ��ȣ��� + 64��Ʈ���
KERNEL32SECTORCOUNT:	dw		0x02	;��ȣ��� Ŀ���� �� ���ͼ�

SECTORNUMBER:		db			0x02	; sector init number for os image (1 is bootloader MasterBootRecord)
HEADNUMBER:			db			0x00	; head number of disk
TRACKNUMBER:		db			0x00	; track number of disk
times	510 - ($-$$)	db	0x00	; $ : current addr
									; $$: section start addr
									; $ - $$ : size of code in this section
									; times : repeat db 0x00
db 0x55	; BootLoader Magic String
db 0xAA
