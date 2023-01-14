/*static char *SCCSID = "@(#)fl2geo.c	6.3 92/01/30";*/
/*****************************************************************************/
/*                                                                           */
/*   Component   : IBM PS/2 Floppy Adapter Device Driver  (IBM2FLPY)         */
/*                                                                           */
/*   Module      : FL2GEO.C                                                  */
/*                                                                           */
/*   Description : Routines to implement the geometry commands.              */
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
/*   Routine     : GetMediaGeometry                                          */
/*                                                                           */
/*   Description : This routine sets up the ABIOS request block to do        */
/*                 a Read Media Parameters function.                         */
/*                                                                           */
/*****************************************************************************/

VOID NEAR GetMediaGeometry()
{
   PIORB_GEOMETRY pIORB = (PIORB_GEOMETRY)pHeadIORB;
   NPABRB_DSKT_READMEDIAPARMS pRB = (NPABRB_DSKT_READMEDIAPARMS)RequestBlock;

   pRB->abrbh.Function  = ABFC_DSKT_READ_MEDIA_PARMS;
   pRB->abrbh.Unit      = pIORB->iorbh.UnitHandle;
   pRB->abrbh.RC        = ABRC_START;
   pRB->Reserved_1      = 0;     /* +16H reserved field  */

   CompletionRoutine = GetMediaGeometryComplete;
   Retry = 0;
   Stage = ABIOS_EP_START;
   NextStage();
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : GetMediaGeometryComplete                                  */
/*                                                                           */
/*   Description : This routine is called when the ABIOS Read Media          */
/*                 Parameters function completes.  If the function           */
/*                 completed OK then the geometry information is copied      */
/*                 from the ABIOS request block into the IORB and into       */
/*                 the drive media geometry structure.                       */
/*                                                                           */
/*****************************************************************************/

VOID FAR GetMediaGeometryComplete()
{
   PIORB_GEOMETRY pIORB = (PIORB_GEOMETRY)pHeadIORB;
   NPABRB_DSKT_READMEDIAPARMS pRB = (NPABRB_DSKT_READMEDIAPARMS)RequestBlock;
   NPGEOMETRY pGeometry;

   if ( pRB->abrbh.RC == ABRC_COMPLETEOK )
      {
         pIORB->pGeometry->BytesPerSector  = 512;
         pIORB->pGeometry->NumHeads        = pRB->cHeads;
         pIORB->pGeometry->TotalCylinders  = pRB->cCylinders;
         pIORB->pGeometry->SectorsPerTrack = pRB->SectorsPerTrack;
         pIORB->pGeometry->TotalSectors    =
            pRB->cCylinders * pRB->cHeads * pRB->SectorsPerTrack;

         pGeometry = &(Drive[pIORB->iorbh.UnitHandle].Geometry[MEDIA]);

         pGeometry->BytesPerSector  = 512;
         pGeometry->NumHeads        = pRB->cHeads;
         pGeometry->TotalCylinders  = pRB->cCylinders;
         pGeometry->SectorsPerTrack = pRB->SectorsPerTrack;
         pGeometry->TotalSectors    = pIORB->pGeometry->TotalSectors;

         Drive[pIORB->iorbh.UnitHandle].Flags.UnknownMedia = FALSE;
         Drive[pIORB->iorbh.UnitHandle].Flags.LogicalMedia = FALSE;

         Drive[pIORB->iorbh.UnitHandle].FormatGap = pRB->FormatGap;
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
/*   Routine     : SetMediaGeometry                                          */
/*                                                                           */
/*   Description : This routine is executed in preparation for a             */
/*                 format and defines the geometry of the media to be        */
/*                 formatted.  The ABIOS Set Media Type function is          */
/*                 called.                                                   */
/*                                                                           */
/*****************************************************************************/

VOID NEAR SetMediaGeometry()
{
   PIORB_GEOMETRY pIORB = (PIORB_GEOMETRY)pHeadIORB;
   NPABRB_DSKT_SETMEDIATYPE pRB = (NPABRB_DSKT_SETMEDIATYPE)RequestBlock;

   pRB->abrbh.Function  = ABFC_DSKT_SET_MEDIA_TYPE;
   pRB->abrbh.Unit      = pIORB->iorbh.UnitHandle;
   pRB->abrbh.RC        = ABRC_START;
   pRB->SectorsPerTrack = pIORB->pGeometry->SectorsPerTrack;
   pRB->BlockSize       = 0x02;  /* 512 bytes per sector */
   pRB->Reserved_1      = 0;     /* +16H reserved field  */
   pRB->cTracks         = pIORB->pGeometry->TotalCylinders;
   pRB->FillByte        = Drive[pIORB->iorbh.UnitHandle].FillByte;

   /* Set the format gap based on the media geometry */
   switch( pIORB->pGeometry->TotalSectors )
      {
         case 640:  pRB->FormatGap = 0x50; break;  /*  320K */
         case 720:  pRB->FormatGap = 0x50; break;  /*  360K */
         case 1440: pRB->FormatGap = 0x50; break;  /*  720K */
         case 2400: pRB->FormatGap = 0x54; break;  /*  1.2M */
         case 2880: pRB->FormatGap = 0x65; break;  /* 1.44M */
         case 5760: pRB->FormatGap = 0x53; break;  /* 2.88M */
         default:   pRB->FormatGap = Drive[pIORB->iorbh.UnitHandle].FormatGap;
      }

   CompletionRoutine = SetMediaGeometryComplete;
   Retry = 0;
   Stage = ABIOS_EP_START;
   NextStage();
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : SetMediaGeometryComplete                                  */
/*                                                                           */
/*   Description : This function is called when the ABIOS Set Media          */
/*                 Type function completes.  If the function completed       */
/*                 successfully then the drive media geometry structure      */
/*                 is updated to reflect the new media geometry.             */
/*                                                                           */
/*****************************************************************************/

VOID FAR SetMediaGeometryComplete()
{
   PIORB_GEOMETRY pIORB = (PIORB_GEOMETRY)pHeadIORB;
   NPABRB_DSKT_SETMEDIATYPE pRB = (NPABRB_DSKT_SETMEDIATYPE)RequestBlock;
   NPGEOMETRY pGeometry;

   if ( pRB->abrbh.RC == ABRC_COMPLETEOK )
      {
         pGeometry = &(Drive[pIORB->iorbh.UnitHandle].Geometry[MEDIA]);

         pGeometry->TotalSectors    = pIORB->pGeometry->TotalSectors;
         pGeometry->BytesPerSector  = pIORB->pGeometry->BytesPerSector;
         pGeometry->NumHeads        = pIORB->pGeometry->NumHeads;
         pGeometry->TotalCylinders  = pIORB->pGeometry->TotalCylinders;
         pGeometry->SectorsPerTrack = pIORB->pGeometry->SectorsPerTrack;

         Drive[pIORB->iorbh.UnitHandle].Flags.LogicalMedia = FALSE;
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
/*   Routine     : GetDeviceGeometry                                         */
/*                                                                           */
/*   Description : This routine simply copies the device geometry info       */
/*                 from the drive device geometry structure to the IORB.     */
/*                 The device geometry information was obtained at           */
/*                 initialization time from ABIOS.                           */
/*                                                                           */
/*****************************************************************************/

VOID NEAR GetDeviceGeometry()
{
   PIORB_GEOMETRY pIORB = (PIORB_GEOMETRY)pHeadIORB;
   NPGEOMETRY pDeviceGeometry;

   pDeviceGeometry = &(Drive[pIORB->iorbh.UnitHandle].Geometry[DEVICE]);

   pIORB->pGeometry->TotalSectors    = pDeviceGeometry->TotalSectors;
   pIORB->pGeometry->BytesPerSector  = pDeviceGeometry->BytesPerSector;
   pIORB->pGeometry->NumHeads        = pDeviceGeometry->NumHeads;
   pIORB->pGeometry->TotalCylinders  = pDeviceGeometry->TotalCylinders;
   pIORB->pGeometry->SectorsPerTrack = pDeviceGeometry->SectorsPerTrack;

   IORBDone();
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : SetLogicalGeometry                                        */
/*                                                                           */
/*   Description : The specified logical geometry is copied from the         */
/*                 IORB to the drive media geometry structure and the        */
/*                 Logical Media flag is set.  Logical geometry means        */
/*                 that the media is formatted in some non-standard way.     */
/*                 For example: a 1.44M diskette is formatted as 720K.       */
/*                 Another example: a 1.44M diskette formatted as 1.2M.      */
/*                 ABIOS assumes that a diskette is always formatted to      */
/*                 its highest capacity.  However, this ADD can still        */
/*                 handle non-standard diskette formats by not letting       */
/*                 ABIOS cross a head boundry on an IO operation.  The       */
/*                 Logical Media flag tells the IO routines to break up      */
/*                 read, write and verify operations so that an ABIOS        */
/*                 request does not cross a head boundry.                    */
/*                                                                           */
/*****************************************************************************/

VOID NEAR SetLogicalGeometry()
{
   PIORB_GEOMETRY pIORB = (PIORB_GEOMETRY)pHeadIORB;
   NPGEOMETRY pLogicalGeometry;

   pLogicalGeometry = &(Drive[pIORB->iorbh.UnitHandle].Geometry[MEDIA]);

   pLogicalGeometry->TotalSectors    = pIORB->pGeometry->TotalSectors;
   pLogicalGeometry->BytesPerSector  = pIORB->pGeometry->BytesPerSector;
   pLogicalGeometry->NumHeads        = pIORB->pGeometry->NumHeads;
   pLogicalGeometry->TotalCylinders  = pIORB->pGeometry->TotalCylinders;
   pLogicalGeometry->SectorsPerTrack = pIORB->pGeometry->SectorsPerTrack;

   Drive[pIORB->iorbh.UnitHandle].Flags.LogicalMedia = TRUE;

   IORBDone();
}


