/*static char *SCCSID = "@(#)adsksm.c	6.6 92/02/04";*/

/***********************************************************************/
/*                                                                     */
/* Driver Name: ABIOS Adapter Device Driver for DASD Devices           */
/*              --------------------------------------------           */
/*                                                                     */
/* Source File Name: ADSKSM.C                                          */
/*                                                                     */
/* Descriptive Name: ABIOS Disk Driver - ABIOS State Machine           */
/*                                                                     */
/* Function:                                                           */
/*                                                                     */
/***********************************************************************/

#define INCL_NOBASEAPI
#define INCL_NOPMAPI
#include <os2.h>

#include <devcmd.h>

#define INCL_INITRP_ONLY
#include <reqpkt.h>

#include <scb.h>
#include <abios.h>

#include <iorb.h>
#include <addcalls.h>

#include <dhcalls.h>

#include <adskcons.h>
#include <adsktype.h>
#include <adskpro.h>
#include <adskextn.h>


/*-------------------------------------------------------------*/
/*                                                             */
/* Start New I/O Operation                                     */
/* -----------------------                                     */
/*                                                             */
/*                                                             */
/*-------------------------------------------------------------*/


VOID NEAR StartLCB( npLCB )

NPLCB   npLCB;
{
  if ( npLCB->pIORB = GetNextIORB( npLCB ) )
    {
      if ( StartDeviceIO( npLCB, 0 ) == REQUEST_DONE )
        {
          QueueLCBComplete( npLCB );
          ProcessLCBComplete();
        }
    }
}

/*-------------------------------------------------------------*/
/*                                                             */
/* Get Next IORB                                               */
/* -------------                                               */
/*                                                             */
/*                                                             */
/*-------------------------------------------------------------*/

PIORB NEAR GetNextIORB( npLCB )

NPLCB   npLCB;

{
  PIORB    pIORB = NULL;
  NPUCB    npCurrentUCB;
  NPUCB    npUCB;

  DISABLE

  npCurrentUCB     =  npUCB = npLCB->npCurUCB;
  npLCB->IntFlags &= ~LCBF_ACTIVE;

  do
    {
      npUCB = (!(npUCB->Flags & UCBF_LAST)) ? npUCB->npNextUCB : npLCB->npFirstUCB;

      if ( npUCB->pQueueHead )
        {
          pIORB = npUCB->pQueueHead;

          if ( !(npUCB->pQueueHead = pIORB->pNxtIORB) )
            {
              npUCB->pQueueFoot = 0;
            }
          npLCB->IntFlags |= LCBF_ACTIVE;
          npLCB->npCurUCB  = npUCB;

          break;
        }
    }
  while ( npUCB != npCurrentUCB );

  ENABLE

  return ( pIORB );
}


/*-------------------------------------------------------------*/
/*                                                             */
/* Start New I/O Request                                       */
/* ---------------------                                       */
/*                                                             */
/*                                                             */
/*-------------------------------------------------------------*/


USHORT NEAR StartDeviceIO( npLCB, npIOBuf )

NPLCB          npLCB;
NPIOBUF_POOL   npIOBuf;
{
  PIORB                 pIORB       =                   npLCB->pIORB;
  NPABRBH               npABRB      =  (NPABRBH)        npLCB->ABIOSReq;
  NPABRB_DISK_RWV       npABRWV     = (NPABRB_DISK_RWV) npABRB;
  USHORT                CmdCode     =                   pIORB->CommandCode;
  USHORT                CmdModifier =                   pIORB->CommandModifier;
  USHORT                Rc          =                   REQUEST_NOT_DONE;

  USHORT                cSGList;
  USHORT                OpFlags;
  ULONG                 LogBuf;
  ULONG                 PhysBuf;
  USHORT                ABIOSRc;


  /*-------------------------------------------------------------*/
  /*                                                             */
  /* Handle Entry after S/G Buffer Obtained                      */
  /* --------------------------------------                      */
  /*                                                             */
  /* If this routine is being called after a buffer-wait has     */
  /* been satisfied, then the LCB is partially initialized       */
  /* and some of the initialization may be skipped.              */
  /*                                                             */
  /* However, care must be taken that stack variables are        */
  /* set properly for the remainder of the routine to            */
  /* function properly.                                          */
  /*                                                             */
  /*-------------------------------------------------------------*/

  if ( npIOBuf )
    {
      OpFlags = npLCB->IntFlags;
      npLCB->npIOBuf = npIOBuf;

      goto StartLCB_BufferAllocated;
    }

  /*-------------------------------------------------------------*/
  /*                                                             */
  /* Determine the ABIOS Function Code                           */
  /* ---------------------------------                           */
  /*                                                             */
  /* The following LCB flags are set:                            */
  /*                                                             */
  /* 1.) LCBF_READ / LCBF_WRITE - Read/Write type operation      */
  /* 2.) LCBF_DATA_XFER         - Data transfer required         */
  /* 2.) LCBF_SGBUF_REQUIRED    - Blocking-Deblocking Buffer req */
  /*                                                             */
  /*-------------------------------------------------------------*/

  if ( CmdCode == IOCC_EXECUTE_IO )
    {
      if (CmdModifier == IOCM_READ )
        {
          OpFlags          = (LCBF_READ  | LCBF_DATA_XFER);
          npABRB->Function = ABFC_DISK_READ;
        }
      else if ( CmdModifier == IOCM_WRITE )
        {
          OpFlags          = (LCBF_WRITE | LCBF_DATA_XFER);
          npABRB->Function = ABFC_DISK_WRITE;
        }
      else if ( CmdModifier == IOCM_WRITE_VERIFY )
        {
          OpFlags          = (LCBF_WRITE | LCBF_DATA_XFER);
          npABRB->Function = ABFC_DISK_WRITE_VERIFY;
        }
      else if ( CmdModifier == IOCM_READ_VERIFY )
        {
          OpFlags           = LCBF_READ;
          npABRB->Function  = ABFC_DISK_VERIFY;
        }
      else
        {
          pIORB->ErrorCode = IOERR_CMD_NOT_SUPPORTED;
          Rc = REQUEST_DONE;

          _asm { int 3 }

          goto StartLCB_Exit;
        }
      cSGList = ((PIORB_EXECUTEIO) pIORB)->cSGList;
    }
  else if ( CmdCode == IOCC_UNIT_STATUS )
    {
      OpFlags           = LCBF_READ;
      npABRB->Function  = ABFC_DISK_VERIFY;
      cSGList           = 0;
    }
  else
    {
      pIORB->ErrorCode = IOERR_CMD_NOT_SUPPORTED;
      Rc = REQUEST_DONE;

      _asm { int 3 }

      goto StartLCB_Exit;
    }

  npLCB->HwMaxXfer    = npLCB->npCurUCB->HwMaxXfer;
  npLCB->ABIOSRetry   = npLCB->npCurUCB->ABIOSRetry;
  npABRWV->abrbh.Unit = npLCB->npCurUCB->ABIOSUnit;

  OpFlags |= ( OpFlags & LCBF_DATA_XFER && cSGList > 1 )
                                              ? LCBF_SGBUF_REQUIRED : 0;

  DISABLE
  npLCB->IntFlags &= ~LCBF_RESET_FLAGS;
  npLCB->IntFlags |= OpFlags;
  ENABLE

  npLCB->CmdCode     = CmdCode;
  npLCB->CmdModifier = CmdModifier;
  npLCB->Timeout     = pIORB->Timeout;


  /*-------------------------------------------------------------*/
  /*                                                             */
  /* Allocate Scatter/Gather Blocking-Deblocking Buffer          */
  /* --------------------------------------------------          */
  /*                                                             */
  /* 1.) Attempt to allocate a buffer if the operation           */
  /*     requires data transfer to > 1 block of memory           */
  /*                                                             */
  /*                                                             */
  /* 2.) If a buffer is not immediately available, then this     */
  /*     routine exits and will be recalled by AllocSGBuffer()   */
  /*     when a buffer becomes available.                        */
  /*                                                             */
  /*-------------------------------------------------------------*/

  if ( npLCB->IntFlags & LCBF_SGBUF_REQUIRED )
    {
      if ( !(npLCB->npIOBuf = AllocSGBuffer( npLCB, StartDeviceIO)) )
        {
          goto StartLCB_Exit;
        }
    }

StartLCB_BufferAllocated: ;

  /*-----------------------------------------------*/
  /* Transfer various IORB fields to the LCB to    */
  /* allow for NEAR access.                        */
  /*-----------------------------------------------*/

  if ( CmdCode == IOCC_EXECUTE_IO )
    {
      npLCB->ReqRBA            = npLCB->CurRBA =
                                 ((PIORB_EXECUTEIO) pIORB)->RBA;

      npLCB->ReqSectors        = npLCB->CurSecRemaining =
                                 ((PIORB_EXECUTEIO) pIORB)->BlockCount;

      npLCB->XferData.cSGList  = ((PIORB_EXECUTEIO) pIORB)->cSGList;
      npLCB->XferData.pSGList  = ((PIORB_EXECUTEIO) pIORB)->pSGList;

      if ( npLCB->IntFlags & LCBF_SGBUF_REQUIRED )
        {
          npLCB->XferData.pBuffer  = npLCB->npIOBuf->pBuf;
          npLCB->XferData.iSGList  = 0;
          npLCB->XferData.SGOffset = 0;
          npLCB->XferData.Mode     =
            (OpFlags & LCBF_READ) ? BUFFER_TO_SGLIST : SGLIST_TO_BUFFER;
        }
    }
  else if (CmdCode == IOCC_UNIT_STATUS )
    {
      npLCB->CmdCode          = IOCC_EXECUTE_IO;    /* Issue DUMMY Read Op */
      npLCB->CmdModifier      = IOCM_READ_VERIFY;
      npLCB->ReqSectors       = 1;
      npLCB->XferData.cSGList = 0;
      npLCB->XferData.pSGList = 0;
    }

  /*---------------------------*/
  /* Determine I/O Block Count */
  /*---------------------------*/

  npLCB->CurSectors = CalcMaxXfer( npLCB );
  npLCB->CurBytes   = ((ULONG) npLCB->CurSectors) << SECTOR_SHIFT;

  npABRWV->RBA     = npLCB->CurRBA;
  npABRWV->cBlocks = npLCB->CurSectors;


  /*--------------------------------------------------------------*/
  /*                                                              */
  /* Determine ABIOS Data Pointers                                */
  /* -----------------------------                                */
  /*                                                              */
  /* 1.) If the request is to a blocking-deblocking buffer,       */
  /*     then direct the ABIOS pointers at the allocated          */
  /*     buffer.                                                  */
  /*                                                              */
  /* 2.) If the request is directed to the user buffer, then      */
  /*     direct the ABIOS pointers to the user buffer.            */
  /*     If the ABIOS Lid Flags indicate that Logical (Sel:Offset)*/
  /*     are required, then set a preallocated GDT selector       */
  /*     to point to the user buffer.                             */
  /*                                                              */
  /*--------------------------------------------------------------*/

  LogBuf = PhysBuf = 0;

  if ( OpFlags & LCBF_SGBUF_REQUIRED )
    {
      PhysBuf = npLCB->npIOBuf->ppBuf;
      LogBuf  = (ULONG) npLCB->npIOBuf->pBuf;
    }
  else if ( OpFlags & LCBF_DATA_XFER )
    {
      PhysBuf = npLCB->XferData.pSGList->ppXferBuf;

      if ( npLCB->ABIOSLidFlags & LF_LOGICAL_PTRS )
        {
          if ( DevHelp_PhysToGDTSelector(
                            (ULONG)  PhysBuf,
                            (USHORT) npLCB->CurSectors << SECTOR_SHIFT,
                            (SEL)    npLCB->LogXferSel   ) )
            {
              _asm { int 3 }
            }

          LogBuf = ((ULONG) npLCB->LogXferSel) << 16;
        }
    }


  npABRWV->ppIObuffer = PhysBuf;
  npABRWV->pIObuffer  = LogBuf;


  if ( npLCB->IntFlags & LCBF_WRITE && npLCB->IntFlags & LCBF_SGBUF_REQUIRED )
    {
      npLCB->XferData.numTotalBytes = npLCB->CurBytes;
      ADD_XferBuffData(&npLCB->XferData);
    }


  /*-------------------------------------------------------------*/
  /*                                                             */
  /* Check for Synchronous Completion                            */
  /* --------------------------------                            */
  /*                                                             */
  /* If the ABIOS return code indicates no additional stages are */
  /* required at this point, it uncertain whether completion     */
  /* processing has already been done on an interrupt.           */
  /*                                                             */
  /* If the flag LCBF_STARTPENDING is still set at this point,   */
  /* it indicates that no additional stages have been performed  */
  /* and that this routine must do completion processing         */
  /*                                                             */
  /*-------------------------------------------------------------*/


  npLCB->Retries        = npLCB->ABIOSRetry;
  npLCB->LastABIOSError = 0;

  do
    {
      npLCB->IntFlags  |= LCBF_STARTPENDING;
      npABRWV->cBlocks  = npLCB->CurSectors;

      ABIOSRc = StageABIOSRequest( npLCB, ABIOS_EP_START );
    }
  while ((Rc=ABIOSErrorToRc( npLCB, ABIOSRc, STAGE_START )) == REQUEST_RETRY);

StartLCB_Exit: ;

  return ( Rc );

}


/*-------------------------------------------------------------*/
/*                                                             */
/* Continue On-Going I/O Request                               */
/* -----------------------------                               */
/*                                                             */
/*                                                             */
/*-------------------------------------------------------------*/


USHORT NEAR ContinueDeviceIO( npLCB )

NPLCB   npLCB;
{
  NPABRBH               npABRB;
  NPABRB_DISK_RWV       npABRWV;
  USHORT                Rc;
  ULONG                 PhysBuf;
  USHORT                ABIOSRc;

  npABRB  = (NPABRBH)         npLCB->ABIOSReq ;
  npABRWV = (NPABRB_DISK_RWV) npABRB;

  ABIOSRc = npABRB->RC;

  Rc = REQUEST_DONE;

  RetryLCBBusyQ( npLCB );

  /*-------------------------------------*/
  /* Check ABIOS Rc for error completion */
  /*-------------------------------------*/

  if ( ABIOSRc & ABRC_ERRORBIT )
    {
      if ( (Rc=ABIOSErrorToRc( npLCB, ABIOSRc, STAGE_INT )) == REQUEST_RETRY )
        {
          do
            {
              npLCB->IntFlags |= LCBF_STARTPENDING;
              npABRWV->cBlocks  = npLCB->CurSectors;

              ABIOSRc = StageABIOSRequest( npLCB, ABIOS_EP_START );
            }
          while ((Rc=ABIOSErrorToRc(npLCB, ABIOSRc, STAGE_START)) == REQUEST_RETRY);

          goto ContinueDeviceIO_Exit;
        }
    }


  /*-------------------------------------------*/
  /* If the request was a read and required    */
  /* a blocking buffer, then transfer the data */
  /* to the user buffer.                       */
  /*-------------------------------------------*/

  if ( npLCB->IntFlags & LCBF_SGBUF_REQUIRED )
    {
      if ( npLCB->IntFlags & LCBF_READ )
        {
          npLCB->XferData.numTotalBytes = npLCB->CurBytes;
          ADD_XferBuffData(&npLCB->XferData);
        }
    }

  /*-------------------------------------*/
  /* Check if the request is complete    */
  /*-------------------------------------*/

  if ( !(npLCB->CurSecRemaining -= npLCB->CurSectors) )
    {
      goto ContinueDeviceIO_Exit;
    }

  /*--------------------------------------------*/
  /* Continue a partially completed I/O Request */
  /*--------------------------------------------*/

  npLCB->CurRBA += npLCB->CurSectors;

  /*-----------------------------------*/
  /* Determine the new I/O Block Count */
  /*-----------------------------------*/

  npLCB->CurSectors = CalcMaxXfer( npLCB );
  npLCB->CurBytes   = ((ULONG) npLCB->CurSectors) << SECTOR_SHIFT;

  npABRWV->RBA      = npLCB->CurRBA;
  npABRWV->cBlocks  = npLCB->CurSectors;

  /*-----------------------------------------------------------*/
  /* If the request required a blocking buffer and is a Write, */
  /* Then transfer a new extent to the blocking buffer         */
  /*-----------------------------------------------------------*/

  if ( npLCB->IntFlags & LCBF_SGBUF_REQUIRED )
    {
      if ( npLCB->IntFlags & LCBF_WRITE )
        {
          npLCB->XferData.numTotalBytes = npLCB->CurBytes;
          ADD_XferBuffData(&npLCB->XferData);
        }
    }

  /*-------------------------------------------*/
  /* If the request required a data transfer   */
  /* but did not use a blocking buffer, then   */
  /* advance the pointers to the user buffer.  */
  /*-------------------------------------------*/

  else
    {
      if (npLCB->IntFlags & LCBF_DATA_XFER )
        {
          PhysBuf = npABRWV->ppIObuffer + npLCB->CurBytes;

          if ( npLCB->ABIOSLidFlags & LF_PHYSICAL_PTRS )
            {
              npABRWV->ppIObuffer = PhysBuf;
            }
          if ( npLCB->ABIOSLidFlags & LF_LOGICAL_PTRS )
            {
              if ( DevHelp_PhysToGDTSelector(
                        (ULONG)  PhysBuf,
                        (USHORT) npLCB->CurSectors << SECTOR_SHIFT,
                        (SEL)    npLCB->LogXferSel   ) )
                {
                  _asm { int 3 }
                }
            }
        }
    }

  /*----------------------------------------------------*/
  /* Reset Retry Count and begin next part of operation */
  /*----------------------------------------------------*/

  npLCB->Retries = npLCB->ABIOSRetry;

  do
    {
      npLCB->IntFlags |= LCBF_STARTPENDING;
      npABRWV->cBlocks  = npLCB->CurSectors;

      ABIOSRc = StageABIOSRequest( npLCB, ABIOS_EP_START );
    }
  while ((Rc=ABIOSErrorToRc( npLCB, ABIOSRc, STAGE_START )) == REQUEST_RETRY);

ContinueDeviceIO_Exit: ;

  return (Rc);
}


/*-------------------------------------------------------------*/
/*                                                             */
/* Complete I/O Request                                        */
/* --------------------                                        */
/*                                                             */
/*                                                             */
/*-------------------------------------------------------------*/

VOID NEAR CompleteDeviceIO( npLCB )

NPLCB   npLCB;
{

  PIORB         pIORB       =            npLCB->pIORB;
  USHORT        ABIOSRc     = ((NPABRBH) npLCB->ABIOSReq)->RC;
  USHORT        ErrorCode;
  NPIOBUF_POOL  npIOBuf;

  npLCB->pIORB = 0;

  if ( !(ErrorCode=pIORB->ErrorCode) )
    {
      if ( ABIOSRc != ABRC_COMPLETEOK )
        {
          ErrorCode = ABIOSToIORBError( ABIOSRc );
          if ( ABIOSRc != ABRC_DISK_ECC_CORRECTED )
            {
              pIORB->Status &= ~IORB_RECOV_ERROR;
            }
        }
      else if ( npLCB->LastABIOSError )
        {
          ErrorCode = ABIOSToIORBError( npLCB->LastABIOSError );
        }
    }

  if ( (npLCB->IntFlags & LCBF_SGBUF_REQUIRED) && (npIOBuf=npLCB->npIOBuf) )
    {
      npLCB->npIOBuf = 0;
      FreeSGBuffer( npIOBuf );
    }

  /*-------------------------------------------*/
  /* Calculate Blocks transferred              */
  /* ----------------------------              */
  /* Assume no residual count. If there is a   */
  /* residual count subtract it from the total.*/
  /* If the operation involved a data transfer,*/
  /* then add the partially completed request  */
  /* into the transfer count.                  */
  /*-------------------------------------------*/

  ((PIORB_EXECUTEIO) pIORB)->BlocksXferred =
                                   ((PIORB_EXECUTEIO) pIORB)->BlockCount;

  if ( npLCB->CurSecRemaining )
    {
      ((PIORB_EXECUTEIO) pIORB)->BlocksXferred -=
          npLCB->CurSecRemaining -
          ((npLCB->IntFlags & LCBF_DATA_XFER) ?
                        ((NPABRB_DISK_RWV) npLCB->ABIOSReq)->cBlocks : 0 );
    }

  NotifyIORB( pIORB, ErrorCode );

}


/*-------------------------------------------------------------*/
/*                                                             */
/* Calculate Maximum Request Size                              */
/* ------------------------------                              */
/*                                                             */
/* The following LCB variable must be initialized:             */
/*                                                             */
/* npLCB->IntFlags                                             */
/* npLCB->HwMaxXfer                                            */
/* npLCB->nPIOBuf (if a S/G buffer is required)                */
/* npLCB->CurSecRemaining                                      */
/*                                                             */
/*-------------------------------------------------------------*/

USHORT NEAR CalcMaxXfer( npLCB )

NPLCB   npLCB;
{
  USHORT   MaxXfer;

  /*--------------------------------------------------------------*/
  /*                                                              */
  /* Determine I/O Block Count                                    */
  /* -------------------------                                    */
  /*                                                              */
  /* The maximum request size is the minimum of the following:    */
  /*                                                              */
  /* 1.) The maximum ABIOS transfer length of the selected unit.  */
  /* 2.) The size of the S/G Blocking buffer (if needed).         */
  /* 3.) The actual number of sectors requested.                  */
  /*                                                              */
  /*--------------------------------------------------------------*/

  MaxXfer = npLCB->HwMaxXfer;

  if ( npLCB->IntFlags & LCBF_SGBUF_REQUIRED )
    {
      MaxXfer = (MaxXfer > npLCB->npIOBuf->BufSec)
                               ? npLCB->npIOBuf->BufSec : MaxXfer;
    }

  MaxXfer = ( npLCB->CurSecRemaining > MaxXfer )
                                   ? MaxXfer : npLCB->CurSecRemaining;

  return ( MaxXfer );
}


/*-------------------------------------------------------------*/
/*                                                             */
/* Convert ABIOS Rc to Internal Action Code                    */
/* ----------------------------------------                    */
/*                                                             */
/*                                                             */
/*-------------------------------------------------------------*/

USHORT ABIOSErrorToRc( npLCB, ABIOSRc, Stage )

NPLCB   npLCB;
USHORT  ABIOSRc;
USHORT  Stage;

{
  USHORT   Rc;


  /*--------------------------------------------------------*/
  /* If one or more interrupting stages were handled before */
  /* returning from ABIOS START processing, then no further */
  /* action is required.                                    */
  /*--------------------------------------------------------*/

  if ( (Stage == STAGE_START) && !(npLCB->IntFlags & LCBF_STARTPENDING) )
    {
      Rc = REQUEST_NOT_DONE;
    }

  /*--------------------------------------------------------*/
  /* If the request completed O.K. without any interrupting */
  /* stages, then we need to do completion processing.      */
  /*--------------------------------------------------------*/

  else if ( ABIOSRc == ABRC_COMPLETEOK )
    {
      Rc = REQUEST_DONE;
    }

  /*--------------------------------------------------------*/
  /* If the request completed with error, do take further   */
  /* action based on the error code.                        */
  /*--------------------------------------------------------*/

  else if ( ABIOSRc & ABRC_ERRORBIT )
    {
        npLCB->LastABIOSError = ABIOSRc;

       /*-------------------------------------------------------------*/
       /* Note: Checking for the low byte of the ABIOS return code    */
       /*       for UNDEFINED ERROR is an ABIOS hack for problems     */
       /*       with the Mod 80 ESDI controller which intermittently  */
       /*       reports this error for approx 12s.                    */
       /*-------------------------------------------------------------*/

       if ( !((ABIOSRc ^ ABRC_DISK_UNDEFINED_ERROR) & ABRC_ERRORMSK) )
         {
           Rc = REQUEST_RETRY;
         }

       /*-------------------------------------------------------------*/
       /* Note: Checking for ABRC_BUSY is an ABIOS                    */
       /*       hack for the Mod 80 ESDI controller which reports     */
       /*       that its a concurrent controller, but is not really   */
       /*       fully concurrent, i.e. sometimes operations are       */
       /*       rejected with a BUSY Rc.                              */
       /*-------------------------------------------------------------*/

       else if ( (ABIOSRc & ~ABRC_RETRYBIT) == ABRC_BUSY )
         {
           /*--------------------------------------------------*/
           /* If there was more than 1 unit active on the LID, */
           /* then queue the failed operation for latter retry */
           /*--------------------------------------------------*/

           if ( !LidIOCount[npLCB->LidIndex] )
             {
               _asm { int 3 }
             }
           else if ( LidIOCount[npLCB->LidIndex] == 1 )
             {
               Rc = ( npLCB->Retries-- ) ? REQUEST_RETRY : REQUEST_DONE;
             }
           else
             {
               DISABLE

               npLCB->npNextBusyQLCB = npLCBBusyQHead;
               npLCBBusyQHead        = npLCB;
               npLCB->IntFlags       |= LCBF_ONBUSYQ;

               LidIOCount[npLCB->LidIndex]--;

               ENABLE

               Rc = REQUEST_NOT_DONE;
             }
         }

       /*-------------------------------------------------------------*/
       /* For other errors, if the RETRY bit in the ABIOS Rc is on    */
       /* then retry the operation if the retry count is not          */
       /* not exhausted.                                              */
       /*-------------------------------------------------------------*/

       else if ((ABIOSRc & ABRC_RETRYBIT) && npLCB->Retries-- )
         {
           Rc = REQUEST_RETRY;
         }
       else
         {
           Rc = REQUEST_DONE;
         }

       /*-------------------------------------------------------------*/
       /* If we had an error, but have not given up, set the          */
       /* RECOVERED error bit in the IORB status.                     */
       /*-------------------------------------------------------------*/

       if (Rc != REQUEST_DONE)
         {
            npLCB->pIORB->Status |= IORB_RECOV_ERROR;
         }
    }

  /*-------------------------------------------------------------*/
  /* All other ABIOS return codes should be > 0, i.e. a staged   */
  /* request which have been processed by StageABIOSRequest().   */
  /*-------------------------------------------------------------*/

  else
    {
      Rc = REQUEST_NOT_DONE;
    }

  return (Rc);

}

/*-------------------------------------------------------------*/
/*                                                             */
/* Convert ABIOS to IORB Error Codes                           */
/* ---------------------------------                           */
/*                                                             */
/*                                                             */
/*-------------------------------------------------------------*/


USHORT NEAR ABIOSToIORBError( ABIOSRc )

USHORT  ABIOSRc;
{
  NPSZ       npABRC_To_Index;
  NPUSHORT   npIndex_To_IORBErr;
  USHORT     MaxIndex;

  USHORT     IORBError;
  USHORT     ABIOSErrClass;
  USHORT     i;
  UCHAR      ABIOSErrCode;


  /*---------------------------------------------------------------------*/
  /*                                                                     */
  /* See ADSKERRT.H for information on how ABIOS errors are translated   */
  /*                                                                     */
  /*---------------------------------------------------------------------*/

  if ( ABIOSRc & ABRC_ERRORBIT )
    {
      ABIOSErrClass = ABIOSRc & (ABRC_ERRORBIT | ABRC_PARMERROR |
                                          ABRC_TIMEOUTERROR | ABRC_DEVERROR);

      ABIOSErrCode  = (UCHAR) (ABIOSRc & ABRC_ERRORMSK);

      if ( ABIOSErrClass == 0xC000 )
        {
          MaxIndex           = AB_Cxxx_Max_Index;
          npABRC_To_Index    = AB_Cxxx_To_Index;
          npIndex_To_IORBErr = AB_Cxxx_Index_To_IORBErr;
        }
      else if ( ABIOSErrClass == 0x8000 )
        {
          MaxIndex           = AB_Cxxx_Max_Index;
          npABRC_To_Index    = AB_8xxx_To_Index;
          npIndex_To_IORBErr = AB_8xxx_Index_To_IORBErr;
        }
      else if ( ABIOSErrClass == 0x9000 || ABIOSErrClass == 0xB000
                                                 || ABIOSErrClass == 0xA000 )
        {
          MaxIndex           = AB_9xxx_Max_Index;
          npABRC_To_Index    = AB_9xxx_To_Index;
          npIndex_To_IORBErr = (ABIOSErrClass == 0xA000) ? AB_Axxx_Index_To_IORBErr
                                                         : AB_9xxx_Index_To_IORBErr;
        }
      else
        {
          _asm { int 3 }
        }

      for ( i = 0;
            i < MaxIndex && npABRC_To_Index[i] != ABIOSErrCode;
            i++ )
        ;

      IORBError = npIndex_To_IORBErr[i];
    }

  return ( IORBError );
}

/*-------------------------------------------------------------*/
/*                                                             */
/* Retry LCB Busy Q                                            */
/* ----------------                                            */
/*                                                             */
/* This routine is called when an ABIOS request has completed  */
/* with either an error or non-error return code.              */
/*                                                             */
/* The count of outstanding I/Os on the LID is checked.        */
/* If the count reaches 0, then the ADD's BUSY queue is        */
/* checked for any requests that have been suspended due to    */
/* a busy condition. If a request is found that is waiting     */
/* on the LID that completed, then the request removed from    */
/* the busy queue and retried.                                 */
/*                                                             */
/*                                                             */
/*-------------------------------------------------------------*/

VOID RetryLCBBusyQ( npLCB )

NPLCB   npLCB;

{
  NPLCB         npCurrBusyLCB;
  NPLCB         npPrevBusyLCB;
  USHORT        LidIndex;
  USHORT        Rc;
  USHORT        ABIOSRc;


  DISABLE

  LidIndex = npLCB->LidIndex;

  if ( !LidIOCount[LidIndex] )
    {
      _asm { int 3 }
    }

  if ( --LidIOCount[LidIndex] )
    {
      ENABLE
      goto RetryLCBBusy_Exit;
    }

  npPrevBusyLCB = 0;
  npCurrBusyLCB = npLCBBusyQHead;

  while ( npCurrBusyLCB )
    {
      if ( npCurrBusyLCB->LidIndex == LidIndex )
        {
          if ( !npPrevBusyLCB )
            {
              npLCBBusyQHead = npCurrBusyLCB->npNextBusyQLCB;
            }
          else
            {
              npPrevBusyLCB->npNextBusyQLCB = npCurrBusyLCB->npNextBusyQLCB;
            }

          npCurrBusyLCB->npNextBusyQLCB  = 0;
          npCurrBusyLCB->IntFlags       &= ~LCBF_ONBUSYQ;

          break;
        }
      npPrevBusyLCB = npCurrBusyLCB;
      npCurrBusyLCB = npPrevBusyLCB->npNextBusyQLCB;
    }

  ENABLE

  if ( npCurrBusyLCB )
    {
      do
        {
          npCurrBusyLCB->IntFlags |= LCBF_STARTPENDING;

          ((NPABRB_DISK_RWV) npCurrBusyLCB->ABIOSReq)->cBlocks
                                                  = npCurrBusyLCB->CurSectors;

          ABIOSRc = StageABIOSRequest( npCurrBusyLCB, ABIOS_EP_START );
        }
      while ((Rc=ABIOSErrorToRc(npCurrBusyLCB, ABIOSRc, STAGE_START)) == REQUEST_RETRY);

      if ( Rc == REQUEST_DONE )
        {
          QueueLCBComplete( npCurrBusyLCB );
        }
    }


RetryLCBBusy_Exit: ;

  return;
}
