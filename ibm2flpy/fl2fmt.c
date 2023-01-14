/*static char *SCCSID = "@(#)fl2fmt.c	6.3 92/01/30";*/
/*****************************************************************************/
/*                                                                           */
/*   Component   : IBM PS/2 Floppy Adapter Device Driver  (IBM2FLPY)         */
/*                                                                           */
/*   Module      : FL2FMT.C                                                  */
/*                                                                           */
/*   Description : Routines to implement the format function.                */
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
/*   Routine     : Format                                                    */
/*                                                                           */
/*   Description : This routine sets up the ABIOS request block to           */
/*                 do a format operation.                                    */
/*                                                                           */
/*****************************************************************************/

VOID NEAR Format()
{
   PIORB_FORMAT pIORB  = (PIORB_FORMAT)pHeadIORB;
   NPABRB_DSKT_FMT pRB = (NPABRB_DSKT_FMT)RequestBlock;
   NPGEOMETRY pGeometry;
   PFORMAT_CMD_TRACK pCmd;
   union
      {
         CHS_ADDR CHS;
         ULONG    RBA;
      } Addr;

   pCmd      = (PFORMAT_CMD_TRACK)pIORB->pFormatCmd;
   pGeometry = &(Drive[pIORB->iorbh.UnitHandle].Geometry[MEDIA]);

   /* Transfer track table from the S/G List to the DMA buffer */
   XferData.Mode          = SGLIST_TO_BUFFER;
   XferData.cSGList       = pIORB->cSGList;
   XferData.pSGList       = pIORB->pSGList;
   XferData.pBuffer       = pDMABuffer;
   XferData.iSGList       = 0;
   XferData.SGOffset      = 0;
   XferData.numTotalBytes = pCmd->cTrackEntries * 4;
   if ( f_ADD_XferBuffData( &XferData ) )
      {
         pIORB->iorbh.Status   |= IORB_ERROR;
         pIORB->iorbh.ErrorCode = IOERR_CMD_SGLIST_BAD;
         IORBDone();
         return;
      }

   /* Calculate initial CHS */
   if ( pIORB->iorbh.RequestControl & IORB_CHS_ADDRESSING )
      Addr.RBA = pCmd->RBA;
   else if ( f_ADD_ConvRBAtoCHS( pCmd->RBA, pGeometry, (PCHS_ADDR)&Addr ) )
      {
         pIORB->iorbh.Status   |= IORB_ERROR;
         pIORB->iorbh.ErrorCode = IOERR_RBA_ADDRESSING_ERROR;
         IORBDone();
         return;
      }

   pRB->abrbh.Function = ABFC_DKST_FORMAT;              /* DSKT */
   pRB->abrbh.Unit     = pIORB->iorbh.UnitHandle;
   pRB->ppFormatTable  = ppDMABuffer;
   pRB->Subfunction    = 0;
   pRB->Cylinder       = Addr.CHS.Cylinder;
   pRB->Head           = Addr.CHS.Head;

   NextFormatStep();
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : NextFormatStep                                            */
/*                                                                           */
/*   Description : This routine calls NextStage to do a format function.     */
/*                 If a verify is required after the format, then this       */
/*                 routine is called a second time to do the verify.         */
/*                                                                           */
/*****************************************************************************/

VOID NEAR NextFormatStep()
{
   NPABRB_DSKT_RWV pRB = (NPABRB_DSKT_RWV)RequestBlock;

   pRB->abrbh.RC   = ABRC_START;
   pRB->Reserved_1 = 0;           /* +10H reserved field           */
   pRB->Reserved_2 = 0L;          /* +16H and +18H reserved fields */
   pRB->Reserved_3 = 0;           /* +1EH reserved field           */

   CompletionRoutine = FormatComplete;
   Retry = 0;
   Stage = ABIOS_EP_START;
   NextStage();
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : FormatComplete                                            */
/*                                                                           */
/*   Description : This routine is called when ABIOS completes the           */
/*                 format operation.  If a verify is requested then          */
/*                 NextFormatStep is called again to do the verify.          */
/*                                                                           */
/*****************************************************************************/

VOID FAR FormatComplete()
{
   PIORB_FORMAT pIORB  = (PIORB_FORMAT)pHeadIORB;
   NPABRB_DSKT_RWV pRB = (NPABRB_DSKT_RWV)RequestBlock;
   PFORMAT_CMD_TRACK pCmd;

   pCmd = (PFORMAT_CMD_TRACK)pIORB->pFormatCmd;

   if ( pRB->abrbh.RC == ABRC_COMPLETEOK )
      {
         if ( pRB->abrbh.Function == ABFC_DKST_FORMAT )   /* DSKT */
            {
               /* If verify is requested */
               if ( pCmd->Flags & FF_VERIFY )
                  {
                     /* Setup the ABIOS request block for verify */
                     pRB->abrbh.Function = ABFC_DSKT_VERIFY;
                     pRB->cSectors       = pCmd->cTrackEntries;
                     pRB->Sector         = 1; /* Start at first sector */
                     NextFormatStep();
                     return;
                  }
            }
      }
   else pIORB->iorbh.ErrorCode = TranslateErrorCode( pRB->abrbh.RC );

   if ( pIORB->iorbh.ErrorCode == IOERR_MEDIA_CHANGED )
      Drive[pIORB->iorbh.UnitHandle].Flags.UnknownMedia = TRUE;

   StartTimer( pIORB->iorbh.UnitHandle,
              Drive[pIORB->iorbh.UnitHandle].MotorOffDelay ,TurnOffMotor );

   IORBDone();
}


