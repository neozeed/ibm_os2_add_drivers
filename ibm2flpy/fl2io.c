/*static char *SCCSID = "@(#)fl2io.c	6.3 92/01/30";*/
/*****************************************************************************/
/*                                                                           */
/*   Component   : IBM PS/2 Floppy Adapter Device Driver  (IBM2FLPY)         */
/*                                                                           */
/*   Module      : FL2IO.C                                                   */
/*                                                                           */
/*   Description : Routines to implement read, write and verify.             */
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
/*   Routine     : IO                                                        */
/*                                                                           */
/*   Description : This routine checks to see if the geometry of the media   */
/*                 is known.  If the media geometry is known, then we        */
/*                 proceed directly to RWV to perform the read, write        */
/*                 or verify.  If the media geometry is not known, then      */
/*                 we must first determine the media geometry before         */
/*                 proceeding to RWV.  To determime the media geometry,      */
/*                 we will do an ABIOS Verify followed by an ABIOS Read      */
/*                 Media Parameters.  ABIOS determines the media geometry    */
/*                 on the Verify, but we must call Read Media Parameters     */
/*                 to find out what the media geometry is.                   */
/*                                                                           */
/*****************************************************************************/

VOID NEAR IO()
{
   NPABRB_DSKT_RWV pRB = (NPABRB_DSKT_RWV)RequestBlock;

   if ( Drive[pHeadIORB->UnitHandle].Flags.UnknownMedia )
      {
         /* Verify the first sector on the diskette */
         pRB->abrbh.Function = ABFC_DSKT_VERIFY;
         pRB->abrbh.Unit     = pHeadIORB->UnitHandle;
         pRB->Cylinder       = 0;
         pRB->Head           = 0;
         pRB->Sector         = 1;
         pRB->cSectors       = 1;
         pRB->abrbh.RC       = ABRC_START;
         pRB->Reserved_2     = 0;     /* +16H reserved field  */
         pRB->Reserved_3     = 0;     /* +1EH reserved field  */

         CompletionRoutine = VerifyComplete;
         Retry = 0;
         Stage = ABIOS_EP_START;
         NextStage();
      }
   else RWV();
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : VerifyComplete                                            */
/*                                                                           */
/*   Description : This routine is called when the Verify operation          */
/*                 completes.  If the Verify completed successfully          */
/*                 then call Read Media Parameters.                          */
/*                                                                           */
/*****************************************************************************/

VOID FAR VerifyComplete()
{
   NPABRB_DSKT_READMEDIAPARMS pRB = (NPABRB_DSKT_READMEDIAPARMS)RequestBlock;

   if ( pRB->abrbh.RC == ABRC_COMPLETEOK )
      {
         /* Call Read Media Parameters */
         pRB->abrbh.Function  = ABFC_DSKT_READ_MEDIA_PARMS;
         pRB->abrbh.RC        = ABRC_START;
         pRB->Reserved_1      = 0;     /* +16H reserved field  */

         CompletionRoutine = ReadMediaParamsComplete;
         Retry = 0;
         Stage = ABIOS_EP_START;
         NextStage();
      }
   else  /* Error trying to verify to establish media geometery */
      {
         pHeadIORB->ErrorCode = TranslateErrorCode( pRB->abrbh.RC );

         StartTimer( pHeadIORB->UnitHandle,
                     Drive[pHeadIORB->UnitHandle].MotorOffDelay ,TurnOffMotor );

         IORBDone();
      }
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : ReadMediaParamsComplete                                   */
/*                                                                           */
/*   Description : This routine is called when Read Media Parameters         */
/*                 is complete.  If it completed successfully then           */
/*                 this routine updates the drive media geometry             */
/*                 structure with the new geometry and RWV is called         */
/*                 to do the read, write or verify.                          */
/*                                                                           */
/*****************************************************************************/

VOID FAR ReadMediaParamsComplete()
{
   NPABRB_DSKT_READMEDIAPARMS pRB = (NPABRB_DSKT_READMEDIAPARMS)RequestBlock;
   NPGEOMETRY pGeometry;

   if ( pRB->abrbh.RC == ABRC_COMPLETEOK )
      {
         pGeometry = &(Drive[pHeadIORB->UnitHandle].Geometry[MEDIA]);

         pGeometry->BytesPerSector  = 512;
         pGeometry->NumHeads        = pRB->cHeads;
         pGeometry->TotalCylinders  = pRB->cCylinders;
         pGeometry->SectorsPerTrack = pRB->SectorsPerTrack;
         pGeometry->TotalSectors    =
            pRB->cCylinders * pRB->cHeads * pRB->SectorsPerTrack;

         Drive[pHeadIORB->UnitHandle].Flags.UnknownMedia = FALSE;
         Drive[pHeadIORB->UnitHandle].Flags.LogicalMedia = FALSE;

         Drive[pHeadIORB->UnitHandle].FormatGap = pRB->FormatGap;

         RWV();
      }
   else  /* Error trying to read media parms to establish media geometery */
      {
         pHeadIORB->ErrorCode = TranslateErrorCode( pRB->abrbh.RC );

         StartTimer( pHeadIORB->UnitHandle,
                    Drive[pHeadIORB->UnitHandle].MotorOffDelay ,TurnOffMotor );

         IORBDone();
      }
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : RWV                                                       */
/*                                                                           */
/*   Description : This routine does the initial setup of the ABIOS          */
/*                 request block and the XferData structure for the          */
/*                 read, write or verify.                                    */
/*                                                                           */
/*****************************************************************************/

VOID NEAR RWV()
{
   PIORB_EXECUTEIO pIORB = (PIORB_EXECUTEIO)pHeadIORB;
   NPABRB_DSKT_RWV pRB   = (NPABRB_DSKT_RWV)RequestBlock;
   union
      {
         CHS_ADDR CHS;
         ULONG    RBA;
      } Addr;

   /* Calculate initial CHS */
   if ( pIORB->iorbh.RequestControl & IORB_CHS_ADDRESSING )
      Addr.RBA = pIORB->RBA;
   else
      {
         if ( f_ADD_ConvRBAtoCHS( pIORB->RBA,
              &(Drive[pIORB->iorbh.UnitHandle].Geometry[MEDIA]), (PCHS_ADDR)&Addr ) )
            {
               pIORB->iorbh.Status   |= IORB_ERROR;
               pIORB->iorbh.ErrorCode = IOERR_RBA_ADDRESSING_ERROR;
               IORBDone();
               return;
            }
      }

   pRB->Cylinder        = Addr.CHS.Cylinder;
   pRB->Head            = Addr.CHS.Head;
   pRB->Sector          = Addr.CHS.Sector;

   pRB->abrbh.Unit      = pIORB->iorbh.UnitHandle;
   pRB->ppIObuffer      = ppDMABuffer;

   XferData.cSGList     = pIORB->cSGList;
   XferData.pSGList     = pIORB->pSGList;
   XferData.pBuffer     = pDMABuffer;
   XferData.iSGList     = 0;
   XferData.SGOffset    = 0;

   pIORB->BlocksXferred = 0;

   NextRWV();
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : NextRWV                                                   */
/*                                                                           */
/*   Description : This routine is called repeatively during a large         */
/*                 IORB read, write or verify request to break the           */
/*                 request up into smaller chunks.                           */
/*                                                                           */
/*                 An IORB request may need to be broken up into several     */
/*                 ABIOS requests, because ABIOS can read at most one        */
/*                 cylinder at a time.  A single ABIOS request can not       */
/*                 cross a cylinder boundry.  If the media geometry is       */
/*                 logical, then an ABIOS request can not cross a head       */
/*                 boundry.  Also, if the DMA buffer is small, then we       */
/*                 may have to break the request up into buffer size         */
/*                 chunks.                                                   */
/*                                                                           */
/*                 If the request is a write request, then this routine      */
/*                 transfers the write data from the scatter/gather list     */
/*                 to the DMA buffer.                                        */
/*                                                                           */
/*   Input  : pRB->Cylinder  \                                               */
/*            pRB->Head       > location of next sector to RWV               */
/*            pRB->Sector    /                                               */
/*                                                                           */
/*            pIORB->BlockCount       > difference is sectors left to RWV    */
/*            pIORB->BlocksXferred   /                                       */
/*                                                                           */
/*****************************************************************************/

VOID NEAR NextRWV()
{
   PIORB_EXECUTEIO pIORB = (PIORB_EXECUTEIO)pHeadIORB;
   NPABRB_DSKT_RWV pRB   = (NPABRB_DSKT_RWV)RequestBlock;
   USHORT SectorsLeft,SectorsPerBuffer;
   NPGEOMETRY pGeometry;

   pGeometry = &(Drive[pIORB->iorbh.UnitHandle].Geometry[MEDIA]);

   if ( Drive[pIORB->iorbh.UnitHandle].Flags.LogicalMedia )
      /* SectorCnt = max possible sectors so head boundry is not crossed */
      SectorCnt = pGeometry->SectorsPerTrack + 1 - pRB->Sector;
   else
      /* SectorCnt = max possible sectors so cylinder boundry is not crossed */
      SectorCnt = ((pGeometry->NumHeads - pRB->Head) * pGeometry->SectorsPerTrack) + 1 - pRB->Sector;

   /* Don't do more sectors than can fit into the DMA buffer */
   SectorsPerBuffer = BuffSize / pGeometry->BytesPerSector;
   if ( SectorCnt > SectorsPerBuffer ) SectorCnt = SectorsPerBuffer;

   /* Don't do more sectors than requested */
   SectorsLeft = pIORB->BlockCount - pIORB->BlocksXferred;
   if ( SectorCnt > SectorsLeft ) SectorCnt = SectorsLeft;

   switch( pIORB->iorbh.CommandModifier )
      {
         case IOCM_WRITE:
         case IOCM_WRITE_VERIFY:
               {
                  /* Transfer data from the S/G List to the DMA buffer */
                  XferData.Mode = SGLIST_TO_BUFFER;
                  XferData.numTotalBytes = SectorCnt * pGeometry->BytesPerSector;
                  if ( f_ADD_XferBuffData( &XferData ) )
                     {
                        pIORB->iorbh.Status   |= IORB_ERROR;
                        pIORB->iorbh.ErrorCode = IOERR_CMD_SGLIST_BAD;
                        StartTimer( pIORB->iorbh.UnitHandle,
                           Drive[pIORB->iorbh.UnitHandle].MotorOffDelay ,TurnOffMotor );
                        IORBDone();
                        return;
                     }

                  pRB->abrbh.Function = ABFC_DSKT_WRITE;
               }
            break;

         case IOCM_READ:
            pRB->abrbh.Function = ABFC_DSKT_READ;
            break;

         case IOCM_READ_VERIFY:
            pRB->abrbh.Function = ABFC_DSKT_VERIFY;
            break;
      }

   NextRWVStep();
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : NextRWVStep                                               */
/*                                                                           */
/*   Description : This routine does the final setup before calling          */
/*                 NextStage to do the read, write or verify.  If the        */
/*                 operation is a write/verify then this routine will        */
/*                 be called a second time to do the verify (or read).       */
/*                                                                           */
/*****************************************************************************/

VOID NEAR NextRWVStep()
{
   NPABRB_DSKT_RWV pRB = (NPABRB_DSKT_RWV)RequestBlock;

   pRB->cSectors   = SectorCnt;
   pRB->abrbh.RC   = ABRC_START;
   pRB->Reserved_1 = 0;           /* +10H reserved field           */
   pRB->Reserved_2 = 0L;          /* +16H and +18H reserved fields */
   pRB->Reserved_3 = 0;           /* +1EH reserved field           */

   CompletionRoutine = RWVComplete;
   Retry = 0;
   Stage = ABIOS_EP_START;
   NextStage();
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : RWVComplete                                               */
/*                                                                           */
/*   Description : This routine is called when the ABIOS read, write or      */
/*                 verify operation completes.                               */
/*                                                                           */
/*                 If the operation was a read then data is tranferred       */
/*                 from the DMA buffer to the scatter/gather list.           */
/*                                                                           */
/*                 If the operation is a write/verify and the write step     */
/*                 has just completed successfully, then this routine        */
/*                 sets up the ABIOS request block for the verify step.      */
/*                 If read back is required, then the verify is              */
/*                 actually a read back.  When the read back completes,      */
/*                 then the last byte of each sector read back is            */
/*                 compared to what should have been written.  If it         */
/*                 does not match then the write is done again.              */
/*                                                                           */
/*                 This routine updates the BlocksXferred field in the       */
/*                 IORB.  If the IORB request is not finished, then          */
/*                 the CHS for the next block of sectors is calculated       */
/*                 and NextRWV is called to read, write or verify them.      */
/*                                                                           */
/*****************************************************************************/

VOID FAR RWVComplete()
{
   PIORB_EXECUTEIO pIORB = (PIORB_EXECUTEIO)pHeadIORB;
   NPABRB_DSKT_RWV pRB   = (NPABRB_DSKT_RWV)RequestBlock;
   NPGEOMETRY pGeometry;
   USHORT cTracks,x;
   PBYTE pDataWritten,pDataReadBack;

   pGeometry = &(Drive[pIORB->iorbh.UnitHandle].Geometry[MEDIA]);

   if ( pIORB->iorbh.CommandModifier == IOCM_READ )
      {
         /* Transfer data from the DMA buffer to the S/G List */
         XferData.Mode = BUFFER_TO_SGLIST;
         XferData.numTotalBytes = pRB->cSectors * pGeometry->BytesPerSector;
         if ( f_ADD_XferBuffData( &XferData ) )
            {
               pIORB->iorbh.Status   |= IORB_ERROR;
               pIORB->iorbh.ErrorCode = IOERR_CMD_SGLIST_BAD;
               StartTimer( pIORB->iorbh.UnitHandle,
                  Drive[pIORB->iorbh.UnitHandle].MotorOffDelay ,TurnOffMotor );
               IORBDone();
               return;
            }
      }
   else if ( pIORB->iorbh.CommandModifier == IOCM_WRITE_VERIFY )
      {
         if ( pRB->abrbh.RC == ABRC_COMPLETEOK )
            {
               if ( pRB->abrbh.Function == ABFC_DSKT_WRITE )
                  {
                     if ( Drive[pIORB->iorbh.UnitHandle].Flags.ReadBackReq )
                        {
                           pRB->ppIObuffer     = ppReadBackBuffer;
                           pRB->abrbh.Function = ABFC_DSKT_READ;
                        }
                     else /* Read back not required; just do a verify */
                        {
                           pRB->abrbh.Function = ABFC_DSKT_VERIFY;
                        }
                     NextRWVStep();
                     return;
                  }
               else if ( pRB->abrbh.Function == ABFC_DSKT_READ )
                  {
                     pRB->ppIObuffer     = ppDMABuffer;
                     pRB->abrbh.Function = ABFC_DSKT_WRITE;

                     /* Check the last byte of each sector */
                     pDataWritten   = pDMABuffer      + pGeometry->BytesPerSector - 1;
                     pDataReadBack  = pReadBackBuffer + pGeometry->BytesPerSector - 1;
                     for ( x=0; x<pRB->cSectors; x++ )
                        {
                           if ( *pDataReadBack != *pDataWritten )
                              {
                                 NextRWVStep();  /* Write it again */
                                 return;
                              }
                           pDataWritten  += pGeometry->BytesPerSector;
                           pDataReadBack += pGeometry->BytesPerSector;
                        }
                  }
               else pRB->abrbh.Function = ABFC_DSKT_WRITE;  /* Verify done */
            }
      }

   pIORB->BlocksXferred += pRB->cSectors;

   if ( pRB->abrbh.RC == ABRC_COMPLETEOK )
      {
         if ( pIORB->BlocksXferred < pIORB->BlockCount ) /* if not yet done */
            {
               /* Calculate new CHS */
               cTracks        =  (pRB->Sector-1+pRB->cSectors)/pGeometry->SectorsPerTrack;
               pRB->Sector    = ((pRB->Sector-1+pRB->cSectors)%pGeometry->SectorsPerTrack)+1;
               pRB->Cylinder +=  (pRB->Head + cTracks) / pGeometry->NumHeads;
               pRB->Head      =  (pRB->Head + cTracks) % pGeometry->NumHeads;

               NextRWV();  /* Do next RWV */
               return;
            }
      }
   else pIORB->iorbh.ErrorCode = TranslateErrorCode( pRB->abrbh.RC );

   if ( pIORB->iorbh.ErrorCode == IOERR_MEDIA_CHANGED )
      Drive[pIORB->iorbh.UnitHandle].Flags.UnknownMedia = TRUE;

   StartTimer( pIORB->iorbh.UnitHandle,
              Drive[pIORB->iorbh.UnitHandle].MotorOffDelay ,TurnOffMotor );

   IORBDone();
}


