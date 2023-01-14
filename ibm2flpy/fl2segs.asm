;       SCCSID = @(#)fl2segs.asm	6.2 92/01/17
;*****************************************************************************
;*                                                                           *
;*   Component   : IBM PS/2 Floppy Adapter Device Driver  (IBM2FLPY)         *
;*                                                                           *
;*   Module      : FL2SEGS.ASM                                               *
;*                                                                           *
;*   Description : Assembler helper to order segments                        *
;*                                                                           *
;*                                                                           *
;*****************************************************************************


DDHeader        segment dword public 'DATA'     ;Device driver header
DDHeader        ends

CONST           segment dword public 'CONST'    ;Unused C stuff
CONST           ends

_BSS            segment dword public 'BSS'      ;Unused C stuff
_BSS            ends

LIBDATA         segment dword public 'DATA'     ;ADD common services data
LIBDATA         ends

_DATA           segment dword public 'DATA'     ;Device driver data
_DATA           ends


StaticCode      segment dword public 'CODE'     ;Permanently resident code
StaticCode      ends

Code            segment dword public 'CODE'     ;DevHelp code
Code            ends

_TEXT           segment dword public 'CODE'     ;Unused C stuff
_TEXT           ends

InitCode        segment dword public 'CODE'     ;Initialization code
InitCode        ends


SwapCode        segment dword public 'CODE'     ;Swappable code
SwapCode        ends

LIBCODE         segment dword public 'CODE'     ;ADD common services code
LIBCODE         ends


DGROUP          group   DDHeader, CONST, _BSS, LIBDATA, _DATA
StaticGroup     group   StaticCode, Code, _TEXT, InitCode
SwapGroup       group   SwapCode, LIBCODE

        end
