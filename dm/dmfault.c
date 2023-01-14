/*static char *SCCSID = "@(#)dmfault.c	6.5 92/02/06";*/
/*static char *SCCSID = "@(#)dmfault.c	6.5 92/02/06";*/
#define SCCSID  "@(#)dmfault.c	6.5 92/02/06"

/*--------------------------------------------------------
 *       (c) Copyright IBM Corporation 1981, 1990        *
 *                                                       *
 *       All Rights Reserved                             *
 *-------------------------------------------------------*/

#include "dmh.h"
#include "dmfault.h"

USHORT NEAR FT_Get_VolCB_Addr (USHORT, NPVOLCB FAR *);

/*------------------------------------------------------------------------
;
;** GIO_FaultTolerance - IOCTL to enable fault tolerance support
;
;   IOCTL to enable fault tolerance support
;
;   USHORT GIO_RWVTrack (NPCWA pCWA)
;
;   ENTRY:    pCWA             - CWA
;
;   RETURN:   USHORT           - Packet Status word
;
------------------------------------------------------------------------*/
USHORT GIO_FaultTolerance (pCWA)

NPCWA pCWA;
{
   PFT_IOCTL_param pParmPkt;
   PPDT   pPDT;
   USHORT TotalPartitions, i, rc;
   NPVOLCB pVolCB;

   if ((rc = CheckFTPacket(pCWA)) & STDON)
      return(rc);

   pParmPkt = (PFT_IOCTL_param) pCWA->pParmPkt;
   pPDT = pParmPkt->pPDT;

   TotalPartitions = NumPartitions + NumFTPartitions;


   /* Fill in the partition descriptor table header */

   ((PPDTH)pPDT)->NumControllers = (UCHAR) NumAdapters;
   ((PPDTH)pPDT)->NumPhysDisks =  (UCHAR) NumFixedDisks;
   ((PPDTH)pPDT)->NumPartitions = (UCHAR) TotalPartitions;
   ((PPDTH)pPDT)->NumLogUnits = (UCHAR) NumPartitions;
   ((PPDTH)pPDT)->PartNumOffset = 0;
   ((PPDTH)pPDT)->Reserved_1 = 0;
   ((PPDTH)pPDT)->Reserved_2 = 0;
   ((PPDTH)pPDT)->Reserved_3 = 0;
   ((PPDTH)pPDT)->Reserved_4 = 0;


   /* Fill in the partition decriptor entries for each logical drive */

   OFFSETOF (pPDT) = OFFSETOF (pPDT) + sizeof(PDTH);

   pVolCB = pVolCB_DriveC;

   for (i = 0; i < TotalPartitions; i++, pVolCB = pVolCB->pNextVolCB, pPDT++)
   {
      pPDT->Controller = (UCHAR) pVolCB->pUnitCB->AdapterNumber;
      pPDT->PhysDisk = (UCHAR) pVolCB->pUnitCB->PhysDriveNum;
      pPDT->PartitionType = pVolCB->PartitionType;
      pPDT->LogUnit = (UCHAR) pVolCB->LogDriveNum;
      if (pVolCB->Flags & vf_FTPartition)
         pPDT->LogUnit = -1;

      pPDT->StartSec = pVolCB->PartitionOffset+pVolCB->MediaBPB.HiddenSectors;

      if (pVolCB->MediaBPB.TotalSectors != 0)
         pPDT->EndSec = pPDT->StartSec + pVolCB->MediaBPB.TotalSectors - 1;
      else
         pPDT->EndSec = pPDT->StartSec + pVolCB->MediaBPB.BigTotalSectors - 1;

      pPDT->Reserved = 0;

      pVolCB->FT_PartitionNumber = i + 1;
   }


   /* Fill in the partition decriptor entries for each physical drive */

   for (i = 0; i < NumFixedDisks; i++, pVolCB = pVolCB->pNextVolCB, pPDT++)
   {
      pPDT->Controller = (UCHAR) pVolCB->pUnitCB->AdapterNumber;
      pPDT->PhysDisk = (UCHAR) pVolCB->pUnitCB->PhysDriveNum;
      pPDT->PartitionType = 0;
      pPDT->LogUnit = 0;
      pPDT->StartSec = 0;
      pPDT->EndSec = 0;
      pPDT->Reserved = 0;
   }

   if (pParmPkt->Command == FT_ENABLE)
   {
      pDiskFT_Request = (PVOID) pParmPkt->FT_Request;
      pDiskFT_Done = (PVOID) pParmPkt->FT_Done;
      DiskFT_DS = pParmPkt->FT_ProtDS;
      DDFlags |= DDF_FT_ENABLED;
   }
   else if (pParmPkt->Command == FT_DISABLE)
      DDFlags &= ~DDF_FT_ENABLED;

}

/*------------------------------------------------------------------------
;
;** CheckFTPacket - Check Fault Tolerance packets
;
;   USHORT CheckFTPacket (NPCWA pCWA)
;
;   ENTRY:    pCWA             - CWA
;
;   RETURN:   USHORT           - Packet Status word
;
------------------------------------------------------------------------*/
USHORT CheckFTPacket (pCWA)

NPCWA pCWA;
{
   USHORT rc, MaxPDTsize;
   PFT_IOCTL_param pParmPkt;
   PFT_IOCTL_data pDataPkt;
   PPDT pPDT;

   pParmPkt = (PFT_IOCTL_param) pCWA->pParmPkt;
   pDataPkt = (PFT_IOCTL_data) pCWA->pDataPkt;

   /* Verify access to the data and parameter packets */

   if ((rc = LockUserPacket(pCWA, LOCK_DATAPKT + LOCK_VERIFYONLY,
                            sizeof(FT_IOCTL_data))) & STERR)

      return(rc);


   if (rc = LockUserPacket(pCWA, LOCK_PARMPKT + LOCK_VERIFYONLY,
                            sizeof(FT_IOCTL_param)) & STERR)

      return(rc);


   /* Verify Access to the data block for the partition table         */
   /* Max number of PDT entries is: 1 for PDT header + max 24 logical */
   /* drives + number of fixed disks.                                 */

   pPDT = pParmPkt->pPDT;

   MaxPDTsize = (1 + 24 + NumFixedDisks) * sizeof (PDT);

   if (DevHelp_VerifyAccess
           (SELECTOROF(pPDT), MaxPDTsize, OFFSETOF(pPDT), 1) != 0)

      return(STDON + STERR + ERROR_I24_INVALID_PARAMETER);


   /* Check the signature, version and if any FT partitions exist */

   pDataPkt->FT_SigDD = FT_SIG_DD;

   if (pParmPkt->FT_SigFT != FT_SIG)
   {
      pDataPkt->SupportCode = FT_INVALID_SIGNATURE;
      return(STDON);
   }

   if (pParmPkt->FT_Version != FT_VERSION)
   {
      pDataPkt->SupportCode = FT_VERSION_INCOMPAT;
      return(STDON);
   }

   if (NumFTPartitions == 0)
      pDataPkt->SupportCode = FT_NO_FT_PARTITIONS;

   else
      pDataPkt->SupportCode = FT_SUPPORTED;

   return(0);
}


/*------------------------------------------------------------------------
;
;** FT_PutPriorityQueue - Put a request on a priority queue.
;
;   Put a Request Packet or Request List Entry on one of the priority
;   queues in the Unit Control Block for the specified unit.
;
;   VOID PutPriorityQueue  (NPUNITCB pUnitCB, PBYTE pRequest, pUnitCB1,
;                                                            pUnitCB2)
;
;   ENTRY:    pUnitCB          - Pointer to UnitCB
;             pRequest         - Pointer to Request Packet or Request List Entry
;             pUnitCB1         - returned pointer to UnitCB1
;             pUnitCB2         - returned pointer to UnitCB2
;
;
;   RETURN:   VOID
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
VOID FT_PutPriorityQueue (pUnitCBin, pRequest, pUnitCB1, pUnitCB2)

NPUNITCB pUnitCBin;
PBYTE    pRequest;
NPUNITCB FAR *pUnitCB1;
NPUNITCB FAR *pUnitCB2;


{
   FT_RESULTS FT_Results;
   UCHAR      ActionCode, Cmd, LogDriveNum;
   USHORT     SzReqPkt;
   PBYTE      pShadowReq;
   NPVOLCB    pSecVolCB;
   PFTDB      pFTDB;
   NPVOLCB    pVolCB;
   NPUNITCB   pUnitCB;
   PReq_List_Header pRLH;

   if ( ((PRPH)pRequest)->Cmd == PB_REQ_LIST )
   {
      pFTDB = &((PRHFT)pRequest)->ftdb;
      Cmd = ((PPB_Read_Write)pRequest)->RqHdr.Command_Code;
      SzReqPkt = ((((PPB_Read_Write)pRequest)->SG_Desc_Count) << 3) +
                 sizeof(Req_List_Header) + sizeof(PB_Read_Write);
      LogDriveNum = ((PRHFT)pRequest)->Block_Dev_Unit;
   }
   else
   {
      pFTDB = &((PRPFT)pRequest)->ftdb;
      Cmd = ((PRPH)pRequest)->Cmd;
      SzReqPkt = MAXRPSIZE;
      LogDriveNum = ((PRPH)pRequest)->Unit;
   }

   Get_VolCB_Addr(LogDriveNum, (NPVOLCB FAR *) &pVolCB);

   pUnitCB = pVolCB->pUnitCB;

   if (f_FT_Request( (pVolCB->FT_PartitionNumber << 8) + Cmd, SzReqPkt,
               (PBYTE FAR *)&FT_Results, (PBYTE FAR *)&ActionCode) != 0)

     /* ??????? Handle failure case */
     _asm{int 3};

   if (ActionCode != ACT_PRIMARY)
       FT_Get_VolCB_Addr( (USHORT)FT_Results.SecPartNum,
                          (NPVOLCB FAR *)&pSecVolCB);

   /* IF ACT_EITHER, do load balancing and put the request on the queue */
   /* with the least number of requests outstanding.                    */

   if (ActionCode == ACT_EITHER)
   {
      if ( (pUnitCB->NumReqsInProgress +
            pUnitCB->NumReqsWaiting)  <=
           (pSecVolCB->pUnitCB->NumReqsInProgress +
            pSecVolCB->pUnitCB->NumReqsWaiting) )

        ActionCode = ACT_PRIMARY_FIRST;

      else

        ActionCode = ACT_SECONDARY_FIRST;
   }

   /* Initialize FTDB area within the original request */

   pFTDB->FT_Flags = FTF_FT_REQUEST;
   pFTDB->OrigRequest.RequestHandle = FT_Results.RequestHandle;

   *pUnitCB1 = 0;
   *pUnitCB2 = 0;

   switch (ActionCode)
   {
      case ACT_PRIMARY:
         PutPriorityQueue (pUnitCB, pRequest);
         *pUnitCB1 = pUnitCB;
         break;

      case ACT_PRIMARY_FIRST:
         pFTDB->AltLogDriveNum = pSecVolCB->LogDriveNum;
         PutPriorityQueue (pUnitCB, pRequest);
         *pUnitCB1 = pUnitCB;
         break;

      case ACT_SECONDARY:
      case ACT_SECONDARY_FIRST:
         pFTDB->AltLogDriveNum = pVolCB->LogDriveNum;

         if ( ((PRPH)pRequest)->Cmd == PB_REQ_LIST )
            ((PRHFT)pRequest)->Block_Dev_Unit = pSecVolCB->LogDriveNum;
         else
         {
            ((PRP_RWV)pRequest)->rph.Unit = pSecVolCB->LogDriveNum;

            ((PRP_RWV)pRequest)->rba = ((PRP_RWV)pRequest)->rba -
               pVolCB->PartitionOffset - pVolCB->MediaBPB.HiddenSectors +
               pSecVolCB->PartitionOffset + pSecVolCB->MediaBPB.HiddenSectors;
         }

         PutPriorityQueue (pSecVolCB->pUnitCB, pRequest);
         *pUnitCB2 = pSecVolCB->pUnitCB;
         break;

      case ACT_BOTH:
         pFTDB->FT_Flags |= FTF_ACT_BOTH;
         pShadowReq = FT_Results.pShadowReq;

         if ( ((PRPH)pRequest)->Cmd == PB_REQ_LIST )
         {
            pRLH = (PReq_List_Header)pRequest;
            OFFSETOF(pRLH) = OFFSETOF(pRequest) -
                     (USHORT) (((PPB_Read_Write)pRequest)->RqHdr.Head_Offset);

            ((PReq_List_Header)pShadowReq)->Count = 1;
            ((PReq_List_Header)pShadowReq)->Request_Control = RLH_Single_Req;
            if (pRLH->Request_Control & RLH_Exe_Req_Seq)
              ((PReq_List_Header)pShadowReq)->Request_Control |= RLH_Exe_Req_Seq;

            ((PReq_List_Header)pShadowReq)->Lst_Status = 0;
            ((PReq_List_Header)pShadowReq)->Block_Dev_Unit =
                                            pSecVolCB->LogDriveNum;
            ((PReq_List_Header)pShadowReq)->y_Done_Count = 0;
            ((PReq_List_Header)pShadowReq)->y_PhysAddr = VirtToPhys(pRequest);

            OFFSETOF(pShadowReq) = OFFSETOF(pShadowReq)+sizeof(Req_List_Header);
            pFTDB = &((PRHFT)pShadowReq)->ftdb;
            f_BlockCopy(pShadowReq, pRequest, SzReqPkt - sizeof(Req_List_Header));
            ((PPB_Read_Write)pShadowReq)->RqHdr.Length = -1;
            ((PRHFT)pShadowReq)->Block_Dev_Unit = pSecVolCB->LogDriveNum;
            ((PPB_Read_Write)pShadowReq)->RqHdr.Head_Offset
                                          = sizeof(Req_List_Header);
            ((PPB_Read_Write)pShadowReq)->RqHdr.Req_Control = 0;
            ((PPB_Read_Write)pShadowReq)->RqHdr.Status = 0;
         }
         else
         {
            pFTDB = &((PRPFT)pShadowReq)->ftdb;

            f_BlockCopy(pShadowReq, pRequest, MAXRPSIZE);
            ((PRP_RWV)pShadowReq)->rph.Unit = pSecVolCB->LogDriveNum;
            ((PRP_RWV)pShadowReq)->rba = ((PRP_RWV)pRequest)->rba -
                pVolCB->PartitionOffset - pVolCB->MediaBPB.HiddenSectors +
                pSecVolCB->PartitionOffset + pSecVolCB->MediaBPB.HiddenSectors;
         }
         pFTDB->FT_Flags = FTF_FT_REQUEST | FTF_ACT_BOTH | FTF_SHADOW_REQUEST;
         pFTDB->pOrigRequest = pRequest;

         PutPriorityQueue (pUnitCB, pRequest);
         PutPriorityQueue (pSecVolCB->pUnitCB, pShadowReq);

         *pUnitCB1 = pUnitCB;
         *pUnitCB2 = pSecVolCB->pUnitCB;

   }
}



/*------------------------------------------------------------------------
;
;** FT_NotifyDoneIORB_RLE  - Done processing for fault tolerance requests
;
;   USHORT FT_NotifyDone (NPIORB pIORB, PPB_Read_Write pRequest,
;                                       PPB_Read_Write pOrigRequest)
;
;   ENTRY:    pIORB            - pointer to completed IORB
;             pRequest         - pointer to Request List Entry
;             pOrigRequest     - return pointer to original RLE
;
;   RETURN:   VOID
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
USHORT FT_NotifyDoneIORB_RLE (pIORB, pRequest, pOrigRequest)

NPIORB            pIORB;
PPB_Read_Write    pRequest;
PPB_Read_Write    FAR *pOrigRequest;

{
   USHORT  DoneParm1;
   USHORT  RequestHandle;
   USHORT  RC;
   NPVOLCB pVolCB;
   PFTDB   pFTDB;

   pFTDB = &((PRHFT)pRequest)->ftdb;
   Get_VolCB_Addr ( ((PRHFT)pRequest)->Block_Dev_Unit, (NPVOLCB FAR*)&pVolCB);
   DoneParm1 = pVolCB->FT_PartitionNumber << 8;

   if ( (pRequest->RqHdr.Status & 0xF0) == RH_NO_ERROR)
      (UCHAR) DoneParm1 = 0xFF;
   else
      (UCHAR) DoneParm1 = pRequest->RqHdr.Error_Code;

   if (pFTDB->FT_Flags & FTF_SHADOW_REQUEST)
   {
      (PBYTE) *pOrigRequest = pFTDB->pOrigRequest;
      RequestHandle = ((PRHFT)(*pOrigRequest))->ftdb.OrigRequest.RequestHandle;
   }
   else
   {
      *pOrigRequest = pRequest;
      RequestHandle = pFTDB->OrigRequest.RequestHandle;
   }

   RC = f_FT_Done(DoneParm1, RequestHandle, pRequest->Start_Block);


   if  (RC & STDON)
   {
      pRequest = *pOrigRequest;

      if (RC & STERR)
      {
         pRequest->RqHdr.Status = RH_UNREC_ERROR;
         if ( (UCHAR)RC >= I24_MIN_RECOV_ERROR )
            pRequest->RqHdr.Status = RH_RECOV_ERROR;
         pRequest->RqHdr.Error_Code = (UCHAR) RC;
      }
      else
      {
         pRequest->RqHdr.Status = RH_NO_ERROR;
         pRequest->RqHdr.Error_Code = 0;
      }
      return(STDON);
   }

  /* Request is not done.  For mirrored writes, no further work is  */
  /* needed. Just wait for the second write to complete.  For reads,*/
  /* re-queue the request to the alternate unit.                    */

  if ( !(pFTDB->FT_Flags & FTF_ACT_BOTH) )
  {
     Get_VolCB_Addr(pFTDB->AltLogDriveNum, (NPVOLCB FAR *)&pVolCB);
     ((PRHFT)pRequest)->Block_Dev_Unit = (UCHAR) pVolCB->LogDriveNum;
     PutPriorityQueue (pVolCB->pUnitCB, (PBYTE) pRequest);
  }
  return(0);
}


/*------------------------------------------------------------------------
;
;** FT_NotifyDoneIORB_RP  - Done processing for fault tolerance requests
;
;   USHORT FT_NotifyDoneIORB_RP (NPIORB pIORB,  PBYTE FAR *pRequest,
;                                               PBYTE FAR *pOrigRequest)
;
;   ENTRY:    pIORB            - pointer to completed IORB
;             pRequest         - pointer to Request Packet
;             pOrigRequest     - returned pointer to orig shadow RP
;
;   RETURN:   VOID
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
USHORT FT_NotifyDoneIORB_RP  (pIORB, pRequest, pOrigRequest)

NPIORB    pIORB;
PRP_RWV   pRequest;
PRP_RWV   FAR *pOrigRequest;

{
   USHORT  DoneParm1;
   USHORT  RequestHandle;
   ULONG   StartBlock;
   USHORT  RC;
   NPVOLCB pVolCB;
   PFTDB   pFTDB;

   pFTDB = &((PRPFT)pRequest)->ftdb;
   Get_VolCB_Addr(pRequest->rph.Unit, (NPVOLCB FAR *)&pVolCB);
   DoneParm1 = pVolCB->FT_PartitionNumber << 8;

   StartBlock = pRequest->rba - pVolCB->PartitionOffset -
                                pVolCB->MediaBPB.HiddenSectors;

   if (pRequest->rph.Status & STERR)
      (UCHAR) DoneParm1 = (UCHAR) pRequest->rph.Status;
   else
      (UCHAR) DoneParm1 = 0xff;

   if (pFTDB->FT_Flags & FTF_SHADOW_REQUEST)
   {
       (PBYTE) *pOrigRequest = pFTDB->pOrigRequest;
       RequestHandle = ((PRPFT)(*pOrigRequest))->ftdb.OrigRequest.RequestHandle;
   }
   else
   {
      *pOrigRequest = pRequest;
      RequestHandle = pFTDB->OrigRequest.RequestHandle;
   }

   RC = f_FT_Done(DoneParm1, RequestHandle, StartBlock);

   if  (RC & STDON)
   {
      pRequest = *pOrigRequest;
      pRequest->rph.Status = RC;
      return(STDON);
   }

  /* Request is not done.  For mirrored writes, no further work is  */
  /* needed. Just wait for the second write to complete.  For reads,*/
  /* re-queue the request to the alternate unit.                    */

  if ( !(pFTDB->FT_Flags & FTF_ACT_BOTH) )
  {
     Get_VolCB_Addr(pFTDB->AltLogDriveNum, (NPVOLCB FAR *)&pVolCB);
     pRequest->rph.Unit = pVolCB->LogDriveNum;
     pRequest->rba = StartBlock + pVolCB->PartitionOffset +
                                  pVolCB->MediaBPB.HiddenSectors;
     PutPriorityQueue (pVolCB->pUnitCB, (PBYTE) pRequest);
  }
  return(0);
}
/*------------------------------------------------------------------------
;
;** FT_CheckFTSupport  - See if request requires fault tolerance support
;
;   Check to see if the request requires fault tolerance support
;
;   USHORT    FT_CheckFTSupport (NPUNITCB pUnitCB, PBYTE pRequest)
;
;   ENTRY     pUnitCB     - Pointer to UnitCB
;             pRequest    - Pointer to Request Packet or Request List Entry
;
;   RETURN:   USHORT      - YES = FT required,
;                           NO = No FT required
;   EFFECTS:
;
------------------------------------------------------------------------*/
USHORT FT_CheckFTSupport (pUnitCB, pRequest)

NPUNITCB   pUnitCB;
PBYTE      pRequest;

{

   static UCHAR FTCmdTable[] =
   {
      CMDINPUT, CMDOUTPUT, CMDOUTPUTV,
      CMDInputBypass, CMDOutputBypass, CMDOutputBypassV,
      PB_READ_X, PB_WRITE_X, PB_WRITEV_X
   };

   UCHAR  Cmd;
   USHORT i;
   PReq_List_Header pRLH;

   /* See if FT is enabled */

   if ( !(DDFlags & DDF_FT_ENABLED) )
      return(NO);

   /* No FT support for removable media                           */

   if (pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE)
      return(NO);

   /* No FT support for I/O to physical drives, i.e. 0x80, etc)   */

   if ( ((PRPH)pRequest)->Cmd == PB_REQ_LIST )
   {
      SELECTOROF(pRLH) = SELECTOROF(pRequest);
      OFFSETOF(pRLH) = OFFSETOF(pRequest) -
              (USHORT) ((PPB_Read_Write)pRequest)->RqHdr.Head_Offset;

      if (pRLH->Block_Dev_Unit & 0x80)
         return(NO);

      Cmd = ((PPB_Read_Write)pRequest)->RqHdr.Command_Code;
   }
   else
   {
      if ( ((PRPH)pRequest)->Unit & 0x80 )
        return(NO);

      Cmd = ((PRPH)pRequest)->Cmd;
   }

   /* See if it's a command which requires FT support */

   for (i = 0; i < sizeof(FTCmdTable); i++)
   {
      if (Cmd == FTCmdTable[i])
         return(YES);
   }

   return(NO);
}


/*------------------------------------------------------------------------
;
;** FT_Get_VolCB_Addr - Return a pointer to a VolCB for the specified Drive
;
;   USHORT NEAR FT_Get_VolCB_Addr   (USHORT PartNum, NPVOLCB *pVolCB)
;
;   ENTRY:    ParmNum          - FT Partition Number
;             pVolCB           - returned pointer to VolCB
;
;   RETURN:   USHORT           - Result Code (NO_ERROR if VolCB found)
;
;   EFFECTS:
;
;   NOTES:
------------------------------------------------------------------------*/

USHORT FT_Get_VolCB_Addr (PartNum, pVolCB)

USHORT  PartNum;
NPVOLCB FAR *pVolCB;
{
   USHORT  found = FALSE;
   NPVOLCB pVolCBx;

   if ( !(DDFlags & DDF_NO_MEDIA) )
   {
      pVolCBx = VolCB_Head;

      while (pVolCBx != NULL && found == FALSE)
      {
         if (pVolCBx->FT_PartitionNumber == PartNum)
            found = TRUE;
         else
            pVolCBx = pVolCBx->pNextVolCB;
      }
   }

   if (found == TRUE)
   {
      *pVolCB = pVolCBx;
      return(NO_ERROR);
   }
   else
      return(ERROR);
}
