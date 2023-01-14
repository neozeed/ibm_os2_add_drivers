/*static char *SCCSID = "@(#)adskstr1.c	6.3 92/02/06";*/

/****************************************************************************
 *                                                                          *
 *                (c) Copyright  IBM Corporation  1991                      *
 *                           All Rights Reserved                            *
 *                                                                          *
 ****************************************************************************/
/******************************************************************************
 * Change Log                                                                 *
 *                                                                            *
 * Mark    Date      Programmer  Comment                                      *
 * @2626   02/06/92  GAG         Fix the Install check problem                *
/***********************************************************************/
/*                                                                     */
/* Driver Name: ABIOS Adapter Device Driver for DASD Devices           */
/*              --------------------------------------------           */
/*                                                                     */
/* Source File Name: ADSKSTR1.C                                        */
/*                                                                     */
/* Descriptive Name: ABIOS Disk Driver - Strategy 1 EP                 */
/*                                                                     */
/* Function: Routes initialization packet, rejects all others.         */
/*                                                                     */
/***********************************************************************/

#define INCL_NOBASEAPI
#define INCL_NOPMAPI
#include <os2.h>

#include <devcmd.h>

#define INCL_INITRP_ONLY
#include <reqpkt.h>

#include <scb.h>
#include <abios.h>

#include <iorb.h>
#include <addcalls.h>

#include <adskcons.h>
#include <adsktype.h>
#include <adskpro.h>
#include <adskextn.h>

extern USHORT InitComplete;

VOID NEAR ADSKStr1()
{

 PRPH           pRPH;
 USHORT         Cmd;

 _asm
   {
     mov word ptr pRPH[0], bx
     mov word ptr pRPH[2], es
   }

 Cmd = pRPH->Cmd;
 if ((Cmd == CMDInitBase) && !InitComplete )
   {
     pRPH->Status = ADSKInit( (PRPINITIN) pRPH );
   }
 else
   if ((Cmd == CMDOpen) || (Cmd == CMDClose))       /* @2626 */
     {                                              /* @2626 */
       pRPH->Status |= STATUS_DONE;                 /* @2626 */
     }                                              /* @2626 */
   else
     {
       StatusError( pRPH, STATUS_ERR_UNKCMD );
     }

  _asm
    {
      leave
      retf
    }

 }


/*---------------------------------------------------------------------------*/
/* Strategy 1 Error Processing                                               */
/* ---------------------------                                               */
/*                                                                           */
/*                                                                           */
/*---------------------------------------------------------------------------*/

 VOID NEAR StatusError( pRPH, ErrorCode )
 PRPH           pRPH;
 USHORT         ErrorCode;
 {
   pRPH->Status |= ErrorCode;
   pRPH->Status |= STATUS_DONE;                     /* @2626 */
   return;
 }


