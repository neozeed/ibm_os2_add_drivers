;       SCCSID = @(#)iorbtype.inc	6.1 92/01/08
;***    IORBTYPE.INC
;
;
;       Basic constants and types
;       Copyright (c) 1991  IBM Corporation
;
;
;       MODIFICATION HISTORY
;           11/20/91  JRP(Adaptec)  Created.
;

;*** Basic constants
;


NULL  equ     0
TRUE  equ    -1
FALSE equ     0

CTRUE   EQU     1

KSIZE   EQU     1024

ifdef  INCL_PAGE
PAGESIZE        EQU     4096
endif


;*** Basic data types
;


AddTypes        struc

_INT            DD ?
UINT            DD ?
BOOL            DD ?

_BYTE           db ?
_WORD           dw ?                    ;w
_DWORD          dd ?                    ;dw

CHAR            db ?                    ;ch
UCHAR           db ?                    ;uch
_SHORT          dw ?                    ;s
USHORT          dw ?                    ;us
LONG            dd ?                    ;l
ULONG           dd ?                    ;ul


;*** Basic pointer types
;
PVOID           dd ?                    ;p
NPVOID          dd ?                    ;np


;*** Basic pointer-to-basic-data types
;
PINT            dd ?                    ;pi
NPINT           dw ?                    ;npi

PUINT           dd ?                    ;pu
NPUINT          dw ?                    ;npu

PBYTE           dd ?                    ;pb
NPBYTE          dw ?                    ;npb

PWORD           dd ?                    ;pw
NPWORD          dw ?                    ;npw

PDWORD          dd ?                    ;pdw
NPDWORD         dw ?                    ;npdw

PCHAR           dd ?                    ;pch
NPCHAR          dw ?                    ;npch

PSHORT          dd ?                    ;ps
NPSHORT         dw ?                    ;nps

PLONG           dd ?                    ;pl
NPLONG          dw ?                    ;npl

PUCHAR          dd ?                    ;puch
NPUCHAR         dw ?                    ;npuch

PUSHORT         dd ?                    ;pus
NPUSHORT        dw ?                    ;npus

PULONG          dd ?                    ;pul
NPULONG         dw ?                    ;npul

PBOOL           dd ?                    ;pf
NPBOOL          dw ?                    ;npf

PBBOOL          dd ?                    ;pbf
NPBBOOL         dw ?                    ;npbf

PSBOOL          dd ?                    ;psf
NPSBOOL         dw ?                    ;npsf

PLBOOL          dd ?                    ;plf
NPLBOOL         dw ?                    ;nplf

PFLAGS          dd ?                    ;pfl
NPFLAGS         dw ?                    ;npfl

PPVOID          dd ?                    ;pp
NPPVOID         dw ?                    ;npp


;*** Other common types (and their pointers)
;
PBUF            dd ?                    ;pbuf

PSZ             dd ?                    ;psz
NPSZ            dw ?                    ;npsz
PSZZ            dd ?                    ;pszz
NPSZZ           dw ?                    ;npszz

PSEL            dd ?                    ;psel
NPSEL           dw ?                    ;npsel

PFN             dd ?                    ;pfn
NPFN            dw ?                    ;npfn
FPFN            dq ?                    ;fpfn
FPVOID          dq ?                    ;fp (the only kind of FPVOID allowed)

AddTypes ends

BBOOL   equ UCHAR                       ;bf
SBOOL   equ USHORT                      ;sf
LBOOL   equ ULONG                       ;lf

FLAGS   equ ULONG                       ;fl
PORT    equ ULONG                       ;port


BUF     equ _BYTE                       ;buf
NAM     equ CHAR                        ;nam
SZ      equ CHAR                        ;sz
SZZ     equ CHAR                        ;szz
SEL     equ USHORT                      ;sel
SOFFSET equ USHORT                      ;soff
_OFFSET equ ULONG                       ;off

PPHYS   equ ULONG                       ;pphys


;***  types used by reqpkt.h
ReqPktdefs      struc

PBPB            DD ?
P_DRIVERCAPS    DD ?
P_VOLCHARS      DD ?

ReqPktdefs      ends

;***  types used by iorb.h
iorbdefs      struc

PSCATGATENTRY   DD ?

iorbdefs      ends

