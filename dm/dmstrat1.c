/*static char *SCCSID = "@(#)dmstrat1.c	6.5 92/02/06";*/
/*static char *SCCSID = "@(#)dmstrat1.c	6.5 92/02/06";*/
#define SCCSID  "@(#)dmstrat1.c	6.5 92/02/06"

/*--------------------------------------------------------
 *       (c) Copyright IBM Corporation 1981, 1990        *
 *                                                       *
 *       All Rights Reserved                             *
 *-------------------------------------------------------*/

#include "dmh.h"
#include "dmfault.h"

USHORT (near *Strat1Near[])() =
{                     /*--------------------------------------*/
   CmdErr,            /* 0x00  now an error                   */
   MediaCheck,        /* 0x01  check the media                */
   BuildBPB,          /* 0x02  build BPB                      */
   CmdErr,            /* 0x03  reserved                       */
   ReadWriteV,        /* 0x04  read                           */
   StatusDevReady,    /* 0x05  non-destructive read           */
   StatusComplete,    /* 0x06  input status                   */
   StatusComplete,    /* 0x07  input flush                    */
   ReadWriteV,        /* 0x08  write                          */
   ReadWriteV,        /* 0x09  write with verify              */
   CmdErr,            /* 0x0A  get output status              */
   CmdErr,            /* 0x0B  flush output                   */
   CmdErr,            /* 0x0C  reserved                       */
   StatusComplete,    /* 0x0D  open                           */
   StatusComplete,    /* 0x0E  close                          */
   RemovableMedia,    /* 0x0F  removable media                */
   DriveGenIOCTL,     /* 0x10  generic IOCTL                  */
   ResetMedia,        /* 0x11  reset uncertain media          */
   GetLogDriveMap,    /* 0x12  get Logical Drive Map          */
   SetLogDriveMap,    /* 0x13  set Logical Drive Map          */
   CmdErr,            /* 0x14  de-Install this device         */
   CmdErr,            /* 0x15  reserved                       */
   PartFixedDisks,    /* 0x16  get number of partitions       */
   GetUnitMap,        /* 0x17  get unit map                   */
   ReadWriteV,        /* 0x18  no caching read                */
   ReadWriteV,        /* 0x19  no caching write               */
   ReadWriteV,        /* 0x1A  no caching write/verify        */
   DriveInit,         /* 0x1B  initialize                     */
   CmdErr,            /* 0x1C  reserved for Request List code */
   GetDriverCaps      /* 0x1D  Get Driver Capabilities        */
};                    /*--------------------------------------*/

void near DMStrat1()
{
  PRPH          pRPH;
  NPVOLCB       pVolCB;
  USHORT        Cmd, Status;

  _asm
  {
     push es
     push bx
     mov word ptr pRPH[0], bx
     mov word ptr pRPH[2], es
  }

  pRPH->Status = 0;
  pRPH->Flags = 0;

  Cmd = pRPH->Cmd;

  /*-----------------------------*/
  /* Filter out invalid requests */
  /*-----------------------------*/

  if (Cmd > MAX_DISKDD_CMD)
  {
     Status = STDON + STERR + ERROR_I24_BAD_COMMAND;
     goto  ExitDiskDD;
  }

  if ((Get_VolCB_Addr(pRPH->Unit, (NPVOLCB FAR *) &pVolCB) != NO_ERROR) &&
                     (Cmd != CMDGenIOCTL) &&
                     (Cmd != CMDInitBase) &&
                     (Cmd != CMDPartfixeddisks) )
  {
      Status = STDON + STERR + ERROR_I24_BAD_UNIT;
      goto ExitDiskDD;
  }

  if ( (Cmd != CMDInitBase) && IsTraceOn() )
        Trace(TRACE_STRAT1 | TRACE_ENTRY, (PBYTE) pRPH, pVolCB);

  /*---------------------*/
  /* Call Worker Routine */
  /*---------------------*/

  Status = (*Strat1Near[Cmd])(pRPH, pVolCB);

  /*--------------------------------------------------------------------*/
  /* Finish up by setting the Status word in the Request Packet Header  */
  /*--------------------------------------------------------------------*/

ExitDiskDD:  ;

  DISABLE;

  pRPH->Status = Status;             /* Save status in Request Packet */

  ENABLE;

  if ( (TraceFlags != 0) && (pRPH->Status & STDON) && (Cmd != CMDInitBase) )
     Trace(TRACE_STRAT1 | TRACE_EXIT, (PBYTE) pRPH, pVolCB);


  _asm
  {
     pop bx
     pop es
  }
DMStrat1_Exit:
  _asm
  {
     LEAVE
     retf
  }

}

/*------------------------------------------------------------------------
;
;** MediaCheck - Check the Media    (Command = 0x01)
;
;   Checks to see if the media in the drive has changed.
;
;   USHORT MediaCheck   (PRP_MEDIACHECK pRP, NPVOLCB pVolCB)
;
;   ENTRY:    pRP              - Request Packet
;             pVolCB           - Pointer to VolCB
;
;   RETURN:   USHORT           - Packet Status word
;
;   EFFECTS:  The Return Code in the Request Packet is set to one
;             of the following:
;
;                 -1 = Media has been changed
;                  0 = Unsure if media has been changed
;                  1 = Media unchanged
;
------------------------------------------------------------------------*/
USHORT NEAR MediaCheck(pRP, pVolCB)

PRP_MEDIACHECK    pRP;
NPVOLCB           pVolCB;
{

   pRP->rc = w_MediaCheck (pRP->rph.Unit, pVolCB);

   return (pRP->rph.Status | STDON);
}

/*------------------------------------------------------------------------
;
;** BuildBPB - Build the BPB      (Command = 0x02)
;
;   Builds the BPB.  This is requested when the media has changed
;   of when the media type is uncertain.
;
;   USHORT BuildBPB   (PRP_BUILDBPB pRP, NPVOLCB pVolCB)
;
;   ENTRY:    pRP              - Request Packet
;             pVolCB           - Pointer to VolCB
;
;   RETURN:   USHORT           - Packet status word
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
USHORT near BuildBPB(pRP, pVolCB)
PRP_BUILDBPB   pRP;
NPVOLCB        pVolCB;
{
   USHORT rc;

   if ((rc = BPBFromBoot (pVolCB, (PDOSBOOTREC) pRP->XferAddr)) != NO_ERROR)
      rc = BPBFromScratch (pVolCB);

   if (rc == NO_ERROR)
   {
      pRP->MediaDescr = pVolCB->MediaBPB.MediaType;
      pRP->bpb = (PVOID) &(pVolCB->MediaBPB);
      rc = STDON;
   }
   else
      rc |= (STDON | STERR);

   return (rc);
}

/*------------------------------------------------------------------------
;
;** ReadWriteV - Read, Write and Write with Verify  (Commands 0x04, 0x08, 0x09)
;
;   These are the basic strategy-1 I/O routines for the driver.
;   The request is queued and sent to the adapter driver for
;   processing.
;
;   USHORT ReadWriteV   (PRP_RWV pRP, NPVOLCB pVolCB)
;
;   ENTRY:    pRP              - Request Packet
;             pVolCB           - Pointer to VolCB
;
;   RETURN:   USHORT           - Packet Status word
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
USHORT ReadWriteV (pRP, pVolCB)

PRP_RWV   pRP;
NPVOLCB   pVolCB;
{
   USHORT Uncertain;
   ULONG  EndSector;

   USHORT rc = NO_ERROR;

   if (pRP->NumSectors == 0)
      return (STDON);

   /* Check for a valid partition */

   if ((rc=CheckWithinPartition(pVolCB,pRP->rba,(ULONG)pRP->NumSectors)) & STERR)
      return(rc);

   /*--------------------------------------------------------------*/
   /*  If it's for a psuedo-drive which is not currently mapped to */
   /*  it's associated unit, then a disk swap is needed.           */
   /*--------------------------------------------------------------*/

   if  (pVolCB->pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE)
   {
      if (CheckPseudoChange(pRP->rph.Unit, pVolCB) == -1)
      {
         pRP->NumSectors = 0;
         return(STDON + STERR + ERROR_I24_DISK_CHANGE);
      }

      if ( (!(pVolCB->Flags & vf_ForceRdWrt)) &&
             (pVolCB->Flags & vf_UncertainMedia))
      {
         pRP->NumSectors = 0;
         return(STDON + STERR + ERROR_I24_UNCERTAIN_MEDIA);
      }
   }
   /*  Add in the partition offset and the hidden sectors and do the I/O */

   pRP->rba = pRP->rba + pVolCB->PartitionOffset+pVolCB->MediaBPB.HiddenSectors;
   pRP->sfn = 512;                      /* Use sfn field for sector size */
   pVolCB->Flags &= ~vf_ForceRdWrt;


   rc =  DiskIO ((PBYTE)pRP, pVolCB);

   return(rc);

}
/*------------------------------------------------------------------------
;
;** RemovableMedia - Check for Removable Media    (Command = 0x0F)
;
;   USHORT RemovableMedia (PRPH pRPH, NPVOLCB pVolCB)
;
;   ENTRY:    pRPH             - Request Packet Header
;             pVolCB           - Pointer to VolCB
;
;   RETURN:   USHORT           - Packet Status word
;
;   EFFECTS:  The busy bit of the status word is set as follows:
;
;                  1 = Media is non-removable
;                  0 = Media is removable
;
------------------------------------------------------------------------*/

USHORT RemovableMedia(pRPH, pVolCB)
PRPH    pRPH;
NPVOLCB pVolCB;
{
   if (pVolCB->pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE)
      return (STDON);
   else
      return (STDON + STBUI);

}

/*------------------------------------------------------------------------
;
;** ResetMedia - Reset Uncertain Media  (Command = 0x11)
;
;   USHORT ResetMedia (PRPH pRPH, NPVOLCB pVolCB)
;
;   ENTRY:    pRPH             - Request Packet Header
;             pVolCB           - Pointer to VolCB
;
;   RETURN:   USHORT           - Packet Status word
;
;   EFFECTS:
;
------------------------------------------------------------------------*/

USHORT ResetMedia (pRPH, pVolCB)

PRPH    pRPH;
NPVOLCB pVolCB;
{
  if ( !(pVolCB->Flags & vf_ReturnFakeBPB) )
     pVolCB->MediaBPB = pVolCB->RecBPB;

  pVolCB->Flags &= ~(vf_UncertainMedia | vf_ChangedByFormat | vf_Changed);

  pVolCB->Flags |= vf_ForceRdWrt;

  return (STDON);

}
/*------------------------------------------------------------------------
;
;** GetLogDriveMap - Get Logical Drive Mapping  (Command = 0x12)
;
;   Returns which logical drive is currently mapped onto a particular
;   physical drive.  A zero is returned if only one logical drive is
;   mapped to the physical drive.
;
;   USHORT GetLogDriveMap (PRPH pRPH, NPVOLCB pVolCBIn)
;
;   ENTRY:    pRPH             - Request Packet Header
;             pVolCBIn         - Pointer to VolCB
;
;   RETURN:   USHORT           - Packet Status word
;
;   EFFECTS: The logical drive is returned in the unit field
;            of the request packet header.  The logical drive
;            is actually LogDriveNum + 1, which represents the
;            drive letter, i.e. C: = 3.  A zero is returned
;            if only one logical drive is mapped to the physical drive.
;
------------------------------------------------------------------------*/

USHORT GetLogDriveMap (pRPH, pVolCB)

PRPH    pRPH;
NPVOLCB pVolCB;
{
   NPVOLCB pVolCBx;

   for (pVolCBx = VolCB_Head; pVolCBx != NULL; pVolCBx = pVolCBx->pNextVolCB)
   {
      if ( (pVolCBx->pUnitCB->PhysDriveNum == pVolCB->pUnitCB->PhysDriveNum) &&
           (pVolCBx->Flags & vf_OwnPhysical) )
         break;
   }

   if (pVolCBx == NULL)
      return (STDON + STERR + ERROR_I24_BAD_UNIT);  /* Unit not found */

   if (pVolCBx->Flags & vf_AmMult)
      pRPH->Unit = pVolCBx->LogDriveNum + 1;  /* ret drive letter value */
   else
      pRPH->Unit = 0;                        /* ret 0 if one drive mapped */

   return (STDON);

}
/*------------------------------------------------------------------------
;
;** SetLogDriveMap - Set Logical Drive Mapping  (Command = 0x13)
;
;   Maps the specified logical drive onto the physical drive.
;
;   USHORT SetLogDriveMap (PRPH pRPH, NPVOLCB pVolCB)
;
;   ENTRY:    pRPH             - Request Packet Header
;             pVolCB           - Pointer to VolCB
;
;   RETURN:   USHORT           - Packet Status word
;
;   EFFECTS: The logical drive is returned in the unit field
;            of the request packet header.  The logical drive
;            is actually LogDriveNum + 1, which represents the
;            drive letter, i.e. C: = 3.  A zero is returned
;            if only one logical drive is mapped to the physical drive.
;
------------------------------------------------------------------------*/

USHORT SetLogDriveMap (pRPH, pVolCB)

PRPH    pRPH;
NPVOLCB pVolCB;
{
   Update_Owner (pVolCB);                    /* Update vf_OwnPhysical */

   if (pVolCB->Flags & vf_AmMult)
      pRPH->Unit = pVolCB->LogDriveNum + 1;   /* If mapped, ret drive letter */
   else
      pRPH->Unit = 0;                         /* else ret 0 */

   return (STDON);
}


/*------------------------------------------------------------------------
;
;** PartFixedDisks - Get number of fixed disks  (Command = 0x16)
;
;   Returns the number of fixed disks supported by the driver.
;
;   USHORT PartFixedDisks (PRP_PARTFIXEDDISKS pRP, NPVOLCB pVolCB)
;
;   ENTRY:    pRP              - Request Packet
;             pVolCB           - Pointer to VolCB
;
;   RETURN:   USHORT           - Packet Status word
;
;   EFFECTS:
;
------------------------------------------------------------------------*/

USHORT PartFixedDisks (pRP, pVolCB)

PRP_PARTFIXEDDISKS  pRP;
NPVOLCB             pVolCB;
{
   pRP->NumFixedDisks = (UCHAR) NumFixedDisks;

   return (STDON);
}

/*------------------------------------------------------------------------
;
;** GetUnitMap - Get logical units mapped to a physical unit (Command = 0x17)
;
;   Looks at all the VolCB entries and builds a double
;   word bit map that indicates what logical drives exist
;   on the given physical drive.  The low order bit of the
;   map corresponds to logical unit 0 (A:).  Bits are set for
;   those units that exist on the given physical drive.
;
;   USHORT GetUnitMap (PRP_GETUNITMAP pRP, NPVOLCB pVolCB)
;
;   ENTRY:    pRP              - Request Packet
;             pVolCB           - Pointer to VolCB
;
;   RETURN:   USHORT           - Packet Status word
;
;   EFFECTS:
;
------------------------------------------------------------------------*/

USHORT GetUnitMap (pRP, pVOLCB)

PRP_GETUNITMAP  pRP;
NPVOLCB         pVOLCB;
{
   NPVOLCB pVolCBx;
   UCHAR   PhysUnit;
   ULONG   Mask = 1;
   ULONG   UnitMap = 0;

   if (NumFixedDisks == 0 || pRP->rph.Unit >= NumFixedDisks)
      return (STDON + STERR + ERROR_I24_BAD_UNIT);

   PhysUnit = pRP->rph.Unit | 0x80;         /* Make disk 0x80 based */

   for (pVolCBx=VolCB_Head; pVolCBx->LogDriveNum != 0x80;
                                                 pVolCBx=pVolCBx->pNextVolCB)
   {
      /* Unit on specified drive ? */

      if (pVolCBx->pUnitCB->PhysDriveNum == PhysUnit)
        UnitMap |= Mask;

      Mask <<= 1;                      /* Adjust mask for next unit */
   }

   pRP->UnitMap = UnitMap;             /* Return unit mask */

   return (STDON);
}


/*------------------------------------------------------------------------
;
;** GetDriverCapabilites - Get Device Driver Capabilities  (Command = 0x1D)
;
;   Returns informations describing the functional characteristics
;   of the device driver.  The control blocks returned are the
;   Driver Capabilites Structure and the Volume Characteristics
;   structure.  This command must be supported to support the
;   Strategy-2, extended device driver interface.
;
;   USHORT GetUnitMap (PRP_GETDRIVERCAPS pRP, NPVOLCB pVolCB)
;
;   ENTRY:    pRP              - Request Packet
;             pVolCB           - Pointer to VolCB
;
;   RETURN:   USHORT           - Packet Status word
;
;   EFFECTS:
;
------------------------------------------------------------------------*/

USHORT GetDriverCaps (pRP, pVolCB)

PRP_GETDRIVERCAPS  pRP;
NPVOLCB            pVolCB;
{
   pRP->pDCS = (PVOID) &DriverCapabilities;
   pRP->pVCS = (PVOID) pVolCB->pVolChar;
   return (STDON);
}

/*-----------------------------------------------*/
/* Near Entry Point to swappable IOCTL routine.  */
/*-----------------------------------------------*/

USHORT DriveGenIOCTL(pRP, pVolCB)

PRP_GENIOCTL pRP;
NPVOLCB      pVolCB;
{
  USHORT rc;

  rc = f_DriveGenIOCTL (pRP, pVolCB);

  rc |= STDON;

  return(rc);

}


/*-----------------------------------------------------------*/
/*   Packet Status return functions:                         */
/*                                                           */
/*   CmdErr, StatusError, StatusDevReady, StatusComplete     */
/*-----------------------------------------------------------*/

USHORT near CmdErr (pRPH, pVolCB)
PRPH    pRPH;
NPVOLCB pVolCB;
{
   return (STERR + STDON + ERROR_I24_BAD_COMMAND);
}


USHORT StatusDevReady(pRPH, pVolCB)
PRPH    pRPH;
NPVOLCB pVolCB;
{
  return (STDON + STBUI);
}

USHORT StatusComplete(pRPH, pVolCB)
PRPH    pRPH;
NPVOLCB pVolCB;
{
  return (STDON);
}


USHORT StatusError( pRPH, ErrorCode )

PRPH    pRPH;
USHORT  ErrorCode;
{
  return (STDON + STERR);
}


/*------------------------------------------------------------------------
;
;** DiskIO - Common disk access routine
;
;   DiskIO is the common routine for all operations that need to
;   be performed on a device.
;
;   USHORT DiskIO (PBYTE pRP, NPVOLCB pVolCB)
;
;   ENTRY:    pRP              - Request Packet
;             pVolCB           - Pointer to VolCB
;
;   RETURN:   USHORT           - Packet Status word
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
USHORT DiskIO (pRP, pVolCB)

PBYTE    pRP;
NPVOLCB  pVolCB;

{
   NPUNITCB pUnitCB1 = 0;
   NPUNITCB pUnitCB2 = 0;

  /* -------------------------------------------------------------
   ; Check to see if the request is for zero sectors, and if     ;
   ; so simply return.  We must do this as the LAST check to     ;
   ; make sure that various status flags are properly set (if    ;
   ; we had a floppy change error, and the retry was for         ;
   ; zero bytes in length, we would not release the floppy       ;
   ; change semaphore).                                          ;
   --------------------------------------------------------------*/


// if (pRP->NumSectors == 0)
//    return (STDON);

   ((PRPFT)pRP)->ftdb.FT_Flags = 0;

   if ( FT_CheckFTSupport(pVolCB->pUnitCB, (PBYTE) pRP) == YES)
   {
         FT_PutPriorityQueue(pVolCB->pUnitCB, pRP,
                     (NPUNITCB FAR *) &pUnitCB1, (NPUNITCB FAR *) &pUnitCB2);

         if (pUnitCB1 != 0)
            SubmitRequestsToADD(pUnitCB1);

         if (pUnitCB2 != 0)
            SubmitRequestsToADD(pUnitCB2);
   }
   else
   {
      PutPriorityQueue_RP (pVolCB->pUnitCB, pRP);

      SubmitRequestsToADD (pVolCB->pUnitCB);
   }
   DISABLE;

   if ( !( ((PRPH)pRP)->Status & STDON) )
      ((PRPH)pRP)->Status = STBUI;

   return(((PRPH)pRP)->Status);

}

/*------------------------------------------------------------------------
;
;** DiskIO_Wait - Perform disk I/O, blocking until done
;
;   Calls DiskIO, and blocks this thread until the operation
;   is complete, i.e. the STDON bit is set in the request packet.
;
;   USHORT DiskIO_Wait (PBYTE pRPH, NPVOLCB pVolCB)
;
;   ENTRY:    pRP              - Request Packet
;             pVolCB           - Pointer to VolCB
;
;   RETURN:   USHORT           - Packet Status word
;
;   EFFECTS:
;
------------------------------------------------------------------------*/
USHORT FAR f_DiskIO_Wait (pRP, pVolCB)

PBYTE pRP;
NPVOLCB pVolCB;
{

   return(DiskIO_Wait(pRP, pVolCB));
}


USHORT DiskIO_Wait (pRP, pVolCB)

PBYTE pRP;
NPVOLCB pVolCB;

{
   ((PRPH)pRP)->Status = 0;

   /* Block until I/O done.  Infinite timeout, Non interruptable. */

   DiskIO (pRP, pVolCB);                /* Perform the I/O       */
   DISABLE;
   while ( !( ((PRPH)pRP)->Status & STDON ) )
   {
      DevHelp_ProcBlock ((ULONG)pRP, -1L, 1);
      DISABLE;                          /* Block does an enable  */
   }
   ENABLE;

   return ( ((PRPH)pRP)->Status);   /* return packet status  */
}

