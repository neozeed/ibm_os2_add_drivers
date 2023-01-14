;       SCCSID = @(#)fl2math.asm	6.3 92/01/17
;*****************************************************************************
;*                                                                           *
;*   Component   : IBM PS/2 Floppy Adapter Device Driver  (IBM2FLPY)         *
;*                                                                           *
;*   Module      : FL2MATH.ASM                                               *
;*                                                                           *
;*   Description :                                                           *
;*                                                                           *
;*                                                                           *
;*****************************************************************************


StaticCode segment dword public 'CODE'
        assume  cs: StaticCode

        .386

; unsigned long far _aFuldiv( unsigned long Dividend, unsigned long Divisor )

        public  __aFuldiv

__aFuldiv proc far

;       Divisor  = 10
;       Dividend = 6

        push    bp
        mov     bp,sp

        mov     edx, 0
        mov     eax, [bp+6]        ;put dividend in eax
        div     dword ptr [bp+10]  ;divide by divisor
        mov     edx, eax           ;put high word of eax into dx
        shr     edx, 16

        pop     bp
        ret     8                  ;clean up the stack

__aFuldiv endp

StaticCode ends

END

