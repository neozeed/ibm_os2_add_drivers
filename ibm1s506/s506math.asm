;       SCCSID = @(#)s506math.asm	6.1 92/01/08
title IBM Family 1 ST506 Device Driver Math Routines
page 60,120

_TEXT segment dword public 'CODE'
        assume  cs: _TEXT

        .386

; unsigned long near _aNulmul( unsigned long 1st op, unsigned long 2nd op )

        public  __aNulmul

__aNulmul proc near

;       1st op = 4
;       2nd op = 8

        push    bp
        mov     bp,sp

        mov     eax, [bp+4]        ;put 1st operand in eax
        mul     dword ptr [bp+8]   ;multiply by second operand
        mov     edx, eax           ;put high word of eax into dx
        shr     edx, 16

        pop     bp
        ret     8                  ;clean up the stack

__aNulmul endp

_TEXT ends

END

