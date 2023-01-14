;       SCCSID = @(#)abmac.inc	6.1 92/01/08
;       SCCSID = @(#)abmac.inc  6.1 90/11/17
;       SCCSID = @(#)abmac.inc  1.7 90/04/03
; ****************************************************************************
; *                                                                          *
; *                This file is IBM unique                                   *
; *                IBM Confidential                                          *
; *                (c) Copyright  IBM Corporation  1981, 1990                *
; *                           All Rights Reserved                            *
; *                                                                          *
; ****************************************************************************

ENABLE  macro
        jmp  $+2                ; timing delay
        sti                     ; enable processor external interrupts
        endm

ENAB    macro
        sti                     ; enable processor external interrupts
        endm

DISABLE macro
        cli                     ; disable processor external interrupts
        endm

DEVHLP  macro   function
        mov     dl,function
        call    dword ptr [_Device_Help]
        endm



IOWait  macro
        local   l
        jmp     short   l
l       label   near
        endm

DebugPublic macro label
  if abdebug
         public label
  endif
  endm

INT3    macro
  if abdebug
        INT 03H
  endif
  endm

ListLevel macro list,bug
  if bug
    if1
      %out Warning: debuging code is being generated
    endif
  endif
  ifidn <list>,<1>                     ;; .list and .xlist
          .list                        ;; .lall .xall .sall
          .sfcond                      ;; .lfcond .sfconf (.tfcond)
          .sall
  else
    ifidn <list>,<2>
          .list
          .sfcond
          .xall
    else
      ifidn <list>,<3>
          .list
          .lfcond
          .lall
          if1
            %out listing level = 3
          endif
      else
          .xlist
          .sfcond
          .sall
      endif
    endif
  endif
  endm

TRACEREQUEST  macro    ;; trace requested DOS command on entry
  if abdebug           ;; placed in ABDSK1.ASM
    if abtrace
      ifndef ReqTrace
        extrn ReqTrace:near
      endif
      call ReqTrace
    endif
  endif
  endm

TRACERESULT  macro     ;; trace requested DOS command and result on complete.
  if abdebug           ;; placed in ABSUBR.ASM
    if abtrace
      if ndef RsltTrace
        call RsltTrace
      endif
    endif
  endif
  endm

VideoPause MACRO parm
    push ax
    push dx
    mov al,parm
    mov dl,DevHlp_VideoPause
    CALL dword ptr [DevHlpFunc]
    pop dx
    pop ax
    ENDM

;
;
;  Build Entry Point Table for Strategy 1
;
;  DDTableStart
;
;  Args:
;
;    <resgroup>  - Group name containing resident routines
;    <swapgroup> - Group name containing swappable routines
;    <epmask>    - Label for distance bit mask (0=Resident, 1=Swappable)
;    <epcount>   - Number of entry points in table
;
;
;  DDEntry  -- (This macro is repeated for each EP)
;
;  Args:
;
;    <ep>        - Entry point -- (Defined in <resgroup> or <swapgroup>)
;    <eptype>    - "RESIDENT" or "SWAPPABLE"
;
;
;  DDTableEnd
;
;  Args:
;
;     none
;
;  These macros generate a entry point table (2-bytes) per entry point, and
;  a bit mask containing the 'distance' for each entry point (near/far).
;
;

DDTableStart    macro   resgroup,swapgroup,epmask,epcount
;
?bitmask0 = 0                                   ; Distance Bit Mask DWORD 1
?bitmask1 = 0                                   ;                   DWORD 2
?fcount   = 0

?maskname       equ  <epmask>                   ; Copy args to pass to
?tblcount       equ  <epcount>                  ; other macros
?swapgroup      equ  <swapgroup>
?resgroup       equ  <resgroup>
;
                endm
;
;

DDTableEnd      macro
;
?genop          macro   op,p1,p2
&p1             &op     p2
                endm

;
                ?genop  <equ>,%?tblcount,?fcount  ; Set <epcount>
                ?genop  <dd>,%?maskname,?bitmask0 ; Gen Distance mask DWORD 1

                if      ?fcount gt 32             ; If More than 32 ep's ?
                  dd    ?bitmask1                 ; --Gen Distance mask DWORD 2
                endif

;               purge   ?genop
;
                endm
;
;
DDEntry         macro   ep,eptype

                ifidn   <&eptype>,<SWAPPABLE>
                  dw    offset ?swapgroup:&ep     ; Gen offset from swap group
                  if    ?fcount le 31
                    ?bitmask0 =  (1 shl ?fcount) or ?bitmask0
                  else
                    ?bitmask1 =  (1 shl (?fcount-32)) or ?bitmask1
                  endif
                else
                  dw    offset ?resgroup:&ep
                endif                             ; Gen offset from res group
                ?fcount = ?fcount + 1

                endm
