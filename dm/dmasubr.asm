;       SCCSID = @(#)dmasubr.asm	6.5 92/02/06
;       SCCSID = @(#)dmasubr.asm	6.5 92/02/06
        page    ,132
; ****************************************************************************
; *                                                                          *
; *                This file is IBM unique                                   *
; *                (c) Copyright  IBM Corporation  1981, 1990                *
; *                           All Rights Reserved                            *
; *                                                                          *
; ****************************************************************************
        title   ddasubr - OS/2 2.0 OS2DASD.SYS device driver
        name    ddasubr

        page    ,132

        .xlist
        include struc.inc
        include abmac.inc
        include basemaca.inc
        include devhlp.inc
        include devsym.inc
        include strat2.inc
        .list

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

;/*---------------*/
;/* ABIOS Related */
;/*---------------*/

;StaticCode SEGMENT use16 'CODE'
;           assume cs:StaticCode

_DATA   segment dword public 'DATA'
        extrn   _Device_Help:dword
        extrn   _pDiskFT_Request:dword
        extrn   _pDiskFT_Done:dword
        extrn   _DiskFT_DS:word
        extrn   _pFSD_EndofInt:dword
        extrn   _pFSD_AccValidate:dword

_DATA   ends

_TEXT   segment dword public 'CODE'
        extrn   _DD_ChgPriority:dword
_TEXT   ends

_TEXT     segment dword public 'CODE'
          assume  CS:_TEXT, DS:_DATA

        .386

;--------------------------------------------------------------------
;
;  SUBROUTINE NAME: SortPriorityQueue
;
;  DESCRIPTIVE NAME: Sort a Request on a priorty queue
;
;  FUNCTION:  This routine inserts a request on the priority
;             based waiting queue for the device.  The
;             request is inserted on the queue using an elevator
;             algorithm based on the starting sector number of the
;             request. If the new request is less than the queue
;             head request, the new request is placed in the queue
;             in descending order by placing it just before the
;             first entry less than the new entry.  If the new
;             request is greater than the queue head, the new entry
;             is inserted in ascending order (just before the first
;             entry greater than the new one).  This algorithm
;             places new entries into the queue according to an
;             elevator algorithm intended to reduce cylinder to
;             cylinder seeks when stepping through requests in the
;             queue.
;             NOTE: This algorithm has the side effect of never
;                   replacing the entry at the head of the queue.
;
;             NOTE:  This routine will accept either an old style
;                    request packet or a new style request entry
;                    as input.  However, the queue which the request
;                    is inserted on must be of a single type.
;
;
;   VOID NEAR SortPriorityQueue (NPBYTE pPrtyQHead, PBYTE pReq, )
;
;   ENTRY:    pPrtyQHead       -  Priority Queue head pointer
;             pReq             -  Request Packet or Request List Entry
;
;   RETURN:   VOID
;
;   EFFECTS:
;--------------------------------------------------------------------
pPrtyQHead      EQU     [bp+4]
pReq            EQU     [bp+6]

Procedure _SortPriorityQueue,near

   LocalVar NewRequest,dword
   LocalVar SWQ_Offset,word
   EnterProc

   pushf
   pushad
   DISABLE

   mov     si,pPrtyQHead                ; ds:si -> pPrtyQHead
   les     bx,pReq                      ; es:bx -> pReq

   movzx   esi,si                       ; So we can use as index reg
   movzx   ebx,bx                       ; So we can use as index reg

;  This routine handles both request packets and request list entries.
;  This is done by testing which case is being handled and then putting
;  the offsets for the two relevant fields (forward link and RBA) into
;  index registers.  These fields can then be accessed using indexed
;  references so the code is freed from the specific structure format.
;
   .IF <es:[bx].RH_Old_Command eq PB_REQ_LIST>  ; Request List Entry ?
      mov   edi,RH_Waiting                      ; edi -> offset for link
      mov   edx,PB_Start_Block                  ; edx -> offset for RBA
   .ELSE                                ; else it's a request packet
      mov  edi,PktDevLink               ; edi -> offset for link
      mov  edx,IOPhysRBA                ; edx -> offset for RBA
   .ENDIF

SWQ_Link  equ  [edi]                    ; use equates for readability
SWQ_RBA   equ  [edx]                    ;

   mov  es:dword ptr [ebx].SWQ_Link,0   ; Zero this entry's forward link ptr
   mov  word ptr NewRequest,bx          ; Save pointer to request
   mov  word ptr NewRequest+2,es        ;
;
;  If the queue is empty then make the new request the new queue head
;
   cmp dword ptr [si],0                 ; Empty queue ?
   jne SWQ_ChkSort                      ; N: then continue
   push  NewRequest                     ; Make new request the new head
   push  NewRequest                     ;
   pop   dword ptr [si]                 ;
   pop   dword ptr [si+4]               ; and the new tail
   jmp  SWQ_Ret                         ;  and return

SWQ_ChkSort:
   les   bx,[si]                        ; es:bx -> first entry on the queue
;
;  Sort the request on the queue. First determine if the queue starts
;  out in ascending or descending order.
;    es:[ebx] = Q[n],   gs:[esi] = Q[n+1]
;
   mov  ecx,es:dword ptr [ebx].SWQ_RBA  ; ecx = this request's RBA
SWQ_Order:
   cmp  es:dword ptr [ebx].SWQ_Link,0   ; Is Q[n] last entry on the queue ?
   je   SWQ_Insert                      ; Y: insert new request after it
   mov  eax,es:[ebx].SWQ_RBA            ; eax = Q[n].RBA
   lgs  si,es:[ebx].SWQ_Link            ; Q[n+1] = Q[n].QLink
   cmp  eax,gs:[esi].SWQ_RBA            ;
   jb   SWQ_asc                         ; If Q[n].RBA < Q[n+1].RBA then asc
   ja   SWQ_desc                        ; If Q[n].RBA > Q[n+1].RBA then desc
                                        ;  else Q[n].RBA = Q[n+1].RBA
   push gs                              ;
   pop  es                              ;
   mov  bx,si                           ; Q[n] = Q[n+1]
   jmp  SWQ_Order                       ; Get next Q entry and try again
;----------
;  The queue starts out in ascending order.  If the new request is less than
;  the first request in the queue, then put the new request on the descending
;  slope of the queue.
;----------
SWQ_asc:                                ; Queue Ascending
   mov  SWQ_Offset,offset SWQ_asc_desc  ; Set offset for return jmp
   cmp  ecx,eax                         ; New.RBA < Q[n].RBA ?
   jb   SWQ_asc_desc                    ; Y: insert new entry on desc. slope
   mov  SWQ_Offset,offset SWQ_asc_asc   ; Set offset for return jmp
;
;  The queue starts out ascending and we want to place the request
;  on the ascending slope.
;
SWQ_asc_asc:                            ; Insert on ascending slop
   cmp  ecx,gs:[esi].SWQ_RBA            ; New.RBA <= Q[n+1].RBA ?
   jbe  SWQ_Insert                      ; Y: insert new before Q[n+1]
   cmp  eax,gs:[esi].SWQ_RBA            ; At the top of the ascending slope ?
   ja   SWQ_Insert                      ; Y: put new entry at the top
   jmp  SWQ_try_next                    ;  else get next Q entry
;
;  The queue starts out ascending and we want to place the request
;  on the descending slope.
;
SWQ_asc_desc:
   cmp  ecx,gs:[esi].SWQ_RBA            ; New.RBA >= Q[n+1].RBA
   jae  SWQ_Insert                      ; Y: insert new before Q[n+1]
   jmp  SWQ_try_next                    ;  else get next Q entry

;----------
;  The queue starts out in descending order.  If the new request greater than
;  the first request in the queue, then put the new request on the ascending
;  slope of the queue.
;----------
SWQ_desc:                               ; Queue Descending
   mov  SWQ_Offset,offset SWQ_desc_asc  ; Set offset for return jmp
   cmp  ecx,eax                         ; New.RBA > Q[n].RBA ?
   ja   SWQ_desc_asc                    ; Y: insert new on asc. slope
   mov  SWQ_Offset,offset SWQ_desc_desc ; Set offset for return jmp
;
;  The queue starts out descending and we want to place the request
;  on the descending slope.
;
SWQ_desc_desc:                          ; Insert on descending slope
   cmp  ecx,gs:[esi].SWQ_RBA            ; New.RBA >= Q[n+1].RBA
   jae  SWQ_Insert                      ; Y: insert new before Q[n+1]
   cmp  eax,gs:[esi].SWQ_RBA            ; At the bottom of descending slope ?
   jb   SWQ_Insert                      ; Y: put new entry at the bottom
   jmp  SWQ_try_next                    ;  else get next queue entry
;
;  The queue starts out descending, but we want to put the new request
;  on the ascending slope of the queue.
;
SWQ_desc_asc:
   cmp  ecx,gs:[esi].SWQ_RBA            ; New.RBA <= Q[n+1].RBA
   jb   SWQ_Insert                      ; Y: insert new before Q[n+1]
;----------
;  Get the next entry in the queue.  Jump indirect back to the specified
;  insertion routine.
;----------
SWQ_try_next:
   push gs                              ;
   pop  es                              ;
   mov  bx,si                           ; Q[n] = Q[n+1]
   cmp  es:dword ptr [ebx].SWQ_Link,0   ; Is Q[n] last entry on the queue ?
   je   SWQ_Insert                      ; Y: Insert new request after it
   mov  eax,es:dword ptr [ebx].SWQ_RBA  ; eax = Q[n].RBA
   lgs  si,dword ptr es:[ebx].SWQ_Link  ; Q[n+1] = Q[n].Qlink
   jmp  [SWQ_Offset]                    ; jmp back for another try
;
;  Insert the new request after the request pointed to by es:[ebx].
;
SWQ_Insert:
   mov   si,word ptr NewRequest           ;
   mov   gs,word ptr NewRequest+2         ; gs:si -> New Request
   mov   eax,es:[ebx].SWQ_Link            ; eax = Q[n+1]
   mov   es:word ptr [ebx].SWQ_Link,si    ; Insert new after Q[n]
   mov   es:word ptr [ebx+2].SWQ_Link,gs  ;
   mov   gs:dword ptr [esi].SWQ_Link,eax  ; Link new to Q[n+1]

SWQ_Ret:
   popad
   popf
   LeaveProc
   ret

EndProc _SortPriorityQueue


;/*------------------------------------------------------------------------
;
;** SWait - Wait for a semaphore
;
;   This is the same routine as the DOS "ram semaphore" routine.
;   It operates on a semaphore made up of a pair of bytes- an "in
;   use" flag and a "someone is wait" flag.
;
;   USABLE IN SYSTEM and USER MODE.
;
;   WARNING - Enables Interrupts
;
;   DB      busyflag
;   DB      waitingflag
;
;   If busyflag is clear then
;           set busyflag
;           return
;   else
;           set waitingflag (to notify owner to do a wakeup when he's done)
;           ProcBlock on address of busyflag
;
;   Note that when ProcBlock returns interrupts will be enabled.
;   Since the entire procblock interval was an "interrupts enabled"
;   window we figure we must be called with interrupts on and
;   thus don't mess with PUSHF/POPFF but just jam the interrupts
;   on and off as necessary.  (In keeping with good practice we
;   keep them on as much as possible.)
;
;
;   VOID NEAR SWait (NPUSHORT Semaphore)
;
;   VOID FAR  f_SWait (NPUSHORT Semaphore)
;
;   ENTRY:    Semaphore        - semaphore to wait on
;
;   RETURN:   VOID
;
;   EFFECTS:
;
;   NOTES:
;------------------------------------------------------------------------*/

BIOS_BLOCK_ID   equ     0fffch

         public _f_SWait
_f_SWait proc far

        push    bp
        mov     bp,sp
        push    bx
        mov     bx,[bp+6]
        push    bx
        call    _SWait
        pop     bx
        pop     bx
        pop     bp
        ret

_f_SWait endp


WaitSem         equ     [bp+4]

      public _SWait
_SWait proc near

        push    bp
        mov     bp,sp

        SaveReg <ax,bx>
        mov     bx,WaitSem
;       retest semaphore
;
;       (ds:bx) = address
;       (TOS) = caller's AX

swa0:   DISABLE
        mov     al,1
        xchg    al,BYTE PTR ds:[bx]     ; set semaphore

;       If busyflag was set then its still set and (al) = 1
;       If busyflag was clear then its set now and (al) = 0

        and     al,al
        jnz     swa1                    ; foo - it was already set so Block
        ENABLE
        RestoreReg <bx, ax>
        pop     bp
        ret                             ; done  - we've got it

;       The thing is set.  We're going to have to block on it
;
;       (al) = 1

swa1:   mov     BYTE PTR ds:1[bx],al        ; set waiting

        SaveReg <bx,cx,dx,di>
        mov     ax,BIOS_BLOCK_ID
        mov     di,-1
        mov     cx,-1                   ; Never time out
        mov     dh,1                    ; Non-Interruptible
        DEVHLP DevHlp_ProcBlock         ; Block until Proc_Run
        RestoreReg <di,dx,cx,bx>

        jmp     swa0                            ; Re-Test this.

_SWait   endp


;/*------------------------------------------------------------------------
;
;** SSig- Signal a disk semaphore
;
;   SSig releases a semaphore (it is presumed that the caller indeed
;   owns it!) and, if anyone was waiting, wakes them up.
;
;   Usable in SYSTEM-TASK, USER and INTERRUPT mode.
;   Callable in BOOT mode if we're sure no one is "waiting" on the
;           semaphore (which should always be the case)
;
;   VOID NEAR SSig (NPUSHORT Semaphore)
;
;   VOID FAR  f_SSig (NPUSHORT Semaphore)
;
;   ENTRY:    Semaphore        - semaphore to signal
;
;   RETURN:   VOID
;
;   EFFECTS:
;
;   NOTES:
;------------------------------------------------------------------------*/

         public _f_SSig
_f_SSig  proc far

        push    bp
        mov     bp,sp
        push    bx
        mov     bx,[bp+6]
        push    bx
        call    _SSig
        pop     bx
        pop     bx
        pop     bp
        ret

_f_SSig  endp


SigSem  EQU  [bp+4]

      public _SSig
_SSig proc near

        push    bp
        mov     bp,sp
        SaveReg <ax,bx>

        mov     bx,SigSem
        xor     ax,ax
        xchg    ax,ds:[bx]              ; clear semaphore and wait flag

;       (al) = semaphore flag
;       (ah) = wait flag

        and     al,al
        jz      SSig8                   ; TROUBLE! Not SET!
        and     ah,ah
        jz      SSig2                   ; nobody waiting, don't ProcRun

;       Someone is waiting on this semaphore.  Wake him/them up.
;
;       (AX:BX) = block/run code value

        SaveReg <bx, cx, dx>
        mov     ax, BIOS_BLOCK_ID
        DEVHLP  DevHlp_ProcRun          ; Block until Proc_Run
        RestoreReg <dx, cx, bx>
SSig2:
        RestoreReg <bx, ax>
        pop     bp
        ret

SSig8:  jmp     SSig8
        db      "Semaphore cleared that was never set!!"

_SSig    endp




;--------------------------------------
; ULONG = add32 (ULONG operand1, ULONG operand2)
;
;
;--------------------------------------
AddOp1  EQU    [bp+6]
AddOp2  EQU    [bp+10]

public _f_add32
_f_add32 proc far

        push    bp
        mov     bp,sp
        mov     eax,AddOp1
        add     eax,AddOp2
        jc      Over
        mov     edx,eax
        shr     edx,16
        jmp     short NotOver
Over:
        mov     ax,0
        mov     dx,0
NotOver:
        pop     bp
        ret


_f_add32 endp


;-----------------------------------------------------;
; VOID _f_ZeroCB (PBYTE ControlBlock, USHORT Length)  ;
;                                                     ;
; Function: Zero fill the input control block         ;
;                                                     ;
;-----------------------------------------------------;
CtrlBlk EQU [bp+6]
BlkLen  EQU [bp+10]

public _f_ZeroCB
_f_ZeroCB proc far

        push    bp
        mov     bp,sp

        SaveReg <es,bx,cx,di>

        xor     eax,eax
        les     di,CtrlBlk
        mov     cx,BlkLen
        mov     bx,cx
        and     bx,3
        shr     cx,2
        cmp     cx,0
        je      zcb_rem
        rep     stosd

zcb_rem:
        mov     cx,bx
        cmp     cx,0
        je      zcb_ret
        rep     stosb

zcb_ret:
        RestoreReg <di,cx,bx,es>

        pop     bp
        ret

_f_ZeroCB endp


;----------------------------------------------------------;
; VOID _f_BlockCopy (PBYTE Dest, PBYTE Orig, USHORT Length ;
;                                                          ;
; Function: Copy a block of data                           ;
;                                                          ;
;----------------------------------------------------------;
BlkDest  EQU [bp+6]
BlkSrc   EQU [bp+10]
BlkCLen  EQU [bp+14]

public _f_BlockCopy
_f_BlockCopy proc near

        push    bp
        mov     bp,sp

        SaveReg <ds,es,di,si,cx,bx>

        les     di,BlkDest
        lds     si,BlkSrc
        mov     cx,BlkCLen

        mov     bx,cx
        and     bx,3
        shr     cx,2
        cmp     cx,0
        je      bc_rem
        rep     movsd

bc_rem:
        mov     cx,bx
        cmp     cx,0
        je      bc_ret
        rep     movsb

bc_ret:
        RestoreReg <bx,cx,si,di,es,ds>

        pop     bp

        ret

_f_BlockCopy endp

;----------------------------------------------------------------------------;
;                                                                            ;
;** _FSD_Notify - Strategy-2 File System callout notification                ;
;                                                                            ;
;   Notify the file system a strategy-2 request has completed.               ;
;                                                                            ;
;                                                                            ;
;   VOID _f_FSD_Notify (PBYTE pRequest, PVOID NotifyAdress, USHORT ErrorFlag ;
;                                                                            ;
;   ENTRY:    pRequest         - Far pointer to RLE or RLH                   ;
;             NotifyAddress    - Far pointer containing notification address ;
;             ErrorFlag        - 0= no error, 1=error                        ;
;                                                                            ;
;                                                                            ;
;----------------------------------------------------------------------------;
pRequest EQU     [bp+4]
NtfyAddr EQU     [bp+8]
ErrFlag  EQU     [bp+12]


public _FSD_Notify
_FSD_Notify proc near

        push    bp
        mov     bp,sp

        push    ds
        push    es
        pusha

        les     bx,pRequest             ; es:bx = pRequest
        shr     word ptr ErrFlag,1      ; CY = error
        call    dword ptr NtfyAddr      ; call [NotifyAddress]

        popa
        pop     es
        pop     ds
        pop     bp

        ret

_FSD_Notify endp


;----------------------------------------------------------------------------;
;                                                                            ;
;** f_FT_Request - Call DISKFT's FT_Request entry point                      ;
;                                                                            ;
;   USHORT f_FT_Request (USHORT CmdPart, USHORT SzReqPkt, PBYTE pResults,    ;
;                                                          PBYTE ActionCode  ;
;                                                                            ;
;   ENTRY:    CmdPart          - Command and Partition Number                ;
;             SzReqPkt         - size of reqpkt for shadow write             ;
;             pResults         - pointer to results structure                ;
;             pActionCode      - returned Action Code                        ;
;                                                                            ;
;                                                                            ;
;   RETURN:   USHORT           -  Zero = No Error, 1 = Error                 ;
;                                                                            ;
;----------------------------------------------------------------------------;
CmdPart  EQU     [bp+6]
SzReqPkt EQU     [bp+8]
pResults EQU     [bp+10]
pActCode EQU     [bp+14]


public _f_FT_Request
_f_FT_Request proc far

        push    bp
        mov     bp,sp

        SaveReg <bx,cx,dx,si,di,ds,es>

        push    ds
        pop     es

        push    CmdPart              ; CmdPart Parm
        push    0
        push    SzReqPkt             ; SzReqPkt Parm
        push    dword ptr pResults   ; Pointer to results
        mov     ax,_DiskFT_DS        ;
        mov     ds,ax                ; ds = DiskFT's ds
        call    es:_pDiskFT_Request  ; call DiskFT's Request routine

        les     bx,pActCode
        mov     es:[bx],al             ; Save Return Code

        RestoreReg <es,ds,di,si,dx,cx,bx>
        pop     bp

        mov     ax,0
        jnc     ftreq_ok
        mov     ax,1
ftreq_ok:
        ret

_f_FT_Request endp

;----------------------------------------------------------------------------;
;                                                                            ;
;** f_FT_Done - Call DISKFT's FT_Done entry point                            ;
;                                                                            ;
;   USHORT f_FT_Done (USHORT ErrPart, USHORT ReqHandle, ULONG RelativeSec)   ;
;                                                                            ;
;   ENTRY:    ErrPart          - ErrorCode and Partition Number              ;
;             ReqHandle        - Request Handle                              ;
;             RelativeSec      - sector number from start of partition       ;
;                                                                            ;
;   RETURN:   USHORT           -  Packet Status                              ;
;                                                                            ;
;----------------------------------------------------------------------------;
ErrPart   EQU [bp+6]
ReqHandle EQU [bp+8]
RelSect   EQU [bp+10]


public _f_FT_Done
_f_FT_Done proc far

        push    bp
        mov     bp,sp

        SaveReg <bx,cx,dx,si,di,ds,es>

        push    ds
        pop     es

        push    ErrPart              ; ErrPart Parm
        push    0
        push    ReqHandle            ; ReqHandle Parm
        push    dword ptr RelSect    ; Relative Sector
        mov     ax,_DiskFT_DS        ;
        mov     ds,ax                ; ds = DiskFT's ds
        call    es:_pDiskFT_Done     ; call DiskFT's Done routine

        mov     bx,0
        jnc     noerr                ; Error ?
        or      bx,STERR             ; set error bit
        mov     bl,ah                ;   and error code
noerr:
        cmp     al,0                 ; request done ?
        jne     notdone
        or      bx,STDON             ; yes, set done bit
notdone:
        mov     ax,bx                ; return status in ax

        RestoreReg <es,ds,di,si,dx,cx,bx>
        pop     bp
        ret

_f_FT_Done endp

;----------------------------------------------------------------------------;
;                                                                            ;
;** FSD_EndofInt - Call FSD's End of interrupt routine                       ;
;                                                                            ;
;   VOID FSD_EndofInt (VOID)                                                 ;
;                                                                            ;
;   ENTRY:    VOID                                                           ;
;                                                                            ;
;   RETURN:   VOID                                                           ;
;                                                                            ;
;----------------------------------------------------------------------------;
public _FSD_EndofInt
_FSD_EndofInt proc near

   pusha
   push ds
   push es

   call _pFSD_EndofInt          ; call FSD's end of interrupt routine

   pop  es
   pop  ds
   popa

   ret

_FSD_EndofInt endp

;----------------------------------------------------------------------------;
;                                                                            ;
;** f_FSD_AccValidate - Call FSD's Access Validation routine                 ;
;                                                                            ;
;   USHORT f_FSD_AccValidate (USHORT Destructive)                            ;                   ;
;                                                                            ;
;   ENTRY:    Destructive               = 0, not destructive                 ;
;                                       = 1, destructive                     ;
;                                                                            ;
;   RETURN:   USHORT                    = 0, validate ok                     ;
;                                       = 1, validation failure              ;
;                                                                            ;
;----------------------------------------------------------------------------;
DestrFlag   EQU  [bp+6]

public _f_FSD_AccValidate
_f_FSD_AccValidate proc far

        push    bp
        mov     bp,sp

        mov     ax,DestrFlag            ; Get destructive code
        call    _pFSD_AccValidate       ;
        mov     ax,0
        jnc     Acc_ret
        mov     ax,1

Acc_ret:
        pop     bp
        ret

_f_FSD_AccValidate endp

;----------------------------------------------------------------------------;
;                                                                            ;
;** _DD_ChgPriority_asm - Change priority entry point for FSD                ;
;                                                                            ;
;   ENTRY:    es:bx             Pointer to request entry                     ;
;                al             Priority                                     ;
;                                                                            ;
;   RETURN:   NC                Priority changed                             ;
;             CY Set            Priority not changed                         ;
;                                                                            ;
;----------------------------------------------------------------------------;
public _DD_ChgPriority_asm
_DD_ChgPriority_asm  proc far

        push    ds
        push    es

        push    DGROUP
        pop     ds

        push    es
        push    bx
        push    ax
        call    _DD_ChgPriority
        cmp     ax,0
        je      DDC_Retok
        stc
        jmp     short DDC_Ret

DDC_Retok:
        clc

DDC_Ret:
        add     sp,6
        pop     es
        pop     ds
        ret

_DD_ChgPriority_asm  endp


_TEXT ends
     end

