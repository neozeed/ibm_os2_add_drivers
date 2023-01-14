/*static char *SCCSID = "@(#)s506timr.c	6.1 92/01/08";*/

/****************************************************************************
 *                                                                          *
 *                (c) Copyright  IBM Corporation  1991                      *
 *                           All Rights Reserved                            *
 *                                                                          *
 ****************************************************************************/

/********************* Start of Specifications ******************************
 *                                                                          *
 *  Source File Name: S506TIMR.C                                            *
 *                                                                          *
 *  Descriptive Name: AT Hard Disk Timer Routines                           *
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

VOID NEAR DDDTimer()
{
  NPACB         npACB;

  if (TimerActive & FIXEDT1)
    {
    npACB = &acb;               // get address of adapter control block
    FTimer(npACB);              // call Fixed state machine
    }

  _asm
    {
       leave
       retf
    }
}


/****************************************************************************
 *                                                                          *
 *    FTimer                                                                *
 *                                                                          *
 ****************************************************************************/
VOID NEAR FTimer(NPACB npACB)
{
  DISABLE
  if (TimerActive & FIXEDT1)
    {
    if ( --npACB->TOCnt == (ULONG) 0 )
      {
        npACB->XFlags |= FTIMERINT;     // discard all interrupts until later on
        TimerActive &= ~FIXEDT1;        // do not accept anymore timer 1 ints
        ENABLE                          // enable interrupts
        --npACB->TimerCall;             // occurred before interrupt!!!
                                        // active drive had a time out so
        npACB->IORegs[FI_PERR] = FDGetStat(npACB); // get status & save
        npACB->State = SERROR;          //   it's an error

        #ifdef DEBUG
        TraceIt(npACB, TIMEOUT_MINOR);
        #endif

        FixedExecute(npACB);            //   call state machine

        #ifdef DEBUG
        TraceIt(npACB, (TIMEOUT_MINOR | TRACE_CMPLT));
        #endif

        npACB->XFlags &= ~FTIMERINT;
      } /*endif*/
    } /*endif*/
  ENABLE                                 // enable interrupts
}

/****************************************************************************
 *                                                                          *
 *  SetTimeoutTimer   - Set timeout timer in case interrupt does not        *
 *                        occur.                                            *
 *                                                                          *
 ****************************************************************************/
VOID NEAR SetTimeoutTimer(NPACB npACB)
{
  PIORBH pIORB;

  DISABLE
  pIORB = npACB->pHeadIORB;

  if (npACB->XFlags & FINTERRUPTSTATE)        // pending interrupt
    {
//  Check for disk interrupt happening before setting the timer.
//  if TimerCall > 0, then interrupt has not occurred - set the timer

    if ((++npACB->TimerCall) > 0)
      {
      if (pIORB->Timeout != -1)       // if not an infinite timeout
        {
        if (pIORB->Timeout == 0)      // use default timeout value?
          npACB->TOCnt = (ULONG) 60;  // (60 x 32 x 31ms) = 60.0s
        else
          npACB->TOCnt = pIORB->Timeout;        // use iorb timeout value

        TimerActive |= npACB->TimerMask;        // enable timer

        #ifdef DEBUG
        TraceIt(npACB, SETTIMEOUT_MINOR);
        #endif
        }
      }
    }
  else
    if (npACB->NestedInts > 0)
      ++npACB->TimerCall;
  ENABLE
}
