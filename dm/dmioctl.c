/*static char *SCCSID = "@(#)dmioctl.c	6.8 92/02/06";*/
/*static char *SCCSID = "@(#)dmioctl.c	6.8 92/02/06";*/
#define SCCSID  "@(#)dmioctl.c	6.8 92/02/06"

/*--------------------------------------------------------
 *       (c) Copyright IBM Corporation 1981, 1990        *
 *                                                       *
 *       All Rights Reserved                             *
 *-------------------------------------------------------*/

#include "dmh.h"

typedef struct _IOCTL_TABLE_ENTRY
{
   USHORT Function;
   USHORT (NEAR *pIOCTL_Routine)(NPCWA);
} IOCTL_TABLE_ENTRY;

/*------------------------------------------------------------------------
;
;** f_DriveGenIOCTL - Category 8/9 IOCTL routines
;
;   Category 8/9 IOCTL routines and IOCTL function router.
;
;   USHORT f_DriveGenIOCTL (PRP_GENIOCTL pRP, NPVOLCB pVolCB)
;
;   ENTRY:    pRP              - Request Packet
;             pVolCB           - Pointer to VolCB
;
;   RETURN:   USHORT           - Packet Status word
;
;   NOTES:    SGL was here.
;
------------------------------------------------------------------------*/

USHORT FAR f_DriveGenIOCTL (pRP, pVolCB)

PRP_GENIOCTL  pRP;
NPVOLCB       pVolCB;

{

   /* Category 8 - Logical Disk Control Command Router Table */

   static IOCTL_TABLE_ENTRY Cat8_IOCTL_Table[] =
   {
      {IOPD_ED, GIO_AliasDrive},         // 22H - Install Drive alias
      {IODC_SP, GIO_SetDeviceParms},     // 43H - set device parameters
      {IODC_WT, GIO_RWVTrack},           // 44H - write track
      {IODC_FT, GIO_FormatVerify},       // 45H - format & verify track
      {IODC_QD, GIO_DsktControl},        // 5DH - quiesce/restart diskette
      {IOPD_RB, GIO_ReadBack},           // 5EH - DMA readback function
      {IODC_MS, GIO_MediaSense},         // 60H - get media sense
      {IODC_GP, GIO_GetDeviceParms8},    // 63H - get device parameters
      {IODC_RT, GIO_RWVTrack},           // 64H - read track
      {IODC_VT, GIO_RWVTrack},           // 65H - verify track
      {-1},                              // End of Table
   };

   /* Category 9 - Physical Disk Control Command Router Table */

   static IOCTL_TABLE_ENTRY Cat9_IOCTL_Table[] =
   {
      {IODC_WT, GIO_RWVTrack},           // 44H - write track
      {IODC_FT, GIO_FormatVerify},       // 45H - format & verify track
      {IODC_GP, GIO_GetDeviceParms9},    // 63H - get device parameters
      {IODC_RT, GIO_RWVTrack},           // 64H - read track
      {IODC_VT, GIO_RWVTrack},           // 65H - verify track
      {-1},                              // End of Table
   };


   /* Category 88H - Disk Fault Tolerance Control Command Router Table */

   static IOCTL_TABLE_ENTRY Cat88_IOCTL_Table[] =
   {
//    {FT_IOCTL_Func, GIO_FaultTolerance},  // 51H - Fault Tolerance IOCTL
      {-1},                                 // End of Table
   };


   NPCWA   pCWA;
   USHORT  rc, Unit, i;
   USHORT (NEAR *pIOCTL_Routine)(NPCWA);
   IOCTL_TABLE_ENTRY *pTable;


   /* Verify it's an IOCTL Category the disk driver supports */
   /* and point to the appropriate function table.           */

   switch (pRP->Category)
   {
      case IOC_DC:
         pTable = Cat8_IOCTL_Table;
         break;

      case IOC_PD:
         pTable = Cat9_IOCTL_Table;
         break;

      case FT_IOCTL_Cat:
         pTable = Cat88_IOCTL_Table;
         break;

      default:
         return (STERR + STDON + ERROR_I24_BAD_COMMAND);
   }

   /* Verify it's an IOCTL Function the disk driver supports */
   /* and get the entry point of the IOCTL function to call */

   for (i = 0; pTable[i].Function != -1; i++)
   {
      if (pRP->Function == pTable[i].Function)
      {
         pIOCTL_Routine = pTable[i].pIOCTL_Routine;
         break;
      }
   }

   if (pTable[i].Function == -1)
      return (STERR + STDON + ERROR_I24_BAD_COMMAND);


   /* Valid Category and Command Code.  Set up to call the appropriate */
   /* Function command routine.                                        */

   Unit = pRP->rph.Unit;

   if (pRP->Category == IOC_PD)         /* if cat 9, then unit 0x80 based */
      Unit += 0x80;

   if ((rc = f_Get_VolCB_Addr (Unit, (PVOID) &pVolCB)) == ERROR)
      return (STDON + STERR + ERROR_I24_BAD_UNIT);

   if ((rc = BuildCWA (pRP, pVolCB, (NPCWA FAR *) &pCWA)) == ERROR)
      return (STDON + STERR + ERROR_I24_GEN_FAILURE);

   rc = (*pIOCTL_Routine)(pCWA);     /* call the appropriate function routine */

   ReleaseCWA (pCWA);                /* release the CWA and return */

   return (rc);

}



/*------------------------------------------------------------------------
;
;** GIO_RWVTrack - Process the IOCTLs Read/Write/Verify Track.
;
;   Process the IOCTLs Read/Write/Verify Track.
;
;   USHORT GIO_RWVTrack (NPCWA pCWA)
;
;   ENTRY:    pCWA             - CWA
;
;   RETURN:   USHORT           - Packet Status word
;
;
------------------------------------------------------------------------*/
USHORT GIO_RWVTrack (pCWA)

NPCWA pCWA;

{
   USHORT rc;
   PRP_INTERNAL pIRP;

   if ((rc = CheckRWVPacket(pCWA)) & STERR)
      return(rc);

   if (CheckFloppy(pCWA) & STERR)
      return(STDON + STERR + ERROR_I24_BAD_UNIT);

   if (DevHelp_AllocReqPacket(0, (PBYTE *) &(pCWA->pIRP)) != NO_ERROR)
      return(STDON + STERR + ERROR_I24_GEN_FAILURE);

   pIRP = pCWA->pIRP;

   pIRP->rph.Unit = pCWA->pRP->rph.Unit;

   pIRP->rph.Flags = RPF_Internal;

   pIRP->SectorSize = 512;              /* Default sector size is 512 */

   switch (pCWA->pRP->Function)
   {
      case IODC_RT:                                /* Read track */
         pIRP->rph.Cmd = CMDINPUT;
         break;

      case IODC_WT:                                /* Write track */
         pIRP->rph.Cmd = CMDOUTPUT;
         break;

      case IODC_VT:                                /* Verify Track */
         pIRP->rph.Cmd = CMDInternal;
         pIRP->Function = DISKOP_READ_VERIFY;
         break;

      default:
         return(STDON + STERR + ERROR_I24_INVALID_PARAMETER);
   }

   if (pCWA->Flags & (TT_NOT_CONSEC + TT_NOT_SAME_SIZE + TT_NON_STD_SECTOR))
      rc = ExecRWVMultiPass(pCWA);
   else
      rc = ExecRWVOnePass(pCWA);

   return(rc);
}

/*------------------------------------------------------------------------
;
;** CheckRWVPacket - Check Read, Write Verify IOCTL packets.
;
;   Check the read, write, verify IOCTL packets.
;
;   USHORT CheckRWV (NPCWA pCWA)
;
;   ENTRY:    pCWA             - CWA
;
;   RETURN:   USHORT           - Packet Status word
;
;
------------------------------------------------------------------------*/
USHORT CheckRWVPacket (pCWA)

NPCWA pCWA;

{
   USHORT rc, i, LockFlag;
   ULONG  BufferSize = 0;
   PDDI_RWVPacket_param   pRWVP;

   pRWVP = (PDDI_RWVPacket_param) pCWA->pParmPkt;

   /* Verify access to the parameter packet */

   if ((rc = LockUserPacket(pCWA, LOCK_PARMPKT + LOCK_VERIFYONLY,
                            sizeof(DDI_RWVPacket_param))) & STERR)

      return(rc);

   /*
      The parameter packet is safe to read and at least one
      entry in the track table is accessible.  We must now
      determine if we will have to go past this entry in the
      track table.  If we do, LockUserPacket will have to be
      called again with a new size.
   */

   if  ( (pRWVP->FirstSector + pRWVP->NumSectors) > 1)
   {
        if (DevHelp_VMUnLock(plDataSeg +
                            (ULONG) ((USHORT)(&(pCWA->hLockParmPkt)))) != 0)
           return(STDON + STERR + ERROR_I24_INVALID_PARAMETER);

        pCWA->Flags &= ~LOCKED_PARMPKT;

        if ((rc = LockUserPacket( pCWA, LOCK_PARMPKT + LOCK_VERIFYONLY,
                sizeof(DDI_RWVPacket_param) +
                (sizeof(TLT) * (pRWVP->FirstSector + pRWVP->NumSectors - 1)))
                    & STERR) )
            return(rc);
    }
   /* Validate the cylinder and head values are ok */

   if ( (pRWVP->Cylinder >= pCWA->pVolCB->NumPhysCylinders) ||
        (pRWVP->Head >= pCWA->pVolCB->MediaBPB.NumHeads) )

      return (STDON+STERR+ERROR_I24_INVALID_PARAMETER);


   /* Initialize the required fields in the CWA */

   pCWA->StartSector = pRWVP->FirstSector;
   pCWA->Cylinder = pRWVP->Cylinder;
   pCWA->Head = pRWVP->Head;
   pCWA->NumSectors = pRWVP->NumSectors;
   pCWA->TTSectors = pRWVP->FirstSector + pRWVP->NumSectors;

   /*
      Determine:

      1) Whether sector table starts with sector number 1.
      2) Whether sectors in track table are consecutive.
      3) Whether sectors in track table are all the same size.
      4) Total buffer length.
   */

   if (pRWVP->TrackTable[0].SectorNumber != 1)
      pCWA->Flags |= TT_START_NOT_SECTOR_ONE;

   for (i = 0; i < pCWA->TTSectors; i++)
   {
      if (i > 0 && pRWVP->TrackTable[i].SectorNumber !=
                   (pRWVP->TrackTable[i-1].SectorNumber + 1) )
         pCWA->Flags |= TT_NOT_CONSEC;


      if (pRWVP->TrackTable[i].SectorSize != pRWVP->TrackTable[0].SectorSize)
         pCWA->Flags |= TT_NOT_SAME_SIZE;

      if (i >= pRWVP->FirstSector)
         BufferSize += pRWVP->TrackTable[i].SectorSize;
   }

   /* Non standard sector sizes and tables with different sector sizes */
   /* are only supported for diskettes.  Return error for fixed disks. */

   if ( (pRWVP->TrackTable[0].SectorSize != 512) ||
        (pCWA->Flags & TT_NOT_SAME_SIZE) )
   {
      if (pCWA->pVolCB->pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE)
         pCWA->Flags |= TT_NON_STD_SECTOR;
      else
         return (STDON + STERR + ERROR_I24_INVALID_PARAMETER);
   }

   if (pRWVP->Command == RWV_SIMPLE_TRACK_TABLE)
   {
      if (pCWA->Flags & (TT_START_NOT_SECTOR_ONE | TT_NOT_CONSEC))
         return (STDON + STERR + ERROR_I24_INVALID_PARAMETER);
   }
   else if (pRWVP->Command != RWV_COMPLEX_TRACK_TABLE)
         return (STDON + STERR + ERROR_I24_INVALID_PARAMETER);

   /* Lock down the data buffer for a read or write */

   if (pCWA->pRP->Function == IODC_RT || pCWA->pRP->Function == IODC_WT)
        LockFlag = LOCK_DATAPKT;
   {
      if (pCWA->pRP->Function == IODC_RT)  /* if read, then writing TO memory */
         LockFlag |= LOCK_WRITE;

      rc = LockUserPacket (pCWA, LockFlag, BufferSize);
   }

   return (rc);
}

/*------------------------------------------------------------------------
;
;** ExecRWVOnePass - Execute single pass Read/Write/Verify operation.
;
;   Setup a Read/Write/Verify IORB and submit it for processing.
;
;   USHORT ExecRWVOnePass (NPCWA pCWA)
;
;   ENTRY:    pCWA             - CWA
;
;   RETURN:   USHORT           - Packet Status word
;
;
------------------------------------------------------------------------*/
USHORT ExecRWVOnePass (pCWA)

NPCWA pCWA;
{
  USHORT rc;
  PDDI_RWVPacket_param   pRWVP;
  PRP_INTERNAL pIRP;

  pIRP = pCWA->pIRP;
  pRWVP = (PDDI_RWVPacket_param) pCWA->pParmPkt;

  if (pCWA->pDataPkt != 0)
     pIRP->XferAddr = pCWA->ppDataPkt;     /* Save physical data xfer addr */

  /* Calculate RBA from Cylinder, Head and Starting Sector Number in    */
  /* the track table.                                                   */

  pIRP->rba = f_CHS_to_RBA(pCWA->pVolCB, pCWA->Cylinder, (UCHAR) pCWA->Head, 1) +
              pCWA->pVolCB->PartitionOffset - 1 +
              pRWVP->TrackTable[pCWA->StartSector].SectorNumber;

  pCWA->pVolCB->Flags |= vf_ForceRdWrt;

  /* Issue a synchronous I/O call which waits until the I/O is done */

  pIRP->NumSectors = pCWA->NumSectors;
  rc = IOCTL_IO( (PBYTE)pIRP, pCWA->pVolCB );

  /* If Uncertain Media error, then retry the I/O */

  if (rc == STDON + STERR + ERROR_I24_UNCERTAIN_MEDIA)
  {
     pIRP->NumSectors = pCWA->NumSectors;
     rc = IOCTL_IO ( (PBYTE) pIRP, pCWA->pVolCB);
  }
  return (rc);
}

/*------------------------------------------------------------------------
;
;** ExecRWVMultiPass - Execute multiple pass Read/Write/Verify operation.
;
;   Setup a Read/Write/Verify IORB and submit it for processing.
;
;   USHORT ExecRWVMultiPass (NPCWA pCWA)
;
;   ENTRY:    pCWA             - CWA
;
;   RETURN:   USHORT           - Packet Status word
;
;
------------------------------------------------------------------------*/

USHORT ExecRWVMultiPass (pCWA)

NPCWA  pCWA;
{
  PRP_INTERNAL pIRP;
  PDDI_RWVPacket_param   pRWVP;
  ULONG  TrackRBA, Buf;
  USHORT i, rc;
  PCHS_ADDR pCHS;

  pRWVP = (PDDI_RWVPacket_param) pCWA->pParmPkt;

  pIRP = pCWA->pIRP;

  if (pCWA->pDataPkt != 0)
     pIRP->XferAddr = pCWA->ppDataPkt;     /* Save physical data xfer addr */

  /* If the request is for a diskette, then use CHS addressing for the */
  /* non-standard diskette layout, otherwise use RBA addressing.       */

  if (pCWA->pVolCB->pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE)
  {
     pCHS = (PCHS_ADDR) &(pIRP->rba);
     pCHS->Cylinder = pCWA->Cylinder;
     pCHS->Head = (UCHAR) pCWA->Head;
     pIRP->rph.Flags |= RPF_CHS_ADDRESSING;
  }
  else
  {
     /* Calculate RBA for the start of the track.   */

     TrackRBA = f_CHS_to_RBA(pCWA->pVolCB,pCWA->Cylinder,(UCHAR)pCWA->Head,1) +
                pCWA->pVolCB->PartitionOffset;
  }

  Buf = pCWA->ppDataPkt;

  /* Process each entry in the track table from starting sector on.   */

  for (i = 0; i < pCWA->NumSectors; i++ )
  {
     pIRP->rph.Status = 0;
     pIRP->XferAddr = Buf;

     if (pCWA->pVolCB->pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE)
     {
        pCHS->Sector = (UCHAR) pRWVP->TrackTable[i].SectorNumber - 1;
        pIRP->SectorSize = pRWVP->TrackTable[i].SectorSize;
     }
     else
     {
        pIRP->rba = TrackRBA + pRWVP->TrackTable[i].SectorNumber - 1;
     }

     pCWA->pVolCB->Flags |= vf_ForceRdWrt;

     /* Issue a synchronous I/O call which waits until the I/O is done */

     pIRP->NumSectors = 1;
     rc = IOCTL_IO ( (PBYTE)pIRP, pCWA->pVolCB);

     /* If Uncertain Media error, then retry the I/O */

     if (rc == STDON + STERR + ERROR_I24_UNCERTAIN_MEDIA)
     {
        pIRP->NumSectors = 1;
        rc = IOCTL_IO ( (PBYTE)pIRP, pCWA->pVolCB);
     }

     if (rc != NO_ERROR)
        break;

     Buf += pRWVP->TrackTable[i].SectorSize;
  }

  return(pIRP->rph.Status);
}



/*------------------------------------------------------------------------
;
;** GIO_FormatVerify - Process a Format & Verify track IOCTL
;
;   Format, then verify a track.  For diskettes, if a standard formatting
;   is requested, then invoke the diskette state machine to do a
;   format/verify operation.  If non-standard diskette formatting
;   is requested, first do the formatting, and then perform a separate
;   verify operation.
;
;   USHORT ExecFormatVerify (NPCWA pCWA)
;
;   ENTRY:    pCWA             - CWA
;
;   RETURN:   USHORT           - Packet Status word
;
;
------------------------------------------------------------------------*/
USHORT GIO_FormatVerify (pCWA)

NPCWA pCWA;

{
   PDDI_FormatPacket_param   pFP;
   USHORT rc;
   PRP_INTERNAL pIRP;

   pFP = (PDDI_FormatPacket_param) pCWA->pParmPkt;

   if ((rc = CheckFormatPacket(pCWA)) & STERR)
      return(rc);

   if ((rc = CheckFloppy(pCWA)) & STERR)
      return(rc);

   if (DevHelp_AllocReqPacket(0, (PBYTE *) &(pCWA->pIRP)) != NO_ERROR)
      return(STDON + STERR + ERROR_I24_GEN_FAILURE);

   pIRP = pCWA->pIRP;

   pIRP->rph.Unit = pCWA->pRP->rph.Unit;

   pIRP->rph.Flags = RPF_Internal;

   pIRP->SectorSize = 512;              /* Default sector size is 512 */

   if (pFP->Command == FP_COMMAND_MULTITRACK)
      rc = ExecMultiTrkVerify(pCWA);

   else if (pCWA->pVolCB->pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE)
   {
      /* Perform separate verify step for non-standard diskettes. */

      if ( ((rc = ExecDisketteFormat(pCWA)) == NO_ERROR) &&
           (pCWA->Flags & (TT_NOT_CONSEC | TT_NOT_SAME_SIZE |
                             TT_NON_STD_SECTOR)) )

         rc = ExecCheckTrack(pCWA);
   }
   else
      rc = ExecCheckTrack(pCWA);

   return(rc);
}

/*------------------------------------------------------------------------
;
;** CheckFormatPacket - Check the Format IOCTL packet.
;
;   Check the Format IOCTL packet.
;
;   USHORT CheckFormatPacket (NPCWA pCWA)
;
;   ENTRY:    pCWA             - CWA
;
;   RETURN:   USHORT           - Packet Status word
;
;
------------------------------------------------------------------------*/
USHORT CheckFormatPacket (pCWA)

NPCWA (pCWA);
{
   USHORT rc, i;
   PDDI_FormatPacket_param   pFP;

   pFP = (PDDI_FormatPacket_param) pCWA->pParmPkt;

   /* Lock down the parameter packet since the format track   */
   /* table is passed to the diskette controller and it must  */
   /* locked down first.                                      */

   if ((rc = LockUserPacket(pCWA, LOCK_PARMPKT,
                            sizeof(DDI_FormatPacket_param))) & STERR)
      return(rc);

   /*
      The parameter packet is safe to read and at least one
      entry in the track table is accessible.  We must now
      determine if we will have to go past this entry in the
      track table.  If we do, LockUserPacket will have to be
      called again with a new size.
   */

   if (pFP->Command == FP_COMMAND_MULTITRACK)
      pCWA->TTSectors = pFP->NumSectors;
   else
      pCWA->TTSectors = 1;

   if (pCWA->TTSectors > 1)
   {
        if (DevHelp_VMUnLock(plDataSeg +
                            (ULONG) ((USHORT)(&(pCWA->hLockParmPkt)))) != 0)
            return(rc);

        pCWA->Flags &= ~LOCKED_PARMPKT;

        if ((rc = LockUserPacket(pCWA, LOCK_PARMPKT,
          sizeof(DDI_FormatPacket_param) +
          (sizeof(FTT) * (pCWA->TTSectors - 1))) & STERR) )

            return(rc);
   }

   /* Validate the cylinder and head values are ok */

   if ( (pFP->Cylinder >= pCWA->pVolCB->NumPhysCylinders) ||
        (pFP->Head >= pCWA->pVolCB->MediaBPB.NumHeads) )

      return (STDON+STERR+ERROR_I24_INVALID_PARAMETER);


   /* Initialize the required fields in the CWA */

   pCWA->Cylinder = pFP->Cylinder;
   pCWA->Head = pFP->Head;
   pCWA->NumSectors = pFP->NumSectors;

   /*
      Determine:

      1) Whether sector table starts with sector number 1.
      2) Whether sectors in track table are consecutive.
      3) Whether sectors in track table are all the same size.
      4) Whether CH ids in the track table matches the CH information
         in the parameter packet.
   */

   if (pFP->FmtTrackTable[0].Sector != 1)

        pCWA->Flags |= TT_START_NOT_SECTOR_ONE;

   if (SectorIndexToSectorSize(pFP->FmtTrackTable[0].BytesPerSectorIndex)!=512)

        pCWA->Flags |= TT_NON_STD_SECTOR;


   for (i = 0; i < pCWA->TTSectors; i++)
   {
      if (i > 0 && pFP->FmtTrackTable[i].Sector !=
                   (pFP->FmtTrackTable[i-1].Sector + 1) )

            pCWA->Flags |= TT_NOT_CONSEC;


      if (pFP->FmtTrackTable[i].BytesPerSectorIndex !=
          pFP->FmtTrackTable[0].BytesPerSectorIndex)

            pCWA->Flags |= TT_NOT_SAME_SIZE;

      if (pCWA->Cylinder != pFP->Cylinder || pCWA->Head != pFP->Head)

            return(STDON + STERR + ERROR_I24_INVALID_PARAMETER);
   }

   /* Now that we have all the track table flag bits set, make */
   /* sure the track layout is valid for the command specified */

   if (pFP->Command == FP_COMMAND_SIMPLE)
   {
      if (pCWA->Flags & (TT_START_NOT_SECTOR_ONE | TT_NOT_CONSEC))
         return(STDON + STERR + ERROR_I24_INVALID_PARAMETER);
   }
   else if (pFP->Command == FP_COMMAND_MULTITRACK)
   {
      if (pCWA->Flags & TT_NON_STD_SECTOR)
         return(STDON + STERR + ERROR_I24_INVALID_PARAMETER);
   }
   else if (pFP->Command != FP_COMMAND_COMPLEX)
         return(STDON + STERR + ERROR_I24_BAD_COMMAND);

   return (STDON);

}

/*------------------------------------------------------------------------
;
;** ExecDisketteFormat - Format a diskette
;
;   Format a diskette.
;
;   USHORT ExecDisketteFormat (NPCWA pCWA)
;
;   ENTRY:    pCWA             - CWA
;
;   RETURN:   USHORT           - Packet Status word
;
;
------------------------------------------------------------------------*/
USHORT ExecDisketteFormat (pCWA)

NPCWA pCWA;

{
  USHORT rc;
  PRP_INTERNAL pIRP;
  PCHS_ADDR pCHS;
  PDDI_FormatPacket_param   pParmPkt;

  pIRP = pCWA->pIRP;
  pParmPkt = (PDDI_FormatPacket_param) pCWA->pParmPkt;


  /* First set the media geometry prior to the format */

  pIRP->rph.Cmd = CMDInternal;
  pIRP->Function = DISKOP_SET_MEDIA_GEOMETRY;
  pIRP->NumSectors = (USHORT) pCWA->pVolCB;  /* Store pVolCB in IRP */

  rc = IOCTL_IO ( (PBYTE)pIRP, pCWA->pVolCB);

  if (rc & STERR)
     return(rc);


  /* If non-standard track layout, then verify the sectors in a
  /* separate operation. */

  if (pCWA->Flags & (TT_NOT_CONSEC | TT_NOT_SAME_SIZE | TT_NON_STD_SECTOR))
     pIRP->Function = DISKOP_FORMAT;
  else
     pIRP->Function = DISKOP_FORMAT_VERIFY;

  pIRP->XferAddr = pCWA->ppParmPkt + sizeof(DDI_FormatPacket_param)
                                   - sizeof(FTT);
  pIRP->NumSectors = pCWA->NumSectors;

  pIRP->SectorSize =
       SectorIndexToSectorSize(pParmPkt->FmtTrackTable[0].BytesPerSectorIndex);

  /* Use Cylinder, Head, Sector addressing for formatting diskettes */

//pIRP->rph.Flags |= RPF_CHS_ADDRESSING;
//pCHS = (PCHS_ADDR) &(pIRP->rba);
//pCHS->Cylinder = pCWA->Cylinder;
//pCHS->Head = (UCHAR) pCWA->Head;
//pCHS->Sector = 1;

  pIRP->rba = f_CHS_to_RBA (pCWA->pVolCB, pParmPkt->Cylinder,
                                          (UCHAR) pParmPkt->Head, 1) +
              pCWA->pVolCB->PartitionOffset;

  pCWA->pVolCB->Flags |= vf_ForceRdWrt;

//Set_Dasd();   Done in floppy ADD         /* Set type of medium in drive */


  /* Issue a synchronous I/O call which waits until the I/O is done */

  rc = IOCTL_IO ( (PBYTE)pIRP, pCWA->pVolCB);

  /* If Uncertain Media error, then retry the I/O */

  if (rc == STDON + STERR + ERROR_I24_UNCERTAIN_MEDIA)
  {
     pIRP->NumSectors = pCWA->NumSectors;
     rc = IOCTL_IO ( (PBYTE)pIRP, pCWA->pVolCB);
  }

  pCWA->pVolCB->Flags |= vf_ChangedByFormat;

  return(rc);
}

/*------------------------------------------------------------------------
;
;** ExecCheckTrack - Execute multiple pass verify operation.
;
;   Verify operation for each entry in the format track table.
;
;   USHORT ExecCheckTrack (NPCWA pCWA)
;
;   ENTRY:    pCWA             - CWA
;
;   RETURN:   USHORT           - Packet Status word
;
;
------------------------------------------------------------------------*/
USHORT ExecCheckTrack (pCWA)

NPCWA pCWA;
{
  USHORT i, rc;
  ULONG  TrackRBA;

  PRP_INTERNAL pIRP;
  PCHS_ADDR pCHS;
  PDDI_FormatPacket_param pParmPkt;

  pIRP = pCWA->pIRP;
  pParmPkt = (PDDI_FormatPacket_param) pCWA->pParmPkt;

  pIRP->rph.Cmd = CMDInternal;
  pIRP->Function = DISKOP_READ_VERIFY;

  /* If the request is for a diskette, then use CHS addressing,        */
  /* otherwise use RBA addressing.                                     */

  if (pCWA->pVolCB->pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE)
  {
     pCHS = (PCHS_ADDR) &(pIRP->rba);
     pCHS->Cylinder = pCWA->Cylinder;
     pCHS->Head = (UCHAR) pCWA->Head;
     pIRP->rph.Flags |= RPF_CHS_ADDRESSING;
  }
  else
  {
     TrackRBA=f_CHS_to_RBA(pCWA->pVolCB,pCWA->Cylinder,(UCHAR)pCWA->Head,1) +
              pCWA->pVolCB->PartitionOffset;
  }

  pIRP->XferAddr = 0;                      /* No data to transfer */

  for (i = 0; i < pCWA->NumSectors; i++)
  {
     if (pCWA->pVolCB->pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE)
     {
       pCHS->Sector = pParmPkt->FmtTrackTable[i].Sector - 1;
       pIRP->SectorSize =
        SectorIndexToSectorSize(pParmPkt->FmtTrackTable[i].BytesPerSectorIndex);
     }
     else
     {
        pIRP->rba = TrackRBA + pParmPkt->FmtTrackTable[i].Sector - 1;
     }

     pCWA->pVolCB->Flags |= vf_ForceRdWrt;

     /* Issue a synchronous I/O call which waits until the I/O is done */

     pIRP->NumSectors = 1;
     rc = IOCTL_IO ( (PBYTE)pIRP, pCWA->pVolCB);

     /* If Uncertain Media error, then retry the I/O */

     if (rc == STDON + STERR + ERROR_I24_UNCERTAIN_MEDIA)
     {
        pIRP->NumSectors = 1;
        rc = IOCTL_IO ( (PBYTE)pIRP, pCWA->pVolCB);
     }

     if (rc & STERR)
        break;
  }
  return(rc);
}

/*------------------------------------------------------------------------
;
;** ExecMultiTrkVerify - Perform multitrack verify
;
;   The input IOCTL points to a parameter packet which
;   indicates the starting Cylinder/Head and the length of the
;   verify in Tracks/Sectors. The input IOCTL also points to
;   a data packet which contains a byte which indicates the
;   starting sector for the verify.
;
;   If a nonrecoverable verify error is encountered, this routine
;   performs an inverse mapping from RBA to CHS, saves this
;   information back into the user's parameter packet, and
;   sets a return code indicating that an error had occurred.
;
;   Returning this information back to the 'Formatting Program'
;   will allow it to continue after marking the appropriate
;   area of the disk as defective.
;
;   USHORT ExecMultiTrkVerify (NPCWA pCWA)
;
;   ENTRY:    pCWA             - CWA
;
;   RETURN:   USHORT           - Packet Status word
;
;
------------------------------------------------------------------------*/
USHORT ExecMultiTrkVerify (pCWA)

NPCWA pCWA;

{
  PRP_INTERNAL pIRP;
  PDDI_FormatPacket_param pFP;
  PDDI_FormatPacket_data  pFD;
  NPVOLCB pVolCB;
  ULONG rba, StartRBA, ErrorRBA, TrksDone;
  USHORT CurrentNumSectors, SectorsLeftToDo, i, rc;
  CHS_ADDR chs;

  pIRP = pCWA->pIRP;
  pFP = (PDDI_FormatPacket_param) pCWA->pParmPkt;
  pFD = (PDDI_FormatPacket_data) pCWA->pDataPkt;
  pVolCB = pCWA->pVolCB;

  /* MultiTrack only supported for fixed disks */

  if ( (pVolCB->pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE) &&
       ( ! (pVolCB->pUnitCB->Flags & UCF_REMOVABLE_NON_FLOPPY) ) )
  {
     pFP->NumTracks = ERROR_MTF_NOT_SUPPORTED;
     return(STDON);
  }

  pIRP->rph.Cmd = CMDInternal;
  pIRP->Function = DISKOP_READ_VERIFY;

  rba = f_CHS_to_RBA(pVolCB, pCWA->Cylinder, (UCHAR) pCWA->Head,
                  (UCHAR)(pFD->StartSector+1)) + pCWA->pVolCB->PartitionOffset;
  StartRBA = rba;
  pIRP->rba = rba;
  pIRP->XferAddr = 0;                      /* No data to transfer */

  /* Calculate number of sectors in multitrack verify */

  SectorsLeftToDo = pFP->NumTracks * pVolCB->MediaBPB.SectorsPerTrack;

  pCWA->NumSectors = SectorsLeftToDo;
  pIRP->NumSectors = SectorsLeftToDo;
  CurrentNumSectors = SectorsLeftToDo;

  do
  {
     /* Setup internal request packet and do the I/O */

//???if (SectorsLeftToDo > pVolCB->pUnitCB->UnitInfo.MaxBlocksPerXfer)
//   {
//      CurrentNumSectors = pVolCB->pUnitCB->UnitInfo.MaxBlocksPerXfer;
//      pIRP->NumSectors = CurrentNumSectors;
//   }

     pIRP->rba = rba;
     pIRP->rph.Status = 0;
     rc = IOCTL_IO ( (PBYTE) pIRP, pVolCB);

     /* If there is an error, then find the exact RBA of the bad sector */
     /* by calling DISKIO_Wait one sector at a time, starting after     */
     /* the point where we know the I/O was successful.                 */

     if (rc & STERR)
     {
        for (i = 0, pIRP->rba = rba; i < CurrentNumSectors; i++, pIRP->rba++)
        {
           pIRP->NumSectors = 1;
           rc = IOCTL_IO ( (PBYTE) pIRP, pVolCB);

           /* See if this is the failing sector */
           if (rc & STERR)
           {
              /* Subtract out completed tracks.  Round Start and  */
              /* end points downwards to nearest track.  Do not   */
              /* count first (partial) track.  Save the number of */
              /* tracks remaining to be done in pFP->NumSectors.  */

              ErrorRBA = pIRP->rba - pVolCB->PartitionOffset;
              TrksDone = (ErrorRBA / pVolCB->MediaBPB.SectorsPerTrack) -
                         ((StartRBA - pVolCB->PartitionOffset) /
                            pVolCB->MediaBPB.SectorsPerTrack) + 1;

              pFP->NumSectors = pFP->NumTracks - TrksDone;

              /* Convert ErrorRBA to Cylinder/Head/Sector and save in RP */

              RBA_to_CHS(pCWA->pVolCB, ErrorRBA, (PVOID) &chs);

              pFP->Cylinder = chs.Cylinder;
              pFP->Head = chs.Head;
              pFD->StartSector = chs.Sector;

              pFP->NumTracks = ERROR_MTF_VERIFY_FAILURE;
              return(STDON);
           }
        }
     }
     rba += CurrentNumSectors;
     SectorsLeftToDo -= CurrentNumSectors;

  } while (SectorsLeftToDo > 0);


  pFP->NumTracks = MTF_SUCCESSFUL;

  return(STDON);
}


/*------------------------------------------------------------------------
;
;** GIO_GetDeviceParms8 - Process Cat 8 Get Device Parameters IOCTL
;
;   Transfers the BPB, Number of Cylinders, Device Type and Device
;   attributes to the IOCTL data packet from the Volume Control Block.
;
;   USHORT GIO_GetDeviceParms8 (NPCWA pCWA)
;
;   ENTRY:    pCWA             - CWA
;
;   RETURN:   USHORT           - Packet Status word
;
------------------------------------------------------------------------*/
USHORT GIO_GetDeviceParms8(pCWA)

NPCWA pCWA;
{
   PDDI_DeviceParameters_param pParmPkt;
   PDDI_DeviceParameters_data pDataPkt;
   NPUNITCB pUnitCB;
   USHORT rc;
   UCHAR DeviceType;

   pParmPkt = (PDDI_DeviceParameters_param) pCWA->pParmPkt;
   pDataPkt = (PDDI_DeviceParameters_data) pCWA->pDataPkt;

   /* Lock the Parameter and Data Packets */

   if ((rc = LockUserPacket(pCWA, LOCK_PARMPKT + LOCK_VERIFYONLY,
                            sizeof(DDI_DeviceParameters_param))) & STERR)
     return(rc);


   if ((rc = LockUserPacket(pCWA, LOCK_DATAPKT + LOCK_VERIFYONLY,
                            sizeof(DDI_DeviceParameters_data))) & STERR)
     return(rc);


   /* Determine whether to return recommended BPB or media BPB */

   if (pParmPkt->Command == GDP_RETURN_REC_BPB)
      pDataPkt->bpb = pCWA->pVolCB->RecBPB;
   else
   {
      if ( !(pCWA->pVolCB->Flags & vf_ReturnFakeBPB) &&
            ((rc = BuildNewBPB(pCWA)) & STERR) )

         return(rc);

      pDataPkt->bpb = pCWA->pVolCB->MediaBPB;
   }

   /* Return the Number of Cylinders, Device Type and Device Attributes */

   pDataPkt->NumCylinders = pCWA->pVolCB->NumLogCylinders;

   pUnitCB = pCWA->pVolCB->pUnitCB;

   if ( !(pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE) )
      pDataPkt->DeviceType = TYPE_FIXED_DISK;
   else
   {
      switch (pCWA->pVolCB->RecBPB.TotalSectors)
      {
         case 5760:
            pDataPkt->DeviceType = TYPE_288MB;
            break;
         case 2880:
            pDataPkt->DeviceType = TYPE_144MB;
            break;
         case 1440:
            pDataPkt->DeviceType = TYPE_720KB;
            break;
         case 2400:
            pDataPkt->DeviceType = TYPE_12MB;
            break;
         default:
            if (pUnitCB->Flags & UCF_REMOVABLE_NON_FLOPPY)
//             pDataPkt->DeviceType = TYPE_OTHER;
               pDataPkt->DeviceType = TYPE_144MB;
            else
               pDataPkt->DeviceType = TYPE_360KB;
            break;
      }
   }

   pDataPkt->DeviceAttr = 0;

   if ( !(pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE) )
      pDataPkt->DeviceAttr |= DP_DEVICEATTR_NON_REMOVABLE;

   if (pUnitCB->UnitInfo.UnitFlags & UF_CHANGELINE)
      pDataPkt->DeviceAttr |= DP_DEVICEATTR_CHANGELINE;

   if (pUnitCB->Flags & UCF_16M)
      pDataPkt->DeviceAttr |= DP_DEVICEATTR_GT16MBSUPPORT;

   return(STDON);
}

/*------------------------------------------------------------------------
;
;** BuildNewBPB - Build a new BPB for the medium in the drive
;
;   BuildNewBPB builds the BPB for the drive by first reading the
;   boot sector and passing it on to BPBFromBoot.  The sequence of
;   events is the same as for the BuildBPB command.
;
;   USHORT BuildNewBPB (NPCWA pCWA)
;
;   ENTRY:    pCWA             - CWA
;
;   RETURN:   USHORT           - Packet Status word
;
------------------------------------------------------------------------*/
USHORT BuildNewBPB (pCWA)

NPCWA pCWA;

{
   USHORT rc;

   /* First check to make sure right logical unit attached */

   if ((rc = CheckFloppy(pCWA)) & STERR)
      return(rc);

   /* Wait for scratchbuffer and read in boot sector         */
   /* If it's a valid boot sector, build the BPB from the    */
   /* boot sector copy, otherwise build BPB from scratch.    */

   f_SWait(&ScratchBufSem);

   if ( !((rc = f_ReadSecInScratch_RBA(pCWA->pVolCB, 0, 0)) & STERR) )
   {
      rc = f_BPBFromBoot(pCWA->pVolCB, (PDOSBOOTREC) &ScratchBuffer);
      f_SSig(&ScratchBufSem);
      if (rc & STERR)
         rc = f_BPBFromScratch(pCWA->pVolCB);
   }
   else
   {
      f_SSig(&ScratchBufSem);
      if (rc != STDON + STERR + ERROR_I24_NOT_READY)
        rc = STDON + STERR + ERROR_I24_NOT_READY;
   }

   return(rc);
}

/*------------------------------------------------------------------------
;
;** GIO_SetDeviceParms - Process Cat 8 Set Device Parameters IOCTL
;
;   Transfers the BPB, Number of Cylinders, Device Type and Device
;   attributes from the IOCTL data packet to the Volume Control Block.
;
;   USHORT ExecSetDeviceParms (NPCWA pCWA)
;
;   ENTRY:    pCWA             - CWA
;
;   RETURN:   USHORT           - Packet Status word
;
------------------------------------------------------------------------*/
USHORT GIO_SetDeviceParms(pCWA)

NPCWA pCWA;

{
  USHORT rc;
  PDDI_DeviceParameters_param pParmPkt;
  PDDI_DeviceParameters_data  pDataPkt;

  /* Lock the Parameter and Data Packets */

  if ((rc = LockUserPacket(pCWA, LOCK_PARMPKT + LOCK_VERIFYONLY,
                            sizeof(DDI_DeviceParameters_param))) & STERR)
    return(rc);


  if ((rc=LockUserPacket(pCWA, LOCK_DATAPKT + LOCK_VERIFYONLY,
                          sizeof(DDI_DeviceParameters_data))) & STERR)
    return(rc);


  /* Determine the subcommand of this IOCTL and process accordingly */

  pParmPkt = (PDDI_DeviceParameters_param) pCWA->pParmPkt;
  pDataPkt = (PDDI_DeviceParameters_data) pCWA->pDataPkt;

  switch (pParmPkt->Command)
  {
     case SDP_USE_MEDIA_BPB:

         /* Revert to building the BPB off the medium for all subsequent  */
         /* BuildBPB calls.  This is used after a format operation        */
         /* to reset the device parameters to their original state.       */

         pCWA->pVolCB->Flags &= ~vf_ReturnFakeBPB;
         break;

     case SDP_TEMP_BPB_CHG:

         /* Change the Recommended BPB for the physical device in the VolCB */
         /* to the BPB passed as input in the data packet of the IOCTL.     */

         /* Make sure not changing Removable bit */

         if ( ((pCWA->pVolCB->pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE) &&
              !(pDataPkt->DeviceAttr & DP_DEVICEATTR_NON_REMOVABLE))  ||
             ( !(pCWA->pVolCB->pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE) &&
               (pDataPkt->DeviceAttr & DP_DEVICEATTR_NON_REMOVABLE)) )
         {
            /* Update the Recommended physical BPB in the VolCB with the   */
            /* BPB specified as input in the data packet of the IOCTL.     */

            pCWA->pVolCB->RecBPB = pDataPkt->bpb;

            pCWA->pVolCB->NumLogCylinders = pDataPkt->NumCylinders;
//?????     pCWA->pVolCB->pUnitCB->UnitInfo.FormFactor = pDataPkt->DeviceType;

            if (pDataPkt->DeviceAttr & DP_DEVICEATTR_CHANGELINE)
               pCWA->pVolCB->pUnitCB->UnitInfo.UnitFlags |= UF_CHANGELINE;
            else
               pCWA->pVolCB->pUnitCB->UnitInfo.UnitFlags &= !(UF_CHANGELINE);
         }
         else
           rc = STDON + STERR + ERROR_I24_INVALID_PARAMETER;

         break;

     case SDP_PERM_BPB_CHG:

         /* Update the Media BPB in the VolCB with the BPB              */
         /* specified as input in the data packet of the IOCTL.         */
         /* This is used to prepare the device for a format media       */
         /* operation according to the device parms specified.          */

         pCWA->pVolCB->MediaBPB = pDataPkt->bpb;
         pCWA->pVolCB->NumLogCylinders = pDataPkt->NumCylinders;

         /* If formatting 360KB media in 1.2M drive, must update */
         /* the NumLogCylinders field to match the 360KB media   */

         if (pCWA->pVolCB->MediaBPB.TotalSectors == 720)
            pCWA->pVolCB->NumLogCylinders = 40;

         pCWA->pVolCB->Flags |= vf_ReturnFakeBPB;
         break;

     default:                           /* Invalid command code */
         rc = STDON + STERR + ERROR_I24_INVALID_PARAMETER;
  }
  return(rc);

}


/*------------------------------------------------------------------------
;
;** GIO_GetDeviceParms9 - Execute Cat 9 Get Device Parameters IOCTL
;
;   Returns device parameters for a physical device.
;
;   USHORT ExecPhysGetDevParms (NPCWA pCWA)
;
;   ENTRY:    pCWA             - CWA
;
;   RETURN:   USHORT           - Packet Status word
;
------------------------------------------------------------------------*/

USHORT GIO_GetDeviceParms9(pCWA)

NPCWA pCWA;
{
  USHORT rc;

  PDDI_PhysDeviceParameters_data pDataPkt;

  pDataPkt = (PDDI_PhysDeviceParameters_data) pCWA->pDataPkt;

  /* Verify access to data packet */

  if ((rc=LockUserPacket(pCWA, LOCK_DATAPKT + LOCK_VERIFYONLY,
                          sizeof(DDI_PhysDeviceParameters_data))) & STERR)
      return(rc);


  /* Return required device parms in data packet */

  pDataPkt->NumCylinders = pCWA->pVolCB->NumPhysCylinders;
  pDataPkt->NumHeads = pCWA->pVolCB->MediaBPB.NumHeads;
  pDataPkt->SectorsPerTrack = pCWA->pVolCB->MediaBPB.SectorsPerTrack;

  return(STDON);
}


/*------------------------------------------------------------------------
;
;** GIO_ReadBack - Process DMA ReadBack IOCTL packet.
;
;   Execute DMA ReadBack IOCTL packet.
;
;   USHORT GIO_ReadBack (NPCWA pCWA)
;
;   ENTRY:    pCWA             - CWA
;
;   RETURN:   USHORT           - Packet Status word
;
;   NOTES: We're not supporting the ReadBack subcommands Query Readback
;          Statistics and Reset Readback statistics.
;
------------------------------------------------------------------------*/
USHORT GIO_ReadBack(pCWA)

NPCWA pCWA;

{
   PDDI_ReadBack_param pParmPkt;
   PDDI_ReadBack_data  pDataPkt;

   USHORT rc = STDON;

   pParmPkt = (PDDI_ReadBack_param) pCWA->pParmPkt;
   pDataPkt = (PDDI_ReadBack_data) pCWA->pDataPkt;

   /* Verify access to the parameter packet.        */

   if ((rc = LockUserPacket(pCWA, LOCK_PARMPKT + LOCK_VERIFYONLY,
                            sizeof(DDI_ReadBack_param))) & STERR)
      return(rc);


   /* Process the command */

   switch (pParmPkt->Command)
   {
      case DMA_SET_READBACK_FLAG:
         DDFlags |= DDF_DMAReadBack;
         break;

      case DMA_RESET_READBACK_FLAG:
         DDFlags &= ~DDF_DMAReadBack;
         break;

      case DMA_QUERY_READ_STATS:        /* Not supported fully */

         /* Verify access to the data packet */

         if ((rc = LockUserPacket(pCWA, LOCK_DATAPKT + LOCK_VERIFYONLY,
                            sizeof(DDI_ReadBack_data))) & STERR)
            return(rc);

         pDataPkt->DmaOverrunCount = 0;
         pDataPkt->Count = 0;
         pDataPkt->Miscompares = 0;
         pDataPkt->Action = !(DDFlags & DDF_DMAReadBack);
         break;

      case DMA_RESET_READ_STATS:        /* Not supported */
         break;

      default:
         rc = STDON + STERR + ERROR_I24_INVALID_PARAMETER;
   }

   return(rc);
}


/*------------------------------------------------------------------------
;
;** GIO_MediaSense - Get media sense in the drive.
;
;   This function senses what media is present in the drive.
;
;   USHORT GIO_MediaSense (NPCWA pCWA)
;
;   ENTRY:    pCWA             - CWA
;
;   RETURN:   USHORT           - Packet Status word
;
------------------------------------------------------------------------*/
USHORT GIO_MediaSense(pCWA)

NPCWA pCWA;

{

   NPIORB_UNIT_STATUS pIORB;
   USHORT rc = STDON;

   PDDI_MediaSense_data  pDataPkt;
   pDataPkt = (PDDI_MediaSense_data) pCWA->pDataPkt;

   /* Verify access to the data packet */

   if ((rc = LockUserPacket(pCWA, LOCK_PARMPKT + LOCK_VERIFYONLY,
                            sizeof(DDI_MediaSense_data))) & STERR)
      return(rc);


   /* If non-removable unit then cant determine media type */

   pDataPkt->MediaSense = 0;

   if ( !(pCWA->pVolCB->pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE) )
        return(STDON);

   /* If wrong psuedo drive then return error */

   if ((rc = CheckFloppy(pCWA)) & STERR)
        return(rc);

   if (DevHelp_AllocReqPacket(0, (PBYTE *) &(pCWA->pIRP)) != NO_ERROR)
      return(STDON + STERR + ERROR_I24_GEN_FAILURE);

   pCWA->pIRP->rph.Cmd = CMDInternal;
   pCWA->pIRP->rph.Unit = pCWA->pRP->rph.Unit;
   pCWA->pIRP->rph.Flags = RPF_Internal;
   pCWA->pIRP->Function = DISKOP_GET_MEDIA_SENSE;

   /* Call the adapter driver to get the media sense info.  If the   */
   /* driver doesnt support it, then just return Unable to Determine */
   /* Media Type in the media sense information.                     */

   rc = IOCTL_IO( (PBYTE)pCWA->pIRP, pCWA->pVolCB );

   if ( !(rc & STERR) )
      pDataPkt->MediaSense = (UCHAR) pCWA->pIRP->RetStatus;

   return(STDON);
}


/*------------------------------------------------------------------------
;
;** GIO_AliasDrive - Install alias drive for a floppy
;
;   Install an alias drive for a floppy.
;
;   USHORT GIO_AliasDrive (NPCWA pCWA)
;
;   ENTRY:    pCWA             - CWA
;
;   RETURN:   USHORT           - Packet Status word
;
------------------------------------------------------------------------*/
USHORT GIO_AliasDrive(pCWA)

NPCWA pCWA;
{
   PRPINITOUT pRP;

   NPVOLCB pVolCBNew, pVolCBx, pVolCBLast, pVolCBNext;
   PDDI_DriveAlias_param pParmPkt;
   USHORT rc;
   USHORT VolCBfound = NO;

   pParmPkt = (PDDI_DriveAlias_param) pCWA->pParmPkt;
   pRP = (PRPINITOUT) pCWA->pRP;

   /* First make sure a free VolCB exists to allocate an alias drive */

   if (NumExtraVolCBs == 0)
      return(STDON + STERR + ERROR_I24_BAD_COMMAND);

   pVolCBNew = pExtraVolCBs;

   /* Perform aliasing */

   for (pVolCBx = VolCB_Head; pVolCBx != NULL; pVolCBx = pVolCBx->pNextVolCB);
   {
      if (pVolCBx->PhysDriveNum == pParmPkt->PhysDriveNum)
      {
         if ( !(pVolCBx->pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE) )
            return(STDON + STERR + ERROR_I24_BAD_UNIT);

         /* Set up the new Volume Control Block */

         pVolCBx->Flags |= vf_AmMult;           /* Indicate aliases exist */

         *pVolCBNew = *pVolCBx;
         pVolCBNew->MediaBPB = pParmPkt->rbpb;
         pVolCBNew->RecBPB = pParmPkt->rbpb;
         pVolCBNew->PhysDriveNum = pParmPkt->PhysDriveNum;
         pVolCBNew->LogDriveNum = NumLogDrives;
         pVolCBNew->NumPhysCylinders = pParmPkt->cCyln;
         pVolCBNew->NumLogCylinders = pParmPkt->cCyln;
         pVolCBNew->Flags &= ~vf_OwnPhysical;   /*  Dont own physical drive   */
      }
   }
   /* If a VolCB wasnt found for this physical drive, then return an error */

   if (pVolCBx == NULL)
      return(STDON + STERR + ERROR_I24_BAD_UNIT);


   /* Link this VolCB into the VolCB Chain */

   rc = f_Get_VolCB_Addr(NumLogDrives - 1, (PVOID) &pVolCBLast);

   DISABLE;

   pVolCBNext = pVolCBLast->pNextVolCB;
   pVolCBLast->pNextVolCB = pVolCBNew;
   pVolCBNew->pNextVolCB = pVolCBNext;

   NumLogDrives++;
   NumVolCBs++;
   NumExtraVolCBs--;
   pExtraVolCBs++;

   ENABLE;

   /* Return unit number and pointer to BPB array in original RP */

   pRP->rph.Unit = pVolCBNew->LogDriveNum;

   DummyBPB = (PVOID) ( &(pVolCBNew->MediaBPB) );

   pRP->BPBArray = (PVOID) (&DummyBPB);

   return(STDON);
}





/*------------------------------------------------------------------------
;
;** GIO_DsktControl - Suspend/Resume diskette driver I/O
;
;   This IOCTL provides for Internal Tape Support. An Internal
;   Tape Drive is an option which plugs into a diskette drive
;   slot. A separate OS/2 device driver directly programs the
;   diskette controller to provide support for this option.
;
;   This IOCTL is issued by the tape utilities prior to using
;   the tape drive. The Diskette Driver performs an orderly suspension
;   or restart of diskette activity as requested by this IOCTL.
;
;   USHORT ExecDsktControl (NPCWA pCWA)
;
;   ENTRY:    pCWA             - CWA
;
;   RETURN:   USHORT           - Packet Status word
;
------------------------------------------------------------------------*/
USHORT GIO_DsktControl (pCWA)

NPCWA pCWA;

{
   PDDI_DsktControl_param pParmPkt;

   USHORT RetData;
   USHORT rc = STDON;

   pParmPkt = (PDDI_DsktControl_param) pCWA->pParmPkt;

   if (NumRemovableDisks == 0)
      return (STDON + STERR + ERROR_I24_INVALID_PARAMETER);

   if (DevHelp_AllocReqPacket(0, (PBYTE *) &(pCWA->pIRP)) != NO_ERROR)
      return(STDON + STERR + ERROR_I24_GEN_FAILURE);

   pCWA->pIRP->rph.Cmd = CMDInternal;
   pCWA->pIRP->rph.Flags = RPF_Internal;

   /* Make sure this request gets to the diskette controller */

   pCWA->pIRP->rph.Unit = 0;
   pCWA->pVolCB = pVolCB_DriveA;

   switch (pParmPkt->Command)
   {
      case DSKT_SUSPEND:
         if (DDFlags & DDF_DsktSuspended)
            rc = STDON + STERR + ERROR_I24_INVALID_PARAMETER;
         else
         {
            pCWA->pIRP->Function = DISKOP_SUSPEND_DEFERRED;

            IOCTL_IO( (PBYTE)pCWA->pIRP, pCWA->pVolCB );

            if ( !(pCWA->pIRP->rph.Status & STERR) )
               DDFlags |= DDF_DsktSuspended;
         }
         break;

      case DSKT_RESUME:
         if ( !(DDFlags & DDF_DsktSuspended) )
            rc = STDON + STERR + ERROR_I24_INVALID_PARAMETER;
         else
         {
            pCWA->pIRP->Function = DISKOP_RESUME;

            IOCTL_IO( (PBYTE)pCWA->pIRP, pCWA->pVolCB);

            if ( !(pCWA->pIRP->rph.Status & STERR) )
               DDFlags &= ~DDF_DsktSuspended;
         }
         break;

      case DSKT_QUERY:
         if ( (pCWA->pVolCB->pUnitCB->NumReqsInProgress != 0) ||
              (pCWA->pVolCB->pUnitCB->NumReqsWaiting != 0) )

            rc = STDON + STERR + ERROR_I24_DISK_CHANGE;

         break;

      default:
         rc = STDON + STERR + ERROR_I24_INVALID_PARAMETER;
         break;
   }
   return(rc);
}

/*------------------------------------------------------------------------
;
;** GIO_ChangePartition - Process Change Partition type IOCTL
;
;   This IOCTL was used to indicate whether caching should be enabled
;   or disabled when FORMAT changes the partition type to HPFS or
;   FAT.  Since caching is no longer in the device driver, this routine
;   simply returns with status done.
;
;   USHORT GIO_ChangePartition (NPCWA pCWA)
;
;   ENTRY:    pCWA             - CWA
;
;   RETURN:   USHORT           - Packet Status word
;
------------------------------------------------------------------------*/
USHORT GIO_ChangePartition (pCWA)

NPCWA   pCWA;

{
 return(STDON);

}



/*------------------------------------------------------------------------
;
** CheckFloppy - Check to ensure the correct floppy is in the drive
;
;   Checks to ensure the correct floppy disk is in the drive.
;   This is required for drives that have more than one logical
;   drive assigned to them.
;
;   USHORT CheckFloppy (NPCWA pCWA)
;
;   ENTRY:    pCWA             - Returned pointer to CWA
;
;   RETURN:   USHORT           - Packet Status word
;
;
------------------------------------------------------------------------*/

USHORT CheckFloppy (pCWA)

NPCWA pCWA;

{
   USHORT rc = STDON;

   if ( (pCWA->pVolCB->pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE) &&
        (f_CheckPseudoChange(pCWA->pRP->rph.Unit, pCWA->pVolCB) == -1) )
         rc = STDON + STERR + ERROR_I24_DISK_CHANGE;

   return(rc);
}



/*------------------------------------------------------------------------
;
** BuildCWA - Build Common Work Area for IOCTL processing
;
;   Obtains a work area for interroutine communication from the
;   device driver storage pool and initializes it.
;
;   USHORT BuildCWA (PRP_GENIOCTL pRP, NPVOLCB pVolCB, NPCWA *pCWA)
;
;   ENTRY:    pRP              - Request Packet
;             pVolCB           - VolCB
;             pCWA             - Returned pointer to CWA
;
;   RETURN:   USHORT           - Packet Status word
;
;
------------------------------------------------------------------------*/
USHORT BuildCWA (pRP, pVolCB, pCWA)

PRP_GENIOCTL pRP;
NPVOLCB      pVolCB;
NPCWA        FAR *pCWA;

{
   AllocCWA_Wait (pCWA);

   if (pRP->Category == IOC_PD)         /* if cat 9, then unit 0x80 based */
      pRP->rph.Unit += 0x80;

   (*pCWA)->pRP = pRP;
   (*pCWA)->pVolCB = pVolCB;
   (*pCWA)->pParmPkt = pRP->ParmPacket;
   (*pCWA)->pDataPkt = pRP->DataPacket;

   return (NO_ERROR);
}



/*------------------------------------------------------------------------
;
;** ReleaseCWA - Release Common Work Area
;
;   Release the Common Work Area and unlock any locked buffers.
;
;   VOID ReleaseCWA (NPCWA pCWA)
;
;   ENTRY:    pCWA             - Common work area
;
;   RETURN:   VOID
;
;
------------------------------------------------------------------------*/
VOID ReleaseCWA (pCWA)

NPCWA pCWA;

{
   if (pCWA != NULL)
   {
      if (pCWA->Flags & LOCKED_PARMPKT)
        DevHelp_VMUnLock((LIN)(plDataSeg+(ULONG) ((USHORT)&(pCWA->hLockParmPkt))));

      if (pCWA->Flags & LOCKED_DATAPKT)
        DevHelp_VMUnLock((LIN)(plDataSeg+(ULONG) ((USHORT)&(pCWA->hLockDataPkt))));

      if (pCWA->pIRP != 0)
         DevHelp_FreeReqPacket((PBYTE)pCWA->pIRP);        /* Free internal RP   */

      FreeCWA (pCWA);
   }
}

/*------------------------------------------------------------------------
;
;** IOCTL_IO - Perform IOCTL I/O
;
;   Calls f_DiskIO_Wait and blocks this thread until the operation
;   is complete, i.e. the STDON bit is set in the request packet.
;
;   USHORT IOCTL_IO (PBYTE pRPH, NPVOLCB pVolCB)
;
;   ENTRY:    pRP              - Request Packet
;             pVolCB           - Pointer to VolCB
;
;   RETURN:   USHORT           - Packet Status word
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
USHORT NEAR IOCTL_IO (pRP, pVolCB)

PBYTE pRP;
NPVOLCB pVolCB;
{
   USHORT rc;

   /* Do access validation check */

   if ((rc = Chk_AccValidate( (PRP_RWV) pRP, pVolCB)) == 0 )
      rc = f_DiskIO_Wait(pRP, pVolCB);
   else
      ((PRP_RWV)pRP)->rph.Status = rc;

   return(rc);
}

/*------------------------------------------------------------------------
;
;** Chk_AccValidate - Check access validation for IOCTLs
;
;   Determines if access to the partition is allowed via IOCTL I/O/
;
;   USHORT Chk_AccValidate (PRP_RWV pRP, NPVOLCB pVolCB)
;
;   ENTRY:    pRP              - Request Packet
;             pVolCB           - Pointer to VolCB
;
;   RETURN:   USHORT           - Packet Status word
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
USHORT Chk_AccValidate (pRP, pVolCB)

PRP_RWV pRP;
NPVOLCB pVolCB;
{
   USHORT Destructive = NO;
   USHORT rc;
   NPVOLCB pVolCBx;
   ULONG PartitionSize;

   if (pFSD_AccValidate == 0)     /* Only validate if requested     */
      return(0);

   if (pVolCB->pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE)
      return(0);                  /* Only validate for fixed disks  */


   switch (pRP->rph.Cmd)
   {
      case CMDINPUT:
         break;

      case CMDOUTPUT:
      case CMDOUTPUTV:
         Destructive = YES;
         break;

      case CMDInternal:
         if  ( ((PRP_INTERNAL)pRP)->Function != DISKOP_READ_VERIFY )
            return(0);
         break;

     default:
         return(0);
   };

   /* If request is to the boot sector or to an HPFS partition then */
   /* it must be validated.                                         */

   if (pRP->rba == 0 || pVolCB->PartitionType == PARTITION_IFS)
      goto Validate;

   if ( !(pRP->rph.Unit & 0x80) )
     return(0);

   for (pVolCBx = pVolCB_DriveC; pVolCBx != 0; pVolCBx = pVolCBx->pNextVolCB)
   {
      /* Quit when we get to physical VolCBs */

      if (pVolCBx->LogDriveNum & 0x80)
         return(0);

      /* See if volume is on the same physical unit */
      /* and rba falls within the partition.        */

      if (pVolCBx->PhysDriveNum == pVolCB->PhysDriveNum)
      {
          if (pVolCBx->MediaBPB.TotalSectors != 0)
             PartitionSize = pVolCBx->MediaBPB.TotalSectors;
          else
             PartitionSize = pVolCBx->MediaBPB.BigTotalSectors;

          if ( (pRP->rba >= pVolCBx->PartitionOffset)  &&
               (pRP->rba < (pVolCBx->PartitionOffset + PartitionSize)) )
          {
             if (pVolCBx->PartitionType == PARTITION_IFS)
                goto Validate;
             else
                return(0);
          }
      }
   }
Validate:
   rc = f_FSD_AccValidate(Destructive);
   if (rc)
      rc = ERROR_I24_WRITE_PROTECT | STERR;

   return(rc);

}

/*------------------------------------------------------------------------
;
;** RBA_to_CHS - Convert RBA to Cylinder/Head/Sector
;
;   Convert RBA to Cylinder/Head/Sector format using device
;   information in the VolCB.
;
;   VOID RBA_to_CHS (NPVOLCB pVolCB, ULONG rba, PULONG *chs)
;
;   ENTRY:    pVolCB           - Volume Control Block
;             rba              - rba to convert
;             pCHS             - returned chs information
;
;
;   RETURN:   VOID
;
;
------------------------------------------------------------------------*/
VOID RBA_to_CHS (pVolCB, rba, pCHS)

NPVOLCB   pVolCB;
ULONG     rba;
CHS_ADDR  FAR *pCHS;

{
   ULONG Tracks;

   Tracks = rba / pVolCB->MediaBPB.SectorsPerTrack;

   pCHS->Sector = rba % pVolCB->MediaBPB.SectorsPerTrack;

   pCHS->Cylinder = Tracks / pVolCB->MediaBPB.NumHeads;

   pCHS->Head = Tracks % pVolCB->MediaBPB.NumHeads;

}


/*------------------------------------------------------------------------
;
;** LockUserPacket -  Verify/Lock parameter or data packet in IOCTL
;                     request packet.
;
;   This routine will either:
;
;      1) Provide a verify only lock which will verify access to the
;         parameter or data packets.  This is used for all packets
;         which do not need to be physically locked or made contiguous.
;
;      2) Lock the data or parameter packet in place and make it
;         contiguous.  This is required for packets which are used
;         in DMA operations, or packets which contain a format track
;         table which is passed to the diskette controller.
;
;   USHORT LockUserPacket (NPCWA pCWA, USHORT LockFlags, USHORT Length, )
;
;   ENTRY:    pCWA             - pointer to CWA
;             LockFlags        - Lock Flags
;                                 LOCK_PARMPKT = Lock Parameter Packet
;                                 LOCK_DATAPKT = Lock Data Packet
;                                 LOCK_WRITE = Lock for write access
;                                 LOCK_VERIFYONLY
;                                       0 = LOCK and VERIFY;
;                                       1 = VERIFY ONLY;
;             Length           - Length to Lock/verify
;
;   RETURN:   USHORT           - Packet status word
;
------------------------------------------------------------------------*/
USHORT LockUserPacket (pCWA, LockFlags, Length)

NPCWA   pCWA;
USHORT  LockFlags;
ULONG   Length;

{
   USHORT rc;
   ULONG plPkt;                /* Linear address of Parm or Data Packet */
   ULONG plLockHandle;
   ULONG plPhysAddr;
   ULONG PageListCount;
   ULONG VMLockFlags;

   if (LockFlags & LOCK_PARMPKT)    /* Parm Packet */
   {
      rc = DevHelp_VirtToLin(SELECTOROF(pCWA->pParmPkt),
                            (ULONG) OFFSETOF(pCWA->pParmPkt),
                            (PVOID) &plPkt);

      plLockHandle = plDataSeg + (ULONG) ((USHORT) &(pCWA->hLockParmPkt));
      plPhysAddr =   plDataSeg + (ULONG) ((USHORT) &(pCWA->ppParmPkt));
   }
   else                 /* Data Packet */
   {
      rc = DevHelp_VirtToLin(SELECTOROF(pCWA->pDataPkt),
                            (ULONG) OFFSETOF(pCWA->pDataPkt),
                            (PVOID) &plPkt);

      plLockHandle = plDataSeg + (ULONG) ((USHORT) &(pCWA->hLockDataPkt));
      plPhysAddr   = plDataSeg + (ULONG) ((USHORT) &(pCWA->ppDataPkt));
   }

   /* Issue the DevHelp to verify access to the packet */

   if (LockFlags & LOCK_VERIFYONLY)
      VMLockFlags = VMDHL_VERIFY;
   else
      VMLockFlags = VMDHL_16M + VMDHL_CONTIGUOUS;

   if (LockFlags & LOCK_WRITE)
      VMLockFlags |= VMDHL_WRITE;

   if (DevHelp_VMLock(VMLockFlags, plPkt, Length, plPhysAddr,
                  plLockHandle, (PULONG) &PageListCount) != 0)

      return(STDON + STERR + ERROR_I24_INVALID_PARAMETER);

   if (LockFlags & LOCK_PARMPKT)
      pCWA->Flags |= LOCKED_PARMPKT;
   else
      pCWA->Flags |= LOCKED_DATAPKT;

   return(STDON);
}

/*------------------------------------------------------------------------
;
;** AllocCWA_Wait - Allocate a CWA for IOCTL processing
;
;   Allocates a Common Work Area (CWA) from the Control Block pool
;   for IOCTL processing.  Since IOCTL requests can block, this
;   routine will block until a CWA is available for allocation.
;
;   VOID   AllocCWA_Wait  (NPCWA *pCWA)
;
;   ENTRY:    pCWA              - returned pointer to CWA
;
;   RETURN:
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
VOID  AllocCWA_Wait(pCWA)

NPCWA FAR *pCWA;

{
   USHORT Allocated = FALSE;
   NPCWA  npCWA;

   DISABLE;

   do
   {
      if (CB_FreeList != 0)             /* Allocate from free list */
      {
         npCWA = (NPCWA) CB_FreeList;
         (NPIORBH) CB_FreeList = ((NPIORBH) CB_FreeList)->pNxtIORB;
         Allocated = TRUE;
      }
      else                              /* else wait till control block free */
      {
         ENABLE;
         PoolSem = 1;                   /* Indicate at least 1 thread blocked */
         DevHelp_ProcBlock((ULONG) ppDataSeg, (ULONG)-1, 0);
      }
   } while (Allocated == FALSE);

   ENABLE;

   /* Zero fill the CWA */

   f_ZeroCB((PBYTE)npCWA, sizeof(CWA));

   *pCWA = npCWA;

}

/*------------------------------------------------------------------------
;
;** FreeCWA - Free a CWA
;
;   Return a CWA back to the control block pool.
;
;   VOID   FreeCWA  (NPCWA pCWA)
;
;   ENTRY:    pCWA              -  pointer to CWA
;
;   RETURN:
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
VOID FreeCWA (pCWA)

NPCWA pCWA;

{
   USHORT AwakeCount;

   DISABLE;

   ((NPIORBH) pCWA)->pNxtIORB = (NPIORBH) CB_FreeList;
   CB_FreeList = (NPBYTE) pCWA;

   ENABLE;

   /* If any threads waiting for a CWA, then wake them up */

   if (PoolSem != 0)
   {
      PoolSem = 0;
      DevHelp_ProcRun((ULONG)ppDataSeg, &AwakeCount);
   }
}

/*------------------------------------------------------------------------
;
;** SectorSizeToSectorIndex - Convert Sector Size to Index
;
;   Converts a sector size from bytes to an index that can be used
;   by the NEC disk controller.
;
;   (0=>128, 1=>256, 2=>512, 3=>1024)
;
;   UCHAR  NEAR SectorSizeToSectorIndex (USHORT BytesPerSector)
;
;   ENTRY:    BytesPerSector   - Number of bytes in the sector
;
;   RETURN:   UCHAR            - Sector size index
;
;   EFFECTS:
;
;   NOTES:
------------------------------------------------------------------------*/
UCHAR NEAR SectorSizeToSectorIndex (BytesPerSector)

USHORT BytesPerSector;
{
   UCHAR SectorIndex;

   SectorIndex = 3;
   if (BytesPerSector <= 512)
      SectorIndex = BytesPerSector / 256;

   return(SectorIndex);
}


/*------------------------------------------------------------------------
;
;** SectorIndexToSectorSize - Convert Sector Index to Sector Size
;
;   Converts a sector index to a size in bytes.
;
;   (0=>128, 1=>256, 2=>512, 3=>1024)
;
;   USHORT  NEAR SectorIndexToSectorSize (UCHAR SectorIndex)
;
;   ENTRY:    SectorIndex      - Sector Size Index
;
;   RETURN:   USHORT           - Bytes per Sector
;
;   EFFECTS:
;
;   NOTES:
------------------------------------------------------------------------*/
USHORT NEAR SectorIndexToSectorSize (SectorIndex)

UCHAR SectorIndex;
{
   USHORT BytesPerSector;

   BytesPerSector = 128;
   return (BytesPerSector << SectorIndex);

}
