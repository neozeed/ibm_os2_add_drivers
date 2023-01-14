;       SCCSID = @(#)fl2headr.asm	6.4 92/02/05
;*****************************************************************************
;*                                                                           *
;*   Component   : IBM PS/2 Floppy Adapter Device Driver  (IBM2FLPY)         *
;*                                                                           *
;*   Module      : FL2HEADR.ASM                                              *
;*                                                                           *
;*   Description :                                                           *
;*                                                                           *
;*                                                                           *
;*****************************************************************************

extrn   _Strategy: near

DDHeader segment dword public 'DATA'

public  _Fl2Header

_Fl2Header      DD      0FFFFFFFFH
                DW      0C180H          ; Char device, accepts IOCTLs, Level 3
                DW      _Strategy       ; Strategy entry point
                DW      _Strategy       ; IDC entry point
                DB      'IBM2FLP$'
                DW      00H
                DW      00H
                DW      00H
                DW      00H
                DD      08H             ; Adapter Device Driver

DDHeader    ends

end


