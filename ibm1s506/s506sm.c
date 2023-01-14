/*static char *SCCSID = "@(#)s506sm.c	6.2 92/01/10";*/

/****************************************************************************
 *                                                                          *
 *                (c) Copyright  IBM Corporation  1991                      *
 *                           All Rights Reserved                            *
 *                                                                          *
 ****************************************************************************/

/********************* Start of Specifications ******************************
 *                                                                          *
 *  Source File Name: S506SM.C                                              *
 *                                                                          *
 *  Descriptive Name: AT Hard Disk State processing control                 *
 *                                                                          *
 *  Copyright:                                                              *
 *                                                                          *
 *  Status:                                                                 *
 *                                                                          *
 *  Function:                                                               *
 *                                                                          *
 *  Notes:                                                                  *
 *    Dependencies:                                                         *
 *    Restrictions:                                                         *
 *                                                                          *
 *  Entry Points:                                                           *
 *                                                                          *
 *  External References:  See EXTRN statements below                        *
 *                                                                          *
 ********************** End of Specifications *******************************/

 #define INCL_NOBASEAPI
 #define INCL_NOPMAPI
 #define INCL_NO_SCB
 #define INCL_INITRP_ONLY
 #include "os2.h"                  // C:\DRV6\H
 #include "dos.h"                  // C:\DRV6\H

 #include "iorb.h"                 // C:\DRV6\SRC\DEV\DASD\DISKH
 #include "reqpkt.h"               // C:\DRV6\SRC\DEV\DASD\DISKH
 #include "dhcalls.h"

 #include "s506hdr.h"
 #include "s506ext.h"
 #include "s506pro.h"

 #include "addcalls.h"

/****************************************************************************
 *                                                                          *
 *      DRIVE EXECUTE                                                       *
 *              This routine dispatches the current IORB processing         *
 *              dependent upon DriveState.                                  *
 *                                                                          *
 *                                                                          *
 *      This routine dispatches the processing of the current IORB; with    *
 *      the basic processing cycle being:                                   *
 *              IDLE -> STARTofIORB -> CALC -> SELECT -> SEEK ->            *
 *                      SETTLE -> TRANSFER -> DONE -> IDLE                  *
 *                                                                          *
 *      Processing progresses from state to state during an invocation of   *
 *      this routine until either:                                          *
 *           continuation to the next state requires a hardware interrupt   *
 *           response or a time delay to check equipment status             *
 *       or                                                                 *
 *           all IORBs in queue have completed                              *
 *                                                                          *
 *                                                                          *
 *      This routine is invoked from:                                       *
 *                                                                          *
 *              ADDEntryPoint  -  if current state is IDLE, and a QUEUE IORB*
 *                                 is received.                             *
 *                                                                          *
 *              IORBContinue  -  Hardware interrupt entry point             *
 *                                                                          *
 *              S506Timer  -  Entry point for Hardware Timeout or delay     *
 *                             timer processing                             *
 *                                                                          *
 *                                                                          *
 *      Termination of this routine:                                        *
 *                                                                          *
 *              Registration of IORBContinue for a hardware interrupt       *
 *                                                                          *
 *              Registration of S506Timer for time out / delay timer        *
 *                                                                          *
 *              Simple return to await next IORB                            *
 *                                                                          *
 ****************************************************************************/


/****************** Start of Specifications *********************************
 *                                                                          *
 * Subroutine Name: FixedInterrupt                                          *
 *                                                                          *
 * Descriptine Name: Fixed disk controller interrupt handler                *
 *                                                                          *
 * Function: service fixed disk controller interrupts - INT 76H             *
 *                                                                          *
 * Notes: entered in real and protect mode                                  *
 *        entered in interrupt mode context                                 *
 *                                                                          *
 *        This code replaces the ROM INT D handler (fixed disk              *
 *        emulation). We first check to see if we are emulating             *
 *        a ROM request.  if we were, we pass control off to the            *
 *        ROM routine, otherwise (if the drive is active), we               *
 *        reset the WD1000 fixed disk controller, and check to see          *
 *        if it is in an error condition.  If it is in an error,            *
 *        we reissue the request for ErrLim times.  If there is no          *
 *        error, we just call FixedExecute and return.                      *
 *                                                                          *
 * Entry Point: FixedInterrupt                                              *
 *                                                                          *
 * Linkage:  far call from kernel interrupt handler                         *
 *           DS    - BiosGroup                                              *
 *                                                                          *
 * Input: none                                                              *
 *                                                                          *
 * Exit-Normal: far ret to kernel                                           *
 *                                                                          *
 * Exit-Error:  far ret to kernel                                           *
 *                                                                          *
 * Effects:                                                                 *
 *                                                                          *
 * Internal References:                                                     *
 *                                                                          *
 * External References:                                                     *
 *                                                                          *
 ************************ End of Specifications *****************************/

VOID NEAR FixedInterrupt()
{
  NPACB                  npACB;

  npACB = &acb;            // get address of adapter control block

  #ifdef DEBUG
//TraceIt(npACB, INTERRUPT_MINOR);
  #endif

  if (npACB->Active)      // if device is active
    {
    TimerActive &= ~FIXEDT1;    // ignore timeout timer
    if (npACB->XFlags & FINTERRUPTSTATE) // if !spurious interrupt
      {
      npACB->XFlags &= ~FINTERRUPTSTATE;  // clear flag
      --npACB->TimerCall;
// Check to see if we are restarting the command via TimeOut timer (F2Timer).
      if (!(npACB->XFlags & FTIMERINT))
        {
        if (++npACB->NestedInts == 0)  // (-1 = no interrupt, 0 = level 1)
          {
          DevHelp_EOI(npACB->IntLevel);
          do
            {                                         // Do post processing
            ENABLE
            FixedExecute(npACB);     // call state machine
            DISABLE
            }
          while ( (--npACB->NestedInts) != -1 );
          } /*endif*/
        } /*endif*/
      } /*endif*/
    } /*endif*/

  DevHelp_EOI(npACB->IntLevel);

  #ifdef DEBUG
//TraceIt(npACB, (INTERRUPT_MINOR | TRACE_CMPLT));
  #endif

  _asm
    {
      clc                            // must inform INT mrg that it is ours
      leave
      retf
    }
}

/****************** Start of Specifications *********************************
 *                                                                          *
 * Subroutine Name: FixedExecute                                            *
 *                                                                          *
 * Descriptine Name: Fixed Disk device state machine                        *
 *                                                                          *
 * Function: Perform hard disk I/O operation.                               *
 *                                                                          *
 * Notes:                                                                   *
 *                                                                          *
 * Entry Point: FixedExecute                                                *
 *                                                                          *
 * Linkage:  near call                                                      *
 *           DS    - BiosGroup                                              *
 *                                                                          *
 * Input: SI address of current io structure.                               *
 *                                                                          *
 * Exit-Normal:                                                             *
 *                                                                          *
 * Exit-Error:                                                              *
 *                                                                          *
 * Effects:                                                                 *
 *                                                                          *
 * Internal Refereences:                                                    *
 *                                                                          *
 * External References:                                                     *
 *                                                                          *
 ************************* End of Specifications ****************************/

/** Fixed Execute ***********************************************************
 *                                                                          *
 *      FixedExecute processes a disk request after it has been set up.     *
 *      When the disk is inactive (State = Idle), it is called to start     *
 *      the device.  For all subsequent events, it is called on the disk    *
 *      interrupt which signalled the completion of that subfunction.       *
 *      Some states do not involve waiting for an interrupt to occur.       *
 *      This routine now is re-entrant on a per-controller basis and runs   *
 *      off the ACB structure.                                              *
 *                                                                          *
 *      This routine is called from FixedRead, if the state of the machine  *
 *      is Idle, and from FixedInterrupt if we are not in the middle of a   *
 *      request to the ROM.                                                 *
 ****************************************************************************/

VOID NEAR FixedExecute(NPACB npACB)
{

  #ifdef DEBUG
  TraceIt(npACB, SM_MINOR);
  #endif

  do
    {
    switch (npACB->State)
      {
      case START:
        {
        STARTState(npACB);
        break;
        }
      case CALC:
        {
        CALCState(npACB);
        break;
        }
      case VERIFY:
        {
        VERIFYState(npACB);
        break;
        }
      case DONE:
        {
        DONEState(npACB);
        break;
        }
      case IDLE:
        {
        IDLEState(npACB);
        break;
        }
      case SERROR:
        {
        SERRORState(npACB);
        break;
        }
      case RECAL:
        {
        RECALState(npACB);
        break;
        }
      case READ:
        {
        READState(npACB);
        break;
        }
      case WRITE:
        {
        WRITEState(npACB);
        break;
        }
      case SETPARAM:
        {
        SETPARAMState(npACB);
        break;
        }
      }
    }
  while (npACB->DrvStateReturn == NEXTSTATENOW);

  #ifdef DEBUG
  TraceIt(npACB, (SM_MINOR | TRACE_CMPLT));
  #endif

}

/*****************************************************************************
 *                                                                           *
 *     START STATE                                                           *
 *       Fixed state Start                                                   *
 *                                                                           *
 *       Do setup calculations to figure out sector, start                   *
 *       up motor, advance to Calc state.                                    *
 *                                                                           *
 *  Entered on initially picking up a new request to do and on error retries.*
 *  If error retries start here, then multiple sector requests will always   *
 *  start at the beginning rather than at the point of the error! Why?       *
 *                                                                           *
 *  Error retries for HPFS386 start here.                                    *
 *                                                                           *
 *  Input: SI -> IOStruc (this is needed for multicontroller support)        *
 *                                                                           *
 *****************************************************************************/
VOID NEAR STARTState(NPACB npACB)
{

  #ifdef DEBUG
  TraceIt(npACB, START_MINOR);
  #endif

  if ( !(npACB->Flags & FCOMPLETEINIT) ) // IOCM_COMPLETE_INIT IORB processed ?
    Initialize_hardfile();

  Enable_Hardfile();

  Setup(npACB);

// Set up the disk controller by telling it what type of device it is
// talking to. We have obtained this info. from the CMOS database.
// For multi-sector I/O, we need to tell the controller the max. numbers
// of heads and sectors/track.

  npACB->State = SETPARAM;      // Next state is SetParam
  npACB->DrvStateReturn = WAITSTATE;
  if ( (FDParam(npACB) == SUCCESS ) &&
       (npACB->DrvStateReturn == WAITSTATE) )
    SetTimeoutTimer(npACB);

  #ifdef DEBUG
  TraceIt(npACB, (START_MINOR | TRACE_CMPLT));
  #endif

}

/****************************************************************************
 *                                                                          *
 *    CALC STATE                                                            *
 *                                                                          *
 *     Fixed state Calc                                                     *
 *                                                                          *
 *     Calculate cylinder, head and sector. Set up command packet           *
 *     for controller. Issue the Read or Write command.                     *
 *                                                                          *
 *     Entered after SetParam state and also on further sectors of a        *
 *     multiple sector request.                                             *
 *                                                                          *
 *     Input: SI -> IOStruc                                                 *
 *            DI -> BDS                                                     *
 *                                                                          *
 ****************************************************************************/
VOID NEAR CALCState(NPACB npACB)
{
  UCHAR         Data;
  BOOL          rc;
  PIORB_EXECUTEIO pIORB = (PIORB_EXECUTEIO)npACB->pHeadIORB;
  UCHAR i = pIORB->iorbh.UnitHandle;          // set unit number (0 or 1)
  PGEOMETRY pGeometry;
  PCHS_ADDR pCHS_Addr = &(npACB->chs);
  CYL cyl;

  #ifdef DEBUG
  TraceIt(npACB, CALC_MINOR);
  #endif

  if ( npACB->unit[i].XlateFlags & XLATE_ENABLE )  // if in xlate mode
    pGeometry = &(npACB->unit[i].PhysGeometry);    // use physical geometry
  else
    pGeometry = &(npACB->unit[i].LogGeometry);     // else use logical geometry

  rc = ADD_ConvRBAtoCHS(npACB->First, pGeometry, pCHS_Addr);  // Get head, cylinder and sector

// Set up command packet to be sent to disk controller

  if ( npACB->SecToGo >= 256 )
    {
    npACB->cSectors = 256;              // save count of sectors for this multi i/o
    npACB->NumSectors = 0;              // do 256 sectors for this multi i/o
    npACB->Flags |= FMULTIIO;           // set multiple i/o for request
    }
  else
    {
    npACB->cSectors   = npACB->SecToGo;      // Save number of sectors this i/o
    npACB->NumSectors = (UCHAR)npACB->SecToGo;  // Number of sectors this i/o
    npACB->Flags &= ~FMULTIIO;              // reset mulitple i/o for request
    }

  npACB->IORegs[FI_PWRP] = (npACB->unit[i].WrtPrCmp)/4;  // Write precompensation
  npACB->IORegs[FI_PSECCNT] = npACB->NumSectors;        // Number of sectors
  npACB->IORegs[FI_PSECNUM] = npACB->chs.Sector;     // Sector number is 1-based

  cyl.w.WordCyl = npACB->chs.Cylinder;
  npACB->IORegs[FI_PCYLL] = cyl.b.LowCyl;     // Cylinder (low)
  npACB->IORegs[FI_PCYLH] = cyl.b.HighCyl;    // Cylinder (high)

  Data = (npACB->drivebit & 1);         // get the 0/1 for first or second disk
  Data = (Data << 4);
  Data |= 0x00a0;
  Data |= npACB->chs.Head;
  npACB->IORegs[FI_PDRHD] = Data;       // Drive and Head

// Now do the operation (Read or Write - Verify comes later)

  if ( npACB->Flags & FREAD )
    {

    // The controller issues an interrupt when the buffer is ready to be read by
    // the system. We issue the Read command to the controller and then return.
    // i.e. "Wait" for the interrupt.

    npACB->State = READ;                        // Next state is READ
    npACB->DrvStateReturn = WAITSTATE;          // Wait for an interrupt
    if ( npACB->unit[i].DriveFlags & FMULTIPLEMODE )
      npACB->IORegs[FI_PCMD] = FX_CREADMUL;
    else
      npACB->IORegs[FI_PCMD] = FX_CREAD;
    if ( SendCmdPkt(npACB) == SUCCESS ) // Send off command packet to controller
      SetTimeoutTimer(npACB);
    }
  else
    {
    if ( npACB->Flags & FWRITE )
      {
      // The controller issues an interrupt when it is done writing out the
      // buffer to the disk. We write out 512 bytes to the data port, once
      // we get a DRQ status, and then return i.e. "wait" for an interrupt.

      npACB->State = WRITE;                     // Next state is WRITE
      npACB->DrvStateReturn = WAITSTATE;        // Wait for an interrupt
      if ( npACB->unit[i].DriveFlags & FMULTIPLEMODE )
        npACB->IORegs[FI_PCMD] = FX_CWRITEMUL;
      else
        npACB->IORegs[FI_PCMD] = FX_CWRITE;       // write with retry

      if ( (SendCmdPkt(npACB) == SUCCESS) &&    // Send off cmd pkt to controller
           (FDWrite(npACB) == SUCCESS) )        // Write out 512 bytes to controller
        SetTimeoutTimer(npACB);                 // "Wait" for interrupt
      }
    else  // Assume Verify if not Read or Write
      {
      Do_Verify(npACB);
      }
    }

  #ifdef DEBUG
  TraceIt(npACB, (CALC_MINOR | TRACE_CMPLT));
  #endif

}

/****************************************************************************
 *                                                                          *
 *    VERIFY STATE                                                          *
 *                                                                          *
 ****************************************************************************/
VOID NEAR VERIFYState(NPACB npACB)
{
  UCHAR         Status;
  PIORB_EXECUTEIO pIORB = (PIORB_EXECUTEIO)npACB->pHeadIORB;

  #ifdef DEBUG
  TraceIt(npACB, VERIFY_MINOR);
  #endif

  Status = FDGetStat(npACB);             // Get status of controller

  if ( Status & FX_ERROR )
    {
    npACB->State = SERROR;                      // next state is SERROR
    npACB->DrvStateReturn = NEXTSTATENOW;
    npACB->IORegs[FI_PERR] = Status;            // Save status in DCB
    }
  else
    {

    if ( pIORB->iorbh.CommandModifier == IOCM_READ_VERIFY )
      {
      pIORB->BlocksXferred += npACB->cSectors;
      npACB->SecToGo -= npACB->cSectors;  // dec number of sectors left in req.
      }

    if ( npACB->SecToGo )     // if request is not finished
      {
      npACB->First += 256;                      // next RBA
      npACB->State = CALC;                      // Next stat is CALC
      npACB->DrvStateReturn = NEXTSTATENOW;
      }
    else
      {
      npACB->State = DONE;                    // Next state is DONE
      npACB->DrvStateReturn = NEXTSTATENOW;
      npACB->Flags &= ~FMULTIIO;              // reset mulitple i/o for request
      }
    }

  #ifdef DEBUG
  TraceIt(npACB, (VERIFY_MINOR | TRACE_CMPLT));
  #endif

}

/****************************************************************************
 *                                                                          *
 *    DONE STATE                                                            *
 *                                                                          *
 *      Fixed state Done                                                    *
 *                                                                          *
 * If whole request is now complete, mark the request as done and then start*
 * the next one if there is one.                                            *
 *                                                                          *
 ****************************************************************************/
VOID NEAR DONEState(NPACB npACB)
{

  #ifdef DEBUG
  TraceIt(npACB, DONE_MINOR);
  #endif

  npACB->TimerCall = 0;

//mov  ax,0                         ; status = good (bad already traced)
//Call TraceIO                          ; trace the complete event
//  call DeQueue_QLink                    ; get Current Request (ES:BX)
//  mov di, si                            ; di -> IOStruc
//
// LOGENTRY                              ; *** Error Log and Alert ***

  npACB->RetryCounter = 0;              // Reset the retry counter
//npACB->LastRequest = 0;               // reset last request

//call DoneRequest                      ; unblock and redrive
//
// Note: DoneRequest will complete the request, reset any state machine
//       variables and check to see it there are more requests queued
//       up. If so, the next request is started and State = START, else
//       State = IDLE. For now, just set State to IDLE. See
//       subroutine DoneRequest for details.
//

  npACB->State = IDLE;                        // Next stat is IDLE
  npACB->DrvStateReturn = NEXTSTATENOW;

  IORBDone(npACB);

  #ifdef DEBUG
  TraceIt(npACB, (DONE_MINOR | TRACE_CMPLT));
  #endif

}

/****************************************************************************
 *                                                                          *
 *    IDLE STATE                                                            *
 *                                                                          *
 *Fixed state Idle                                                          *
 *                                                                          *
 *    Nothing happening, become inactive.                                   *
 *                                                                          *
 ****************************************************************************/
VOID NEAR IDLEState(NPACB npACB)
{

  #ifdef DEBUG
  TraceIt(npACB, IDLE_MINOR);
  #endif

  Disable_hardfile();
  npACB->DrvStateReturn = WAITSTATE;
  ENABLE                        // interrupts are cleared by donerequest

  #ifdef DEBUG
  TraceIt(npACB, (IDLE_MINOR | TRACE_CMPLT));
  #endif

}

/****************************************************************************
 *                                                                          *
 *    SERROR STATE                                                          *
 *                                                                          *
 *   Fixed state Error                                                      *
 *                                                                          *
 *       Entered when a non-recoverable error is detected.                  *
 *       A sense block has been requested and put into the                  *
 *       DCB.                                                               *
 *                                                                          *
 ****************************************************************************/
VOID NEAR SERRORState(NPACB npACB)
{
  PIORB_EXECUTEIO pIORB = (PIORB_EXECUTEIO)npACB->pHeadIORB;

  #ifdef DEBUG
  TraceIt(npACB, SERROR_MINOR);
  #endif

//      Trace I/O completion event      ; Failure, before retry
//      .IF  <Trace_Cmplt_f eq 1>       ; completion event needed
//         mov     ah,[si].DCB          ;   Fill in completion status
//         mov     dx,FX_PERR           ;   (DCB1 and Error status)
//      WHICHCTRL 2                     ;                               $100
//         in      al,dx
//         call    TraceIO
//      .ENDIF

  if ( (!(npACB->pHeadIORB->RequestControl & IORB_DISABLE_RETRY)) &&
       (npACB->IORegs[FI_PERR] & FX_READY) &&   // if Drive is Ready AND
       (npACB->ErrCnt < ErrLim)            &&   // have not exceeded retries AND
       (!(npACB->Flags & FVERIFY)) )            // No retries on Verify Op
                                                //   (includes write/verify)
    {
    npACB->unit[0].fDriveSetParam = 1;    // Set param for the next req
    npACB->unit[1].fDriveSetParam = 1;    // Set param for the next req

    ++npACB->ErrCnt;                      // We are doing another try

    ++npACB->RetryCounter;                // used for error logging

    if ( npACB->ErrCnt == ErrLim-1 )    //  Try a recal after 3 retries
      {
      npACB->State = RECAL;                     // Restart the request
      npACB->DrvStateReturn = WAITSTATE;        // Wait for an interrupt
      npACB->IORegs[FI_PCMD] = FX_RECAL;        // Command - recalibrate
      if ( SendCmdPkt(npACB) == SUCCESS ) // Send off command packet to controller
        SetTimeoutTimer(npACB);
      }
    else  // Retriable, reset current address and try it again
      {
      Do_Retry(npACB);
      }
    }
  else  // FixedFails
    {
    npACB->State = DONE;                        // Request is done.
    npACB->DrvStateReturn = NEXTSTATENOW;

    pIORB->iorbh.Status = IORB_ERROR;
    pIORB->iorbh.ErrorCode = MapHardError(npACB);
    pIORB->BlocksXferred = pIORB->BlockCount - npACB->SecToGo;
    }

  #ifdef DEBUG
  TraceIt(npACB, (SERROR_MINOR | TRACE_CMPLT));
  #endif

}

/****************************************************************************
 *                                                                          *
 *    RECAL STATE                                                           *
 *                                                                          *
 *  Fixed state Recal                                                       *
 *                                                                          *
 *  Get status of recal command and if no error retry previous command      *
 *  else goto handle error                                                  *
 *                                                                          *
 ****************************************************************************/
VOID NEAR RECALState(NPACB npACB)
{
  UCHAR         Status;

  #ifdef DEBUG
  TraceIt(npACB, RECAL_MINOR);
  #endif

  if ( ((Status=FDGetStat(npACB)) & FX_ERROR) )
    {
    npACB->State = SERROR;              // No interrupt expected if error
    npACB->DrvStateReturn = NEXTSTATENOW;
    npACB->IORegs[FI_PERR] = Status;            // Save status
    }
  else
    {
    npACB->State = START;                       // Next state is START
    npACB->DrvStateReturn = NEXTSTATENOW;
    }

  Do_Retry(npACB);

  #ifdef DEBUG
  TraceIt(npACB, (RECAL_MINOR | TRACE_CMPLT));
  #endif

}

/****************************************************************************
 *                                                                          *
 *    READ STATE                                                            *
 *                                                                          *
 *      Fixed state read                                                    *
 *                                                                          *
 *  Entered on interrupt from the controller when it has performed the read *
 *  operation and has 512 bytes to be read by the system.                   *
 *  We ensure that no error occurred in the previous operation.             *
 *  We read the 512 bytes of data from the port in any case, since the      *
 *  controller reads the sector even if there was an error.                 *
 *  If there was an error, or we are done, we go to the next state as no    *
 *  more interrupts will come our way.                                      *
 *  If we have more data to read, we return i.e. "wait" for an interrupt.   *
 *                                                                          *
 ****************************************************************************/
VOID NEAR READState(NPACB npACB)
{
  UCHAR         Status;
  PIORB_EXECUTEIO pIORB = (PIORB_EXECUTEIO)npACB->pHeadIORB;
  UCHAR i = pIORB->iorbh.UnitHandle;          // set unit number (0 or 1)

  #ifdef DEBUG
  TraceIt(npACB, READ_MINOR);
  #endif

  Status = FDGetStat(npACB);             // Get status of controller

// *** WARNING ***
// We are now going to read in the data from the data port. We do not want to
// be interrupted since we would lose data, so we disable interrupts.
// We know we are in protected mode so we don't anticipate a mode change
// and wait to enable interrupts until after we establish the next state.
// We need to be careful about enabling interrupts before ALL the fields in
// our data structures have been updated. If we get a spurious or a nested
// interrupt, we have to ensure that we will get to the correct state.

  PB_FxExRead(npACB);           // xfer physical record to SGList buffers

  if ( Status & FX_ERROR )
    {
    npACB->State = SERROR;
    npACB->DrvStateReturn = NEXTSTATENOW;
    npACB->IORegs[FI_PERR] = Status;            // Save status in DCB
    ENABLE                                      // OK to take interrupts now
    }
  else
    {
    npACB->XFlags |= FINTERRUPTSTATE;

    if ( npACB->unit[i].DriveFlags & FMULTIPLEMODE )
      {
      if ( --npACB->BlocksToGo )        // dec number of blocks left in request
        {
        pIORB->BlocksXferred += npACB->unit[i].id.NumSectorsPerInt;
        npACB->SecToGo       -= npACB->unit[i].id.NumSectorsPerInt;
        npACB->NumSectors    -= (UCHAR) npACB->unit[i].id.NumSectorsPerInt;
        }
      else
        {
        pIORB->BlocksXferred += npACB->LastBlockNumSec;
        npACB->SecToGo       -= npACB->LastBlockNumSec;
        npACB->NumSectors    -= (UCHAR) npACB->LastBlockNumSec;
        }
      }
    else // interrupt for every sector
      {
      ++pIORB->BlocksXferred;     // bump number of blocks xferred
      --npACB->SecToGo;           // dec number of sectors left in request
      --npACB->NumSectors;        // dec number of sectors left in multi io for request
      }

    if ( npACB->NumSectors )
      {
      ENABLE                            // OK to take interrupts now
      SetTimeoutTimer(npACB);
      }
    else
      {
      if ( npACB->SecToGo )     // if request is not finished
        {
        npACB->First += 256;                    // next RBA
        npACB->State = CALC;                    // Next stat is CALC
        npACB->DrvStateReturn = NEXTSTATENOW;
        ENABLE                                  // OK to take interrupts now
        }
      else
        {
        npACB->State = DONE;                    // Next state is DONE
        npACB->DrvStateReturn = NEXTSTATENOW;
        npACB->XFlags &= ~FINTERRUPTSTATE;      // Wrong, we're finished
        npACB->Flags &= ~FMULTIIO;              // reset mulitple i/o for request
        ENABLE                                  // OK to take interrupts now
        }
      }
    }

  #ifdef DEBUG
  TraceIt(npACB, (READ_MINOR | TRACE_CMPLT));
  #endif

}

/****************************************************************************
 *                                                                          *
 *    WRITE STATE                                                           *
 *                                                                          *
 *       Fixed state Write                                                  *
 *                                                                          *
 * Entered on interrupt from the controller after the first 512 bytes of the*
 * operation have been written out to the disk, and on subsequent Writes.   *
 * We ensure that no error occurred in the previous operation and if more   *
 * data needs to be written out.                                            *
 * If there is an error, or we are done, we go to the next state (Error,    *
 * Verify or Done) as no more interrupts will come our way.                 *
 * If more data needs to be written out, we write out the next 512 bytes to *
 * the data port, and return i.e. "wait" for an interrupt.                  *
 *                                                                          *
 ****************************************************************************/
VOID NEAR WRITEState(NPACB npACB)
{
  UCHAR         Status;
  PIORB_EXECUTEIO pIORB = (PIORB_EXECUTEIO)npACB->pHeadIORB;
  UCHAR i = pIORB->iorbh.UnitHandle;          // set unit number (0 or 1)

  #ifdef DEBUG
  TraceIt(npACB, WRITE_MINOR);
  #endif

  Status = FDGetStat(npACB);             // Get status of controller

  if ( Status & FX_ERROR )
    {
    npACB->State = SERROR;                      // next state is SERROR
    npACB->DrvStateReturn = NEXTSTATENOW;
    npACB->IORegs[FI_PERR] = Status;            // Save status in DCB
    }
  else
    {
// It is OK to write out the next 512 bytes to the controller
// Now adjust the appropriate counters and pointers from the previous WRITE

    if ( npACB->unit[i].DriveFlags & FMULTIPLEMODE )
      {
      if ( --npACB->BlocksToGo )        // dec number of blocks left in request
        {
        pIORB->BlocksXferred += npACB->unit[i].id.NumSectorsPerInt;
        npACB->SecToGo       -= npACB->unit[i].id.NumSectorsPerInt;
        npACB->NumSectors    -= (UCHAR) npACB->unit[i].id.NumSectorsPerInt;
        }
      else
        {
        pIORB->BlocksXferred += npACB->LastBlockNumSec;
        npACB->SecToGo       -= npACB->LastBlockNumSec;
        npACB->NumSectors    -= (UCHAR) npACB->LastBlockNumSec;
        }
      }
    else // interrupt for every sector
      {
      ++pIORB->BlocksXferred;     // bump number of blocks xferred
      --npACB->SecToGo;           // dec number of sectors left in request
      --npACB->NumSectors;        // dec number of sectors left in multi io for request
      }

    if ( npACB->NumSectors )            // More sectors to write out?
      {
      npACB->XFlags |= FINTERRUPTSTATE;
      if ( FDWrite(npACB) == SUCCESS )          // Write out next 512 bytes
        SetTimeoutTimer(npACB);                 // "Wait" for next interrupt
      }
    else  // check to see if the next state is Done or Verify
      {
      if ( npACB->Flags & FVERIFY )
        Do_Verify(npACB);
      else
        {
        if ( npACB->SecToGo )     // if request is not finished
          {
          npACB->First += 256;                  // next RBA
          npACB->State = CALC;                  // Next stat is CALC
          npACB->DrvStateReturn = NEXTSTATENOW;
          }
        else
          {
          npACB->State = DONE;                    // Next state is DONE
          npACB->DrvStateReturn = NEXTSTATENOW;
          npACB->Flags &= ~FMULTIIO;              // reset mulitple i/o for request
          }
        }
      }
    }

  #ifdef DEBUG
  TraceIt(npACB, (WRITE_MINOR | TRACE_CMPLT));
  #endif

}

/****************************************************************************
 *                                                                          *
 *    SETPARAM STATE                                                        *
 *                                                                          *
 *      Fixed state SetParam                                                *
 *                                                                          *
 *      Get the status byte off the status register and examine it.         *
 *      If it is Error, then next state is Error                            *
 *      Else next state is Calc                                             *
 *                                                                          *
 ****************************************************************************/
VOID NEAR SETPARAMState(NPACB npACB)
{

  #ifdef DEBUG
  TraceIt(npACB, SETPARAM_MINOR);
  #endif

  if ( (FDGetStat(npACB) & FX_ERROR) )
    {
    npACB->State = SERROR;              // No interrupt expected if error
    npACB->DrvStateReturn = NEXTSTATENOW;
    npACB->IORegs[FI_PERR] = FX_ERROR;          // Save status
    }
  else
    {
    npACB->State = CALC;                        // Next stat is CALC
    npACB->DrvStateReturn = NEXTSTATENOW;
    }

// Note - may want to add this in
//
//       Trace I/O if our event is active
//        si -> IOStruc
//
//        Public  TraceIt
//TraceIt:


  #ifdef DEBUG
  TraceIt(npACB, (SETPARAM_MINOR | TRACE_CMPLT));
  #endif

}

/****************************************************************************
 *                                                                          *
 *    Do_Verify                                                             *
 *                                                                          *
 * Verify is like a Read except that no data is returned. An interrupt is   *
 * issued when the operation is complete or if there is an error.           *
 *                                                                          *
 ****************************************************************************/
VOID NEAR Do_Verify(NPACB npACB)
{
  npACB->State = VERIFY;                      // Next state is VERIFY
  npACB->DrvStateReturn = WAITSTATE;          // Wait for an interrupt
  npACB->IORegs[FI_PCMD] = (FX_VERIFY | FX_NORETRY);
  if ( SendCmdPkt(npACB) == SUCCESS ) // Send off command packet to controller
    SetTimeoutTimer(npACB);
}

/****************************************************************************
 *                                                                          *
 *    Do_Retry                                                              *
 *                                                                          *
 * Retriable, reset current address and try it again                        *
 *                                                                          *
 ****************************************************************************/
VOID NEAR Do_Retry(NPACB npACB)
{

  #ifdef DEBUG
  TraceIt(npACB, RETRY_MINOR);
  #endif

  npACB->CurrAddr = npACB->RealAddr;    // everything else stays ok from
                                        // orgininal setup.

// since HPFS386 requeust aren't cached, we can go back to start state to
// retry request. status is updated with recoverable error until retry
// limit is reached.

  npACB->pHeadIORB->Status   |= IORB_ERROR + IORB_RECOV_ERROR;

  npACB->State = START;                       // Next state is START
  npACB->DrvStateReturn = NEXTSTATENOW;

  #ifdef DEBUG
  TraceIt(npACB, (RETRY_MINOR | TRACE_CMPLT));
  #endif

}

