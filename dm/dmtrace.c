/*static char *SCCSID = "@(#)dmtrace.c	6.4 92/02/06";*/
/*static char *SCCSID = "@(#)dmtrace.c	6.4 92/02/06";*/
#define SCCSID  "@(#)dmtrace.c	6.4 92/02/06"

/*--------------------------------------------------------
 *       (c) Copyright IBM Corporation 1981, 1990        *
 *                                                       *
 *       All Rights Reserved                             *
 *-------------------------------------------------------*/

#include "dmh.h"
#include "infoseg.h"
#include "perfhook.h"

VOID   TraceStrat1Pre (USHORT, PBYTE, NPVOLCB);
VOID   TraceStrat1Post (USHORT, PBYTE);
VOID   TraceStrat2Pre (USHORT, PBYTE, NPVOLCB);
VOID   TraceStrat2Post (USHORT, PBYTE);
VOID   TraceIORBPre (USHORT, PBYTE);
VOID   TraceIORBPost (USHORT, PBYTE);
VOID   TraceIORBDekkoPre (USHORT, PBYTE);
USHORT GetTraceType (USHORT, PBYTE);
USHORT GetCmdString (USHORT);
VOID   DekkoTrace (USHORT, USHORT, PBYTE);
VOID   PerfViewTrace (USHORT, PBYTE, NPUNITCB);
VOID   TraceIORBInternal (USHORT, PBYTE);

typedef struct InfoSegGDT FAR *PInfoSegGDT;

typedef struct _CMD_TABLE_ENTRY
{
   USHORT Cmd;
   USHORT CmdType;
} CMD_TABLE_ENTRY;

typedef struct _TRACEENTRY
{
   UCHAR  TraceData[32];

} TRACEENTRY, FAR *PTRACEENTRY;
/*------------------------------------------------------------------------
;
;** IsTraceOn - Checks to see if RAS/DEKKO/PERFVIEW/Internal trace is
;               enabled and sets the appropriate flag bits in the
;               global TraceFlags variable.
;
;   USHORT IsTraceOn ()
;
;   ENTRY: VOID
;
;   RETURN:   USHORT            TraceFlags
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
USHORT IsTraceOn ()

{
   if (DDFlags & DDF_INIT_TIME)
      return(0);

   TraceFlags &= ~(TF_DEKKO | TF_RAS | TF_PERFVIEW);

   /* First see if DEKKO tracing is required */

   if ( TEST_TRACING(pSIS_mec_table, DEKKO_MAJOR_DISK) )
      TraceFlags |= TF_DEKKO;

   /* Else see if RAS Tracing is required */

   else if ( TEST_RAS_TRACING(pSIS_mec_table, RAS_MAJOR_DISK) )
      TraceFlags |= TF_RAS;

   /* See if PerfView Tracing is enabled */

   if (pVolCB_80->pUnitCB->PerfViewDB.pfdbh.dbh_pfnTmrAdd != 0)
      TraceFlags |= TF_PERFVIEW;

   return(TraceFlags);
}

/*------------------------------------------------------------------------
;
;** Trace - RAS/DEKKO/PERFVIEW/Internal Tracing Functions
;
;   Checks to see if RAS/DEKKO/PERFVIEW/Internal Tracing is enabled
;   and performs the requested tracing function.
;
;   USHORT Trace (USHORT TraceFlags, PBYTE pRequest, NPVOLCB pVolCB)
;
;   ENTRY:    TraceEvent       - Trace Event Flags
;             pRequest         - Request to trace
;             pVolCB           - Trace Point
;
;   RETURN:   VOID
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
VOID Trace (TraceEvent, pRequest, pVolCB)

USHORT  TraceEvent;
PBYTE   pRequest;
NPVOLCB pVolCB;

{
   USHORT TraceType;

   if (DDFlags & DDF_INIT_TIME)
      return;

   if (TraceEvent & TRACE_STRAT1)
   {
      if ( TraceFlags & (TF_DEKKO | TF_RAS) )
      {
         if ((TraceType = GetTraceType(TraceFlags, pRequest)) == 0)
            return;

         TraceEvent |= TraceType;

         if (TraceEvent & TRACE_ENTRY)
            TraceStrat1Pre(TraceEvent, pRequest, pVolCB);
         else
            TraceStrat1Post(TraceEvent, pRequest);
      }
   }
   else if (TraceEvent & TRACE_STRAT2)
   {
      if ( TraceFlags & (TF_DEKKO | TF_RAS) )
      {
         if (TraceEvent & TRACE_ENTRY)
            TraceStrat2Pre(TraceEvent, pRequest, pVolCB);
         else
            TraceStrat2Post(TraceEvent, pRequest);
      }
   }
   else if (TraceEvent & TRACE_IORB)
   {
      if (TraceFlags & TF_INTERNAL)
         TraceIORBInternal(TraceEvent, pRequest);

      if ((TraceType = GetTraceType(TraceEvent, pRequest)) == 0)
         return;

      TraceEvent |= TraceType;


      if (TraceFlags & TF_PERFVIEW)
         PerfViewTrace (TraceEvent, pRequest, (NPUNITCB) pVolCB);

      if ( TraceFlags & (TF_DEKKO | TF_RAS) )
      {
         if (TraceEvent & TRACE_ENTRY)
            TraceIORBPre(TraceEvent, pRequest);
         else
            TraceIORBPost(TraceEvent, pRequest);
      }
   }
}

/*------------------------------------------------------------------------
;
;** TraceStrat1Pre - Trace Strategy-1 Pre-Invocation Request
;
;   VOID TraceStrat1Request (USHORT TraceEvent, PBYTE pRequest, pVolCB)
;
;   ENTRY:    TraceEvent       - Trace Flags
;             pRequest         - Request to trace
;             pVolCB           - Pointer to VolCB
;
;   RETURN:   VOID
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
VOID TraceStrat1Pre (TraceEvent, pRequest, pVolCB)

USHORT  TraceEvent;
PBYTE   pRequest;
NPVOLCB pVolCB;

{
   UCHAR  TraceBuffer[sizeof(TCB)];
   PTCB   pTraceBuffer = (PTCB) TraceBuffer;
   USHORT MinorCode = 0;
   PBYTE  pPkt;

   if (((PRPH)pRequest)->Flags & RPF_Internal)
      return;

   pTraceBuffer->pRequest = pRequest;
   pTraceBuffer->Unit = pVolCB->pUnitCB->PhysDriveNum;

   if ( ((PRPH)pRequest)->Unit <= 26)
      (USHORT) pTraceBuffer->Drive[0] = (USHORT) (((PRPH)pRequest)->Unit + 'A');
   else
      (USHORT) pTraceBuffer->Drive[0] = ' ';
   pTraceBuffer->Drive[1] = 0;

   pTraceBuffer->CommandCode = ((PRPH)pRequest)->Cmd;
   (USHORT) (pTraceBuffer->CmdString[0]) = GetCmdString(TraceEvent);
   pTraceBuffer->CmdString[2]=0;
   pTraceBuffer->RequestControl=0;
   pTraceBuffer->Priority = 3;
   pTraceBuffer->Flags=0;
   pTraceBuffer->cSGList = 1;
   pTraceBuffer->pRLH = 0;

   pTraceBuffer->RBA = ((PRP_RWV)pRequest)->rba + pVolCB->PartitionOffset +
                         pVolCB->MediaBPB.HiddenSectors;

   pTraceBuffer->BlockCount = (ULONG) ((PRP_RWV)pRequest)->NumSectors;

   MinorCode = RAS_MINOR_STRAT1_RWV;

   if (TraceEvent & TRACE_IOCTL)
   {
       MinorCode = RAS_MINOR_IOCTL_RWVF;
       pTraceBuffer->CommandCode = ((PRP_GENIOCTL)pRequest)->Category;
       pTraceBuffer->CommandModifier = ((PRP_GENIOCTL)pRequest)->Function;

       if ( ((PRP_GENIOCTL)pRequest)->Category == 9)
       {
          pTraceBuffer->Unit |= 0x80;
          pTraceBuffer->Drive[0]=' ';
       }

       pPkt = ((PRP_GENIOCTL)pRequest)->ParmPacket;
       pTraceBuffer->RBA = (ULONG) (((PDDI_RWVPacket_param)pPkt)->Cylinder +
                           (((PDDI_RWVPacket_param)pPkt)->Head <<16));

       pTraceBuffer->BlockCount = ((PDDI_RWVPacket_param)pPkt)->NumSectors;

       if (TraceEvent & TRACE_FORMAT)
       {
           pPkt = ((PRP_GENIOCTL)pRequest)->ParmPacket;

           if ( ((PDDI_FormatPacket_param)pPkt)->NumTracks != 0)
           {
               pTraceBuffer->BlockCount =
                          (ULONG) ((PDDI_FormatPacket_param)pPkt)->NumTracks;
               pTraceBuffer->CmdString[0]='M';
           }
       }
   }

   if (TraceFlags & TF_RAS)
      DevHelp_RAS(RAS_MAJOR_DISK, MinorCode, sizeof(TCB), (PBYTE) pTraceBuffer);
   else
      DekkoTrace(MinorCode, sizeof(TCB), (PBYTE) pTraceBuffer);

   return;
}
/*------------------------------------------------------------------------
;
;** TraceStrat1Post - Trace Strategy-1 Post-Invocation Request
;
;   VOID TraceStrat1Post (USHORT TraceEvent, PBYTE pRequest)
;
;   ENTRY:    TraceEvent       - Trace Flags
;             pRequest         - Request to trace
;
;   RETURN:   VOID
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
VOID TraceStrat1Post (TraceEvent, pRequest)

USHORT TraceEvent;
PBYTE  pRequest;

{
   UCHAR  TraceBuffer[sizeof(TCBD)];
   PTCBD  pTraceBuffer = (PTCBD) TraceBuffer;
   USHORT MinorCode = 0;

   if (((PRPH)pRequest)->Flags & (RPF_TraceComplete | RPF_Internal))
      return;

   ((PRPH)pRequest)->Flags |= RPF_TraceComplete;

   pTraceBuffer->pRequest = pRequest;
   pTraceBuffer->Status = (USHORT) (((PRPH)pRequest)->Status >> 8);
   pTraceBuffer->ErrorCode = (USHORT) (((PRPH)pRequest)->Status & 0x00FF);
   pTraceBuffer->BlocksXferred = (ULONG) ((PRP_RWV)pRequest)->NumSectors;

   MinorCode = RAS_MINOR_STRAT1_RWV;
   if ( ((PRPH)pRequest)->Cmd == CMDGenIOCTL )
      MinorCode = RAS_MINOR_IOCTL_RWVF;

   if (TraceFlags & TF_RAS)
   {
     MinorCode |= 0x8000;
     DevHelp_RAS(RAS_MAJOR_DISK, MinorCode, sizeof(TCBD), (PBYTE) pTraceBuffer);
   }
   else
   {
     MinorCode |= 0x0080;
     DekkoTrace(MinorCode, sizeof(TCBD), (PBYTE) pTraceBuffer);
   }
}



/*------------------------------------------------------------------------
;
;** TraceStrat2Pre - Trace Strategy-2 Pre-Invocation
;
;   VOID TraceStrat2Pre  (USHORT TraceEvent, PBYTE pRequest, pVolCB)
;
;   ENTRY:    TraceEvent       - Trace flags
;             pRequest         - Request to trace
;             pVolCB           - pointer to VolCB
;
;   RETURN:   VOID
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
VOID TraceStrat2Pre (TraceEvent, pRequest, pVolCB)

USHORT     TraceEvent;
PBYTE      pRequest;
NPVOLCB    pVolCB;

{
   UCHAR  TraceBuffer[sizeof(TCB)];
   PTCB   pTraceBuffer = (PTCB) TraceBuffer;
   USHORT i, Count, TraceType;
   UCHAR  UnitChar;
   PPB_Read_Write  pPkt;

   /* Trace Request List Header */

   ((PTRLHS)pTraceBuffer)->pRLH = pRequest;
   ((PTRLHS)pTraceBuffer)->Count =  ((PReq_List_Header)pRequest)->Count;
   ((PTRLHS)pTraceBuffer)->Unit = pVolCB->pUnitCB->PhysDriveNum;

   UnitChar = ' ';
   if ( ((PReq_List_Header)pRequest)->Block_Dev_Unit <= 26 )
     UnitChar=(UCHAR)(((PReq_List_Header)pRequest)->Block_Dev_Unit+(UCHAR)'A');

   (USHORT) ((PTRLHS)pTraceBuffer)->Drive[0] = (USHORT) UnitChar;

   ((PTRLHS)pTraceBuffer)->Request_Control =
                            ((PReq_List_Header)pRequest)->Request_Control;

   if (TraceFlags & TF_RAS)
     DevHelp_RAS(RAS_MAJOR_DISK, RAS_MINOR_STRAT2_RLH, sizeof(TRLHS),
                                                 (PBYTE) pTraceBuffer);
   else
     DekkoTrace(RAS_MINOR_STRAT2_RLH, sizeof(TRLHS), (PBYTE) pTraceBuffer);

   /* Trace each request entry */

   Count = ((PReq_List_Header)pRequest)->Count;
   pPkt = (PPB_Read_Write) ((PBYTE)pRequest+sizeof(Req_List_Header));

   for (i = 0; i < Count; i++)
   {
      pTraceBuffer->Unit = pVolCB->pUnitCB->PhysDriveNum;
      (USHORT) pTraceBuffer->Drive[0] = (USHORT) UnitChar;
      pTraceBuffer->CommandCode = pPkt->RqHdr.Command_Code;

      if ((TraceType = GetTraceType(TraceEvent, (PBYTE) pPkt)) == 0)
         return;
      TraceEvent |= TraceType;

      (USHORT) (pTraceBuffer->CmdString[0]) = GetCmdString(TraceEvent);
      pTraceBuffer->CmdString[2]=0;

      pTraceBuffer->RequestControl = pPkt->RqHdr.Req_Control;
      pTraceBuffer->Priority = pPkt->RqHdr.Priority;
      pTraceBuffer->pRequest = (PBYTE) pPkt;
      pTraceBuffer->pRLH = OFFSETOF(pRequest);
      pTraceBuffer->cSGList = pPkt->SG_Desc_Count;
      pTraceBuffer->RBA = pPkt->Start_Block + pVolCB->PartitionOffset +
                                 pVolCB->MediaBPB.HiddenSectors;
      pTraceBuffer->BlockCount = pPkt->Block_Count;

      OFFSETOF(pPkt) = OFFSETOF(pPkt) + pPkt->RqHdr.Length;

      if (TraceFlags & TF_RAS)
         DevHelp_RAS(RAS_MAJOR_DISK, RAS_MINOR_STRAT2_RLE,
                                     sizeof(TCB), (PBYTE) pTraceBuffer);
      else
         DekkoTrace(RAS_MINOR_STRAT2_RLE, sizeof(TCB), (PBYTE) pTraceBuffer);
   }
}
/*------------------------------------------------------------------------
;
;** TraceStrat2Post - Trace Strategy-2 Post-Invocation
;
;   VOID TraceStrat2Post (USHORT TraceEvent, PBYTE pRequest)
;
;   ENTRY:    TraceEvent       - Trace flags
;             pRequest         - Request to trace
;
;   RETURN:   VOID
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
VOID TraceStrat2Post (TraceEvent, pRequest)

USHORT     TraceEvent;
PBYTE      pRequest;

{
   UCHAR  TraceBuffer[sizeof(TCBD)];
   PTCBD  pTraceBuffer = (PTCBD) TraceBuffer;

   pTraceBuffer->pRequest = pRequest;
   pTraceBuffer->Status = (USHORT)((PPB_Read_Write)pRequest)->RqHdr.Status;
   pTraceBuffer->ErrorCode=(USHORT)((PPB_Read_Write)pRequest)->RqHdr.Error_Code;
   pTraceBuffer->BlocksXferred = ((PPB_Read_Write)pRequest)->Blocks_Xferred;

   if (TraceFlags & TF_RAS)
     DevHelp_RAS(RAS_MAJOR_DISK, RAS_MINOR_STRAT2_RLE | 0x8000,
                                 sizeof(TCBD), (PBYTE) pTraceBuffer);
   else
     DekkoTrace(RAS_MINOR_STRAT2_RLE | 0x80, sizeof(TCBD), (PBYTE)pTraceBuffer);


   OFFSETOF(pRequest) = OFFSETOF (pRequest) -
              (USHORT) ((PPB_Read_Write)pRequest)->RqHdr.Head_Offset;

   /* If we're done with the list, then trace the RLH post */

   if ( ((PReq_List_Header)pRequest)->Lst_Status & RLH_All_Req_Done )
   {
      ((PTRLHD)pTraceBuffer)->pRLH = pRequest;
      ((PTRLHD)pTraceBuffer)->DoneCount =
                              ((PReq_List_Header)pRequest)->y_Done_Count;
      ((PTRLHD)pTraceBuffer)->Status =
                            (USHORT) ((PReq_List_Header)pRequest)->Lst_Status;

      if (TraceFlags & TF_RAS)
        DevHelp_RAS(RAS_MAJOR_DISK, RAS_MINOR_STRAT2_RLH | 0x8000,
                                 sizeof(TRLHD), (PBYTE) pTraceBuffer);
      else
        DekkoTrace(RAS_MINOR_STRAT2_RLH | 0x80,sizeof(TRLHD),
                                          (PBYTE)pTraceBuffer);
   }
}


/*------------------------------------------------------------------------
;
;** TraceIORBPre  - Trace IORB Pre_Invocation Request
;
;   VOID TraceIORBPre (USHORT TraceEvent, PBYTE pRequest)
;
;   ENTRY:    TraceEvent       - Trace Flags
;             pRequest         - Request to trace
;
;   RETURN:   VOID
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
VOID TraceIORBPre (TraceEvent, pRequest)

USHORT     TraceEvent;
PBYTE      pRequest;

{
   UCHAR  TraceBuffer[sizeof(TCB)];
   PTCB   pTraceBuffer = (PTCB) TraceBuffer;
   PBYTE  pPkt;
   USHORT TraceType;

TraceNextIORB:
   (NPIORB_DMWORK) pPkt = (NPIORB_DMWORK) &(((NPIORB)pRequest)->DMWorkSpace[0]);
   pTraceBuffer->pRequest = ((NPIORB_DMWORK)pPkt)->pRequest;
   pTraceBuffer->Unit = ((NPIORB_DMWORK)pPkt)->pUnitCB->PhysDriveNum;
   pTraceBuffer->RequestControl = ((NPIORB)pRequest)->RequestControl;
   pTraceBuffer->CommandCode = (UCHAR) ((NPIORB)pRequest)->CommandCode;
   pTraceBuffer->CommandModifier= (UCHAR) ((NPIORB)pRequest)->CommandModifier;


   if ((TraceType = GetTraceType(TraceEvent, pRequest)) == 0)
      return;
   TraceEvent |= TraceType;

   (USHORT) (pTraceBuffer->CmdString[0]) = GetCmdString(TraceEvent);
   pTraceBuffer->CmdString[2]=0;

    switch( ((NPIORB)pRequest)->CommandCode )
    {
       case IOCC_EXECUTE_IO:
          pTraceBuffer->cSGList = ((NPIORB_EXECUTEIO)pRequest)->cSGList;
          pTraceBuffer->RBA = ((NPIORB_EXECUTEIO)pRequest)->RBA;
          pTraceBuffer->BlockCount=((NPIORB_EXECUTEIO)pRequest)->BlockCount;
          break;

       case IOCC_FORMAT:
          pTraceBuffer->cSGList = ((NPIORB_FORMAT)pRequest)->cSGList;
          (PBYTE) pPkt = ((NPIORB_FORMAT)pRequest)->pFormatCmd;
          pTraceBuffer->RBA = ((NPFORMAT_CMD_TRACK)pPkt)->RBA;
          pTraceBuffer->BlockCount =
                        (ULONG) ((NPFORMAT_CMD_TRACK)pPkt)->cTrackEntries;
          pTraceBuffer->Flags = (UCHAR) ((NPFORMAT_CMD_TRACK)pPkt)->Flags;
          break;

    }
    if (TraceFlags & TF_RAS)
     DevHelp_RAS(RAS_MAJOR_DISK,RAS_MINOR_IORB,sizeof(TCB),(PBYTE)pTraceBuffer);
    else
     DekkoTrace(RAS_MINOR_IORB, sizeof(TCB), (PBYTE) pTraceBuffer);

    if  ( ((NPIORB)pRequest)->RequestControl & IORB_CHAIN )
    {
       pRequest = (PBYTE) ((NPIORB)pRequest)->pNxtIORB;
       goto TraceNextIORB;
    }
}
/*------------------------------------------------------------------------
;
;** TraceIORBDekkoPre  - Trace IORB Dekko Pre_Invocation Request
;
;   VOID TraceIORBDekkoPre (USHORT TraceEvent, PBYTE pRequest)
;
;   ENTRY:    TraceEvent       - Trace Flags
;             pRequest         - Request to trace
;
;   RETURN:   VOID
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
VOID TraceIORBDekkoPre (TraceEvent, pRequest)

USHORT     TraceEvent;
PBYTE      pRequest;

{
   UCHAR  TraceBuffer[4];
   PTCB   pTraceBuffer = (PTCB) TraceBuffer;
   PBYTE  pPkt;

   (NPIORB_DMWORK) pPkt = (NPIORB_DMWORK) &(((NPIORB)pRequest)->DMWorkSpace[0]);
   pTraceBuffer->pRequest = ((NPIORB_DMWORK)pPkt)->pRequest;
   DekkoTrace(RAS_MINOR_IORB, sizeof(TraceBuffer), (PBYTE) pTraceBuffer);

}


/*------------------------------------------------------------------------
;
;** TraceIORBPost - Trace IORB Post_Invocation Request
;
;   VOID TraceIORBPost (USHORT TraceEvent, PBYTE pRequest, pCB)
;
;   ENTRY:    TraceEvent       - Trace Flags
;             pRequest         - Request to trace
;
;   RETURN:   VOID
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
VOID TraceIORBPost (TraceEvent, pRequest)

USHORT     TraceEvent;
PBYTE      pRequest;

{
   UCHAR  TraceBuffer[sizeof(TCBD)];
   PTCBD  pTraceBuffer = (PTCBD) TraceBuffer;
   PBYTE  pPkt;

   (NPIORB_DMWORK) pPkt = (NPIORB_DMWORK) &(((NPIORB)pRequest)->DMWorkSpace[0]);
   pTraceBuffer->pRequest = ((NPIORB_DMWORK)pPkt)->pRequest;
   pTraceBuffer->Status = ((NPIORB)pRequest)->Status;
   pTraceBuffer->ErrorCode = ((NPIORB)pRequest)->ErrorCode;

   pTraceBuffer->BlocksXferred = ((NPIORB_EXECUTEIO)pRequest)->BlocksXferred;

   if (TraceFlags & TF_RAS)
      DevHelp_RAS(RAS_MAJOR_DISK, RAS_MINOR_IORB | 0x8000,
                                     sizeof(TCBD), (PBYTE) pTraceBuffer);
   else
      DekkoTrace(RAS_MINOR_IORB | 0x80, sizeof(TCBD), (PBYTE) pTraceBuffer);
}
/*------------------------------------------------------------------------
;
;** TraceIORBInternal  - Internal IORB Trace
;
;   VOID TraceIORBInternal (USHORT TraceEvent, PBYTE pRequest)
;
;   ENTRY:    TraceEvent       - Trace Flags
;             pRequest         - Request to trace
;
;   RETURN:   VOID
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
VOID TraceIORBInternal (TraceEvent, pRequest)

USHORT TraceEvent;
PBYTE  pRequest;

{
   NPITCB pTraceRec;
   PBYTE  pPkt;

   PUSHFLAGS;
   DISABLE;

   pTraceRec = (NPITCB) pDMTraceHead;

   (NPIORB_DMWORK) pPkt = (NPIORB_DMWORK) &(((NPIORB)pRequest)->DMWorkSpace[0]);
   pTraceRec->Unit = ((NPIORB_DMWORK)pPkt)->pUnitCB->PhysDriveNum;
   pTraceRec->pIORB = OFFSETOF(pRequest);
   pTraceRec->CommandModifier = (UCHAR) ((NPIORB)pRequest)->CommandModifier;
   pTraceRec->CommandCode = (UCHAR) ((NPIORB)pRequest)->CommandCode;
   pTraceRec->Status =  ((NPIORB)pRequest)->Status;
   pTraceRec->ErrorCode = ((NPIORB)pRequest)->ErrorCode;

   if ( ((NPIORB)pRequest)->CommandCode == IOCC_EXECUTE_IO )
   {
      pTraceRec->rba = ((NPIORB_EXECUTEIO)pRequest)->RBA;
      pTraceRec->BlockCount = ((NPIORB_EXECUTEIO)pRequest)->BlockCount;
   }
   else
   {
      pTraceRec->rba = 0;
      pTraceRec->BlockCount = 0;
   }

   pTraceRec->Event = RAS_MINOR_IORB;
   if (TraceEvent & TRACE_ASYNCDONE)
      pTraceRec->Event |= 0x80;

   pDMTraceHead += sizeof(ITCB);

   if (pDMTraceHead >= pDMTraceEnd)
      pDMTraceHead = pDMTraceBuf;

   POPFLAGS;
}


/*------------------------------------------------------------------------
;
;** GetCmdString - Get command code string
;
;   USHORT GetCmdString (USHORT TraceEvent)
;
;   ENTRY:    TraceEvent      - TraceFlags
;
;   RETURN:   USHORT          - Two char ASCII command code
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
USHORT GetCmdString (TraceEvent)

USHORT TraceEvent;

{
   USHORT Cmd;

   Cmd = ' ' + ' '*256;
   if (TraceEvent & TRACE_READ)
       Cmd = 'R' + ' '*256;
   if (TraceEvent & TRACE_WRITE)
       Cmd = 'W' + ' '*256;
   if (TraceEvent & TRACE_FORMAT)
       Cmd = 'F' + ' '*256;

   if (TraceEvent & TRACE_VERIFY)
       (UCHAR) Cmd = 'V';
   else if (TraceEvent & TRACE_PREFETCH)
       (UCHAR) Cmd = 'P';

   return(Cmd);

}

/*------------------------------------------------------------------------
;
;** GetTraceType - Get Trace Type
;
;   USHORT GetTraceType (USHORT TraceEvent, pRequest)
;
;   ENTRY:    TraceEvent      - Trace Flags
;             pRequest        - Request
;
;   RETURN:   USHORT          - Trace Flags
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
USHORT GetTraceType (TraceEvent, pRequest)

USHORT TraceEvent;
PBYTE  pRequest;

{
   static CMD_TABLE_ENTRY CmdTable[] =
   {
      {PB_REQ_LIST*256 + PB_READ_X,            TRACE_READ},
      {PB_REQ_LIST*256 + PB_WRITE_X,           TRACE_WRITE},
      {PB_REQ_LIST*256 + PB_WRITEV_X,          TRACE_WRITE + TRACE_VERIFY},
      {PB_REQ_LIST*256 + PB_PREFETCH_X,        TRACE_READ + TRACE_PREFETCH},
      {IOCC_EXECUTE_IO*256+IOCM_READ,          TRACE_READ},
      {IOCC_EXECUTE_IO*256+IOCM_WRITE,         TRACE_WRITE},
      {IOCC_EXECUTE_IO*256+IOCM_WRITE_VERIFY,  TRACE_WRITE | TRACE_VERIFY},
      {IOCC_EXECUTE_IO*256+IOCM_READ_VERIFY,   TRACE_READ | TRACE_VERIFY},
      {IOCC_EXECUTE_IO*256+IOCM_READ_PREFETCH, TRACE_READ | TRACE_PREFETCH},
      {IOCC_FORMAT*256+IOCM_FORMAT_TRACK,      TRACE_FORMAT | TRACE_VERIFY},
      {CMDINPUT,         TRACE_READ},
      {CMDInputBypass,   TRACE_READ},
      {CMDOUTPUT,        TRACE_WRITE},
      {CMDOutputBypass,  TRACE_WRITE},
      {CMDOUTPUTV,       TRACE_WRITE | TRACE_VERIFY},
      {CMDOutputBypassV, TRACE_WRITE | TRACE_VERIFY},
      {CMDGenIOCTL*256 + IODC_RT, TRACE_IOCTL | TRACE_READ},
      {CMDGenIOCTL*256 + IODC_WT, TRACE_IOCTL | TRACE_WRITE},
      {CMDGenIOCTL*256 + IODC_VT, TRACE_IOCTL | TRACE_READ | TRACE_VERIFY},
      {CMDGenIOCTL*256 + IODC_FT, TRACE_IOCTL | TRACE_FORMAT | TRACE_VERIFY},
   };

   USHORT Cmd, i;

   if (TraceEvent & TRACE_STRAT1)
   {
      Cmd = (USHORT) ((PRPH)pRequest)->Cmd;
      if (Cmd == CMDGenIOCTL)
          Cmd = (USHORT) ((Cmd << 8) + ((PRP_GENIOCTL)pRequest)->Function);
   }
   else if (TraceEvent & TRACE_STRAT2)
      Cmd=(USHORT)((PB_REQ_LIST*256)+((PPB_Read_Write)pRequest)->RqHdr.Command_Code);
   else if (TraceEvent & TRACE_IORB)
      Cmd = ((((PIORB)pRequest)->CommandCode) << 8) +
             ((PIORB)pRequest)->CommandModifier;

   for (i = 0; i < sizeof(CmdTable)/sizeof(CMD_TABLE_ENTRY); i++)
      if (CmdTable[i].Cmd == Cmd)
         return(CmdTable[i].CmdType);


  return(0);
}





/*------------------------------------------------------------------------
;
;** DekkoTrace - Perform Dekko tracing
;
;   VOID DekkoTrace (USHORT MinorCode, USHORT TraceSize, PBYTE pTraceBuffer)
;
;   ENTRY:    MinorCode        - Minor Code
;             TraceSize        - Size of data to trace
;             pTraceBuffer     - Pointer to trace buffer
;
;   RETURN:   VOID
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
VOID DekkoTrace (MinorCode, TraceSize, pTraceBuffer)

USHORT MinorCode;
USHORT TraceSize;
PBYTE  pTraceBuffer;

{
   struct Dekko_Addr *pDekko;
   USHORT i;

   SELECTOROF(pDekko) = ((PInfoSegGDT)pSysInfoSeg)->SIS_MMIOBase;
   OFFSETOF(pDekko) = 0;

   pDekko->majmin_code = DEKKO_MAJOR_DISK * 256 + MinorCode;

   for (i = 0; i < TraceSize/4; i++)
   {
      pDekko->perf_data_4byte = (ULONG) pTraceBuffer[i];
   }
}

/*------------------------------------------------------------------------
;
;** PerfViewTrace - Perform perfview tracing
;
;   VOID PerfViewTrace (USHORT TraceEvent,  PBYTE pRequest, NPVOLCB pUnitCB)
;
;   ENTRY:    TraceEvent       - Trace Flags
;             pRequest         - Pointer to request
;             pUnitCB          - Pointer to UnitCB
;
;   RETURN:   VOID
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
VOID PerfViewTrace (TraceEvent, pRequest, pUnitCB)

USHORT   TraceEvent;
PBYTE    pRequest;
NPUNITCB pUnitCB;

{
   USHORT i, Count, TraceType;
   NPPVDB pPerfViewDB;

   pPerfViewDB = &(pUnitCB->PerfViewDB);

   pPerfViewDB->pfdbh.dbh_ulSem++;

   if ( (pPerfViewDB->pfdbh.dbh_pfnTmrSub == 0) ||
        (pPerfViewDB->pfdbh.dbh_pfnTmrAdd == 0) ||
        (pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE) )

     goto PerfRet;

   if ( (TraceEvent & TRACE_IORB) && (TraceEvent & TRACE_ENTRY) )
   {
      if (TraceEvent & TRACE_READ)
      {
         pPerfViewDB->NumReads++;
         pPerfViewDB->ReadBytes +=
                           (ULONG)((NPIORB_EXECUTEIO)pRequest)->BlockCount;
         pPerfViewDB->pfdbh.dbh_pfnTmrSub((PTIMR)&pPerfViewDB->ReadTime);
      }
      else if (TraceEvent & TRACE_WRITE)
      {
         pPerfViewDB->NumWrites++;
         pPerfViewDB->WriteBytes +=
                           (ULONG)((NPIORB_EXECUTEIO)pRequest)->BlockCount;
         pPerfViewDB->pfdbh.dbh_pfnTmrSub((PTIMR)&pPerfViewDB->WriteTime);
      }
   }
   else if ( (TraceEvent & TRACE_IORB) && (TraceEvent & TRACE_ASYNCDONE))
   {
      if (TraceEvent & TRACE_READ)
         pPerfViewDB->pfdbh.dbh_pfnTmrAdd((PTIMR)&pPerfViewDB->ReadTime);
      else if (TraceEvent & TRACE_WRITE)
         pPerfViewDB->pfdbh.dbh_pfnTmrAdd((PTIMR)&pPerfViewDB->WriteTime);
   }

PerfRet:
   pPerfViewDB->pfdbh.dbh_ulSem--;
}
