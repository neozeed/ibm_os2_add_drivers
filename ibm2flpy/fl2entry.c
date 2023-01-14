/*static char *SCCSID = "@(#)fl2entry.c	6.7 92/01/30";*/
/*****************************************************************************/
/*                                                                           */
/*   Component   : IBM PS/2 Floppy Adapter Device Driver  (IBM2FLPY)         */
/*                                                                           */
/*   Module      : FL2ENTRY.C                                                */
/*                                                                           */
/*   Description : This module contains the main entry points to the ADD.    */
/*                 These are the Strategy entry point and the ADD entry      */
/*                 point.                                                    */
/*                                                                           */
/*****************************************************************************/

#define INCL_NOBASEAPI
#define INCL_NOPMAPI
#include <os2.h>
#include <dhcalls.h>
#include <devcmd.h>
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
/*   Flag Notes :                                                            */
/*                                                                           */
/*      ShutdnPending : This flag indicates that a shutdown is in            */
/*                 progress.  The ADD could be:                              */
/*                      1. Waiting for the shutdown timer to expire          */
/*                      2. Waiting for the armed context hook to fire        */
/*                      3. Unlocking the swapable code                       */
/*                      4. Freeing the DMA buffer                            */
/*                      5. Unhooking the IRQ (if suspending)                 */
/*                 Clearing this flag will cause the ADD to abort the        */
/*                 shutdown sequence.                                        */
/*                                                                           */
/*      CtxHookArmed : This flag indicates that the context hook has         */
/*                 been armed and has not yet fired.  This flag is           */
/*                 used to prevent the context hook from being armed         */
/*                 a second time.                                            */
/*                                                                           */
/*      ProcDisabled : If this flag is set then IORB processing is           */
/*                 disabled.  This flag will not allow the processing        */
/*                 of a new IORB to begin.  However, an IORB currently       */
/*                 in progress when this flag is set will continue to        */
/*                 completion.  If this flag is set, then this means         */
/*                 that the ADD is suspended or is awaiting immediate        */
/*                 suspension.                                               */
/*                                                                           */
/*****************************************************************************/


/*****************************************************************************/
/*                                                                           */
/*   Routine     : ADDEntryPoint                                             */
/*                                                                           */
/*   Description : This routine is the entry point by which device           */
/*                 managers and filter adds pass in IORBs.  The              */
/*                 responsibiity of this routine is to place incomming       */
/*                 IORBs on the IORB list.  If the ADD is idle then          */
/*                 the hook handler is called to start IORB processing.      */
/*                                                                           */
/*****************************************************************************/

VOID _loadds FAR ADDEntryPoint( PIORBH pNewIORB )
{
   PIORBH pIORB;

   DisableInts

   if ( pNewIORB->CommandCode == IOCC_DEVICE_CONTROL )
      {
         SuspendResume( pNewIORB );
      }
   else   /* IORB is received which is not a Suspend or Resume */
      {
         if ( pHeadIORB == NULL )    /* If IORB list is empty */
            {
               pHeadIORB = pNewIORB; /* Put new IORB at the head of the list */

               if ( !GFlags.ProcDisabled ) /* If processing is not disabled */
                  {
                     EnableInts
                     /* Call the hook handler to start up IORB processing */
                     _asm mov ax, STARTUP
                     HookHandler();
                  }
            }
         else  /* IORB list is not empty */
            {
               /* Add new IORB at end of list */
               for ( pIORB=pHeadIORB; (pIORB->RequestControl&IORB_CHAIN);
                      pIORB=pIORB->pNxtIORB );
               pIORB->pNxtIORB = pNewIORB;
               pIORB->RequestControl |= IORB_CHAIN;
            }
      }

   EnableInts
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : IORBDone                                                  */
/*                                                                           */
/*   Description : This routine is called whenever an IORB has completed.    */
/*                 This routine will remove the completed IORB from the      */
/*                 list of IORBs and notify the device manager if            */
/*                 requested.  If the IORB list is not empty then the        */
/*                 next IORB is started.                                     */
/*                                                                           */
/*****************************************************************************/

VOID FAR IORBDone()
{
   PIORBH pDoneIORB = pHeadIORB;

   pDoneIORB->Status |= IORB_DONE;   /* Set the done bit */

   DisableInts

   if ( pDoneIORB->RequestControl & IORB_CHAIN ) /* If there is another IORB */
      {
         pHeadIORB = pDoneIORB->pNxtIORB; /* Remove the done IORB from list */
         EnableInts

         /* If asynchronous post is requested then do it */
         if ( pDoneIORB->RequestControl & IORB_ASYNC_POST )
            pDoneIORB->NotifyAddress( pDoneIORB );

         if ( GFlags.ProcDisabled )     /* If processing is disabled  */
            {
               /* Note: This path is taken on immediate suspend */
               GFlags.ShutdnPending = TRUE;
               StartTimer( SHUTDNTIMER, SHUTDNDELAY, (PPOSTFN)ShutDown );
            }
         else NextIORB();               /* Start processing the next IORB */
      }
   else  /* There are no more IORBs to do */
      {
         pHeadIORB = NULL;                /* List is now empty */

         /* Note: If startup failed then IORBDone is called when not locked */
         if ( LockHandle != NULL )
            {
               GFlags.ShutdnPending = TRUE;
               StartTimer( SHUTDNTIMER, SHUTDNDELAY, (PPOSTFN)ShutDown );
            }

         EnableInts

         /* If asynchronous post is requested then do it */
         if ( pDoneIORB->RequestControl & IORB_ASYNC_POST )
            pDoneIORB->NotifyAddress( pDoneIORB );
      }
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : ShutDown                                                  */
/*                                                                           */
/*   Description : This routine is called by the timer handler when the      */
/*                 shutdown timer is expired.  This routine simply           */
/*                 arms the context hook so that the hook handler can        */
/*                 shut down at task time (not interrupt time).              */
/*                                                                           */
/*****************************************************************************/

VOID FAR ShutDown()
{
   if ( !GFlags.CtxHookArmed )
      {
         GFlags.CtxHookArmed = TRUE;
         DevHelp_ArmCtxHook( SHUTDOWN, HookHandle );
      }
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : HookHandler                                               */
/*                                                                           */
/*   Description : This routine has the responsability of shutting down      */
/*                 and starting up the ADD.  Starting up the ADD involves    */
/*                 allocating a DMA buffer, locking the swapable code and    */
/*                 starting the processing of IORBs.  Shutting down the      */
/*                 ADD involves unlocking the swapable code and              */
/*                 deallocting the DMA buffer.  If the ADD is being          */
/*                 suspended, then shutting down also involves unhooking     */
/*                 the interrupt level.                                      */
/*                                                                           */
/*                 This routine is normally called by arming a context       */
/*                 hook.  This is done because locking and unlocking         */
/*                 memory can not be done at interrupt time.                 */
/*                                                                           */
/*   Input:        AX = Action    STARTUP or SHUTDOWN                        */
/*                                                                           */
/*****************************************************************************/

VOID FAR HookHandler()
{
   USHORT Action;
   PFN pFunction;
   USHORT AwakeCount;

   _asm  mov  Action, ax

   GFlags.CtxHookArmed = FALSE;

   if ( Action == STARTUP )
      {
         if ( GFlags.ProcDisabled ) return;

         /* If shutdown is in progress the abort the shutdown */
         if ( GFlags.ShutdnPending )
            {
               GFlags.ShutdnPending = FALSE;
               CancelTimer( SHUTDNTIMER );
            }

         /* If DMA buffer does not exist then create it */
         if ( ppDMABuffer == NULL )
            {
               if ( CreateDMABuffer() == FAILURE )
                  {
                     pHeadIORB->Status |= IORB_ERROR;
                     pHeadIORB->ErrorCode = IOERR_CMD_SW_RESOURCE;
                     IORBDone();
                     return;
                  }
            }

         /* If shutdown sequence is currently unlocking memory, then */
         /* block until the unlock is complete.                      */
         DisableInts
         while ( GFlags.Unlocking )
            {
               DevHelp_ProcBlock( (ULONG)&LockHandle, -1L, WAIT_IS_INTERRUPTABLE );
               DisableInts
            }
         EnableInts

         /* If swapable code is not locked then lock it */
         if ( LockHandle == NULL )
            {
               pFunction = (PFN)NextIORB;
               if ( DevHelp_Lock( SELECTOROF(pFunction), 1, 0, &LockHandle ) )
                  {
                     /* Unsuccessful lock.  Undo everything and error exit. */
                     LockHandle = NULL;
                     DevHelp_FreePhys( ppDMABuffer );
                     ppDMABuffer = NULL;
                     if ( pReadBackBuffer != NULL )
                        {
                           DevHelp_FreePhys( ppReadBackBuffer );
                           ppReadBackBuffer = NULL;
                        }
                     pHeadIORB->Status |= IORB_ERROR;
                     pHeadIORB->ErrorCode = IOERR_CMD_OS_SOFTWARE_FAILURE;
                     IORBDone();
                     return;
                  }
            }

         if ( GFlags.ProcDisabled ) return;

         NextIORB();
      }
   else  /* Action == SHUTDOWN */
      {
         if ( !GFlags.ShutdnPending ) return;

         /* Unlock the swapable code */
         GFlags.Unlocking = TRUE;
         if ( DevHelp_UnLock( LockHandle ) )
            GFlags.ShutdnPending = FALSE;
         else LockHandle = NULL;
         GFlags.Unlocking = FALSE;

         /* If startup is blocked, then this will unblock it */
         DevHelp_ProcRun( (ULONG)&LockHandle, &AwakeCount );

         if ( !GFlags.ShutdnPending ) return;

         /* Deallocate the DMA buffer */
         DevHelp_FreePhys( ppDMABuffer );
         ppDMABuffer = NULL;
         if ( pReadBackBuffer != NULL )
            {
               DevHelp_FreePhys( ppReadBackBuffer );
               ppReadBackBuffer = NULL;
            }

         if ( !GFlags.ShutdnPending ) return;

         if ( pSuspendIORB != NULL )   /* If suspend is pending */
            {
               /* Unhook the IRQ */
               if ( DevHelp_UnSetIRQ( IntLevel ) )
                  {
                     pSuspendIORB->iorbh.Status |= IORB_ERROR;
                     pSuspendIORB->iorbh.ErrorCode = IOERR_CMD_OS_SOFTWARE_FAILURE;
                  }
               else GFlags.ProcDisabled = TRUE;  /* Disable IORB processing */

               pSuspendIORB->iorbh.Status |= IORB_DONE;

               /* If asynchronous post is requested then do it */
               if ( pSuspendIORB->iorbh.RequestControl & IORB_ASYNC_POST )
                  pSuspendIORB->iorbh.NotifyAddress( (PIORBH)pSuspendIORB );

               pSuspendIORB = NULL;
            }

         GFlags.ShutdnPending = FALSE;   /* Shutdown is complete */
      }
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : SuspendResume                                             */
/*                                                                           */
/*   Description : This routine handles the processing of suspend and        */
/*                 resume IORBs.  Suspend and resume IORBs are not           */
/*                 placed on the list of IORBs, but are instead              */
/*                 processed by this routine.                                */
/*                                                                           */
/*                 A suspend will wait until the ADD is shutdown and         */
/*                 then unhook the interrupt level.  If the ADD is           */
/*                 already shutdown, then this routine will unhook the       */
/*                 interrupt level right away.  If the add is not            */
/*                 shutdown and an immediate suspend is requested, then      */
/*                 this routine will disable the processing of IORBs to      */
/*                 hasten the ADD toward shutdown.                           */
/*                                                                           */
/*                 A resume will rehook the interrupt level and reset        */
/*                 the diskette controller.  If the there are IORBs to       */
/*                 process, then IORB processing is kick started.            */
/*                 Note: The second half of resume processing is done        */
/*                 by the ResetComplete routine.                             */
/*                                                                           */
/*****************************************************************************/

VOID NEAR SuspendResume( PIORBH pNewIORB )
{
   if ( pNewIORB->CommandModifier == IOCM_SUSPEND )
      {
         if ( pSuspendIORB != NULL ) /* If a previous suspend is pending */
            {
               /* Immediately return this Suspend IORB */
               pNewIORB->Status |= IORB_DONE;

               EnableInts

               /* If asynchronous post is requested then do it */
               if ( pNewIORB->RequestControl & IORB_ASYNC_POST )
                  pNewIORB->NotifyAddress( pNewIORB );
            }
         else /* No previous suspend is pending */
            {
               pSuspendIORB = (PIORB_DEVICE_CONTROL)pNewIORB;

               /* If idle then we can suspend right now */
               if ( LockHandle == NULL )
                  {
                     /* Unhook the IRQ */
                     if ( DevHelp_UnSetIRQ( IntLevel ) )
                        {
                           pSuspendIORB->iorbh.Status |= IORB_ERROR;
                           pSuspendIORB->iorbh.ErrorCode = IOERR_CMD_OS_SOFTWARE_FAILURE;
                        }
                     else GFlags.ProcDisabled = TRUE;  /* Disable processing */

                     pSuspendIORB->iorbh.Status |= IORB_DONE;

                     EnableInts

                     /* If asynchronous post is requested then do it */
                     if ( pSuspendIORB->iorbh.RequestControl & IORB_ASYNC_POST )
                        pSuspendIORB->iorbh.NotifyAddress( (PIORBH)pSuspendIORB );

                     pSuspendIORB = NULL;
                  }
               else  /* Must wait for ADD to stop running before suspending */
                  {
                     if ( pSuspendIORB->Flags & DC_SUSPEND_IMMEDIATE )
                        /* Don't allow any more IORBs to be processed */
                        GFlags.ProcDisabled = TRUE;
                  }
            }
      }
   else if ( pNewIORB->CommandModifier == IOCM_RESUME )
      {
         if ( pResumeIORB != NULL ) /* If a previous resume is pending */
            {
               /* Immediately return this resume IORB */
               pNewIORB->Status |= IORB_DONE;

               EnableInts

               /* If asynchronous post is requested then do it */
               if ( pNewIORB->RequestControl & IORB_ASYNC_POST )
                  pNewIORB->NotifyAddress( pNewIORB );
            }
         else /* No previous resume is pending */
            {
               pResumeIORB = pNewIORB;

               if ( pSuspendIORB != NULL )      /* If a suspend is pending */
                  {
                     /* Return the suspend IORB */

                     pSuspendIORB->iorbh.Status |= IORB_DONE;

                     EnableInts

                     /* If asynchronous post is requested then do it */
                     if ( pSuspendIORB->iorbh.RequestControl & IORB_ASYNC_POST )
                        pSuspendIORB->iorbh.NotifyAddress( (PIORBH)pSuspendIORB );

                     pSuspendIORB = NULL;     /* Suspend is no longer pending */

                     if ( GFlags.ProcDisabled )  /* If it was an immediate suspend */
                        {
                           GFlags.ProcDisabled = FALSE;

                           if ( GFlags.ShutdnPending && pHeadIORB != NULL )
                              {
                                 GFlags.ShutdnPending = FALSE;
                                 CancelTimer( SHUTDNTIMER );
                                 NextIORB();   /* Kick start IORB processing */
                              }
                        }
                  }
               else if ( GFlags.ProcDisabled )  /* If suspended */
                  {
                     /* Hook the IRQ */
                     if ( DevHelp_SetIRQ( (NPFN)IntHandler, IntLevel, 1 ) )
                        {
                           pResumeIORB->Status |= IORB_ERROR;
                           pResumeIORB->ErrorCode = IOERR_CMD_SW_RESOURCE;
                        }
                     else  /* IRQ was successfully hooked */
                        {
                           EnableInts
                           Reset();
                           return;
                        }
                  }

               /* Return the Resume IORB */

               pResumeIORB->Status |= IORB_DONE;

               EnableInts

               /* If asynchronous post is requested then do it */
               if ( pResumeIORB->RequestControl & IORB_ASYNC_POST )
                  pResumeIORB->NotifyAddress( pResumeIORB );

               pResumeIORB = NULL;
            }
      }
   else   /* Command not supported */
      {
         pNewIORB->Status   |= (IORB_ERROR | IORB_DONE);
         pNewIORB->ErrorCode = IOERR_CMD_NOT_SUPPORTED;

         EnableInts

         /* If asynchronous post is requested then do it */
         if ( pNewIORB->RequestControl & IORB_ASYNC_POST )
            pNewIORB->NotifyAddress( pNewIORB );
      }
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : Reset                                                     */
/*                                                                           */
/*   Description : This routine is called to reset the diskette              */
/*                 controller.  This routine is called when the boot         */
/*                 is complete and when resuming.  This routine calls        */
/*                 the ABIOS Reset function.                                 */
/*                                                                           */
/*****************************************************************************/

VOID FAR Reset()
{
   NPABRB_GENERIC pRBG = (NPABRB_GENERIC)RequestBlock;

   pRBG->abrbh.Function = ABFC_RESET_DEVICE;
   pRBG->abrbh.RC       = ABRC_START;
   pRBG->Offset10H      = 0;          /* +10H reserved field */

   GFlags.Resetting  = TRUE;
   CompletionRoutine = ResetComplete;
   Retry = 0;
   Stage = ABIOS_EP_START;
   NextStage();
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : ResetComplete                                             */
/*                                                                           */
/*   Description : This routine is called when the ABIOS Reset function      */
/*                 is completed.  If a resume is being executed, then        */
/*                 this routine does the second half of resume processing.   */
/*                                                                           */
/*****************************************************************************/

VOID FAR ResetComplete()
{
   NPABRBH pRB = (NPABRBH)RequestBlock;
   UCHAR LastDataRate;

   GFlags.Resetting = FALSE;

   if ( pResumeIORB != NULL )  /* If a resume is being executed */
      {
         if ( pRB->RC != ABRC_COMPLETEOK )  /* If reset was not successful */
            {
               DevHelp_UnSetIRQ( IntLevel );    /* We are still suspended */
               pResumeIORB->Status |= IORB_ERROR;
               pResumeIORB->ErrorCode = IOERR_ADAPTER_NONSPECIFIC;
            }
         else  /* Reset was a success */
            {
               /* Enable the processing of IORBs */
               GFlags.ProcDisabled = FALSE;

               /* Set Configuration Control Register to last data rate */
               LastDataRate = (pDeviceBlock->LastRate)>>6;
               outp( 3F7H, LastDataRate )

               if ( pHeadIORB != NULL )   /* If the IORB list is not empty */
                  {
                     if ( !GFlags.CtxHookArmed )
                        {
                           GFlags.CtxHookArmed = TRUE;
                           DevHelp_ArmCtxHook( STARTUP, HookHandle );
                        }
                  }
            }

         /* Return the Resume IORB */

         pResumeIORB->Status |= IORB_DONE;

         /* If asynchronous post is requested then do it */
         if ( pResumeIORB->RequestControl & IORB_ASYNC_POST )
            pResumeIORB->NotifyAddress( pResumeIORB );

         pResumeIORB = NULL;
      }
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : CreateDMABuffer                                           */
/*                                                                           */
/*   Description : This routine allocates a DMA buffer.                      */
/*                                                                           */
/*****************************************************************************/

BOOL FAR CreateDMABuffer()
{
   for ( BuffSize=MaxBuffSize; BuffSize>=MINBUFFSIZE; BuffSize/=2 )
      {
         /* Allocate DMA buffer */
         if ( !DevHelp_AllocPhys( BuffSize, 0, &ppDMABuffer ) )
            {
               /* Setup GDT selector for the DMA buffer */
               if ( DevHelp_PhysToGDTSelector( ppDMABuffer, (USHORT)BuffSize,
                    SELECTOROF(pDMABuffer) ) )
                  {
                     DevHelp_FreePhys( ppDMABuffer );
                     ppDMABuffer = NULL;
                     return FAILURE;
                  }

               if ( pReadBackBuffer != NULL ) /* If read back buffer needed */
                  {
                     /* Allocate read back buffer */
                     if ( !DevHelp_AllocPhys( BuffSize, 0, &ppReadBackBuffer ) )
                        {
                           /* Setup GDT selector for the read back buffer */
                           if ( DevHelp_PhysToGDTSelector( ppReadBackBuffer,
                               (USHORT)BuffSize, SELECTOROF(pReadBackBuffer) ) )
                              {
                                 DevHelp_FreePhys( ppDMABuffer );
                                 ppDMABuffer = NULL;
                                 DevHelp_FreePhys( ppReadBackBuffer );
                                 ppReadBackBuffer = NULL;
                                 return FAILURE;
                              }
                           else return SUCCESS; /* DMA & read back buffers */
                        }                       /* successfully created    */
                     else /* Read back buffer was not successfully allocated */
                        {
                           if ( DevHelp_FreePhys( ppDMABuffer ) )
                              return FAILURE;
                           else ppDMABuffer = NULL;
                        }
                  }
               else return SUCCESS;  /* DMA buffer successfully created */
            }
      }

   return FAILURE;  /* Not enough physical memory available */
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : Strategy                                                  */
/*                                                                           */
/*   Description : This is the strategy entry point for the device           */
/*                 driver.  The initialization request packet is routed      */
/*                 to InitFloppy.                                            */
/*                                                                           */
/*****************************************************************************/

VOID NEAR Strategy()
{
   PRPH pRPH;

   _asm
      {
         mov word ptr pRPH[0], bx
         mov word ptr pRPH[2], es
      }

   pRPH->Status = STATUS_SUCCESS;

   if ( pRPH->Cmd == CMDInitBase )
      InitFloppy( (PRPINITIN)pRPH );
   else
      pRPH->Status = STATUS_ERR_UNKCMD;

   pRPH->Status |= STATUS_DONE;

   _asm
      {
         leave
         retf
      }
}


