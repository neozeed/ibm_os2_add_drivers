/*static char *SCCSID = "@(#)fl2iorb.c	6.5 92/01/30";*/
/*****************************************************************************/
/*                                                                           */
/*   Component   : IBM PS/2 Floppy Adapter Device Driver  (IBM2FLPY)         */
/*                                                                           */
/*   Module      : FL2IORB.C                                                 */
/*                                                                           */
/*   Description :                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

#define INCL_NOBASEAPI
#define INCL_NOPMAPI
#include <os2.h>
#include <dhcalls.h>
#include <strat2.h>     /* needed to keep reqpkt.h happy */
#include <reqpkt.h>
#include <scb.h>        /* needed to keey abios.h happy */
#include <abios.h>
#include <iorb.h>
#include <addcalls.h>
#include "fl2def.h"
#include "fl2proto.h"
#include "fl2data.h"



/*****************************************************************************/
/*                                                                           */
/*   Routine     : NextIORB                                                  */
/*                                                                           */
/*   Description : This routine starts the processing of the IORB at         */
/*                 the head of the list.  This routine checks for            */
/*                 valid unit handle and that the unit is allocated.         */
/*                 The IORB is then routed to the appropriate                */
/*                 processsing routine.                                      */
/*                                                                           */
/*****************************************************************************/

VOID FAR NextIORB()
{
   if ( pHeadIORB->CommandCode != IOCC_CONFIGURATION )
      {
         /* Verify that UnitHandle is valid */
         if ( pHeadIORB->UnitHandle >= UnitCnt )
            {
               /* UnitHandle is out of legal range */
               pHeadIORB->Status   |= IORB_ERROR;
               pHeadIORB->ErrorCode = IOERR_CMD_SYNTAX;
               IORBDone();
               return;
            }

         /* Verify that unit is allocated */
         if ( pHeadIORB->CommandCode != IOCC_UNIT_CONTROL )
            {
               if ( !Drive[pHeadIORB->UnitHandle].Flags.Allocated )
                  {
                     /* Unit not allocated */
                     pHeadIORB->Status   |= IORB_ERROR;
                     pHeadIORB->ErrorCode = IOERR_UNIT_NOT_ALLOCATED;
                     IORBDone();
                     return;
                  }
            }
      }

   /* Route the IORB to the proper routine */
   switch( pHeadIORB->CommandCode )
      {
         case IOCC_CONFIGURATION:
            switch( pHeadIORB->CommandModifier )
               {
                  case IOCM_GET_DEVICE_TABLE: GetDeviceTable();  break;
                  case IOCM_COMPLETE_INIT:    CompleteInit();    break;
                  default:                    CmdNotSupported(); break;
               }
            break;

         case IOCC_UNIT_CONTROL:
            switch( pHeadIORB->CommandModifier )
               {
                  case IOCM_ALLOCATE_UNIT:   AllocateUnit();    break;
                  case IOCM_DEALLOCATE_UNIT: DeallocateUnit();  break;
                  case IOCM_CHANGE_UNITINFO: ChangeUnitInfo();  break;
                  default:                   CmdNotSupported(); break;
               }
            break;

         case IOCC_GEOMETRY:
            switch( pHeadIORB->CommandModifier )
               {
                  case IOCM_GET_MEDIA_GEOMETRY:   GetMediaGeometry();   break;
                  case IOCM_SET_MEDIA_GEOMETRY:   SetMediaGeometry();   break;
                  case IOCM_GET_DEVICE_GEOMETRY:  GetDeviceGeometry();  break;
                  case IOCM_SET_LOGICAL_GEOMETRY: SetLogicalGeometry(); break;
                  default:                        CmdNotSupported();    break;
               }
            break;

         case IOCC_EXECUTE_IO:
            switch( pHeadIORB->CommandModifier )
               {
                  case IOCM_READ:          IO();              break;
                  case IOCM_READ_VERIFY:   IO();              break;
                  case IOCM_WRITE:         IO();              break;
                  case IOCM_WRITE_VERIFY:  IO();              break;
                  default:                 CmdNotSupported(); break;
               }
            break;

         case IOCC_FORMAT:
            switch( pHeadIORB->CommandModifier )
               {
                  case IOCM_FORMAT_TRACK: Format();          break;
                  default:                CmdNotSupported(); break;
               }
            break;

         case IOCC_UNIT_STATUS:
            switch( pHeadIORB->CommandModifier )
               {
                  case IOCM_GET_UNIT_STATUS:      GetUnitStatus();      break;
                  case IOCM_GET_CHANGELINE_STATE: GetChangelineState(); break;
                  case IOCM_GET_MEDIA_SENSE:      GetMediaSense();      break;
                  default:                        CmdNotSupported();    break;
               }
            break;

         default: CmdNotSupported(); break;
      }
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : GetDeviceTable                                            */
/*                                                                           */
/*   Description : This routine builds a device table in the buffer          */
/*                 specified by the caller.                                  */
/*                                                                           */
/*                 Note: If a filter ADD has previously issued a             */
/*                       change unit info IORB, then the changed unit        */
/*                       information is placed in the device table.          */
/*                                                                           */
/*****************************************************************************/

VOID NEAR GetDeviceTable()
{
   PIORB_CONFIGURATION pIORB = (PIORB_CONFIGURATION)pHeadIORB;
   USHORT LengthNeeded,i;
   ADAPTERINFO far *pADPT;

   LengthNeeded = sizeof(DEVICETABLE) + sizeof(ADAPTERINFO) +
                  ((UnitCnt-1) * sizeof(UNITINFO));

   if ( pIORB->DeviceTableLen < LengthNeeded )
      {
         /* Not enough storage */
         pHeadIORB->Status |= IORB_ERROR;
         pHeadIORB->ErrorCode = IOERR_CMD_SYNTAX;
         IORBDone();
         return;
      }

   pIORB->pDeviceTable->ADDLevelMajor = ADD_LEVEL_MAJOR;
   pIORB->pDeviceTable->ADDLevelMinor = ADD_LEVEL_MINOR;
   pIORB->pDeviceTable->ADDHandle     = ADDHandle;
   pIORB->pDeviceTable->TotalAdapters = 1;
   pIORB->pDeviceTable->pAdapter[0]   =
      (NPADAPTERINFO)( OFFSETOF(pIORB->pDeviceTable) + sizeof(DEVICETABLE) );

   SELECTOROF(pADPT) = SELECTOROF(pIORB->pDeviceTable);
   OFFSETOF  (pADPT) = (USHORT)(pIORB->pDeviceTable->pAdapter[0]);

   /* Copy the adapter name */
   for ( i=0; (pADPT->AdapterName[i]=AdapterName[i])!=0; i++ )
      if ( i == 17 ) break;

   pADPT->AdapterUnits    = UnitCnt;
   pADPT->AdapterDevBus   = AI_DEVBUS_FLOPPY;
   pADPT->AdapterIOAccess = AI_IOACCESS_DMA_SLAVE;
   pADPT->AdapterHostBus  = AI_HOSTBUS_uCHNL | AI_BUSWIDTH_32BIT;
   pADPT->AdapterSCSITargetID = 0;
   pADPT->AdapterSCSILUN  = 0;
   pADPT->AdapterFlags    = AF_CHS_ADDRESSING;
   pADPT->MaxHWSGList     = 0;
   pADPT->MaxCDBTransferLength = 0L;

   for ( i=0; i<UnitCnt; i++ )
      {
         if ( Drive[i].pUnitInfo == NULL )  /* If unit info was not changed */
            {
               pADPT->UnitInfo[i].AdapterIndex     = 0;
               pADPT->UnitInfo[i].UnitIndex        = i;
               pADPT->UnitInfo[i].UnitHandle       = i;
               pADPT->UnitInfo[i].FilterADDHandle  = 0;
               pADPT->UnitInfo[i].UnitType         = UIB_TYPE_DISK;
               pADPT->UnitInfo[i].QueuingCount     = 1;
               pADPT->UnitInfo[i].UnitSCSITargetID = 0;
               pADPT->UnitInfo[i].UnitSCSILUN      = 0;

               pADPT->UnitInfo[i].UnitFlags = UF_REMOVABLE | UF_NOSCSI_SUPT;

               if ( Drive[i].Flags.HasChangeLine )
                  pADPT->UnitInfo[i].UnitFlags |= UF_CHANGELINE;

               if ( i == 0 )          /* First drive */
                  {
                     pADPT->UnitInfo[i].UnitFlags |= UF_A_DRIVE;
                     if ( UnitCnt == 1 )  /* Single drive is both A: and B: */
                        pADPT->UnitInfo[i].UnitFlags |= UF_B_DRIVE;
                  }
               else if ( i == 1 )     /* Second drive */
                  pADPT->UnitInfo[i].UnitFlags |= UF_B_DRIVE;
            }
         else  /* Pass back the changed unit info */
            {
               pADPT->UnitInfo[i].AdapterIndex     = Drive[i].pUnitInfo->AdapterIndex;
               pADPT->UnitInfo[i].UnitIndex        = Drive[i].pUnitInfo->UnitIndex;
               pADPT->UnitInfo[i].UnitHandle       = Drive[i].pUnitInfo->UnitHandle;
               pADPT->UnitInfo[i].FilterADDHandle  = Drive[i].pUnitInfo->FilterADDHandle;
               pADPT->UnitInfo[i].UnitType         = Drive[i].pUnitInfo->UnitType;
               pADPT->UnitInfo[i].QueuingCount     = Drive[i].pUnitInfo->QueuingCount;
               pADPT->UnitInfo[i].UnitSCSITargetID = Drive[i].pUnitInfo->UnitSCSITargetID;
               pADPT->UnitInfo[i].UnitSCSILUN      = Drive[i].pUnitInfo->UnitSCSILUN;
               pADPT->UnitInfo[i].UnitFlags        = Drive[i].pUnitInfo->UnitFlags;
            }
      }

   IORBDone();
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : AllocateUnit                                              */
/*                                                                           */
/*   Description : If unit not already allocated then mark it as             */
/*                 allocated now.                                            */
/*                                                                           */
/*****************************************************************************/

VOID NEAR AllocateUnit()
{
   if ( Drive[pHeadIORB->UnitHandle].Flags.Allocated )
      {
         /* Unit already allocated */
         pHeadIORB->Status   |= IORB_ERROR;
         pHeadIORB->ErrorCode = IOERR_UNIT_ALLOCATED;
      }
   else Drive[pHeadIORB->UnitHandle].Flags.Allocated = TRUE;

   IORBDone();
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : DeallocateUnit                                            */
/*                                                                           */
/*   Description : If unit is allocted then mark it as available.            */
/*                                                                           */
/*****************************************************************************/

VOID NEAR DeallocateUnit()
{
   if ( !Drive[pHeadIORB->UnitHandle].Flags.Allocated )
      {
         /* Unit not allocated */
         pHeadIORB->Status   |= IORB_ERROR;
         pHeadIORB->ErrorCode = IOERR_UNIT_NOT_ALLOCATED;
      }
   else Drive[pHeadIORB->UnitHandle].Flags.Allocated = FALSE;

   IORBDone();
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : ChangeUnitInfo                                            */
/*                                                                           */
/*   Description : Save the pointer to the new unit information.  This       */
/*                 unit information is used when building subsequent         */
/*                 device tables.                                            */
/*                                                                           */
/*****************************************************************************/

VOID NEAR ChangeUnitInfo()
{
   PIORB_UNIT_CONTROL pIORB = (PIORB_UNIT_CONTROL)pHeadIORB;

   if ( !Drive[pHeadIORB->UnitHandle].Flags.Allocated )
      {
         /* Unit not allocated */
         pHeadIORB->Status   |= IORB_ERROR;
         pHeadIORB->ErrorCode = IOERR_UNIT_NOT_ALLOCATED;
         IORBDone();
         return;
      }

   /* Save the Unit Info pointer */
   Drive[pHeadIORB->UnitHandle].pUnitInfo = pIORB->pUnitInfo;

   IORBDone();
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : GetUnitStatus                                             */
/*                                                                           */
/*   Description : PS/2 units are always powered on and ready for use.       */
/*                                                                           */
/*****************************************************************************/

VOID NEAR GetUnitStatus()
{
   PIORB_UNIT_STATUS pIORB = (PIORB_UNIT_STATUS)pHeadIORB;

   pIORB->UnitStatus = US_READY | US_POWER;

   IORBDone();
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : GetChangelineState                                        */
/*                                                                           */
/*   Description : Issue the ABIOS command to Read Change Signal.            */
/*                                                                           */
/*****************************************************************************/

VOID NEAR GetChangelineState()
{
   PIORB_UNIT_STATUS pIORB = (PIORB_UNIT_STATUS)pHeadIORB;
   NPABRB_DSKT_READCHGLINE pRB = (NPABRB_DSKT_READCHGLINE)RequestBlock;

   pRB->abrbh.Function  = ABFC_DSKT_READ_CHGSIGNAL;
   pRB->abrbh.Unit      = pIORB->iorbh.UnitHandle;
   pRB->abrbh.RC        = ABRC_START;
   pRB->Reserved_1      = 0;     /* +16H reserved field  */

   CompletionRoutine = GetChangelineStateComplete;
   Retry = 0;
   Stage = ABIOS_EP_START;
   NextStage();
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : GetChangelineStateComplete                                */
/*                                                                           */
/*   Description : This routine is called when the ABIOS Read Change         */
/*                 Line command finishes.  Updates the IORB with the         */
/*                 changeline state.                                         */
/*                                                                           */
/*****************************************************************************/

VOID FAR GetChangelineStateComplete()
{
   PIORB_UNIT_STATUS pIORB = (PIORB_UNIT_STATUS)pHeadIORB;
   NPABRB_DSKT_READCHGLINE pRB = (NPABRB_DSKT_READCHGLINE)RequestBlock;

   if ( pRB->abrbh.RC == ABRC_COMPLETEOK )
      {
         if ( pRB->ChangeLineStatus & CHANGELINE_ACTIVE )
            pIORB->UnitStatus = US_CHANGELINE_ACTIVE;
         else
            pIORB->UnitStatus = 0;
      }
   else pIORB->iorbh.ErrorCode = TranslateErrorCode( pRB->abrbh.RC );

   StartTimer( pIORB->iorbh.UnitHandle,
               Drive[pIORB->iorbh.UnitHandle].MotorOffDelay ,TurnOffMotor );

   IORBDone();
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : GetMediaSense                                             */
/*                                                                           */
/*   Description : Issues the ABIOS command Get Media Type.                  */
/*                                                                           */
/*****************************************************************************/

VOID NEAR GetMediaSense()
{
   PIORB_UNIT_STATUS pIORB = (PIORB_UNIT_STATUS)pHeadIORB;
   NPABRB_DSKT_GETMEDIATYPE pRB = (NPABRB_DSKT_GETMEDIATYPE)RequestBlock;

   pRB->abrbh.Function  = ABFC_DSKT_GET_MEDIA_TYPE;
   pRB->abrbh.Unit      = pIORB->iorbh.UnitHandle;
   pRB->abrbh.RC        = ABRC_START;
   pRB->Reserved_1      = 0;     /* +16H reserved field  */

   CompletionRoutine = GetMediaSenseComplete;
   Retry = 0;
   Stage = ABIOS_EP_START;
   NextStage();
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : GetMediaSenseComplete                                     */
/*                                                                           */
/*   Description : This routine is called when the ABIOS Get Media Type      */
/*                 command completes.  The IORB is updated with the          */
/*                 type of media in the drive.                               */
/*                                                                           */
/*****************************************************************************/

VOID FAR GetMediaSenseComplete()
{
   PIORB_UNIT_STATUS pIORB = (PIORB_UNIT_STATUS)pHeadIORB;
   NPABRB_DSKT_GETMEDIATYPE pRB = (NPABRB_DSKT_GETMEDIATYPE)RequestBlock;

   if ( pRB->abrbh.RC == ABRC_COMPLETEOK )
      {
         switch( pRB->MediaType )
            {
               case MEDIA1MB: pIORB->UnitStatus = US_MEDIA_720KB;   break;
               case MEDIA2MB: pIORB->UnitStatus = US_MEDIA_144MB;   break;
               case MEDIA4MB: pIORB->UnitStatus = US_MEDIA_288MB;   break;
               default:       pIORB->UnitStatus = US_MEDIA_UNKNOWN; break;
            }
      }
   else pIORB->iorbh.ErrorCode = TranslateErrorCode( pRB->abrbh.RC );

   if ( pIORB->iorbh.ErrorCode == IOERR_MEDIA_CHANGED )
      Drive[pIORB->iorbh.UnitHandle].Flags.UnknownMedia = TRUE;

   StartTimer( pIORB->iorbh.UnitHandle,
               Drive[pIORB->iorbh.UnitHandle].MotorOffDelay ,TurnOffMotor );

   IORBDone();
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : CompleteInit                                              */
/*                                                                           */
/*   Description : This IORB is issued by the loader when the booting        */
/*                 sequence has finished.  When booting OS/2 from            */
/*                 floppy, prior to this IORB, the floppy controller is      */
/*                 driven by INT 13s.  After this IORB, the floppy is        */
/*                 driven by this ADD.  This routine hooks the IRQ and       */
/*                 resets the diskette controller.                           */
/*                                                                           */
/*****************************************************************************/

VOID NEAR CompleteInit()
{
   NPABRBH pRB = (NPABRBH)RequestBlock;
   USHORT  Unit;

   /* Hook the IRQ */
   if ( DevHelp_SetIRQ( (NPFN)IntHandler, IntLevel, 1 ) )
      {
         pResumeIORB->Status |= IORB_ERROR;
         pResumeIORB->ErrorCode = IOERR_CMD_SW_RESOURCE;
      }
   else /* Successfully hooked the IRQ */
      {
         Reset();
         while( GFlags.Resetting );
         if ( pRB->RC != ABRC_COMPLETEOK )
            {
               pResumeIORB->Status |= IORB_ERROR;
               pResumeIORB->ErrorCode = IOERR_ADAPTER_NONSPECIFIC;
               DevHelp_UnSetIRQ( IntLevel );
            }
         else
            {
               GFlags.BootComplete = TRUE;
               for ( Unit=0; Unit<UnitCnt; Unit++ )
                  Drive[Unit].Flags.UnknownMedia = TRUE;
            }
      }

   IORBDone();
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : CmdNotSupported                                           */
/*                                                                           */
/*   Description : IORBs, which are not supported by this ADD, are           */
/*                 routed to this routine.  The appropriate error code       */
/*                 is set and the IORB is returned.                          */
/*                                                                           */
/*****************************************************************************/

VOID NEAR CmdNotSupported()
{
   pHeadIORB->Status   |= IORB_ERROR;
   pHeadIORB->ErrorCode = IOERR_CMD_NOT_SUPPORTED;

   IORBDone();
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : TranslateErrorCode                                        */
/*                                                                           */
/*   Description : This routine translates ABIOS return codes to             */
/*                 IORB return codes.                                        */
/*                                                                           */
/*****************************************************************************/

USHORT NEAR TranslateErrorCode( USHORT ABIOSRetCode )
{
   /* Convert ABIOS return code to IORB return code */
   switch( ABIOSRetCode )
      {
         case ABRC_INVALID_PARM:            return IOERR_CMD_ADD_SOFTWARE_FAILURE;
         case ABRC_DSKT_NOCHGSIG:           return IOERR_CMD_NOT_SUPPORTED;
         case ABRC_DSKT_NO_MEDIA_SENSE:     return IOERR_CMD_NOT_SUPPORTED;
         case ABRC_UNSUPPORTED_FUNCTION:    return IOERR_CMD_NOT_SUPPORTED;

         case ABRC_DSKT_INVALID_VALUE:      return IOERR_DEVICE_NONSPECIFIC;

         case ABRC_DSKT_ADDRMARK_NOTFND:    return IOERR_RBA_ADDRESSING_ERROR;
         case ABRC_DSKT_SECTOR_NOTFND:      return IOERR_RBA_ADDRESSING_ERROR;
         case ABRC_DSKT_BAD_CRC:            return IOERR_RBA_CRC_ERROR;

         case ABRC_DSKT_MEDIA_NOT_PRESENT:  return IOERR_MEDIA_NOT_PRESENT;
         case ABRC_DSKT_WRITE_PROTECT:      return IOERR_MEDIA_WRITE_PROTECT;
         case ABRC_DSKT_MEDIA_CHANGED:      return IOERR_MEDIA_CHANGED;
         case ABRC_DSKT_MEDIA_NOTSUPPORTED: return IOERR_MEDIA_NOT_SUPPORTED;
         case ABRC_DSKT_UNKNOWN_MEDIA:      return IOERR_MEDIA_NOT_SUPPORTED;

         case ABRC_DSKT_RESET_FAIL:         return IOERR_ADAPTER_NONSPECIFIC;
         case ABRC_DSKT_BAD_CONTROLLER:     return IOERR_ADAPTER_NONSPECIFIC;
         case ABRC_DSKT_GENERAL_ERROR:      return IOERR_ADAPTER_NONSPECIFIC;

         case ABRC_DSKT_BAD_SEEK:           return IOERR_DEVICE_NONSPECIFIC;
         case ABRC_BUSY:                    return IOERR_DEVICE_BUSY;
         case ABRC_DSKT_DMA_IN_PROGRESS:    return IOERR_DEVICE_BUSY;
         case ABRC_DSKT_DMA_OVERRUN:        return IOERR_DEVICE_OVERRUN;

         default:                           return IOERR_CMD_ADD_SOFTWARE_FAILURE;
      }
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : TurnOffMotor                                              */
/*                                                                           */
/*   Description : This routine is called by the timer when the motor        */
/*                 off delay has expired.  This routine calls ABIOS          */
/*                 to turn the diskette drive motor off.                     */
/*                                                                           */
/*****************************************************************************/

VOID FAR TurnOffMotor( USHORT Unit )
{
   NPABRB_GENERIC pRBG = (NPABRB_GENERIC)MotorOffReqBlk;

   pRBG->abrbh.Unit = Unit;
   pRBG->abrbh.RC   = ABRC_START;
   pRBG->Offset16H  = 0;          /* +16H reserved field */

   DevHelp_ABIOSCall( LID, (NPBYTE)pRBG, ABIOS_EP_START );
}


