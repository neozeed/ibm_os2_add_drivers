/*static char *SCCSID = "@(#)fl2init.c	6.6 92/01/30";*/
/*****************************************************************************/
/*                                                                           */
/*   Component   : IBM PS/2 Floppy Adapter Device Driver  (IBM2FLPY)         */
/*                                                                           */
/*   Module      : FL2INIT.C                                                 */
/*                                                                           */
/*   Description : Routines to initialize the ADD                            */
/*                                                                           */
/*****************************************************************************/

#define INCL_NOPMAPI
#define INCL_NOBASEAPI
#include <os2.h>
#include <dhcalls.h>
#include <infoseg.h>
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
/*   Routine     : InitCode                                                  */
/*                                                                           */
/*   Description : This routine does nothing.  It is simply the first        */
/*                 routine of the initialization code.  When                 */
/*                 initialization is complete, then this routine and         */
/*                 all following routines will be discarded.                 */
/*                                                                           */
/*****************************************************************************/

VOID NEAR InitCode(){}  /* Marks the beginning of initialization code */


/*****************************************************************************/
/*                                                                           */
/*   Routine     : InitFloppy                                                */
/*                                                                           */
/*   Description : This routine initializes the floppy controller.           */
/*                                                                           */
/*****************************************************************************/

VOID FAR InitFloppy( PRPINITIN pRPInitIn )
{
   PRPINITOUT pRPInitOut = (PRPINITOUT)pRPInitIn;
   NPABRBH pRB  = (NPABRBH)RequestBlock;
   BOOL Initialization = SUCCESS;

   /* Save the DevHelp address */
   Device_Help = pRPInitIn->DevHlpEP;

   if ( !DevHelp_GetLIDEntry( DEVID_DISKETTE, 0, 0, &LID ) )
      {
         if ( GetParameters() == SUCCESS )
            {
               if ( !DevHelp_GetDeviceBlock( LID, &pDeviceBlock ) )
                  {
                     if ( InitTimer() == SUCCESS )
                        {
                           if ( SetupDMABuffer() == SUCCESS )
                              {
                                 if ( !DevHelp_AllocateCtxHook( (NPFN)HookHandler, &HookHandle ) )
                                    {
                                       if ( DevHelp_RegisterDeviceClass( AdapterName,
                                             (PFN)ADDEntryPoint, 0, 1, &ADDHandle ) )
                                          {
                                             Initialization = FAILURE;
                                             DevHelp_FreeCtxHook( HookHandle );
                                          }
                                    }
                                 else Initialization = FAILURE;

                                 if ( Initialization == FAILURE )
                                    {
                                       DevHelp_FreeGDTSelector( SELECTOROF(pDMABuffer) );
                                       if ( pReadBackBuffer != NULL )
                                          DevHelp_FreeGDTSelector( SELECTOROF(pReadBackBuffer) );
                                    }
                              }
                           else Initialization = FAILURE;

                           if ( Initialization == FAILURE )
                              DevHelp_ResetTimer( (NPFN)TimerHandler );
                        }
                     else Initialization = FAILURE;
                  }
               else Initialization = FAILURE;
            }
         else Initialization = FAILURE;

         if ( Initialization == FAILURE )
            {
               DevHelp_FreeLIDEntry( LID );
               pRPInitOut->rph.Status = STATUS_ERR_GENFAIL;
            }
      }
   else  /* Couldn't get a LID.  Must not be a PS/2. */
      {
         Initialization = FAILURE;
         pRPInitOut->rph.Status = STATUS_QUIET_FAIL;
      }


   if ( Initialization == SUCCESS )
      {
         pRPInitOut->CodeEnd  = (USHORT)InitCode;
         pRPInitOut->DataEnd  = (USHORT)&InitData;
      }
   else
      {
         pRPInitOut->CodeEnd  = 0;
         pRPInitOut->DataEnd  = 0;
      }

   pRPInitOut->Unit     = 0;
   pRPInitOut->BPBArray = NULL;
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : GetParameters                                             */
/*                                                                           */
/*   Description : The ABIOS Return LID Parameters function is called        */
/*                 to get the interrupt level and number of units on         */
/*                 the floppy controller.                                    */
/*                                                                           */
/*                 The ABIOS Read Device Parameters function is called       */
/*                 for each unit to get the device geometry for each         */
/*                 unit.                                                     */
/*                                                                           */
/*****************************************************************************/

BOOL NEAR GetParameters()
{
   NPABRB_RETLIDPARMS       pRBLID = (NPABRB_RETLIDPARMS)RequestBlock;
   NPABRB_DSKT_READDEVPARMS pRBDEV = (NPABRB_DSKT_READDEVPARMS)RequestBlock;
   USHORT Unit;

   pRBLID->abrbh.Function = ABFC_RET_LID_PARMS;
   pRBLID->abrbh.Length   = GENERIC_ABRB_SIZE;
   pRBLID->abrbh.LID      = LID;
   pRBLID->abrbh.RC       = ABRC_START;
   pRBLID->SecDeviceID    = 0;          /* +1AH reserved field */
   pRBLID->Revision       = 0;          /* +1BH reserved field */
   pRBLID->Reserved_1     = 0L;         /* +1CH and +1EH reserved fields */

   if ( DevHelp_ABIOSCall( LID, (NPBYTE)pRBLID, ABIOS_EP_START ) )
      return FAILURE;

   IntLevel = pRBLID->HwIntLevel;
   UnitCnt  = pRBLID->cUnits;

   for ( Unit=0; Unit<UnitCnt; Unit++ )
      {
         pRBDEV->abrbh.Function = ABFC_READ_DEVICE_PARMS;
         pRBDEV->abrbh.Unit     = Unit;
         pRBDEV->abrbh.RC       = ABRC_START;
         pRBDEV->Reserved_1     = 0L;   /* +18H reserved field */

         if ( DevHelp_ABIOSCall( LID, (NPBYTE)pRBDEV, ABIOS_EP_START ) )
            return FAILURE;

         Drive[Unit].Geometry[DEVICE].BytesPerSector   = 512;
         Drive[Unit].Geometry[MEDIA].BytesPerSector    = 512;
         Drive[Unit].Geometry[DEVICE].SectorsPerTrack  = pRBDEV->SectorsPerTrack;
         Drive[Unit].Geometry[MEDIA].SectorsPerTrack   = pRBDEV->SectorsPerTrack;
         Drive[Unit].Geometry[DEVICE].NumHeads         = pRBDEV->cHeads;
         Drive[Unit].Geometry[MEDIA].NumHeads          = pRBDEV->cHeads;
         Drive[Unit].Geometry[DEVICE].TotalCylinders   = pRBDEV->cCylinders;
         Drive[Unit].Geometry[MEDIA].TotalCylinders    = pRBDEV->cCylinders;
         Drive[Unit].Geometry[DEVICE].TotalSectors     =
            pRBDEV->cCylinders * pRBDEV->cHeads * pRBDEV->SectorsPerTrack;
         Drive[Unit].Geometry[MEDIA].TotalSectors      =
            Drive[Unit].Geometry[DEVICE].TotalSectors;

         Drive[Unit].MotorOffDelay = pRBDEV->MotorOffTime/1000; /* convert to ms */
         Drive[Unit].RetryCount    = pRBDEV->RetryCount;

         Drive[Unit].FormatGap     = pRBDEV->FormatGap;
         Drive[Unit].FillByte      = pRBDEV->FillByte;

         Drive[Unit].Flags.HasChangeLine =  (pRBDEV->DevCtrlFlags & DP_CHANGELINE_AVAIL);
         Drive[Unit].Flags.ReadBackReq   = !(pRBDEV->DevCtrlFlags & DP_READBACK_NOT_REQ);
         Drive[Unit].Flags.LogicalMedia  =  FALSE;
      }

   /* Initialize the "Turn Off Motor" request block for later use */

   pRBLID = (NPABRB_RETLIDPARMS)MotorOffReqBlk;
   pRBLID->abrbh.Function = ABFC_DSKT_TURN_OFF_MOTOR;
   pRBLID->abrbh.Length   = GENERIC_ABRB_SIZE;
   pRBLID->abrbh.LID      = LID;

   return SUCCESS;
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : InitTimer                                                 */
/*                                                                           */
/*   Description : This procedure gets the clock interval from the           */
/*                 global info seg and calculates the number of              */
/*                 milliseconds per timer tick.  This routine registers      */
/*                 the timer handler with the kernel.                        */
/*                                                                           */
/*****************************************************************************/

BOOL NEAR InitTimer()
{
   struct InfoSegGDT FAR *pGlobalInfoSeg;
   PVOID pDOSVar;

   if ( DevHelp_GetDOSVar( DHGETDOSV_SYSINFOSEG, 0, &pDOSVar ) )
      return FAILURE;

   pGlobalInfoSeg = MAKEP( *((PSEL)pDOSVar), 0 );

   MSPerTick = pGlobalInfoSeg->SIS_ClkIntrvl / 10;

   if ( DevHelp_SetTimer( (NPFN)TimerHandler ) )
      return FAILURE;

   return SUCCESS;
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : SetupDMABuffer                                            */
/*                                                                           */
/*   Description : This routine calculates the maximum size needed for       */
/*                 the DMA buffer.  A GDT selector is allocated for the      */
/*                 DMA buffer.  If this controller requires read back,       */
/*                 then a GDT selector for the read back buffer is also      */
/*                 allocated.                                                */
/*                                                                           */
/*****************************************************************************/

BOOL NEAR SetupDMABuffer()
{
   USHORT  MaxSectorsPerTrack = 0;
   USHORT  MaxBytesPerSector  = 0;
   USHORT  MaxNumHeads        = 0;
   DRVFLGS DriveFlags         = {0};
   USHORT  Unit;
   SEL     BuffSel;

   /* Find the max geometry of all the drives */
   for ( Unit=0; Unit<UnitCnt; Unit++ )
      {
         if ( Drive[Unit].Geometry[DEVICE].SectorsPerTrack > MaxSectorsPerTrack )
            MaxSectorsPerTrack = Drive[Unit].Geometry[DEVICE].SectorsPerTrack;
         if ( Drive[Unit].Geometry[DEVICE].BytesPerSector > MaxBytesPerSector )
            MaxBytesPerSector = Drive[Unit].Geometry[DEVICE].BytesPerSector;
         if ( Drive[Unit].Geometry[DEVICE].NumHeads > MaxNumHeads )
            MaxNumHeads = Drive[Unit].Geometry[DEVICE].NumHeads;
         if ( Drive[Unit].Flags.ReadBackReq ) DriveFlags.ReadBackReq = TRUE;
      }

   /* MaxBuffSize is the number of bytes on a cylinder.              */
   /* This is the largest amount of data that can be read from ABIOS */
   /* on a single read request.                                      */

   MaxBuffSize = MaxNumHeads * MaxSectorsPerTrack * MaxBytesPerSector;

   if ( DevHelp_AllocGDTSelector( &BuffSel, 1 ) )
      return FAILURE;
   else pDMABuffer = MAKEP( BuffSel, 0 );

   if ( DriveFlags.ReadBackReq )
      {
         if ( DevHelp_AllocGDTSelector( &BuffSel, 1 ) )
            return FAILURE;
         else pReadBackBuffer = MAKEP( BuffSel, 0 );
      }
   else pReadBackBuffer = NULL;

   return SUCCESS;
}


