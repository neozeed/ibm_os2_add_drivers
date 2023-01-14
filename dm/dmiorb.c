/*static char *SCCSID = "@(#)dmiorb.c	6.5 92/02/06";*/
/*static char *SCCSID = "@(#)dmiorb.c	6.5 92/02/06";*/
#define SCCSID  "@(#)dmiorb.c	6.5 92/02/06"

/*--------------------------------------------------------
 *       (c) Copyright IBM Corporation 1981, 1990        *
 *                                                       *
 *       All Rights Reserved                             *
 *-------------------------------------------------------*/

#include "dmh.h"
#include "dmfault.h"


/*------------------------------------------------------------------------
;
;** InitCBPool - Initialize the control block pool
;
;   Initializes the control block pool which is used for IORB,
;   CWA and FTDB allocation.
;
;   USHORT InitCBPool ()
;
;   ENTRY:
;
;   RETURN:
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
VOID InitCBPool ()
{
   NPUNITCB pUnitCB;
   NPBYTE   pCB;
   NPIORBH  pIORB;
   USHORT   NumCBs, CBSize, i;
   USHORT   NumIORBs = 0;

   /*--------------------------------*/
   /* Calculate the size of the Pool */
   /*--------------------------------*/

   /* The size of a control block is the max size of an IORB, CWA and FTDB */

   CBSize = MAX_IORB_SIZE;
   if (CBSize < sizeof(CWA))
      CBSize = sizeof(CWA);


   /* Calculate the number of IORBs to allocate by summing the */
   /* Queuing Counts in each UnitCB.                           */

   for (pUnitCB = UnitCB_Head, i = 0; i < NumUnitCBs; i++, pUnitCB++)
   {
     if ( (pUnitCB->UnitInfo.QueuingCount > MAX_QUEUING_COUNT) ||
          (pUnitCB->UnitInfo.QueuingCount == 0) )
        NumIORBs += MAX_QUEUING_COUNT;
     else
        NumIORBs += pUnitCB->UnitInfo.QueuingCount;
   }

   NumCBs = NumIORBs + NUM_DEFAULT_CWAS;   /* add in room for CWAs */

   FreePoolSpace = (USHORT) &InitData - (USHORT) pNextFreeCB;

   /*  Make sure Pool Size isnt greater than available space left in */
   /*  the data segment.                                             */

   if ( ((ULONG)NumCBs * CBSize) < (ULONG)FreePoolSpace)
      PoolSize = NumCBs * CBSize;
   else
   {
      NumCBs = FreePoolSpace / CBSize;
      PoolSize = NumCBs * CBSize;
   }

   /*-----------------------------------*/
   /* Initialize the Control Block Pool */
   /*-----------------------------------*/

   CB_FreeList = pNextFreeCB;

   pCB = CB_FreeList;

   for (i = 0; i < NumCBs; i++)
   {

      ((NPIORBH) pCB)->pNxtIORB = (PIORBH) (pCB + CBSize);
      pCB += CBSize;
   }

   ((NPIORBH) (pCB - CBSize))->pNxtIORB = 0;    /* Zero terminate list */

   pNextFreeCB = pCB;


   /*------------------------------------------*/
   /* Allocate one dedicated IORB to each Unit */
   /*------------------------------------------*/

   pCB = CB_FreeList;

   for (pUnitCB = UnitCB_Head, i = 0; i < NumUnitCBs; i++, pUnitCB++)
   {
      pUnitCB->pDedicatedIORB = (NPIORBH) pCB;
      ((NPIORBH) pCB)->pNxtIORB = 0;
      pCB += CBSize;
   }

   CB_FreeList = pCB;            /* Free List now starts past dedicated IORBs */


}



typedef struct _ReqToIORB_CmdEntry
{
   UCHAR  PktCommand;
   UCHAR  PktFunction;
   USHORT IORBCommandCode;
   USHORT IORBCommandModifier;
} ReqToIORB_CmdEntry;

/*------------------------------------------------------------------------
;
;** SetupIORB - Setup an IORB
;
;   Sets ups an IORB from the input Request Packet or Request List Entry
;
;   VOID SetupIORB  (NPUNITCB pUnitCB, PBYTE pReq, pIORB)
;
;   ENTRY:    pUnitCB          - Pointer to UnitCB
;             pReq             - Pointer to Request Packet or Request List Entry
;             pIORB            - Pointer to IORB
;
;   RETURN:
;
;   EFFECTS:
;
------------------------------------------------------------------------*/

VOID   SetupIORB (pUnitCB, pReq, pIORB)

NPUNITCB pUnitCB;
PBYTE    pReq;
NPIORBH  pIORB;

{

static ReqToIORB_CmdEntry ReqToIORB_CmdTable[] = {

{PB_REQ_LIST, PB_READ_X,     IOCC_EXECUTE_IO, IOCM_READ},
{PB_REQ_LIST, PB_WRITE_X,    IOCC_EXECUTE_IO, IOCM_WRITE},
{PB_REQ_LIST, PB_WRITEV_X,   IOCC_EXECUTE_IO, IOCM_WRITE_VERIFY},
{PB_REQ_LIST, PB_PREFETCH_X, IOCC_EXECUTE_IO, IOCM_READ_PREFETCH},

{CMDINPUT,         0,        IOCC_EXECUTE_IO, IOCM_READ},
{CMDOUTPUT,        0,        IOCC_EXECUTE_IO, IOCM_WRITE},
{CMDOUTPUTV,       0,        IOCC_EXECUTE_IO, IOCM_WRITE_VERIFY},
{CMDInputBypass,   0,        IOCC_EXECUTE_IO, IOCM_READ},
{CMDOutputBypass,  0,        IOCC_EXECUTE_IO, IOCM_WRITE},
{CMDOutputBypassV, 0,        IOCC_EXECUTE_IO, IOCM_WRITE_VERIFY},

{CMDInternal, DISKOP_READ_VERIFY,        IOCC_EXECUTE_IO, IOCM_READ_VERIFY},
{CMDInternal, DISKOP_FORMAT,             IOCC_FORMAT,     IOCM_FORMAT_TRACK},
{CMDInternal, DISKOP_FORMAT_VERIFY,      IOCC_FORMAT,     IOCM_FORMAT_TRACK},
{CMDInternal, DISKOP_GET_CHANGELINE_STATE, IOCC_UNIT_STATUS, IOCM_GET_CHANGELINE_STATE},
{CMDInternal, DISKOP_GET_MEDIA_SENSE,    IOCC_UNIT_STATUS, IOCM_GET_MEDIA_SENSE},
{CMDInternal, DISKOP_SUSPEND_DEFERRED,   IOCC_DEVICE_CONTROL, IOCM_SUSPEND},
{CMDInternal, DISKOP_RESUME,             IOCC_DEVICE_CONTROL, IOCM_RESUME},
{CMDInternal, DISKOP_SET_MEDIA_GEOMETRY, IOCC_GEOMETRY, IOCM_SET_MEDIA_GEOMETRY},
{-1}, };                                // End of Table


   UCHAR PktCommand, PktFunction;
   USHORT i;
   PPB_Read_Write pRLE;
   PReq_List_Header pRLH;
   NPIORB_EXECUTEIO pXIO;
   NPIORB_FORMAT pFIO;
   NPGEOMETRY pGeometry;
   NPFORMAT_CMD_TRACK pFCT;
   PRP_RWV pRP;
   NPVOLCB pVolCB;
   NPIORB_DMWORK pDMWork;

   pDMWork = (NPIORB_DMWORK) &(pIORB->DMWorkSpace);

   /* Setup the IORB header */

   pIORB->UnitHandle = pUnitCB->UnitInfo.UnitHandle;
   pIORB->RequestControl = IORB_ASYNC_POST;
   pIORB->NotifyAddress = NotifyDoneIORB;
   pIORB->Status = 0;
   pDMWork->pUnitCB = pUnitCB;
   pDMWork->pRequest = pReq;

   /* Determine the IORB CommandCode/CommandModifier and goto the     */
   /* associate command handler to initialize the rest of the IORB.   */

   PktCommand = ((PRPH)pReq)->Cmd;
   PktFunction = 0;
   if (PktCommand == PB_REQ_LIST)
      PktFunction = ((PPB_Read_Write)pReq)->RqHdr.Command_Code;
   else if (PktCommand == CMDInternal)
      PktFunction = ((PRP_INTERNAL) pReq)->Function;

   for (i = 0; ReqToIORB_CmdTable[i].PktCommand != -1; i++)
   {
      if ( (PktCommand == ReqToIORB_CmdTable[i].PktCommand) &&
           (PktFunction == ReqToIORB_CmdTable[i].PktFunction) )
      {
         pIORB->CommandCode = ReqToIORB_CmdTable[i].IORBCommandCode;
         pIORB->CommandModifier = ReqToIORB_CmdTable[i].IORBCommandModifier;

         if ( (DDFlags & DDF_DMAReadBack) &&
              (pIORB->CommandCode == IOCC_EXECUTE_IO) &&
              (pIORB->CommandModifier == IOCM_WRITE) &&
              (pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE) )

                 pIORB->CommandModifier = IOCM_WRITE_VERIFY;

         break;
      }
   }

   switch (pIORB->CommandCode)
   {
      case IOCC_EXECUTE_IO:

         if (PktCommand == PB_REQ_LIST)
            goto ExecuteIO_RLE;
         else
            goto ExecuteIO_RP;

      case IOCC_FORMAT:
         goto Format_RP;

      case IOCC_UNIT_STATUS:
         goto UnitStatus_RP;

      case IOCC_DEVICE_CONTROL:
         goto DeviceControl_RP;

      case IOCC_GEOMETRY:
         goto Geometry_RP;
   }


   /*----------------------------------------------*/
   /* Setup EXECUTE_IO IORB for Request List Entry */
   /*----------------------------------------------*/

ExecuteIO_RLE:
   pXIO = (NPIORB_EXECUTEIO) (pIORB);
   pRLE = (PPB_Read_Write) pReq;

   pRLH = (PReq_List_Header) pRLE;
   OFFSETOF(pRLH) = OFFSETOF(pRLE) - pRLE->RqHdr.Head_Offset;

   if ( (DDFlags & DDF_FT_ENABLED) &&
        ( ((PRHFT)pRLE)->ftdb.FT_Flags & FTF_FT_REQUEST) )
      Get_VolCB_Addr(((PRHFT)pRLE)->Block_Dev_Unit, (NPVOLCB FAR *) &pVolCB);
   else
      Get_VolCB_Addr(pRLH->Block_Dev_Unit, (NPVOLCB FAR *) &pVolCB);

   pXIO->iorbh.Length = sizeof(IORB_EXECUTEIO);
   pXIO->RBA = pRLE->Start_Block +
               pVolCB->PartitionOffset + pVolCB->MediaBPB.HiddenSectors;
   pXIO->BlockCount = pRLE->Block_Count;
   pXIO->BlockSize = 512;

   /* Set up pointers to the scatter/gather list for all commands */
   /* except a read prefetch.                                    */

   if (pXIO->iorbh.CommandModifier != IOCM_READ_PREFETCH)
   {
      pXIO->cSGList = pRLE->SG_Desc_Count;
      pXIO->pSGList = (PSCATGATENTRY) pRLE;
      OFFSETOF(pXIO->pSGList) += sizeof(PB_Read_Write);
      pXIO->ppSGList = pRLH->y_PhysAddr + OFFSETOF(pXIO->pSGList) -
                       OFFSETOF(pRLH);
  }
  return;

   /*-------------------------------------------*/
   /* Setup EXECUTE_IO IORB for Request Packets */
   /*-------------------------------------------*/

ExecuteIO_RP:
   pXIO = (NPIORB_EXECUTEIO) pIORB;
   pRP = (PRP_RWV) pReq;

   pXIO->iorbh.Length = sizeof(IORB_EXECUTEIO);
   pXIO->RBA = pRP->rba;
   if ( ((PRPH)pReq)->Flags & RPF_CHS_ADDRESSING)
      pXIO->iorbh.RequestControl |= IORB_CHS_ADDRESSING;
   pXIO->BlockCount = pRP->NumSectors;
   pXIO->BlockSize = 512;

/* *** Need to fix for non-standard block sizes *** */

// if ( ((PRPH)pReq)->Cmd == CMDInternal)
//    pXIO->BlockSize = ((PRP_INTERNAL) pReq)->SectorSize;

   /* Set up pointers to the scatter/gather list for all commands */
   /* except a read verify.  The scatter/gather list for Request  */
   /* Packets is created within a reserved field in the           */
   /* IORB_EXECUTEIO control block.                               */

   if (pXIO->iorbh.CommandModifier != IOCM_READ_VERIFY)
   {
      pXIO->cSGList = 1;
      pXIO->pSGList = (PVOID) pXIO;
      OFFSETOF(pXIO->pSGList) = (USHORT) (&(pDMWork->SGList));
      pXIO->ppSGList =
             (ULONG) (ppDataSeg + (ULONG)((USHORT)&(pDMWork->SGList)));

      /* Set up single entry scatter/gather list within IORB */
      (ULONG) (pDMWork->SGList.ppXferBuf) = pRP->XferAddr;
      (ULONG) (pDMWork->SGList.XferBufLen) = pXIO->BlockCount * pXIO->BlockSize;
   }
   return;

   /*-------------------------------------------*/
   /* Setup FORMAT IORB for Request Packets     */
   /*-------------------------------------------*/
Format_RP:
   pFIO = (NPIORB_FORMAT) pIORB;
   pRP = (PRP_RWV) pReq;
   pFCT = (NPFORMAT_CMD_TRACK) &(pFIO->Reserved_1[0]);

   pFIO->iorbh.Length = sizeof(IORB_FORMAT);

   if ( ((PRPH)pReq)->Flags & RPF_CHS_ADDRESSING)
      pFIO->iorbh.RequestControl |= IORB_CHS_ADDRESSING;

   pFIO->cSGList = 1;
   pFIO->pSGList = (PVOID) pFIO;
   OFFSETOF(pFIO->pSGList) = (USHORT) (&(pDMWork->SGList));
   pFIO->ppSGList =
            (ULONG) (ppDataSeg + (ULONG)((USHORT)&(pDMWork->SGList)));

   /* Set up single entry scatter/gather list within IORB */
   (ULONG) (pDMWork->SGList.ppXferBuf) = ((PRP_INTERNAL)pReq)->XferAddr;
   (ULONG) (pDMWork->SGList.XferBufLen) =
           ((PRP_INTERNAL)pReq)->NumSectors * sizeof(FTT);

   pFIO->FormatCmdLen = sizeof(FORMAT_CMD_TRACK);
   pFIO->pFormatCmd = (PVOID) pFCT;

   pFCT->RBA = ((PRP_INTERNAL)pReq)->rba;
   pFCT->cTrackEntries = ((PRP_INTERNAL)pReq)->NumSectors;

   /* If FormatVerify then turn on the verify flag.  */

   pFCT->Flags = 0;
   if (PktFunction == DISKOP_FORMAT_VERIFY)
      pFCT->Flags |= FF_VERIFY;

   return;

   /*------------------------------------------------*/
   /* Setup UNIT STATUS IORB for Request Packets     */
   /*------------------------------------------------*/
UnitStatus_RP:
   pIORB->Length = sizeof(IORB_UNIT_STATUS);

   return;

   /*---------------------------------------------------*/
   /* Setup DEVICE CONTROL IORB                         */
   /*---------------------------------------------------*/
DeviceControl_RP:
   pIORB->Length = sizeof(IORB_DEVICE_CONTROL);

   if (PktFunction == DISKOP_SUSPEND_DEFERRED)
      ((NPIORB_DEVICE_CONTROL)pIORB)->Flags |= DC_SUSPEND_DEFERRED;

   return;

   /*---------------------------------------------------*/
   /* Setup GEOMETRY IORB                               */
   /*---------------------------------------------------*/
Geometry_RP:
   pVolCB = (NPVOLCB) ((PRP_INTERNAL)pReq)->NumSectors;
   (USHORT) pGeometry = OFFSETOF(pIORB) + sizeof(IORB_GEOMETRY);

   pIORB->Length = sizeof(IORB_GEOMETRY);
   ((NPIORB_GEOMETRY)pIORB)->pGeometry = (PVOID) pGeometry;
   ((NPIORB_GEOMETRY)pIORB)->GeometryLen = sizeof(GEOMETRY);

   if (pVolCB->MediaBPB.TotalSectors != 0)
      pGeometry->TotalSectors = pVolCB->MediaBPB.TotalSectors;
   else
      pGeometry->TotalSectors = pVolCB->MediaBPB.BigTotalSectors;

   pGeometry->BytesPerSector = pVolCB->MediaBPB.BytesPerSector;
   pGeometry->NumHeads = pVolCB->MediaBPB.NumHeads;
   pGeometry->TotalCylinders = pVolCB->NumLogCylinders;
   pGeometry->SectorsPerTrack = pVolCB->MediaBPB.SectorsPerTrack;

   return;
}


/*------------------------------------------------------------------------
;
;** AllocIORB - Allocate an IORB
;
;   Allocates an IORB.
;
;   USHORT AllocIORB  (NPUNITCB pUnitCB, NPIORB *pIORB)
;
;   ENTRY:    pUnitCB           - UnitCB requesting allocation
;             pIORB             - returned pointer to IORB
;
;   RETURN:   USHORT            = 0, IORB allocated
;                               ! 0 , IORB not allocated
;
;   NOTES:
;
------------------------------------------------------------------------*/
USHORT  AllocIORB(pUnitCB, pIORB)

NPUNITCB pUnitCB;
NPIORBH  FAR *pIORB;

{
   USHORT rc = 0;

   PUSHFLAGS;
   DISABLE;     /* Make sure interrupts are disabled */

   /* Try allocating the dedicated IORB for the requesting unit */

   if ( !(pUnitCB->Flags & UCF_IORB_ALLOCATED) )
   {
      pUnitCB->Flags |= UCF_IORB_ALLOCATED;
      *pIORB = pUnitCB->pDedicatedIORB;
   }
   else
   {
     /* Dedicated IORB already allocated, so get an IORB from the pool. */

     if (CB_FreeList != 0)
     {
       *pIORB = (NPIORBH) CB_FreeList;           /* Get IORB from free list */
       CB_FreeList = (NPBYTE) (*pIORB)->pNxtIORB; /* Update free list head  */
     }
     else
       rc = 1;
   }

   /* Zero fill IORB */

   if (rc == 0)
      f_ZeroCB((PBYTE)*pIORB, MAX_IORB_SIZE);

   POPFLAGS;
   return(rc);
}

/*------------------------------------------------------------------------
;
;** AllocIORB_Wait - Allocate an IORB, wait until one is available
;
;   Allocates an IORB from the Control Block pool and block if one
;   is not available.
;
;   VOID AllocIORB_Wait  (NPUNITCB pUnitCB, NPIORB *pIORB)
;
;   ENTRY:    pUnitCB           - UnitCB requesting allocation
;             pIORB             - returned pointer to IORB
;
;   RETURN:   VOID
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
VOID  AllocIORB_Wait (pUnitCB, pIORB)

NPUNITCB pUnitCB;
NPIORBH  FAR *pIORB;

{
   USHORT Allocated = FALSE;

   PUSHFLAGS;
   DISABLE;

   do
   {
      if (CB_FreeList != 0)             /* Allocate from free list */
      {
         *pIORB = (NPIORBH) CB_FreeList;
         (NPIORBH) CB_FreeList = ((NPIORBH) CB_FreeList)->pNxtIORB;
         Allocated = TRUE;
      }
      else                              /* else wait till control block free */
      {
         ENABLE;
         PoolSem = 1;                   /* Indicate at least 1 thread blocked */
         DevHelp_ProcBlock((ULONG) ppDataSeg, (ULONG)-1, 0);
         DISABLE;
      }
   } while (Allocated == FALSE);

   /* Zero fill the IORB */

   f_ZeroCB((PBYTE)pIORB, sizeof(IORB_EXECUTEIO));

   POPFLAGS;
}
/*------------------------------------------------------------------------
;
;** FreeIORB - Free an IORB
;
;   Free an IORB.
;
;   VOID FreeIORB  (NPUNITCB pUnitCB, NPIORB pIORB)
;
;   ENTRY:    pUnitCB           - UnitCB requesting deallocation
;             pIORB             - IORB to deallocate
;
;   RETURN:
;
;   EFFECTS:
;
------------------------------------------------------------------------*/

VOID  FreeIORB (pUnitCB, pIORB)

NPUNITCB pUnitCB;
NPIORBH  pIORB;

{
   USHORT Dummy;        /* Needed so stack isnt messed up by PUSHFLAGS */

   PUSHFLAGS;
   DISABLE;

   /* If the IORB being freed is the unit's dedicated IORB, then simply */
   /* clear the UCF_IORB_ALLOCATED flag in the UnitCB, otherwise return */
   /* the IORB to the free pool.                                        */

   if (pIORB == pUnitCB->pDedicatedIORB)
      pUnitCB->Flags &= ~UCF_IORB_ALLOCATED;
   else
   {
      pIORB->pNxtIORB = (NPIORBH) CB_FreeList;
      CB_FreeList = (NPBYTE) pIORB;
   }

   POPFLAGS;
}



