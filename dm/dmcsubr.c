/*static char *SCCSID = "@(#)dmcsubr.c	6.3 92/01/30";*/
/*static char *SCCSID = "@(#)dmcsubr.c	6.3 92/01/30";*/
#define SCCSID  "@(#)dmcsubr.c	6.3 92/01/30"

/*--------------------------------------------------------
 *       (c) Copyright IBM Corporation 1981, 1990        *
 *                                                       *
 *       All Rights Reserved                             *
 *-------------------------------------------------------*/

#include "dmh.h"

/*------------------------------------------------------------------------
;
;** Get_VolCB_Addr - Return a pointer to a VolCB for the specified Drive
;
;   USHORT NEAR Get_VolCB_Addr   (USHORT DriveNum, NPVOLCB *pVolCB)
;
;   USHORT FAR  f_Get_VolCB_Addr (USHORT DriveNum, NPVOLCB *pVolCB)
;
;   ENTRY:    DriveNum         - Drive Number
;             pVolCB           - returned pointer to VolCB
;
;   RETURN:   USHORT           - Result Code (NO_ERROR if VolCB found)
;
;   EFFECTS:
;
;   NOTES:
------------------------------------------------------------------------*/
USHORT FAR f_Get_VolCB_Addr (DriveNum, pVolCB)

USHORT  DriveNum;
NPVOLCB FAR *pVolCB;
{
   return(Get_VolCB_Addr (DriveNum, (NPVOLCB FAR *) pVolCB));

}

USHORT Get_VolCB_Addr (DriveNum, pVolCB)

USHORT  DriveNum;
NPVOLCB FAR *pVolCB;
{
   USHORT  found = FALSE;
   NPVOLCB pVolCBx;

   if ( !(DDFlags & DDF_NO_MEDIA) )
   {
      pVolCBx = VolCB_Head;

      while (pVolCBx != NULL && found == FALSE)
      {
         if (pVolCBx->LogDriveNum == DriveNum)
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

/*---------------------------------------------------------------
;
;** ReadSecInScratch_RBA - Read sector into scratch buffer
;
;   Reads a sector into the ScratchBuffer.
;
;   USHORT ReadSecInScratch_RBA (NPVOLCB pVolCB, ULONG rba, USHORT type)
;
;   ENTRY:    pVolCB           - Pointer to VolCB
;             rba              - RBA of sector to read
;             type             - Type of request (0= add in hidden sectors,
;                                                 1= dont add in hidden sectors)
;
;   RETURN:   USHORT           - Result Code (NO_ERROR if successful)
;
;   EFFECTS:  Reads a sector into global variable ScratchBuffer.
;
;   NOTES:
;
;   SEGMENT:  StaticCode
;--------------------------------------------------------------*/
USHORT FAR f_ReadSecInScratch_RBA (pVolCB, rba, type)

NPVOLCB  pVolCB;
ULONG    rba;
USHORT   type;
{
   return (ReadSecInScratch_RBA (pVolCB, rba, type));
}

USHORT ReadSecInScratch_RBA (pVolCB, rba, type)

NPVOLCB  pVolCB;
ULONG    rba;
USHORT   type;
{
   PRP_RWV  pRP;
   USHORT rc;

   DevHelp_AllocReqPacket (0, (PBYTE FAR *)&pRP); /* Allocate a request packet */

   pRP->rph.Cmd = CMDINPUT;
   pRP->rph.Flags = RPF_Internal;       /* Internal read request */
   pRP->rph.Unit = pVolCB->LogDriveNum; /* Copy unit number */
   pRP->XferAddr = ppScratchBuffer;     /* Set xfer addr of ScratchBuffer */
   pRP->NumSectors = 1;                 /* read one sector */
   pRP->rba = rba;                      /* Setup rba */

   pVolCB->Flags |= vf_ForceRdWrt;      /* force next read to succeed */

   pRP->rba += pVolCB->PartitionOffset; /* Add in partition offset    */

   if (type == 0)
      pRP->rba += pVolCB->MediaBPB.HiddenSectors; /* Add in Hidden sectors */

   DiskIO_Wait ((PBYTE)pRP, pVolCB);    /* Issue the read request */

   rc = pRP->rph.Status;                /* Get return status */

   DevHelp_FreeReqPacket ((PBYTE)pRP);         /* Free the request packet */

   if (rc & STERR)                      /* Check for error */
      return(ERROR);
   else
      return(NO_ERROR);
}

/*---------------------------------------------------------------
;
;** ReadSecInScratch_CHS - Read sector into scratch buffer
;
;   Reads a sector into the ScratchBuffer.
;
;   USHORT ReadSecInScratch_CHS (NPVOLCB pVolCB, USHORT Cylinder,
;                                UCHAR Head, UCHAR Sector)
;
;   ENTRY:    pVolCB           - pointer to VolCB
;             Cylinder         - Cylinder
;             Head             - Head
;             Sector           - Sector
;
;   RETURN:   USHORT           - Result Code (NO_ERROR if successful)
;
;   EFFECTS:  Reads a sector into global variable ScratchBuffer.
;
;   NOTES:
;
;   SEGMENT:  SwapCode
;--------------------------------------------------------------*/
USHORT FAR f_ReadSecInScratch_CHS (pVolCB, Cylinder, Head, Sector)

NPVOLCB  pVolCB;
USHORT   Cylinder;
UCHAR    Head, Sector;

{
  return (ReadSecInScratch_CHS (pVolCB, Cylinder, Head, Sector));
}

USHORT ReadSecInScratch_CHS (pVolCB, Cylinder, Head, Sector)

NPVOLCB  pVolCB;
USHORT   Cylinder;
UCHAR    Head, Sector;

{
   return (ReadSecInScratch_RBA (pVolCB,
                                 f_CHS_to_RBA (pVolCB, Cylinder, Head, Sector),
                                 1));
}




/*------------------------------------------------------------------------
;
;** w_MediaCheck - Worker routine for Media Check
;
;   Checks to see if the media in the drive has changed.
;
;   USHORT w_MediaCheck   (UCHAR Unit, NPVOLCB pVolCB)
;
;   ENTRY:    Unit             - Logical Unit Number
;             pVolCB           - Pointer to VolCB
;
;   RETURN:   USHORT           - Media Change status as follows:
;                                   -1 = Media has been changed
;                                    0 = Unsure if media has been changed
;                                    1 = Media unchanged
;
------------------------------------------------------------------------*/
USHORT w_MediaCheck (Unit, pVolCB)

UCHAR    Unit;
NPVOLCB  pVolCB;

{
   USHORT  ChangeState;
   NPVOLCB pVolCBx;

   /* Not changed if fixed disk */

   if ( !(pVolCB->pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE) )
      return (MEDIA_UNCHANGED);

   if ( (CheckPseudoChange(Unit, pVolCB) != 0) ||
        (pVolCB->Flags & (vf_ChangedByFormat | vf_Changed)) )

      ChangeState = MEDIA_CHANGED;

   else
   {
      ChangeState = CheckChangeSignal (Unit, pVolCB);

      /* See if changeline support is there for this unit */

      if ( !(pVolCB->pUnitCB->UnitInfo.UnitFlags & UF_CHANGELINE) )
      {
         if (ChangeState != MEDIA_UNCHANGED)
            ChangeState = MEDIA_UNSURE_CHANGED;
      }
      else

      /* Changeline is supported by the drive and the ChangeLine is not
         active.  This does not mean that it was not triggered!!!!
         It could be the case that it was high, but an access to the second
         floppy drive turned it off (this was a problem reported during
         DOS 3.20). We therefore check to see if this unit was the last
         one accessed if we are on a system with a single floppy drive.
         If it was, then we assume the medium has not changed.
      */
      {
         if ( (ChangeState == MEDIA_UNCHANGED) && (NumRemovableDisks == 1) &&
              (pVolCB->LogDriveNum <= 1) && (XActPDrv != pVolCB->LogDriveNum) )

              ChangeState = MEDIA_UNSURE_CHANGED;
      }
   }
      /*
         If the medium may have changed or has changed we note this in the
         VolCB for later use. We *MUST* flag these changes in ALL the
         logical drives that are mapped onto this physical drive.
         We also inform the ROM that the medium may have changed.
         This must be done so that it retries I/O on the medium with
         different data rates, and does not assume a certain rate for
         the particular drive. This is only done for physical drives
         0 and 1 since the ROM only has data areas for these drives.
      */

   if (ChangeState == MEDIA_CHANGED)
      pVolCB->Flags &= ~(vf_ChangedByFormat | vf_Changed);


   if (ChangeState != MEDIA_UNCHANGED)
   {
      for (pVolCBx = VolCB_Head; pVolCBx != NULL; pVolCBx = pVolCBx->pNextVolCB)
      {
         if (pVolCBx->PhysDriveNum == pVolCB->PhysDriveNum)
            pVolCBx->Flags |= vf_UncertainMedia;
      }
   }

   return(ChangeState);
}
/*------------------------------------------------------------------------
;
;***    CheckPseudoChange - check for floppy pseudo drive change
;
;       The DOS/BIOS conspire so that on single-floppy-drive systems there
;       appear to be two floppy drives, "A" and "B".  The Bios keeps track
;       of which of these "pseudo drives" is currently active and if it
;       sees a request to the other drive causes the user to be prompted
;       to "insert drive ?? disk".
;
;       CheckPseudoChange is called when we're doing floppy I/O on a single
;       drive system.  It takes one of two actions:
;
;       1) the request is for the pseudo-drive thats active
;               - return no change required
;
;       2) the request is for the other pseodo-drive:
;               - Return disk change error to dos and let it map the
;                 the logical drive.
;
;   USHORT CheckPseudoChange (USHORT LogDriveNum, NPVOLCB pVolCB)
;
;   ENTRY:    LogDriveNum      - Logical Drive Number
;             pVolCB           - Pointer to VolCB
;
;   RETURN:   USHORT           - Pseudo drive status:
;                                   -1 = Pseudo drive must be changed
;                                    0 = Pseudo drive unchanged
;
------------------------------------------------------------------------*/
USHORT FAR f_CheckPseudoChange (LogDriveNum, pVolCB)

USHORT  LogDriveNum;
NPVOLCB pVolCB;

{
   return(CheckPseudoChange (LogDriveNum, pVolCB));
}

USHORT CheckPseudoChange (LogDriveNum, pVolCB)

USHORT  LogDriveNum;
NPVOLCB pVolCB;

{
   USHORT ChangeState;

   /* If not one of several logical drives or this unit not  */
   /* mapped onto the physical drive then return unchanged   */

   if ( !(pVolCB->Flags & vf_AmMult) || (pVolCB->Flags & vf_OwnPhysical) )
      ChangeState = 0;

   else if (NumRemovableDisks != 1)
      ChangeState = -1;

/*
 *  If we are trying to switch back to either the A: or the B: drive
 *  and if neither of the drives A: or B: owns the physical device then
 *  the device must be allocated to a EXTDSKDD.SYS device and we
 *  cannot trust the ROMData to tell us which device owns the drive
 *  because neither of the ActPDrv(s) own it!
 */

   else if ( (LogDriveNum == 0 || LogDriveNum == 1) &&         /* If A: or B: */
             !(VolCB_Head->Flags & vf_OwnPhysical) &&          /* A: owns ?   */
             !(VolCB_Head->pNextVolCB->Flags & vf_OwnPhysical) )/* B: owns ?   */

        ChangeState = -1;


   else if (XActPDrv == LogDriveNum)
   {
       Update_Owner (pVolCB);
       ChangeState = 0;
   }
   else
       ChangeState = -1;

  return (ChangeState);
}
/*------------------------------------------------------------------------
;
;** Update_Owner - Update the owner of the logical drive
;
;   Update_Owner maps the current drive onto the physical drive by
;   adjusting the vf_OwnPhysical flag bits in the approporiate VolCBs.
;   It also adjusts the ROMs data area.
;
;   VOID Update_Owner (NPVOLCB pVolCB)
;
;   ENTRY:    pVolCB           - Pointer to VolCB
;
;   RETURN:
;
------------------------------------------------------------------------*/
VOID Update_Owner (pVolCB)

NPVOLCB pVolCB;

{
   NPVOLCB pVolCBx;

   /* Reset vf_OwnPhysical for all VolCBs mapped onto this physical drive */

   for (pVolCBx = VolCB_Head; pVolCBx != NULL; pVolCBx = pVolCBx->pNextVolCB)

      if (pVolCB->PhysDriveNum == pVolCBx->PhysDriveNum)
         pVolCBx->Flags &= ~vf_OwnPhysical;


   /* Set ownership for this logical drive */

   pVolCB->Flags |= vf_OwnPhysical | vf_Changed;


  if ( (NumRemovableDisks == 1) && (pVolCB->LogDriveNum <= 1) )
     XActPDrv = pVolCB->LogDriveNum;

}







/*------------------------------------------------------------------------
;
;** CheckChangeSignal - Check floppy change signal
;
;   Checks the floppy change signal.
;
;   USHORT CheckChangeSignal (USHORT LogDriveNum, NPVOLCB pVolCB)
;
;   ENTRY:    LogDriveNum      - Logical Drive Number
;             pVolCB           - Pointer to VolCB
;
;   RETURN:   USHORT           - Media Change status as follows:
;                                   -1 = Media has been changed
;                                    0 = Unsure if media has been changed
;                                    1 = Media unchanged
;
------------------------------------------------------------------------*/
USHORT CheckChangeSignal (LogDriveNum, pVolCB)

USHORT   LogDriveNum;
NPVOLCB  pVolCB;

{
   USHORT ChangeState;
   PRP_INTERNAL pIRP;

   if (DevHelp_AllocReqPacket(0, (PBYTE FAR *) &pIRP) != NO_ERROR)
      return(STDON + STERR + ERROR_I24_GEN_FAILURE);

   pIRP->rph.Cmd = CMDInternal;
   pIRP->rph.Unit = LogDriveNum;
   pIRP->rph.Flags = RPF_Internal;
   pIRP->Function = DISKOP_GET_CHANGELINE_STATE;

   DiskIO_Wait( (PBYTE)pIRP, pVolCB);

   if (pIRP->rph.Status & STERR)
      ChangeState = MEDIA_UNSURE_CHANGED;
   else
   {
      if (pIRP->RetStatus & US_CHANGELINE_ACTIVE)
         ChangeState = MEDIA_CHANGED;
      else
         ChangeState = MEDIA_UNCHANGED;
   }

   DevHelp_FreeReqPacket((PBYTE)pIRP);        /* Free internal RP   */

   return(ChangeState);

}

/*------------------------------------------------------------------------
;
;** CheckWithinPartition - Check I/O falls within partition limits
;
;   Verifies I/O fails within parition limits.
;
;   USHORT CheckWithinPartition (NPVOLCB pVolCB, ULONG rba, ULONG NumSectors)
;
;   ENTRY:    pVolCB           - Pointer to VolCB
;             rba              - rba
;             NumSectors       - Number of sectors to transfer
;
;   RETURN:   USHORT           - Packet status word
;
------------------------------------------------------------------------*/
USHORT CheckWithinPartition (pVolCB, rba, NumSectors)

NPVOLCB pVolCB;
ULONG   rba;
ULONG   NumSectors;

{
   ULONG EndSector;

   /* Check for a valid partition */

   if (pVolCB->Flags & (vf_TooBig | vf_NoDOSPartition))
      return(STDON + STERR + ERROR_I24_SECTOR_NOT_FOUND);

   /* Make sure I/O doesnt go beyond end of partition */

   if (EndSector = f_add32 (rba, NumSectors) == 0)
      return(STDON + STERR + ERROR_I24_SECTOR_NOT_FOUND);

   if (pVolCB->MediaBPB.TotalSectors != 0)
   {
      if (EndSector > pVolCB->MediaBPB.TotalSectors)
          return(STDON + STERR + ERROR_I24_SECTOR_NOT_FOUND);
   }
   else
   {
      if (EndSector > pVolCB->MediaBPB.BigTotalSectors)
          return(STDON + STERR + ERROR_I24_SECTOR_NOT_FOUND);
   }
   return(STDON);
}

/*------------------------------------------------------------------------
;
;** CHS_to_RBA - Convert Cylinder, Head, Sector to RBA
;
;   Converts a Cylinder/Head/Sector address to a double word
;   Relative Block Address using this formula:
;
;   RBA = (((Cylinder * NumHeads) + Head) * SectorsPerTrack) + (Sector - 1)
;
;   ULONG  NEAR CHS_to_RBA   (NPVOLCB pVolCB, USHORT Cylinder,
;                             UCHAR Head, UCHAR Sector)
;
;   ULONG  FAR  f_CHS_to_RBA (NPVOLCB pVolCB, USHORT Cylinder,
;                             UCHAR Head, UCHAR Sector)
;
;   ENTRY:    pVolCB           - VolCB entry for the device
;             Cylinder         - Cylinder
;             Head             - Head
;             Sector           - Sector
;
;   RETURN:   ULONG            - rba
;
;   EFFECTS:
;
;   NOTES:
------------------------------------------------------------------------*/

ULONG FAR f_CHS_to_RBA (pVolCB, Cylinder, Head, Sector)

NPVOLCB   pVolCB;
USHORT    Cylinder;
UCHAR     Head;
UCHAR     Sector;
{
   return(CHS_to_RBA (pVolCB, Cylinder, Head, Sector));
}

ULONG CHS_to_RBA (pVolCB, Cylinder, Head, Sector)

NPVOLCB   pVolCB;
USHORT    Cylinder;
UCHAR     Head;
UCHAR     Sector;
{
  ULONG rba;

  rba = (((((ULONG) Cylinder * pVolCB->MediaBPB.NumHeads) + Head) *
            pVolCB->MediaBPB.SectorsPerTrack) + Sector - 1);

  return (rba);
}


/*------------------------------------------------------------------------
;
;** MapIORBError - Maps an IORB error code to an ERROR_I24 error code.
;
;   Maps an IORB error code to an ERROR_I24 error code.
;
;   UCHAR  MapIORBError (USHORT IORBErrorCode)
;
;   ENTRY:    IORBError        - IORB error code
;
;   RETURN:   UCHAR            - ERROR_I24 error code
;
------------------------------------------------------------------------*/
typedef struct _ERROR_TABLE_ENTRY
{
   USHORT   IORB_ErrorCode;
   UCHAR    I24_ErrorCode;
} ERROR_TABLE_ENTRY;


UCHAR MapIORBError(IORB_ErrorCode)

USHORT IORB_ErrorCode;

{
   static ERROR_TABLE_ENTRY ErrorTable[] =
   {
      {IOERR_UNIT_NOT_READY,        ERROR_I24_NOT_READY},
      {IOERR_RBA_ADDRESSING_ERROR,  ERROR_I24_SECTOR_NOT_FOUND},
      {IOERR_RBA_LIMIT,             ERROR_I24_SECTOR_NOT_FOUND},
      {IOERR_RBA_CRC_ERROR,         ERROR_I24_CRC},
      {IOERR_MEDIA_NOT_FORMATTED,   ERROR_I24_SECTOR_NOT_FOUND},
      {IOERR_MEDIA_NOT_SUPPORTED,   ERROR_I24_SECTOR_NOT_FOUND},
      {IOERR_MEDIA_WRITE_PROTECT,   ERROR_I24_WRITE_PROTECT},
      {IOERR_MEDIA_CHANGED,         ERROR_I24_UNCERTAIN_MEDIA},
      {IOERR_MEDIA_NOT_PRESENT,     ERROR_I24_NOT_READY},
//    {?                            ERROR_I24_BAD_UNIT},
//    {??IOERR_CMD,                 ERROR_I24_BAD_COMMAND},
//    {?,                           ERROR_I24_SEEK},
//    {?,                           ERROR_I24_WRITE_FAULT},
//    {?,                           ERROR_I24_READ_FAULT},
//    {?,                           ERROR_I24_GEN_FAILURE},
      {-1,-1},
   };

   USHORT i;

   /* Map the IORB error to the corresponding ERROR_I24 error code */

   for (i = 0; ErrorTable[i].IORB_ErrorCode != -1; i++)
     if (ErrorTable[i].IORB_ErrorCode == IORB_ErrorCode)
        return(ErrorTable[i].I24_ErrorCode);

   return(ERROR_I24_GEN_FAILURE);
}


/*------------------------------------------------------------------------
;
;** VirtToPhys - Convert a virtual to physical address
;
;   Convert a virtual to a physical address.  This routine does
;   not use DevHlp_VirtToPhys since that devhlp is not callable
;   at interrupt time.
;
;   ULONG  VirtToPhys  (PBYTE VirtAddr)
;
;   ENTRY:    VirtAddr         - 16:16 virutal address
;
;   RETURN:   ULONG            - 32 bit phys address
;
------------------------------------------------------------------------*/
ULONG VirtToPhys (VirtAddr)

PBYTE VirtAddr;

{
  USHORT rc;
  SCATGATENTRY ScatGatEntry;
  ULONG VirtLinAddr;
  ULONG ScatLinAddr;
  ULONG PageListCount;

  rc = DevHelp_VirtToLin((USHORT) (SELECTOROF(VirtAddr)),
                         (ULONG) (OFFSETOF(VirtAddr)),
                         (PLIN) &VirtLinAddr);

  rc = DevHelp_VirtToLin((USHORT) ( ((ULONG)((PVOID) &ScatGatEntry)) >> 16),
                         (ULONG)  ( (USHORT)((PVOID) &ScatGatEntry)),
                         (PLIN) &ScatLinAddr);

  rc = DevHelp_LinToPageList(VirtLinAddr, 1, ScatLinAddr,
                                             (PULONG) &PageListCount);

  return(ScatGatEntry.ppXferBuf);

}





USHORT PowerOf2 (value)

UCHAR  value;
{
   return(1);
}


