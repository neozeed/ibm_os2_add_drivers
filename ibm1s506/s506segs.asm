;       SCCSID = @(#)s506segs.asm	6.2 92/02/06
;/****************************************************************************/
;/*                                                                          */
;/*                (c) Copyright  IBM Corporation  1981, 1990                */
;/*                           All Rights Reserved                            */
;/*                                                                          */
;/****************************************************************************/
;
;/********************* Start of Specifications ******************************/
;/*                                                                          */
;/*  Source File Name: S506SEGS.ASM                                          */
;/*                                                                          */
;/*  Descriptive Name: S506 CODE/DATA segment declarations.                  */
;/*                                                                          */
;/*  Copyright:                                                              */
;/*                                                                          */
;/*  Status:                                                                 */
;/*                                                                          */
;/*  Function: Part of S506 device driver for OS/2 family 1                  */
;/*                                                                          */
;/*                                                                          */
;/*  Notes:                                                                  */
;/*    Dependencies:                                                         */
;/*    Restrictions:                                                         */
;/*                                                                          */
;/*  Entry Points:                                                           */
;/*                                                                          */
;/*  External References:  See EXTRN statements below                        */
;/*                                                                          */
;/********************** End of Specifications *******************************/

        include devhdr.inc

;/*-------------------------------------*/
;/* Assembler Helper to order segments  */
;/*-------------------------------------*/

DDHeader        segment dword public 'DATA'

DiskDDHeader   dd      -1
               dw      DEVLEV_3 + DEV_CHAR_DEV
               dw      _S506Str1
               dw      0
               db      "IBMS506$"
               dw      0
               dw      0
               dw      0
               dw      0
               dd      DEV_ADAPTER_DD
               dw      0

DDHeader        ends

LIBDATA         segment dword public 'DATA'
LIBDATA         ends

_DATA           segment dword public 'DATA'
_DATA           ends

CONST           segment dword public 'CONST'
CONST           ends

_BSS            segment dword public 'BSS'
_BSS            ends

_TEXT           segment dword public 'CODE'

                extrn  _S506Str1:near

_TEXT           ends

Code            segment dword public 'CODE'
Code            ends

LIBCODE         segment dword public 'CODE'
LIBCODE         ends

SwapCode        segment dword public 'CODE'
SwapCode        ends

DGROUP          group   CONST, _BSS, DDHeader, LIBDATA, _DATA
StaticGroup     group   Code, LIBCODE, _TEXT
SwapGroup       group   SwapCode

        end
