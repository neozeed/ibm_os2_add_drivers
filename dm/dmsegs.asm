;       SCCSID = @(#)dmsegs.asm	6.2 92/01/14
;/*-------------------------------------*/
;/* Assembler Helper to order segments  */
;/*-------------------------------------*/

_DATA           segment dword public 'DATA'
_DATA           ends

CONST           segment dword public 'CONST'
CONST           ends

_BSS            segment dword public 'BSS'
_BSS            ends

_TEXT           segment dword public 'CODE'
_TEXT           ends

Code            segment dword public 'CODE'
Code            ends

SwapCode        segment dword public 'CODE'
SwapCode        ends

DGROUP          group   CONST, _BSS, _DATA
StaticGroup     group   Code, _TEXT
SwapGroup       group   SwapCode

        end
