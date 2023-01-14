/*static char *SCCSID = "@(#)s506iorb.c	6.3 92/01/29";*/
#pragma title("IBM ST506 Device Driver")
#pragma subtitle("IORB Processing")
#pragma linesize( 120 )
#pragma pagesize( 60 )

#define INCL_NOBASEAPI
#define INCL_NOPMAPI
#include <os2.h>
#include <dhcalls.h>
#include <strat2.h>     /* needed to keep reqpkt.h happy */
#include <reqpkt.h>
#include <iorb.h>

#include "s506hdr.h"
#include "s506ext.h"
#include "s506pro.h"




VOID NEAR GetDeviceTable(NPACB npACB)
{
   PIORB_CONFIGURATION pIORB = (PIORB_CONFIGURATION)npACB->pHeadIORB;
   USHORT LengthNeeded,i;
   ADAPTERINFO far *pADPT;

   LengthNeeded = sizeof(DEVICETABLE) + sizeof(ADAPTERINFO) +
                  ((NumFix-1) * sizeof(UNITINFO));

   if ( pIORB->DeviceTableLen < LengthNeeded )
      {
         /* Not enough storage */
         npACB->pHeadIORB->Status |= IORB_ERROR;
         /* npACB->pHeadIORB->ErrorCode = IOERR_?????? */
         IORBDone(npACB);
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

   pADPT->AdapterUnits    = NumFix;
   pADPT->AdapterDevBus   = AI_DEVBUS_ST506 | AI_DEVBUS_16BIT;  /*?? 16 ??*/
   pADPT->AdapterIOAccess = AI_IOACCESS_PIO;
   pADPT->AdapterHostBus  = AI_HOSTBUS_ISA | AI_BUSWIDTH_16BIT; /*?? 16 ??*/
   pADPT->AdapterSCSITargetID = 0;
   pADPT->AdapterSCSILUN  = 0;
   pADPT->AdapterFlags    = AF_16M | AF_HW_SCATGAT;
   pADPT->MaxHWSGList     = 0;
   pADPT->MaxCDBTransferLength = 0L;

   for ( i=0; i<NumFix; i++ )
      {
         if ( npACB->unit[i].pUnitInfo == NULL ) /* If unit info was not changed */
            {
               pADPT->UnitInfo[i].AdapterIndex     = 0;
               pADPT->UnitInfo[i].UnitIndex        = i;
               pADPT->UnitInfo[i].UnitHandle       = i;
               pADPT->UnitInfo[i].FilterADDHandle  = 0;
               pADPT->UnitInfo[i].UnitType         = UIB_TYPE_DISK;
               pADPT->UnitInfo[i].QueuingCount     = 1;
               pADPT->UnitInfo[i].UnitSCSITargetID = 0;
               pADPT->UnitInfo[i].UnitSCSILUN      = 0;
               pADPT->UnitInfo[i].UnitFlags        = UF_NOSCSI_SUPT;
            }
         else  /* Pass back the changed unit info */
            {
               pADPT->UnitInfo[i].AdapterIndex     = npACB->unit[i].pUnitInfo->AdapterIndex;
               pADPT->UnitInfo[i].UnitIndex        = npACB->unit[i].pUnitInfo->UnitIndex;
               pADPT->UnitInfo[i].UnitHandle       = npACB->unit[i].pUnitInfo->UnitHandle;
               pADPT->UnitInfo[i].FilterADDHandle  = npACB->unit[i].pUnitInfo->FilterADDHandle;
               pADPT->UnitInfo[i].UnitType         = npACB->unit[i].pUnitInfo->UnitType;
               pADPT->UnitInfo[i].QueuingCount     = npACB->unit[i].pUnitInfo->QueuingCount;
               pADPT->UnitInfo[i].UnitSCSITargetID = npACB->unit[i].pUnitInfo->UnitSCSITargetID;
               pADPT->UnitInfo[i].UnitSCSILUN      = npACB->unit[i].pUnitInfo->UnitSCSILUN;
               pADPT->UnitInfo[i].UnitFlags        = npACB->unit[i].pUnitInfo->UnitFlags;
            }
      }

   IORBDone(npACB);
}


VOID NEAR CompleteInit(NPACB npACB)
{
   Initialize_hardfile();
   npACB->Flags |= FCOMPLETEINIT; // set IOCM_COMPLETE_INIT IORB processed flag

   IORBDone(npACB);
}


VOID NEAR AllocateUnit(NPACB npACB)
{
   if ( npACB->unit[npACB->pHeadIORB->UnitHandle].AllocatedFlag )
      {
         /* Unit already allocated */
         npACB->pHeadIORB->Status   |= IORB_ERROR;
         npACB->pHeadIORB->ErrorCode = IOERR_UNIT_ALLOCATED;
      }
   else npACB->unit[npACB->pHeadIORB->UnitHandle].AllocatedFlag = TRUE;

   IORBDone(npACB);
}


VOID NEAR DeallocateUnit(NPACB npACB)
{
   if ( !npACB->unit[npACB->pHeadIORB->UnitHandle].AllocatedFlag )
      {
         /* Unit not allocated */
         npACB->pHeadIORB->Status   |= IORB_ERROR;
         npACB->pHeadIORB->ErrorCode = IOERR_UNIT_NOT_ALLOCATED;
      }
   else npACB->unit[npACB->pHeadIORB->UnitHandle].AllocatedFlag = FALSE;

   IORBDone(npACB);
}


VOID NEAR ChangeUnitInfo( NPACB npACB )
{
   PIORB_UNIT_CONTROL pIORB = (PIORB_UNIT_CONTROL)npACB->pHeadIORB;

   if ( !npACB->unit[npACB->pHeadIORB->UnitHandle].AllocatedFlag )
      {
         /* Unit not allocated */
         npACB->pHeadIORB->Status   |= IORB_ERROR;
         npACB->pHeadIORB->ErrorCode = IOERR_UNIT_NOT_ALLOCATED;
         IORBDone( npACB );
         return;
      }

   /* Save the Unit Info pointer */
   npACB->unit[npACB->pHeadIORB->UnitHandle].pUnitInfo = pIORB->pUnitInfo;

   IORBDone( npACB );
}


VOID NEAR GetMediaGeometry(NPACB npACB)
{
   PIORB_GEOMETRY pIORB = (PIORB_GEOMETRY)npACB->pHeadIORB;
   NPGEOMETRY npGeometry;

   npGeometry = &(npACB->unit[pIORB->iorbh.UnitHandle].LogGeometry);

   pIORB->pGeometry->BytesPerSector  = npGeometry->BytesPerSector;
   pIORB->pGeometry->NumHeads        = npGeometry->NumHeads;
   pIORB->pGeometry->TotalCylinders  = npGeometry->TotalCylinders;
   pIORB->pGeometry->SectorsPerTrack = npGeometry->SectorsPerTrack;
   pIORB->pGeometry->TotalSectors    = npGeometry->TotalSectors;

   IORBDone(npACB);
}


VOID NEAR GetDeviceGeometry(NPACB npACB)
{
   PIORB_GEOMETRY pIORB = (PIORB_GEOMETRY)npACB->pHeadIORB;
   NPGEOMETRY npGeometry;

   npGeometry = &(npACB->unit[pIORB->iorbh.UnitHandle].LogGeometry);

   pIORB->pGeometry->BytesPerSector  = npGeometry->BytesPerSector;
   pIORB->pGeometry->NumHeads        = npGeometry->NumHeads;
   pIORB->pGeometry->TotalCylinders  = npGeometry->TotalCylinders;
   pIORB->pGeometry->SectorsPerTrack = npGeometry->SectorsPerTrack;
   pIORB->pGeometry->TotalSectors    = npGeometry->TotalSectors;

   IORBDone(npACB);
}


VOID NEAR Read(NPACB npACB)
{

   #ifdef DEBUG
   TraceIt(npACB, READIORB_MINOR);
   #endif

   DISABLE
   if ( npACB->State == IDLE )
      {
         npACB->State = START;
         ENABLE
         FixedExecute(npACB);     // call state machine
      }
   ENABLE

   #ifdef DEBUG
   TraceIt(npACB, (READIORB_MINOR | TRACE_CMPLT) );
   #endif

}


VOID NEAR Write(NPACB npACB)
{

   #ifdef DEBUG
   TraceIt(npACB, WRITEIORB_MINOR);
   #endif

   DISABLE
   if ( npACB->State == IDLE )
      {
         npACB->State = START;
         ENABLE
         FixedExecute(npACB);     // call state machine
      }
   ENABLE

   #ifdef DEBUG
   TraceIt(npACB, (WRITEIORB_MINOR | TRACE_CMPLT) );
   #endif

}


VOID NEAR Verify(NPACB npACB)
{

   #ifdef DEBUG
   TraceIt(npACB, VERIFYIORB_MINOR);
   #endif

   DISABLE
   if ( npACB->State == IDLE )
      {
         npACB->State = START;
         ENABLE
         FixedExecute(npACB);     // call state machine
      }
   ENABLE

   #ifdef DEBUG
   TraceIt(npACB, (VERIFYIORB_MINOR | TRACE_CMPLT) );
   #endif

}


VOID NEAR GetUnitStatus( NPACB npACB )
{
   PIORB_UNIT_STATUS pIORB = (PIORB_UNIT_STATUS)npACB->pHeadIORB;

   pIORB->UnitStatus = US_READY | US_POWER;

   IORBDone( npACB );
}


VOID NEAR CmdNotSupported(NPACB npACB)
{
   npACB->pHeadIORB->Status   |= IORB_ERROR;
   npACB->pHeadIORB->ErrorCode = IOERR_CMD_NOT_SUPPORTED;

   IORBDone(npACB);
}



/* Process the IORB at the head of the list           */
/* This routine should not be called if list is empty */

VOID NEAR NextIORB(NPACB npACB)
{

   #ifdef DEBUG
   TraceIt(npACB, NEXTIORB_MINOR);
   #endif

   if ( npACB->pHeadIORB->CommandCode != IOCC_CONFIGURATION )
      {
         /* Verify that UnitHandle is valid */   /* <--------------------- */
         if ( npACB->pHeadIORB->UnitHandle >= NumFix )
            {
               /* UnitHandle is out of legal range */
               npACB->pHeadIORB->Status   |= IORB_ERROR;
               npACB->pHeadIORB->ErrorCode = IOERR_UNIT;
               IORBDone( npACB );
               return;
            }

         /* Verify that unit is allocated */
         if ( npACB->pHeadIORB->CommandCode != IOCC_UNIT_CONTROL )
            {
               if ( !npACB->unit[npACB->pHeadIORB->UnitHandle].AllocatedFlag )
                  {
                     /* Unit not allocated */
                     npACB->pHeadIORB->Status   |= IORB_ERROR;
                     npACB->pHeadIORB->ErrorCode = IOERR_UNIT_NOT_ALLOCATED;
                     IORBDone( npACB );
                     return;
                  }
            }
      }

   /* Route the IORB to the proper routine */
   switch( npACB->pHeadIORB->CommandCode )
      {
         case IOCC_CONFIGURATION:
            switch( npACB->pHeadIORB->CommandModifier )
               {
                  case IOCM_GET_DEVICE_TABLE: GetDeviceTable(npACB);  break;
                  case IOCM_COMPLETE_INIT:    CompleteInit(npACB);    break;
                  default:                    CmdNotSupported(npACB); break;
               }
            break;

         case IOCC_UNIT_CONTROL:
            switch( npACB->pHeadIORB->CommandModifier )
               {
                  case IOCM_ALLOCATE_UNIT:   AllocateUnit(npACB);    break;
                  case IOCM_DEALLOCATE_UNIT: DeallocateUnit(npACB);  break;
                  case IOCM_CHANGE_UNITINFO: ChangeUnitInfo(npACB);  break;
                  default:                   CmdNotSupported(npACB); break;
               }
            break;

         case IOCC_GEOMETRY:
            switch( npACB->pHeadIORB->CommandModifier )
               {
                  case IOCM_GET_MEDIA_GEOMETRY:   GetMediaGeometry(npACB);   break;
                  case IOCM_GET_DEVICE_GEOMETRY:  GetDeviceGeometry(npACB);  break;
                  default:                        CmdNotSupported(npACB);    break;
               }
            break;

         case IOCC_EXECUTE_IO:
            switch( npACB->pHeadIORB->CommandModifier )
               {
                  case IOCM_READ:          Read(npACB);            break;
                  case IOCM_READ_VERIFY:   Verify(npACB);          break;
                  case IOCM_WRITE:         Write(npACB);           break;
                  case IOCM_WRITE_VERIFY:  Write(npACB);           break;
                  default:                 CmdNotSupported(npACB); break;
               }
            break;

         case IOCC_UNIT_STATUS:
            switch( npACB->pHeadIORB->CommandModifier )
               {
                  case IOCM_GET_UNIT_STATUS:      GetUnitStatus(npACB);      break;
                  default:                        CmdNotSupported(npACB);    break;
               }
            break;

         default: CmdNotSupported(npACB); break;
      }

   #ifdef DEBUG
   TraceIt(npACB, (NEXTIORB_MINOR | TRACE_CMPLT) );
   #endif

}


VOID FAR  _loadds ADDEntryPoint( PIORBH pNewIORB )
{
   PIORBH pIORB;
   NPACB  npACB;

   npACB = &acb;                 // get address of adapter control block

   #ifdef DEBUG
   TraceIt(npACB, ADDENTRY_MINOR);
   #endif

   DISABLE

   if ( npACB->pHeadIORB == NULL )     /* If IORB list is empty */
      {
         npACB->pHeadIORB = pNewIORB;
         ENABLE
         NextIORB(npACB);    /* Start processing the IORB */
      }
   else
      {
         /* Add new IORB to end of list */
         for ( pIORB=npACB->pHeadIORB;
               (pIORB->RequestControl&IORB_CHAIN); pIORB=pIORB->pNxtIORB );
         pIORB->pNxtIORB = pNewIORB;
         pIORB->RequestControl |= IORB_CHAIN;
         ENABLE
      }

   #ifdef DEBUG
   TraceIt(npACB, (ADDENTRY_MINOR | TRACE_CMPLT) );
   #endif

}


VOID NEAR IORBDone(NPACB npACB)
{
   PIORBH pDoneIORB = npACB->pHeadIORB;

   #ifdef DEBUG
   TraceIt(npACB, IORBDONE_MINOR);
   #endif

   /* Set the done bit */
   pDoneIORB->Status |= IORB_DONE;
   npACB->ErrCnt = 0;

   DISABLE

   if ( pDoneIORB->RequestControl & IORB_CHAIN ) /* If there is another IORB */
      {
         npACB->pHeadIORB = pDoneIORB->pNxtIORB; /* Remove the done IORB from list */
         ENABLE

         /* If asynchronous post is requested then post the done IORB */
         if ( pDoneIORB->RequestControl & IORB_ASYNC_POST )
            {

               #ifdef DEBUG
               TraceIt(npACB, NOTIFY_MINOR);
               #endif

               _asm
                  {
                     pusha
                     push ds
                     push es
                  }

               pDoneIORB->NotifyAddress( pDoneIORB );

               _asm
                  {
                     pop es
                     pop ds
                     popa
                  }

               #ifdef DEBUG
               TraceIt(npACB, (NOTIFY_MINOR | TRACE_CMPLT) );
               #endif

            }
         NextIORB(npACB);                 /* Start processing the next IORB */
      }
   else
      {
         npACB->pHeadIORB = NULL;   /* List is empty */
         ENABLE

         /* If asynchronous post is requested then post the done IORB */
         if ( pDoneIORB->RequestControl & IORB_ASYNC_POST )
            {

               #ifdef DEBUG
               TraceIt(npACB, NOTIFY_MINOR);
               #endif

               _asm
                  {
                     pusha
                     push ds
                     push es
                  }

               pDoneIORB->NotifyAddress( pDoneIORB );

               _asm
                  {
                     pop es
                     pop ds
                     popa
                  }

               #ifdef DEBUG
               TraceIt(npACB, (NOTIFY_MINOR | TRACE_CMPLT) );
               #endif

            }

      }

   #ifdef DEBUG
   TraceIt(npACB, (IORBDONE_MINOR | TRACE_CMPLT) );
   #endif

}


