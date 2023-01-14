/*static char *SCCSID = "@(#)dmstrat2.c	6.5 92/02/06";*/
/*static char *SCCSID = "@(#)dmstrat2.c	6.5 92/02/06";*/
#define SCCSID  "@(#)dmstrat2.c	6.5 92/02/06"

/*--------------------------------------------------------
 *       (c) Copyright IBM Corporation 1981, 1990        *
 *                                                       *
 *       All Rights Reserved                             *
 *-------------------------------------------------------*/

#include "dmh.h"
#include "dmfault.h"


/*---------------------------------------------------*/
/* Forward Function References and pragma statements */
/*---------------------------------------------------*/

VOID NEAR ValidateReqList (PReq_List_Header, NPVOLCB FAR *);
VOID ExecReqList (PReq_List_Header, NPUNITCB);
VOID FT_ExecReqList (PReq_List_Header, NPUNITCB);


/*------------------------------------------------------------------------
;
;** DMStrat2 - Strategy2 Request List Entry Point
;
;   Processes Strategy2 request lists
;
;   USHORT DMStrat2    (PReq_List_Header pRLH)
;
;   ENTRY:    pRLH             - Request List Header
;
;   RETURN:   VOID
;
------------------------------------------------------------------------*/
VOID _loadds FAR DMStrat2 ()

{
   PReq_List_Header pRLH;
   NPVOLCB pVolCB;

   _asm { mov word ptr pRLH[0], bx };
   _asm { mov word ptr pRLH[2], es };

   pRLH->Lst_Status = 0;
   pRLH->y_Done_Count = 0;              /* Zero out request done count */

   /* Validate the request list */

   ValidateReqList(pRLH, (NPVOLCB FAR *) &pVolCB);

   /* If request list not aborted from validate, then execute the list */

   if ( !(pRLH->Lst_Status & RLH_All_Req_Done) )
   {
      if ( IsTraceOn() )
         Trace(TRACE_STRAT2 | TRACE_ENTRY, pRLH, pVolCB);

      pVolCB->Flags &= ~vf_ForceRdWrt;
      pRLH->y_PhysAddr = VirtToPhys((PBYTE)pRLH); /* Store phys addr of RLH */

      /* See if Fault Tolerance is enabled */

      if (DDFlags & DDF_FT_ENABLED)
         FT_ExecReqList (pRLH, pVolCB->pUnitCB);
      else
         ExecReqList (pRLH, pVolCB->pUnitCB);
   }
}

/*------------------------------------------------------------------------
;
;** ExecReqList - Execute Strategy2 Request List
;
;   Executes Strategy2 request lists
;
;   USHORT ExecReqList    (PReq_List_Header pRLH, pUnitCB);
;
;   ENTRY:    pRLH         - Request List Header
;             pUnitCB      - Pointer to UnitCB
;
;   RETURN:   VOID
;
------------------------------------------------------------------------*/
VOID ExecReqList (pRLH, pUnitCB)

PReq_List_Header pRLH;
NPUNITCB pUnitCB;
{
   USHORT  i;
   PPB_Read_Write pRLE;

   (PReq_List_Header) pRLE = pRLH + 1;

   /* Loop through the list */

   for (i = 0; i < pRLH->Count; i++)
   {
      if (pRLE->RqHdr.Status == RH_NOT_QUEUED)
      {
         PutPriorityQueue_RLE (pUnitCB, pRLE);

         /* Only queue one at a time if Execute in Sequence specified */

         if (pRLH->Request_Control & RLH_Exe_Req_Seq)
            break;
      }
      OFFSETOF(pRLE) = OFFSETOF(pRLE) + pRLE->RqHdr.Length;
   }

   pRLH->Lst_Status &= ~RLH_Req_Not_Queued;
   pRLH->Lst_Status |= RLH_All_Req_Queued;

   SubmitRequestsToADD(pUnitCB);
}

/*------------------------------------------------------------------------
;
;** FT_ExecReqList - Fault Tolerance Execute Strategy2 Request List
;
;   Executes Strategy2 request lists
;
;   USHORT FT_ExecReqList    (PReq_List_Header pRLH, pUnitCB);
;
;   ENTRY:    pRLH         - Request List Header
;             pUnitCB      - Pointer to UnitCB
;
;   RETURN:   VOID
;
------------------------------------------------------------------------*/
VOID FT_ExecReqList (pRLH, pUnitCB)

PReq_List_Header pRLH;
NPUNITCB pUnitCB;
{
   USHORT  i;
   PPB_Read_Write pRLE;
   NPUNITCB pUnitCB1  = 0;
   NPUNITCB pUnitCB2  = 0;
   NPUNITCB pUnitCB1t = 0;
   NPUNITCB pUnitCB2t = 0;

   (PReq_List_Header) pRLE = pRLH + 1;

   pUnitCB1 = pUnitCB;

   /* Loop through the list */

   for (i = 0; i < pRLH->Count; i++)
   {
      if (pRLE->RqHdr.Status == RH_NOT_QUEUED)
      {
         if (FT_CheckFTSupport(pUnitCB, (PBYTE) pRLE) == NO)
         {
             PutPriorityQueue_RLE (pUnitCB, pRLE);
             pUnitCB1 = pUnitCB;
         }
         else
         {
            FT_PutPriorityQueue(pUnitCB, (PBYTE) pRLE,
                     (NPUNITCB FAR *) &pUnitCB1t, (NPUNITCB FAR *) &pUnitCB2t);

            if (pUnitCB1t != 0)
               pUnitCB1 = pUnitCB1t;

            if (pUnitCB2t != 0)
               pUnitCB2 = pUnitCB2t;
         }

         /* Only queue one at a time if Execute in Sequence specified */

         if (pRLH->Request_Control & RLH_Exe_Req_Seq)
            break;
      }

      OFFSETOF(pRLE) = OFFSETOF(pRLE) + pRLE->RqHdr.Length;
   }

   pRLH->Lst_Status &= ~RLH_Req_Not_Queued;
   pRLH->Lst_Status |= RLH_All_Req_Queued;

   if (pUnitCB1 != 0)
      SubmitRequestsToADD(pUnitCB1);

   if (pUnitCB2 != 0)
      SubmitRequestsToADD(pUnitCB2);
}

/*------------------------------------------------------------------------
;
;** ValidateReqList -  Validate a Strategy-2 request list
;
;   Verify the Request List Header has a valid unit number.  Verify
;   each Request List Entry has a valid command code and that the
;   I/O is within the partition boundaries.
;
;   USHORT ValidateReqList (PReq_List_Header pRLH, NPVOLCB *pVolCB_Addr)
;
;   ENTRY:    pRLH             - Request List Header
;             pVolCB_Addr      - returned pointer to VolCB
;
;   RETURN:   VOID
;
------------------------------------------------------------------------*/
VOID  ValidateReqList (pRLH, pVolCB_Addr)

PReq_List_Header pRLH;
NPVOLCB FAR *pVolCB_Addr;

{
   USHORT  rc, i;
   PPB_Read_Write pRLE;
   NPVOLCB pVolCB;

   /* First validate the Request List Header.  Make sure the header    */
   /* contains a valid unit number.  If the unit is for a floppy drive,*/
   /* make sure the floppy disk doesnt need to be changed.             */

   rc = Get_VolCB_Addr(pRLH->Block_Dev_Unit, (NPVOLCB FAR *) pVolCB_Addr);

   if ( !(rc & STERR) )
   {
      pVolCB = *pVolCB_Addr;

      /*--------------------------------------------------------------*/
      /*  If it's for a psuedo-drive which is not currently mapped to */
      /*  it's associated unit, then a disk swap is needed.           */
      /*--------------------------------------------------------------*/

      if  (pVolCB->pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE)
      {
         if (CheckPseudoChange(pRLH->Block_Dev_Unit, pVolCB) == -1)
            rc = STDON + STERR + ERROR_I24_DISK_CHANGE;

         else if  ( (!(pVolCB->Flags & vf_ForceRdWrt)) &&
              (pVolCB->Flags & vf_UncertainMedia) )
         {
            rc = STDON + STERR + ERROR_I24_UNCERTAIN_MEDIA;
         }
      }
   }

   /* If the Request List Header had an error, then return an error for  */
   /* all entries in the list.                                           */

   if (rc & STERR)
   {
      pRLH->Lst_Status |= RLH_Unrec_Error;

      (PReq_List_Header) pRLE = pRLH + 1;
      for (i = 0; i < pRLH->Count; i++, pRLE++)
      {
         pRLE->Blocks_Xferred = 0;
         pRLE->RqHdr.Status = RH_UNREC_ERROR;
         pRLE->RqHdr.Error_Code = rc;
         NotifyRLE(pRLE);
      }
      return;
   }

   /* The Request List Header is ok. Make sure each request in the  */
   /* list has a valid command code and the I/O fails within the    */
   /* partition boundary.                                           */

   (PReq_List_Header) pRLE = pRLH + 1;
   for (i = 0; i < pRLH->Count; i++)
   {
      rc = 0;

      /* Copy the Block_Dev_Unit from the RLH to each RLE   */
      ((PRHFT)pRLE)->Block_Dev_Unit = pRLH->Block_Dev_Unit;
      ((PRHFT)pRLE)->ftdb.FT_Flags = 0;

      if (pRLE->RqHdr.Command_Code == PB_READ_X ||
          pRLE->RqHdr.Command_Code == PB_WRITE_X ||
          pRLE->RqHdr.Command_Code == PB_WRITEV_X)
        ;
      else if (pRLE->RqHdr.Command_Code == PB_PREFETCH_X)
      {
         if ( !(pVolCB->pUnitCB->UnitInfo.UnitFlags & UF_PREFETCH) )
            rc = RH_DONE + RH_UNREC_ERROR + ERROR_I24_BAD_COMMAND;
      }
      else
         rc = RH_DONE + RH_UNREC_ERROR + ERROR_I24_BAD_COMMAND;

      if (rc == 0)
      {
         if (CheckWithinPartition(pVolCB, pRLE->Start_Block,
                                            pRLE->Block_Count) & STERR)
            rc = RH_DONE + RH_UNREC_ERROR + ERROR_I24_SECTOR_NOT_FOUND;
      }

      /* If the Request List Entry had an error notify the file system */

      if (rc != 0)
      {
         UCHAR Status = (UCHAR) rc/256;
         pRLE->RqHdr.Status = Status;
         pRLE->RqHdr.Error_Code = (UCHAR) rc;
         pRLE->Blocks_Xferred = 0;
         NotifyRLE(pRLE);

         /* Abort the list and return if Abort_Err specified */

         if (pRLH->Request_Control & RLH_Abort_Err)
         {
            AbortReqList(pRLH);
            return;
         }
      }
      OFFSETOF(pRLE) = OFFSETOF(pRLE) + pRLE->RqHdr.Length;
   } /* end for */
}

/*------------------------------------------------------------------------
;
;** AbortReqList -  Abort the request list
;
;   Abort the reqeust list when Abort on Error is specified.
;
;   VOID AbortReqList (PReq_List_Header pRLH)
;
;   ENTRY:    pRLH             - Request List Header
;
;   RETURN:   VOID
;
------------------------------------------------------------------------*/
VOID AbortReqList (pRLH)

PReq_List_Header pRLH;

{
   PPB_Read_Write pRLE;
   USHORT i;

   pRLH->Lst_Status |= RLH_Unrec_Error;

   (PReq_List_Header) pRLE = pRLH + 1;
   for (i = 0; i < pRLH->Count; i++, pRLE++)
   {
      if ( !(pRLE->RqHdr.Status & RH_DONE) )
      {
         pRLE->Blocks_Xferred = 0;
         pRLE->RqHdr.Status |= RH_ABORTED;
         pRLE->RqHdr.Error_Code = 0;
         NotifyRLE(pRLE);
      }
   }
}


/*------------------------------------------------------------------------
;
;** NotifyRLE - Notification callout for Request Lists
;
;   Perform notification callouts to the filesystem when a
;   Request List Entry is done.  Also, do the final Request List
;   notification callout when the entire list is complete.
;
;   USHORT NotifyRLE (PB_Read_Write pRLE)
;
;   ENTRY:    pRLE             - Request List Entry
;
;   RETURN:   VOID
;
------------------------------------------------------------------------*/
VOID NotifyRLE (pRLE)

PPB_Read_Write pRLE;

{
   PReq_List_Header pRLH;

   /* Make sure this request is not already done */

   DISABLE;
   if (pRLE->RqHdr.Status & (RH_DONE | RH_ABORTED) )
   {
      ENABLE;
      return;
   }

   /* Set the status field to RH_DONE and do notification callouts */
   /* if required.                                                 */

   pRLE->RqHdr.Status |= RH_DONE;

   ENABLE;

   pRLH = (PReq_List_Header) pRLE;
   OFFSETOF(pRLH) = OFFSETOF(pRLE) - (USHORT) pRLE->RqHdr.Head_Offset;

   /* If there's an error, see if we should notify on errors */

   if (pRLE->RqHdr.Status & RH_UNREC_ERROR)
   {
     /* Notify with error */

      pRLH->Lst_Status |= RLH_Unrec_Error;
      if (pRLE->RqHdr.Req_Control & (RH_NOTIFY_ERROR | RH_NOTIFY_DONE) )
         FSD_Notify((PBYTE)pRLE, pRLE->RqHdr.Notify_Address, 1);
   }
   else if (pRLE->RqHdr.Req_Control &  RH_NOTIFY_DONE)

         /* Notify with no with error */

         FSD_Notify((PBYTE)pRLE, pRLE->RqHdr.Notify_Address, 0);

   /* Increment the requests completed count in the request list header. */
   /* If all the requests in the list are done, or an error occurred,    */
   /* then call the list notification routine if required.               */

   pRLH->y_Done_Count ++;

   if (pRLH->y_Done_Count == pRLH->Count)
   {
      pRLH->Lst_Status &= ~(RLH_Req_Not_Queued | RLH_All_Req_Queued);
      pRLH->Lst_Status |= RLH_All_Req_Done;
      if (pRLH->Request_Control & RLH_Notify_Done)

         /* List notify when done */

         FSD_Notify((PBYTE)pRLH, pRLH->Notify_Address, 0);
   }
   else if ( (pRLE->RqHdr.Status & RH_UNREC_ERROR) &&
             (pRLH->Request_Control & RLH_Notify_Err) )

         /* List notify on error */

      FSD_Notify((PBYTE)pRLH, pRLH->Notify_Address, 1);
}


/*------------------------------------------------------------------------
;
;** DD_ChgPriority - Change the priority of a Request List Entry
;
;   Change the priority of a Request List Entry
;
;   USHORT DD_ChgPriority   (PPB_ReadWrite pRLE, UCHAR Priority)
;
;   ENTRY:    pRLE             - Request List Entry
;             Priority         - Priority
;
;   RETURN:   USHORT           - Return Code
;                                  0 = No error, priority changed
;                                  1 = Error, priority not changed since
;                                      request no longer on queue
; ------------------------------------------------------------------------*/
USHORT  DD_ChgPriority (pRLE, Priority)

PPB_Read_Write pRLE;
UCHAR  Priority;

{
   USHORT rc;
   PReq_List_Header pRLH;
   NPVOLCB pVolCB;

   /* If same priority as before then no need to change */

   if (pRLE->RqHdr.Priority == Priority)
      rc = 0;
   else if (pRLE->RqHdr.Status == RH_NOT_QUEUED)
   {
      pRLE->RqHdr.Priority = Priority;
      rc = 0;
   }
   else if (pRLE->RqHdr.Status != RH_QUEUED)
      rc = 1;

   else   /* Request is already queued.  Move to new Priority queue. */
   {
      pRLH = (PReq_List_Header) pRLE;
      OFFSETOF(pRLH) = OFFSETOF(pRLE) - (USHORT) pRLE->RqHdr.Head_Offset;
      Get_VolCB_Addr(pRLH->Block_Dev_Unit, (NPVOLCB FAR *)&pVolCB);
      if (RemovePriorityQueue(pVolCB->pUnitCB, pRLE) == NO_ERROR)
      {
         pRLE->RqHdr.Priority = Priority;
         PutPriorityQueue_RLE (pVolCB->pUnitCB, pRLE);
         rc = 0;
      }
      else
         rc = 1;
   }

   return(rc);
}

/*------------------------------------------------------------------------
;
;** DD_SetFSDInfo - Set FSD callout addresses.
;
;   Entry point for FSD to inform us of it's FSD_EndofInt and
;   FSD_AccValidate entry points.
;
;   USHORT DD_SetFSDInfo  (PSet_FSD_Info pFSDInfo)
;
;   ENTRY:    pFSDInfo         - FSD info structure
;
;   RETURN:   USHORT           - Return Code
;                                  0 = No error, FSD info changed
;                                  1 = Error, FSD info not changed
; ------------------------------------------------------------------------*/
USHORT _loadds FAR DD_SetFSDInfo ()

{
   FSDInfo FAR *pFSDInfo;
   USHORT rc;

   _asm { mov word ptr pFSDInfo[0], bx };
   _asm { mov word ptr pFSDInfo[2], es };

   /* Can only set FSDInfo callout addresses once. */

   if (pFSD_EndofInt != 0 || pFSD_AccValidate != 0)
   {
      rc = 1;
      _asm {stc};
   }
   else
   {
      pFSD_EndofInt = pFSDInfo->FSD_EndofInt;
      pFSD_AccValidate = pFSDInfo->FSD_AccValidate;
      rc = 0;
      _asm {clc};
   }

   return(rc);
}





