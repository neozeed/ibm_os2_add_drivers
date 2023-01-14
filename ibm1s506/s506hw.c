/*static char *SCCSID = "@(#)s506hw.c	6.3 92/01/29";*/

/****************************************************************************
 *                                                                          *
 *                (c) Copyright  IBM Corporation  1991                      *
 *                           All Rights Reserved                            *
 *                                                                          *
 ****************************************************************************/

/********************* Start of Specifications ******************************
 *                                                                          *
 *  Source File Name: S506HW.C                                              *
 *                                                                          *
 *  Descriptive Name: Routines that interface to hard disk controller       *
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
 *   MapHardError - Map error code to an IORB error code                    *
 *                                                                          *
 *  MapHardError maps the error code returned by the fixed disk controller  *
 *  onto an IORB error code for return to the DOS.                          *
 *                                                                          *
 *  ENTRY   al - Byte from the controller Error Register                    *
 *                                                                          *
 *  EXIT    bl - DOS error code                                             *
 *                                                                          *
 *  ASSUMPTION - An error HAS occurred. The mapping is done via a look-up   *
 *               table.                                                     *
 *                                                                          *
 *  MODIFIED - ax,bx                                                        *
 ****************************************************************************/
USHORT NEAR MapHardError(NPACB npACB)
{
  USHORT        Port;
  UCHAR         Status;
  UCHAR         Error;
  USHORT        IORBErrorCode;

  Port = npACB->IOPorts[FI_PERR];       // Get error code from error port
  inp(Port, Error);

  Status = npACB->IORegs[FI_PERR];      // Get status byte that was saved earlier

  if ( !(Status & FX_READY) )
    IORBErrorCode = IOERR_UNIT_NOT_READY;
  else if ( Status & FX_WRTFLT )
    IORBErrorCode = IOERR_ADAPTER_REFER_TO_STATUS;
  else if ( Error & FX_BADBLK )
    IORBErrorCode = IOERR_RBA_ADDRESSING_ERROR;
  else if ( Error & FX_ECCERROR )
    IORBErrorCode = IOERR_RBA_CRC_ERROR;
  else if ( Error & FX_IDNF )
    IORBErrorCode = IOERR_RBA_ADDRESSING_ERROR;
  else if ( Error & FX_ABORT )
    IORBErrorCode = IOERR_ADAPTER_REFER_TO_STATUS;
  else if ( Error & FX_TRK0 )
    IORBErrorCode = IOERR_RBA_ADDRESSING_ERROR;
  else if ( Error & FX_AMNF )
    IORBErrorCode = IOERR_RBA_ADDRESSING_ERROR;
  else if ( TRUE )  // default case is adapter timeout
    IORBErrorCode = IOERR_ADAPTER_TIMEOUT;

  return(IORBErrorCode);
}


/****************************************************************************
 *                                                                          *
 *      FDGetStat - Get the status of the fixed disk controller             *
 *                                                                          *
 ****************************************************************************/
UCHAR NEAR FDGetStat(NPACB npACB)
{
  USHORT        Port;
  UCHAR         Status;

  Port = npACB->IOPorts[FI_PSTAT];
  inp(Port, Status);
  if ( Status & 0x0020 )                // if write fault then set error
    Status |= 0x0041;                   // and retry bits
  return(Status);
}


/****************************************************************************
 *                                                                          *
 * FDWrite - Write out 512 bytes to controller                              *
 *                                                                          *
 *     FDWrite - Write out 512 bytes out to the fixed disk controller.      *
 *                                                                          *
 *     ENTRY   Fixed set up for performing the write to the drive.          *
 *             ds = BiosGroup                                               *
 *                                                                          *
 *     EXIT    Data read in at location in [si].CurrAddr.                   *
 *             ALL registers preserved.                                     *
 *                                                                          *
 *     ALGORITHM                                                            *
 *             Wait for the controller to issue a 'Data Request' (DRQ)      *
 *             before sending off the data.                                 *
 *             Move the pointer to the data to the next 512 bytes.          *
 *                                                                          *
 *     Returns   FAILURE if timeout.                                        *
 *               SUCCESS if controller ready and data sent                  *
 *                                                                          *
 ****************************************************************************/
BOOL NEAR FDWrite(NPACB npACB)
{
  USHORT        Port;
  UCHAR         Status;
  USHORT        i;
  BOOL          rc;
  USHORT        DataOff;
  USHORT        DataSel;

  Port = npACB->IOPorts[FI_PSTAT];
  for (i=0; i < 0xffff; i++)
    {
    inp(Port, Status);                  // poll controller
    if ( (Status & FX_DRQ) )
      break;                            // controller send gave OK
    }

  if ( !(Status & FX_DRQ) )
    {
//  We never received the DRQ.
    npACB->State = SERROR;
    npACB->DrvStateReturn = NEXTSTATENOW;
    npACB->IORegs[FI_PERR] = FX_ERROR;           // Set status to error
    rc = FAILURE;
    }
  else
    {

//  We are now going to write out the data to the data port. We do not want to
//  be interrupted since we would lose data, so we disable interrupts and
//  re-enable them when we are done.

    PB_FDWrite(npACB);

    rc = SUCCESS;

    }
}


/****************************************************************************
 *                                                                          *
 * FDParam - Set up fixed disk controller for I/O                           *
 *                                                                          *
 *   FDParam - Set up the controller for the I/O                            *
 *                                                                          *
 *   Entry     DS:DI point to drive parameters                              *
 *             DS:SI pointer to IOStruc                                     *
 *             DS = BiosGroup                                               *
 *                                                                          *
 *   Exit      Fixed disk controller knows what type of drive is in system. *
 *             ALL registers preserved.                                     *
 *   Algorithm                                                              *
 *             FDParam initially sets up the controller for the I/O. It     *
 *             checks the status and then sends it a packet telling it what *
 *             type of device it is talking to.                             *
 *                                                                          *
 ****************************************************************************/
BOOL NEAR FDParam(NPACB npACB)
{
  USHORT        Port;
  UCHAR         Data;
  UCHAR         NumHeads;
  UCHAR         i;
  BOOL          rc;
  NPGEOMETRY    npGeometry;

  i = npACB->drivebit;                  // set unit number (0 or 1)
  rc = SUCCESS;
  Port = npACB->IOPorts[FI_RFDR];
  Data = 0x0008;

  if ( npACB->unit[i].XlateFlags & XLATE_ENABLE )       // if in xlate mode
    npGeometry = &(npACB->unit[i].PhysGeometry);   // use physical geometry
  else
    npGeometry = &(npACB->unit[i].LogGeometry);    // else use logical geometry

  if ( npGeometry->NumHeads < 8 )
    Data = 0;

  outp(Port, Data);             // Tell controller number of heads we have

  Data = i;             // set Data to unit number (0 or 1)
  Data = (Data << 4);
  Data |= 0x00a0;

  NumHeads = (UCHAR) npGeometry->NumHeads;
  --NumHeads;                                   // Head must be 0-baseD
  Data |= NumHeads;
  npACB->IORegs[FI_PDRHD] = Data;       // Drive and Head

  Port = npACB->IOPorts[FI_PDRHD];
  outp(Port, Data);                     // Select the drive

  if ( (FDGetStat(npACB) & FX_ERROR) ||
       (npACB->unit[i].fDriveSetParam) )
    {
//  Set up command packet to be sent to disk controller

    npACB->unit[i].fDriveSetParam = 0;          // No set param for the next req
    npACB->IORegs[FI_PWRP] = (npACB->unit[i].WrtPrCmp)/4;  // Write precompensation
    npACB->IORegs[FI_PSECCNT] = npGeometry->SectorsPerTrack;
    npACB->IORegs[FI_PSECNUM] = 0;          // Sector Zero
    npACB->IORegs[FI_PCYLL] = 0;            // Cylinder (low) - zero
    npACB->IORegs[FI_PCYLH] = 0;            // Cylinder (high) - zero
    npACB->IORegs[FI_PCMD] = FX_SETP;       // Set parameters command
    rc = SendCmdPkt(npACB);                 // Send off command to controller
    }
  else
    npACB->DrvStateReturn = NEXTSTATENOW;   // not expecting an interrupt

  return( rc );
}


/****************************************************************************
 *                                                                          *
 *  FDCommand - Send off command to fixed disk controller.                  *
 *                                                                          *
 *     FDCommand will send the previously set up command packet to the fixed*
 *     disk controller. It first ensures that the controller is ready to    *
 *     accept the data.                                                     *
 *                                                                          *
 *     Entry     DCB fields in Fixed set up for sending off command.        *
 *                                                                          *
 *     Returns   FAILURE if timeout.                                        *
 *               SUCCESS if command issued                                  *
 *                                                                          *
 ****************************************************************************/
BOOL NEAR FDCommand(NPACB npACB)
{
  USHORT        Port;
  UCHAR         Data;
  UCHAR         i;
  BOOL          rc;


//if ( npACB->IORegs[FI_PCMD] == FX_CWRITE )
//  {
//    PIORB_EXECUTEIO pIORB = (PIORB_EXECUTEIO)npACB->pHeadIORB;
//    if ( pIORB->RBA < SafeRBA )
//      {
//      INT3
//      }
//  }

  Port = npACB->IOPorts[FI_PDRHD];
  Data = npACB->IORegs[FI_PDRHD];
  outp(Port, Data);                     // select the drive

  if ( (rc=WaitRdy(npACB)) == SUCCESS )
    {
    for (i=FI_PWRP; i<FI_PCMD; i++)    // 6 bytes to be written out
      {
      Port = npACB->IOPorts[i];
      Data = npACB->IORegs[i];
      outp(Port, Data);
      IOWait;
      }
    if ( (rc=WaitRdy(npACB)) == SUCCESS )
      {
      npACB->XFlags |= FINTERRUPTSTATE;  // expect an interrupt
      npACB->XFlags &= ~FTIMERINT;       // let interrupt come in if restarting
                                         // command from the timeout routine
      Port = npACB->IOPorts[FI_PCMD];
      Data = npACB->IORegs[FI_PCMD];
      outp(Port, Data);             // enable controller with last command
      }
    }
  return( rc );
}


/****************************************************************************
 *                                                                          *
 *     WaitRdy - Wait for fixed disk controller to become ready.            *
 *                                                                          *
 *     WaitRdy waits for the drive controller to become ready. It examines  *
 *     the high bit that is read from the status register.                  *
 *                                                                          *
 *     Entry     NONE                                                       *
 *                                                                          *
 *     Returns   FAILURE if timeout.                                        *
 *               SUCCESS if controller ready.                               *
 *                                                                          *
 *                    if ( WaitRdy() == SUCCESS )                           *
 *                                                                          *
 ****************************************************************************/
BOOL NEAR WaitRdy(NPACB npACB)
{
  USHORT        Port;
  UCHAR         Status;
  ULONG         i;

  Port = npACB->IOPorts[FI_PSTAT];

  for (i=0; i<MAXWAIT; i++)
    {
    inp(Port, Status);
    if ( !(Status & FX_BUSY) )
      {
      if ( Status & FX_READY )
        return SUCCESS;
      else
        return FAILURE;
      }
    }

  return FAILURE;
}


/****************************************************************************
 *                                                                          *
 *     SendCmdPkt - Send off command packet to controller                   *
 *                                                                          *
 *     SendCmdPkt waits until the drive controller is ready and sends off   *
 *     the command packet to the controller.                                *
 *                                                                          *
 *     Entry     NONE                                                       *
 *                                                                          *
 *     Returns   FAILURE if timeout.                                        *
 *               SUCCESS if command issued.                                 *
 *                                                                          *
 *                                                                          *
 ****************************************************************************/
BOOL NEAR SendCmdPkt(NPACB npACB)
{
  BOOL          rc;
  if ( (rc=WaitRdy(npACB)) == SUCCESS )
    rc=FDCommand(npACB);                // Send off command packet to controller
  if ( rc == FAILURE )
    {
//  We got an error from FDCommand - Assume it was a 'Not Ready' error
    npACB->State = SERROR;
    npACB->DrvStateReturn = NEXTSTATENOW;
    npACB->IORegs[FI_PERR] = FX_ERROR;           // Set status to error
    }
  return( rc );
}


/*----------------------------------------------------------------------------*
 * Initialize_hardfile                                                        *
 *                                                                            *
 *----------------------------------------------------------------------------*/
 VOID NEAR Initialize_hardfile()
 {
 _asm
    {
                 ; Hardfile is left disabled, this should fix it!
      cli
      mov  dx, 94h
      mov  al, 20H                            ;System setup
      out  dx, al
      mov  dx, 102h
      in   al, dx
      or   al, 01H                            ;Enable system board
      jmp  next
next:
      mov  dx, 102h
      out  dx, al
      mov  dx, 103h
      in   al, dx
      and  al, 0ddH
      or   al,  08h
      mov  dx, 103h
      out  dx,  al
      mov   dx, 94h
      mov   al, 0a0H
      out   dx,  al
      sti
    }
 return;
 }


/****************************************************************************
 *                                                                          *
 * Enable_Hardfile - make hardfile alert to system requests                 *
 *                                                                          *
 * ENTRY                                                                    *
 *                                                                          *
 * EXIT                                                                     *
 *                                                                          *
 * USES  flags                                                              *
 *                                                                          *
 ****************************************************************************/
VOID NEAR Enable_hardfile()
{
  _asm
    {
    cli
    push dx
    push ax
    mov  dx, 92h
    in   al, dx
    or   al, 0c0H
    out  dx, al
    pop  ax
    pop  dx
    sti
    }
}


/****************************************************************************
 *                                                                          *
 * Disable_hardfile - make hardfile ignore system requests                  *
 *                                                                          *
 * ENTRY                                                                    *
 *                                                                          *
 * EXIT                                                                     *
 *                                                                          *
 * USES  flags                                                              *
 *                                                                          *
 ****************************************************************************/
VOID NEAR Disable_hardfile()
{
  _asm
    {
    cli
    push dx
    push ax
    mov  dx, 92h
    in   al, dx
    and  al, not 0c0H
    out  dx, al
    pop  ax
    pop  dx
    }
}
