
.module SUSP
.globl _EZUSB_Susp

WAKEUPCS	= 0xE682	; XDATA
SUSPEND		= 0xE681	; XDATA
PCON		= 0x87		; DATA

.area CSEG (CODE)

_EZUSB_Susp:
		mov	dptr,#WAKEUPCS	; Clear the Wake Source bit in
		movx	a,@dptr		; the WAKEUPCS register
		orl		a,#0xc0		; clear PA2 and WPIN
		movx	@dptr,a
		
		mov		dptr,#SUSPEND     ; 
		movx	@dptr,a           ; write any walue to SUSPEND register

		orl	PCON,#0b00000001; Place the processor in idle

		nop			; Insert some meaningless instruction
		nop			; fetches to insure that the processor
		nop			; suspends and resumes before RET
		nop
		nop
er_end:		ret

