/*static char *SCCSID = "@(#)fl2intr.c	6.4 92/01/30";*/
/*****************************************************************************/
/*                                                                           */
/*   Component   : IBM PS/2 Floppy Adapter Device Driver  (IBM2FLPY)         */
/*                                                                           */
/*   Module      : FL2INTR.C                                                 */
/*                                                                           */
/*   Description : Routines for implementing multi-staged ABIOS requests     */
/*                 including NextStage, the interrupt handler and timer      */
/*                 routines.                                                 */
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
/*   Routine     : NextStage                                                 */
/*                                                                           */
/*   Description : This is the inner most routine of the device driver.      */
/*                 This routine executes multi-staged ABIOS requests.        */
/*                 The caller of this routine should have setup the          */
/*                 ABIOS request block, put the address of a completion      */
/*                 routine in the CompletionRoutine variable and set         */
/*                 the retry variable to zero.  When the final stage         */
/*                 completes then the completion routine is called.  If      */
/*                 a request completes unsuccessfully then the request       */
/*                 is retryed until it completes successfully or until       */
/*                 the maximum number of retrys have been attempted.         */
/*                                                                           */
/*                 During the course of executing a multi-staged request,    */
/*                 this routine may be called from the interrupt handler,    */
/*                 the timer handler, or from itself (NextStage) to          */
/*                 proceed to the next stage.                                */
/*                                                                           */
/*                 This routine returns TRUE or FALSE to the interrupt       */
/*                 handler, indicating if the interrupt orginated from       */
/*                 the floppy controller.                                    */
/*                                                                           */
/*                 On some older computers, the video controller             */
/*                 sometimes interferred with the floppy controller          */
/*                 causing DMA overrun errors.  If a floppy operation        */
/*                 (write) fails with a DMA overrun error, then this         */
/*                 routine will pause the video and retry the operation.     */
/*                                                                           */
/*****************************************************************************/

BOOL FAR NextStage()
{
   NPABRB_GENERIC pRB = (NPABRB_GENERIC)RequestBlock;
   BOOL ItsMyInt = TRUE;

   DisableInts

   /* Call ABIOS at the "Stage" entry point */
   DevHelp_ABIOSCall( LID, (NPBYTE)pRB, Stage );

   /* Examine the ABIOS return code and take appropriate action */

   if ( pRB->abrbh.RC == ABRC_STAGEONINTERRUPT )
      {
         /* Start fail-safe timer in case interrupt never comes */
         Stage = ABIOS_EP_TIMEOUT;
         StartTimer( pRB->abrbh.Unit, ((pRB->abrbh.Timeout)>>3)*1000, (PPOSTFN)NextStage );
         GFlags.StageOnInt = TRUE;
      }
   else if ( pRB->abrbh.RC == ABRC_STAGEONTIME )
      {
         /* Start timer for the specified duration */
         Stage = ABIOS_EP_INTERRUPT;
         StartTimer( pRB->abrbh.Unit, (USHORT)((pRB->WaitTime)/1000), (PPOSTFN)NextStage );
      }
   else if ( pRB->abrbh.RC == ABRC_NOTMYINTERRUPT )
      {
         ItsMyInt = FALSE;
      }
   else if ( pRB->abrbh.RC & ABRC_ERRORBIT )
      {
         EnableInts

         /* If not doing a reset then set the error bit */
         if ( !GFlags.Resetting ) pHeadIORB->Status |= IORB_ERROR;

         /* If this is a retryable error and retry is not disabled */
         /* and there are more retrys to go, then retry            */
         if ( ( pRB->abrbh.RC & ABRC_RETRYBIT                     ) &&
              ( GFlags.Resetting ||
                !(pHeadIORB->RequestControl & IORB_DISABLE_RETRY) ) &&
              ( Retry++ < Drive[pRB->abrbh.Unit].RetryCount )       )
            {
               /* If DMA overrun error then pause the video */
               if ( (pRB->abrbh.RC == ABRC_DSKT_DMA_OVERRUN) &&
                     !GFlags.VideoPauseOn )
                  {
                     DevHelp_VideoPause( VIDEO_PAUSE_ON );
                     GFlags.VideoPauseOn = TRUE;
                  }

               /* If read, write or verify then reload sector count */
               switch ( pRB->abrbh.Function )
                  {
                     case ABFC_DSKT_READ:
                     case ABFC_DSKT_WRITE:
                     case ABFC_DSKT_VERIFY:
                        pRB->cSectors = SectorCnt;
                        break;
                  }

               pRB->abrbh.RC = ABRC_START;
               Stage         = ABIOS_EP_START;
               NextStage();          /* Recursive !! */
            }
         else  /* Can not retry  */
            {
               /* This is unsuccessful completion.                 */
               /* We certainly have not recovered if we are here.  */
               if ( !GFlags.Resetting )
                  pHeadIORB->Status &= ~IORB_RECOV_ERROR;

               /* If video pause is on then turn it off */
               if ( GFlags.VideoPauseOn )
                  {
                     DevHelp_VideoPause( VIDEO_PAUSE_OFF );
                     GFlags.VideoPauseOn = FALSE;
                  }

               CompletionRoutine();  /* Unsuccessful completion */
            }
      }
   else if ( pRB->abrbh.RC == ABRC_COMPLETEOK )
      {
         EnableInts

         /* This is successful completion.                   */
         /* If we had an error, then we must have recovered. */
         if ( (!GFlags.Resetting) && (pHeadIORB->Status & IORB_ERROR) )
            pHeadIORB->Status |= IORB_RECOV_ERROR;

         /* If video pause is on then turn it off */
         if ( GFlags.VideoPauseOn )
            {
               DevHelp_VideoPause( VIDEO_PAUSE_OFF );
               GFlags.VideoPauseOn = FALSE;
            }

         CompletionRoutine();  /* Successful completion */
      }

   EnableInts

   return ItsMyInt;     /* Used by interrupt handler */
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : IntHandler                                                */
/*                                                                           */
/*   Description : This routine is called when an interrupt occurs.          */
/*                 This routine cancels the fail-safe timer and calls        */
/*                 NextStage to proceed on to the next stage.                */
/*                                                                           */
/*****************************************************************************/

VOID NEAR IntHandler()
{
   NPABRBH pRB = (NPABRBH)RequestBlock;

   if ( GFlags.StageOnInt )  /* If expecting an interrupt */
      {
         GFlags.StageOnInt = FALSE;

         /* Cancel the fail-safe timer */
         CancelTimer( pRB->Unit );

         Stage = ABIOS_EP_INTERRUPT;

         if ( NextStage() )  /* Interrupt came from diskette controller */
            {
               DevHelp_EOI( IntLevel );
               _asm clc
            }
         else _asm stc   /* Interrupt didn't come from diskette controller */
      }
   else _asm stc  /* Interrupt belongs to someone else */

   _asm
      {
         leave
         retf
      }
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : StartTimer                                                */
/*                                                                           */
/*   Description : This routine starts the specified timer.  Each floppy     */
/*                 unit has its own timer, plus there is a timer for         */
/*                 unlocking the swapable code segment.  Unit 0 uses         */
/*                 timer number 0; Unit 1 uses timer number 1 and so         */
/*                 forth.  The last timer is the unlock timer.               */
/*                                                                           */
/*                 This routine converts the duration (in milliseconds)      */
/*                 to timer ticks.                                           */
/*                                                                           */
/*                 The address of a post routine is specified when           */
/*                 starting a timer.  Upon timeout the post routine          */
/*                 is called and is passed the number of the timer           */
/*                 which has expired.                                        */
/*                                                                           */
/*****************************************************************************/

VOID FAR StartTimer( USHORT TimerNum, USHORT Duration, PPOSTFN PostRoutine )
{
   /* Duration is in milliseconds */

   if ( Duration == 0 )
      Timer[TimerNum].TicksToGo = 0;
   else
      Timer[TimerNum].TicksToGo = ( Duration / MSPerTick ) + 2;

   /* Why add two?  One is to round up to the nearest whole timer tick. */
   /* The other one is to disregard the partial timer tick that is      */
   /* currently going by.  These guarantee that, at a minimum, the      */
   /* requested duration will elapse before timing out.                 */

   Timer[TimerNum].NotifyRoutine = PostRoutine;
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : CancelTimer                                               */
/*                                                                           */
/*   Description : Cancel the specified timer by setting it to zero.         */
/*                                                                           */
/*****************************************************************************/

VOID NEAR CancelTimer( USHORT TimerNum )
{
   Timer[TimerNum].TicksToGo = 0;
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : TimerHandler                                              */
/*                                                                           */
/*   Description : This routine is called on each timer tick (every 33       */
/*                 milliseconds).  This routine decrements each non-zero     */
/*                 timer.  If a timer is decremented to zero (timeout),      */
/*                 then the corresponding post routine is called.  The       */
/*                 number of the expired timer is passed to the post         */
/*                 routine.                                                  */
/*                                                                           */
/*****************************************************************************/

VOID NEAR TimerHandler()
{
   USHORT x;

   for ( x=0; x<TIMERCNT; x++ )
      {
         if ( Timer[x].TicksToGo > 0 )
            {
               if ( --Timer[x].TicksToGo == 0 )
                  Timer[x].NotifyRoutine(x);
            }
      }

   _asm
      {
         leave
         retf
      }
}


