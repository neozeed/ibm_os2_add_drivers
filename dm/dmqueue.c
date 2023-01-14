/*static char *SCCSID = "@(#)dmqueue.c	6.7 92/02/06";*/
#define SCCSID  "@(#)dmqueue.c	6.7 92/02/06"

/*-------------------------------------------------------*
 *       (c) Copyright IBM Corporation 1981, 1990        *
 *                                                       *
 *       All Rights Reserved                             *
 *-------------------------------------------------------*/

#include "dmh.h"
#include "dmfault.h"

USHORT NotifyDoneIORB_RLE (NPIORB, PPB_Read_Write, NPUNITCB);
VOID NotifyDoneIORB_RP (NPIORB, PBYTE, NPUNITCB);

/*------------------------------------------------------------------------
;
;** PutPriorityQueue - Put a Request List Entry or RP on a priority queue
;
;   Put a RLE or a RP on one of the priority queues in
;   the Unit Control Block for the specified unit.
;
;   VOID PutPriorityQueue  (NPUNITCB pUnitCB, PBYTE pReq)
;
;   ENTRY:    pUnitCB          - Pointer to UnitCB
;             pReq             - Pointer to Request Packet or Request List Entry
;
;   RETURN:   VOID
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
VOID PutPriorityQueue (pUnitCB, pReq)

NPUNITCB pUnitCB;
PBYTE    pReq;
{
   if ( ((PRPH)pReq)->Cmd == PB_REQ_LIST)
       PutPriorityQueue_RLE (pUnitCB, (PPB_Read_Write) pReq);
   else
       PutPriorityQueue_RP (pUnitCB, pReq);
}

/*------------------------------------------------------------------------
;
;** PutPriorityQueue_RLE - Put a Request List Entry on a priority queue.
;
;   Put a Request List Entry on one of the priority queues in
;   the Unit Control Block for the specified unit.
;
;   VOID PutPriorityQueue_RLE  (NPUNITCB pUnitCB, PPB_Read_Write pRLE)
;
;   ENTRY:    pUnitCB          - Pointer to UnitCB
;             pRLE             - Pointer to Request List Entry
;
;   RETURN:   VOID
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
VOID PutPriorityQueue_RLE (pUnitCB, pRLE)

NPUNITCB pUnitCB;
PPB_Read_Write  pRLE;

{
   PRTYQ *pPrtyQ;

   PUSHFLAGS;
   DISABLE;

   /* For request list entries, put on a priority queue based on the   */
   /* priority value in the request list entry.                        */

   if (pRLE->RqHdr.Status != RH_NOT_QUEUED)
     goto PutRLE_Ret;

   pRLE->RqHdr.Status = RH_QUEUED;
   pRLE->RqHdr.Waiting = 0;

   pPrtyQ =
        (PRTYQ *) (&(pUnitCB->PrtyQRLE[GetPrtyQIndex(pRLE->RqHdr.Priority)]));

   /* If the queue is empty then stick the request at the head */

   if (pPrtyQ->Head == 0)
   {
      pPrtyQ->Head = (PBYTE) pRLE;
      pPrtyQ->Tail = (PBYTE) pRLE;
   }
   else
   {

      if (DDFlags & DDF_ELEVATOR_DISABLED)
      {
         /* Put request at end of queue */

         ((PPB_Read_Write)(pPrtyQ->Tail))->RqHdr.Waiting = (ULONG) pRLE;
         pPrtyQ->Tail = (PBYTE) pRLE;
      }
      else
      {
         /* Sort on queue via an elevator algorithm */

         SortPriorityQueue (pPrtyQ, (PBYTE) pRLE);
      }
   }

   pUnitCB->NumReqsWaiting++;
   NumReqsWaiting++;

   ENABLE;

PutRLE_Ret:
   POPFLAGS;

}

/*------------------------------------------------------------------------
;
;** PutPriorityQueue_RP - Put a request packet on a priority queue.
;
;   Put a Request Packet on one of the priority queues in the
;   Unit Control Block for the specified unit.
;
;   VOID PutPriorityQueue_RP  (NPUNITCB pUnitCB, PBYTE pReq)
;
;   ENTRY:    pUnitCB          - Pointer to UnitCB
;             pReq             - Pointer to Request Packet
;
;   RETURN:   VOID
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
VOID PutPriorityQueue_RP (pUnitCB, pReq)

NPUNITCB pUnitCB;
PBYTE    pReq;

{
   PRTYQ *pPrtyQ;

   PUSHFLAGS;
   DISABLE;

   /* Put a request packet on the priority queue for request packets.    */
   /* The first request packet priority queue is for read/writes/verifys */
   /* which may require elevator sorting.  The second request packet     */
   /* queue is for requests that should not be elevator sorted, i.e.     */
   /* "Get Media Sense", etc.                                            */

   ((PRPH)pReq)->Link = 0;

   if ( ((PRPH)pReq)->Cmd != CMDInternal)
      pPrtyQ = &(pUnitCB->PrtyQRP[0]);
   else
   {
      if (((PRP_INTERNAL)pReq)->Function == DISKOP_READ_VERIFY)
         pPrtyQ = &(pUnitCB->PrtyQRP[0]);
      else
         pPrtyQ = &(pUnitCB->PrtyQRP[1]);
   }

   /* If the queue is empty then stick request at the head */

   if (pPrtyQ->Head == 0)
   {
      pPrtyQ->Head = pReq;
      pPrtyQ->Tail = pReq;
   }
   else
   {
      /* Either sort the request or put at end of queue */

      if ( (pPrtyQ == &(pUnitCB->PrtyQRP[1])) ||
           (DDFlags & DDF_ELEVATOR_DISABLED) )
      {
         /* Put request at end of queue */

         ((PRPH)(pPrtyQ->Tail))->Link = (PRPH) pReq;
         pPrtyQ->Tail = pReq;
      }
      else
      {
         /* Sort on queue via an elevator algorithm */

         SortPriorityQueue (pPrtyQ, pReq);
      }
   }

   pUnitCB->NumReqsWaiting++;
   NumReqsWaiting++;

   ENABLE;
   POPFLAGS;

}

/*------------------------------------------------------------------------
;
;** PullPriorityQueue - Pull a request from a priority queue.
;
;   Pull the highest priority request from the priority queues
;   for the specified unit.  The request returned can either be
;   a request packet of a request list entry.
;
;   USHORT PullPriorityQueue  (NPUNITCB pUnitCB, PBYTE *pReqAddr)
;
;   ENTRY:    pUnitCB          - Pointer to UnitCB
;             pReqAddr         - returned pointer to Request Packet or
;                                Request List entry
;
;   RETURN:   USHORT           - = 0, Request pulled
;                                <> 0, No request pulled, queues empty
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
USHORT PullPriorityQueue (pUnitCB, pReqAddr)

NPUNITCB pUnitCB;
PBYTE    FAR *pReqAddr;

{
   USHORT i, j, rc;
   PBYTE  pReq;
   USHORT PullRequest = 0;

   /* Priority queues are ordered with the highest priority queue as the */
   /* first queue in the array.  The request packet queues are searched  */
   /* after the PRIO_FOREGROUND priority queue for request list entries  */

   PUSHFLAGS;
   DISABLE;

   /* Make sure there's a request waiting to be processed before  */
   /* searching the queues.                                       */

   if (pUnitCB->NumReqsWaiting == 0)
   {
      rc = ERROR;
      goto PullRet;
   }

   /* Search the Request List Priority Queues First  */

   for (i = 0; i < NUM_RLE_QUEUES; i++)
   {
      if (pUnitCB->PrtyQRLE[i].Head != 0)
      {
         PullRequest = 1;
         break;
      }
   }

   /* If not found, or priority queue above 4 (Foreground I/O) found, */
   /* then search the request packet queues.                          */

   if (i > 4)
   {
      for (j = 0; j < NUM_RP_QUEUES; j++)
      {
         if (pUnitCB->PrtyQRP[j].Head != 0)
         {
           PullRequest = 2;
           break;
         }
      }
   }

   /* If PullRequest == 1, then pull a request list entry from the queue */

   if (PullRequest == 1)
   {
      pReq = pUnitCB->PrtyQRLE[i].Head;
      (ULONG) pUnitCB->PrtyQRLE[i].Head=((PPB_Read_Write)pReq)->RqHdr.Waiting;
      ((PPB_Read_Write)pReq)->RqHdr.Waiting = 0;
      if (pUnitCB->PrtyQRLE[i].Head == 0)
         pUnitCB->PrtyQRLE[i].Tail = 0;
   }

   /* If PullRequest = 2, then pull a request packet entry from the queue */

   else if (PullRequest == 2)
   {
      pReq = pUnitCB->PrtyQRP[j].Head;
      pUnitCB->PrtyQRP[j].Head = (PBYTE) (((PRPH)pReq)->Link);
      ((PRPH)pReq)->Link = 0;
      if (pUnitCB->PrtyQRP[j].Head == 0)
         pUnitCB->PrtyQRP[j].Tail = 0;
   }

   /* If request pulled, then return address of request, else return error */

   if (PullRequest != 0)
   {
      *pReqAddr = pReq;
      pUnitCB->NumReqsWaiting--;
      NumReqsWaiting--;
      rc = NO_ERROR;
   }
   else
      rc = ERROR;

PullRet:
   POPFLAGS;
   return(rc);
}

/*------------------------------------------------------------------------
;
;** RemovePriorityQueue - Remove an entry from a priority queue.
;
;   This routine removes a specific Request List entry from one of
;   the priority queues.
;
;   USHORT RemovePriorityQueue  (NPUNITCB pUnitCB, PPB_Read_Write pRLE)
;
;   ENTRY:    pUnitCB          - Pointer to UnitCB
;             pRLE             - Request List Entry
;
;   RETURN:   USHORT           - = 0, Request removed
;                                <> 0, No request removed, entry not found
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
USHORT RemovePriorityQueue (pUnitCB, pRLE)

NPUNITCB pUnitCB;
PPB_Read_Write  pRLE;

{
   PPB_Read_Write  pCurReq, pPrevReq;
   USHORT PrtyQIndex, rc;
   USHORT Removed = NO;

   /* First make sure the entry is queued and the queue is not empty */

   PUSHFLAGS;
   DISABLE;

   if (pRLE->RqHdr.Status & RH_QUEUED)
   {
      PrtyQIndex = GetPrtyQIndex(pRLE->RqHdr.Priority);

      if (pUnitCB->PrtyQRLE[PrtyQIndex].Head != 0)
      {
         /* See if entry is at the head of the queue */

         if (pUnitCB->PrtyQRLE[PrtyQIndex].Head = (PBYTE) pRLE)
         {
            pUnitCB->PrtyQRLE[PrtyQIndex].Head = (PBYTE) pRLE->RqHdr.Waiting;
            if (pRLE->RqHdr.Waiting == 0)
               pUnitCB->PrtyQRLE[PrtyQIndex].Tail = 0;
            else
               pRLE->RqHdr.Waiting = 0;
            Removed = YES;
         }
         else   /* Entry is not at the head, so search for it */
         {
            (PBYTE) pCurReq = pUnitCB->PrtyQRLE[PrtyQIndex].Head;
            while (pCurReq != 0 && Removed == NO)
            {
               if (pCurReq = pRLE)
               {
                  pPrevReq->RqHdr.Waiting = pCurReq->RqHdr.Waiting;
                  if (pUnitCB->PrtyQRLE[PrtyQIndex].Tail = (PBYTE) pCurReq)
                     (ULONG) pUnitCB->PrtyQRLE[PrtyQIndex].Tail =
                               ((PPB_Read_Write)pCurReq)->RqHdr.Waiting;
                   Removed = YES;
               }
               pPrevReq = pCurReq;
               if (pCurReq->RqHdr.Waiting != 0)
                  pCurReq = (PPB_Read_Write) pCurReq->RqHdr.Waiting;
            }
         }
      }
   }

   if (Removed == YES)
   {
      pRLE->RqHdr.Status = RH_NOT_QUEUED;
      rc = 0;
   }
   else
     rc = 1;

   ENABLE;
   POPFLAGS;
   return(rc);

}
/*------------------------------------------------------------------------
;
;** PurgePriorityQueues - Purge Priority Queues
;
;   Purge all requests from all priority queues for a specifed unit.
;
;   USHORT PurgePriorityQueues (NPUNITCB pUnitCB, UCHAR ErrorCode)
;
;   ENTRY:    pUnitCB          - Pointer to UnitCB
;             ErrorCode        - error code stored in Request Packet
;                                or Request List prior to notifying caller
;
;   RETURN:
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
VOID PurgePriorityQueues (pUnitCB, ErrorCode)

NPUNITCB pUnitCB;
UCHAR    ErrorCode;

{
   PBYTE pRequest;
   USHORT AwakeCount;

   while (PullPriorityQueue(pUnitCB, (PBYTE FAR *) &pRequest) == NO_ERROR)
   {
      if ( ((PRPH)pRequest)->Cmd == PB_REQ_LIST )
      {
         ((PPB_Read_Write)pRequest)->Blocks_Xferred = 0;
         ((PPB_Read_Write)pRequest)->RqHdr.Status = RH_UNREC_ERROR;
         ((PPB_Read_Write)pRequest)->RqHdr.Error_Code, ErrorCode;
         NotifyRLE((PPB_Read_Write)pRequest);
      }
      else
      {
         ((PRPH)pRequest)->Status = STDON + STERR + ERROR_I24_UNCERTAIN_MEDIA;
         ((PRP_RWV)pRequest)->NumSectors = 0;

         if (TraceFlags != 0)
            Trace(TRACE_STRAT1 | TRACE_ASYNCDONE, (PBYTE)pRequest,
                                                  pUnitCB->pCurrentVolCB);

         /* If internal request came from us, then issue PRUN */

         if (((PRPH)pRequest)->Flags & RPF_Internal)
            DevHelp_ProcRun((ULONG)pRequest, &AwakeCount);

         else
         {
            /* Normal request from file system, issue DevHelp_DevDone */

            DevHelp_DevDone((PBYTE) pRequest);
         }
      }
   }
}


/*------------------------------------------------------------------------
;
;** GetPrtyQIndex - Get priority queue index
;
;   This function converts an input priority value to an index
;   into the priority queue array in the UnitCB.
;
;   USHORT GetPrtyQIndex  (UCHAR Priority)
;
;   ENTRY:    Priority         - Priority flag value
;
;   RETURN:   USHORT           - Index into priority array
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
USHORT GetPrtyQIndex (Priority)

UCHAR Priority;

{
   USHORT i;

   /* If priority queuing disabled, return highest priority queue index */

   if (DDFlags & DDF_PRTYQ_DISABLED)
      return(0);

   /* Lowest priority value (i.e. 0) is the last queue */

   if (Priority == 0)
      return(NUM_RLE_QUEUES - 1);

   for (i = NUM_RLE_QUEUES - 1; i > 0; i--)
   {
      Priority >>= 1;
      if (Priority == 0)
         return(i-1);
   }
   return(5);           /* Set to background user if invalid priority */


}


/*------------------------------------------------------------------------
;
;** SubmitRequestsToADD - Submit Requests to an Adapter Device Driver
;
;   This function submits requests in priority order to the Adapter
;   Device Driver which manages this unit.  Requests are pulled from
;   the priority waiting queues, IORBs are built and submitted to the
;   ADD.  IORBs will be chained together is the ADD supports a queuing
;   count > 1 and the recommended queuing limit has not been exceded.
;
;   USHORT SubmitRequestsToADD (pUnitCB)
;
;   ENTRY:    pUnitCB          - pointer to unit control block
;
;   RETURN:   VOID
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
VOID SubmitRequestsToADD (pUnitCB)

NPUNITCB pUnitCB;

{
   NPIORB pIORB, pPrevIORB, pFirstIORB;
   PBYTE  pReq;
   USHORT i;
   USHORT NumIORBsBuilt = 0;

   PUSHFLAGS;
   DISABLE;

   /* If the number of requests already submitted to the ADD is     */
   /* less than the ADD's recommended queuing count, then submit    */
   /* additional requests.                                          */

   for (i=pUnitCB->NumReqsInProgress; i < pUnitCB->UnitInfo.QueuingCount; i++)
   {

      /* Allocate an IORB.  If one is available, then pull the next     */
      /* highest priority request from the unit's priority queues.      */

      if (AllocIORB(pUnitCB, (NPIORB FAR *) &pIORB) != NO_ERROR)
         break;

      if (PullPriorityQueue (pUnitCB, (PBYTE FAR *)&pReq) != NO_ERROR)
      {
         FreeIORB(pUnitCB, pIORB);
         break;
      }

      /* We've pulled another request from the queue, so build the IORB    */
      /* for it and chain it to the previous if we've built more than one. */

      pUnitCB->NumReqsInProgress++;
      NumReqsInProgress++;

      ENABLE;

      SetupIORB(pUnitCB, pReq, pIORB);

      NumIORBsBuilt++;

      if (NumIORBsBuilt == 1)
         pFirstIORB = pIORB;
      else
      {
         pPrevIORB->pNxtIORB = (PVOID) pIORB;
         pPrevIORB->RequestControl |= IORB_CHAIN;
      }
      pPrevIORB = pIORB;
      DISABLE;
   }

   ENABLE;
   POPFLAGS;

   /* If at least one IORB has been built, then send the IORB chain */
   /* (or single IORB) to the ADD.                                  */

   if (NumIORBsBuilt > 0)
   {
      if (TraceFlags != 0)
         Trace(TRACE_IORB | TRACE_ENTRY, (PBYTE)pFirstIORB,
                                          pUnitCB);

      (pUnitCB->AdapterDriverEP) ((PVOID) pFirstIORB);
   }
}


/*------------------------------------------------------------------------
;
;** NotifyDoneIORB - Notify routine when IORB is done in ADD
;
;   This function is the notification routine called by an Adapter Device
;   Driver when an IORB is done.  Error status and blocks transferred
;   are updated in the associated Request Packet or Request List entry.  If
;   the request is a Strategy-2 Request List Entry, the file system is
;   notified that the request has completed.  If the request is a
;   Strategy-1 Request Packet, the thread waiting on the request is woken up.
;
;   VOID NotifyDoneIORB (PIORB fpIORB)
;
;   ENTRY:    fpIORB            - far pointer to completed IORB
;
;   RETURN:   VOID
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
VOID _loadds FAR NotifyDoneIORB (fpIORB)

PIORB fpIORB;

{
   PRPH pReq;
   NPUNITCB pUnitCB;
   NPIORB pIORB;
   NPIORB_DMWORK pDMWork;

   pIORB = (NPIORB) (OFFSETOF(fpIORB));
   pDMWork = (NPIORB_DMWORK) &(pIORB->DMWorkSpace);
   pUnitCB = (NPUNITCB) pDMWork->pUnitCB;

   /* Call the file system's end of interrupt routine */

   if (pFSD_EndofInt != 0)
      FSD_EndofInt();

   if (TraceFlags != 0)
      Trace(TRACE_IORB | TRACE_ASYNCDONE, (PBYTE) pIORB, pUnitCB);
   else if (pIORB->Status & IORB_ERROR)
      TraceIORBInternal(TRACE_IORB | TRACE_ASYNCDONE, (PBYTE) pIORB);

   if ( ((PRPH)pDMWork->pRequest)->Cmd == PB_REQ_LIST)
      NotifyDoneIORB_RLE(pIORB, (PPB_Read_Write) pDMWork->pRequest, pUnitCB);
   else
   {
      NotifyDoneIORB_RP (pIORB, (PBYTE) pDMWork->pRequest, pUnitCB);
   }
}

/*------------------------------------------------------------------------
;
;** NotifyDoneIORB_RLE - Notify routine when IORB for a RLE is done
;
;   This function is called when the ADD notifies us that a
;   Request List entry is done.  Error status and blocks transferred
;   are updated in the Request List Entry.  The file system is
;   notified that the request has completed.
;
;   USHORT NotifyDoneIORB_RLE (NPIORB pIORB, PPB_Read_Write pRLE)
;
;   ENTRY:    pIORB            - pointer to completed IORB
;             pRLE             - pointer to Request List Entry
;             pUnitCB          - pointer to UnitCB
;
;   RETURN:   USHORT           - Next Event Code
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
USHORT NotifyDoneIORB_RLE (pIORB, pRLE, pUnitCB)

NPIORB pIORB;
PPB_Read_Write  pRLE;
NPUNITCB pUnitCB;

{
   PReq_List_Header pRLH;
   PPB_Read_Write   pRLEOrig;
   NPIORB_DMWORK pDMWork;
   USHORT NextEvent = 0;
   USHORT FT_Status = 0;
   NPVOLCB pVolCB;

   pDMWork = (NPIORB_DMWORK) &(pIORB->DMWorkSpace);

   pRLH = (PReq_List_Header) pRLE;
   OFFSETOF(pRLH) = OFFSETOF(pRLE) - (USHORT) (pRLE->RqHdr.Head_Offset);

   /* Set the error status and error code and call the file system */
   /* notification routine.                                        */

   if (pIORB->Status & IORB_ERROR)
   {
      if (pIORB->Status & IORB_RECOV_ERROR)
         pRLE->RqHdr.Status = RH_RECOV_ERROR;
      else
      {
         pRLE->RqHdr.Status = RH_UNREC_ERROR;
         pRLE->Blocks_Xferred = ((NPIORB_EXECUTEIO)pIORB)->BlocksXferred;
      }

      pRLE->RqHdr.Error_Code = MapIORBError(pIORB->ErrorCode);
   }
   else
   {
      pRLE->RqHdr.Status = RH_NO_ERROR;
      pRLE->RqHdr.Error_Code = 0;
      pRLE->Blocks_Xferred = ((NPIORB_EXECUTEIO)pIORB)->BlockCount;
   }

   pRLE->Blocks_Xferred = ((NPIORB_EXECUTEIO)pIORB)->BlocksXferred;

   if ( (DDFlags & DDF_FT_ENABLED) &&
        ( ((PRHFT)pRLE)->ftdb.FT_Flags & FTF_FT_REQUEST) )
   {
       if ((FT_Status = FT_NotifyDoneIORB_RLE(pIORB, pRLE,
                        (PPB_Read_Write FAR *)&pRLEOrig)) & STDON)
       {
          pRLE = pRLEOrig;
          pRLH = (PReq_List_Header) pRLE;
          OFFSETOF(pRLH) = OFFSETOF(pRLE) - (USHORT) (pRLE->RqHdr.Head_Offset);
       }
       else
          goto RLE_Cleanup;
   }
   NotifyRLE (pRLE);

   if (TraceFlags != 0)
      Trace(TRACE_STRAT2 | TRACE_ASYNCDONE, (PBYTE) pRLE, (NPVOLCB) 0);

   /* If Uncertain Media error or Abort List on Error specified, */
   /* then abort the entire request list.                        */

   if (pRLE->RqHdr.Status & RH_UNREC_ERROR)
   {

      if (pRLE->RqHdr.Error_Code == ERROR_I24_UNCERTAIN_MEDIA)
      {
         pUnitCB->pCurrentVolCB->Flags |= vf_UncertainMedia;

         PurgePriorityQueues(pUnitCB, ERROR_I24_UNCERTAIN_MEDIA);
      }
      else if ( (pRLH->Lst_Status & RLH_Abort_Err) &&
                (pRLH->Lst_Status & RLH_Exe_Req_Seq) )
      {
         pRLH->Lst_Status |= RLH_Abort_pendings;
         AbortReqList(pRLH);
      }
   }

   /* Clean up request */

RLE_Cleanup:
   FreeIORB(pUnitCB, pIORB);

   PUSHFLAGS;
   DISABLE;
   pUnitCB->NumReqsInProgress --;
   NumReqsInProgress--;
   POPFLAGS;

   /* If Execute in Sequence specified, then queue the next request in */
   /* in the list and submit it for processing.                        */

   if ( (pRLH->Request_Control & RLH_Exe_Req_Seq) &&
        ( !(pRLH->Lst_Status & RLH_All_Req_Done)) )
   {
      if (DDFlags & DDF_FT_ENABLED)
      {
         if (FT_Status & STDON)
         {
            Get_VolCB_Addr(pRLH->Block_Dev_Unit, (NPVOLCB FAR *)&pVolCB);
            FT_ExecReqList(pRLH, pVolCB->pUnitCB);
         }
      }
      else
         ExecReqList(pRLH, pUnitCB);        /* Execute in sequence case */
   }
   else

   /*  Pull the next request from the priority queues and submit it */

      SubmitRequestsToADD(pUnitCB);         /* Normal case */

}


/*------------------------------------------------------------------------
;
;** NotifyDoneIORB_RP - Notify routine when an IORB for a RP is done
;
;   This function is called when the ADD notifies us that a
;   Request Packet is done.  Error status and blocks transferred
;   are updated in the Request Packet.  The thread waiting on
;   the request is woken up.
;
;   VOID NotifyDoneIORB_RP (PIORB pIORB, PBYTE pRP)
;
;   ENTRY:    pIORB            - pointer to completed IORB
;             pRP              - pointer to Request Packet
;             pUnitCB          - pointer to UnitCB
;
;   RETURN:   VOID
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
VOID NotifyDoneIORB_RP (pIORB, pRP, pUnitCB)

NPIORB pIORB;
PBYTE pRP;
NPUNITCB pUnitCB;

{
   USHORT RP_Status;
   USHORT AwakeCount;
   NPIORB_DMWORK pDMWork;
   PBYTE  pRPOrig;

   pDMWork = (NPIORB_DMWORK) &(pIORB->DMWorkSpace);

   RP_Status = STDON;

   /* Set the packet status word and wake up the thread waiting    */
   /* on completition of this request.  Recovered errors are       */
   /* treated as no error, since Request Packets dont handle       */
   /* recovered error conditions.                                  */

   if (pIORB->Status & IORB_ERROR)
   {
      if ( !(pIORB->Status & IORB_RECOV_ERROR) )
      {
         RP_Status |= STERR;

         (UCHAR) RP_Status = MapIORBError(pIORB->ErrorCode);

         if (pIORB->CommandCode == IOCC_EXECUTE_IO)
           ((PRP_RWV)pRP)->NumSectors =
                            (USHORT) ((PIORB_EXECUTEIO)pIORB)->BlocksXferred;

         if ((UCHAR) RP_Status == ERROR_I24_UNCERTAIN_MEDIA)
         {
            pUnitCB->pCurrentVolCB->Flags |= vf_UncertainMedia;

            PurgePriorityQueues(pUnitCB, ERROR_I24_UNCERTAIN_MEDIA);
         }
      }
   }

   /* Set the status word and blocks transferred count in the RP */

   ((PRPH)pRP)->Status = RP_Status;

   if ( (DDFlags & DDF_FT_ENABLED) &&
        ( ((PRPFT)pRP)->ftdb.FT_Flags & FTF_FT_REQUEST ) )
   {
       if (FT_NotifyDoneIORB_RP(pIORB, pRP, (PRPH FAR *)&pRPOrig) & STDON)
          pRP = pRPOrig;
       else
          goto RP_Cleanup;
   }

   if (pIORB->CommandCode == IOCC_UNIT_STATUS)
      ((PRP_INTERNAL)pRP)->RetStatus = ((NPIORB_UNIT_STATUS)pIORB)->UnitStatus;

   /* Wake up the thread waiting on completion of this request  */

   if (TraceFlags != 0)
      Trace(TRACE_STRAT1 | TRACE_ASYNCDONE, (PBYTE) pRP, (NPVOLCB) 0);


   /* If internal request came from us, then issue ProcRun, */
   /* else issue DevDone for normal file system requests.   */

   if (((PRPH)pRP)->Flags & RPF_Internal)
      DevHelp_ProcRun((ULONG)pRP, &AwakeCount);
   else
      DevHelp_DevDone((PBYTE) pRP);


   /* Clean up request and submit next request for processing */

   if (DDFlags & DDF_INIT_TIME)          /* No cleanup if at init time */
      return;

RP_Cleanup:
   FreeIORB(pUnitCB, pIORB);

   PUSHFLAGS;
   DISABLE;
   pUnitCB->NumReqsInProgress --;
   NumReqsInProgress--;
   POPFLAGS;

   SubmitRequestsToADD(pUnitCB);         /* Normal case */

}



