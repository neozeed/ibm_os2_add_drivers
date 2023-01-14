
/***********************************************************************/
/*                                                                     */
/* Driver Name: ABIOS Adapter Device Driver for DASD Devices           */
/*              --------------------------------------------           */
/*                                                                     */
/* Source File Name: ADSKSGB.C                                         */
/*                                                                     */
/* Descriptive Name: ABIOS Disk Driver - Scatter/Gather Buffer Mgr     */
/*                                                                     */
/* Function: Handles Allocation/Deallocation of S/G Buffers            */
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


/*-----------------------------------------------------------*/
/*                                                           */
/* Obtain a Scatter/Gather Emulation Buffer                  */
/* ----------------------------------------                  */
/*                                                           */
/*                                                           */
/*-----------------------------------------------------------*/

NPIOBUF_POOL NEAR AllocSGBuffer( npLCB, CallBackRoutine )

NPLCB   npLCB;
VOID    (NEAR *CallBackRoutine)();
{
  NPIOBUF_POOL npBufPool;

  DISABLE


  /*---------------------------------------------------*/
  /* If a S/G buffer is available, then remove the     */
  /* buffer from the pool and return it to the         */
  /* requestor.                                        */
  /*---------------------------------------------------*/

  if ( npBufPool = npBufPoolHead )
    {
      if ( !(npBufPoolHead = npBufPool->npNextBuf) )
        {
          npBufPoolFoot = 0;
        }
      npBufPool->npNextBuf = 0;
     }
  /*---------------------------------------------------*/
  /* Otherwise, mark the LCB in a buffer wait. Record  */
  /* the CallBack address in the LCB and add the LCB   */
  /* to the list of LCBs waiting on S/G buffers.       */
  /*---------------------------------------------------*/

  else
    {
      npLCB->IntFlags      |= LCBF_ONBUFWAITQ;

      npLCB->npBufNotifyRtn = CallBackRoutine;

      if ( !npLCBBufQFoot )
        {
          npLCBBufQHead = npLCB;
        }
      else
        {
          npLCBBufQFoot->npNextBufQLCB = npLCB;
        }

      npLCBBufQFoot        = npLCB;
      npLCB->npNextBufQLCB = 0;
    }

  ENABLE

  return ( npBufPool );
}


/*-----------------------------------------------------------*/
/*                                                           */
/* Release a Scatter/Gather Emulation Buffer                 */
/* -----------------------------------------                 */
/*                                                           */
/*                                                           */
/*-----------------------------------------------------------*/

VOID NEAR FreeSGBuffer( npIOBuf )

NPIOBUF_POOL   npIOBuf;
{
  NPLCB     npLCB;

  DISABLE

  /*---------------------------------------------------*/
  /* If an LCB was waiting on a buffer, remove the LCB */
  /* from the buffer wait Q and call the LCB's notify  */
  /* address.                                          */
  /*---------------------------------------------------*/

  npIOBuf->npNextBuf = 0;

  if ( npLCB = npLCBBufQHead )
    {
      if ( !(npLCBBufQHead = npLCB->npNextBufQLCB) )
        {
          npLCBBufQFoot = 0;
        }

      npLCB->IntFlags      &= ~LCBF_ONBUFWAITQ;
      npLCB->npNextBufQLCB  =  0;

      ENABLE

      (*npLCB->npBufNotifyRtn)( npLCB, npIOBuf );
    }
  /*---------------------------------------------------*/
  /* Otherwise, return the buffer to the S/G buffer    */
  /* pool.                                             */
  /*---------------------------------------------------*/
  else
    {
      if ( !npBufPoolHead )
        {
          npBufPoolHead = npIOBuf;
        }
      else
        {
          npBufPoolFoot->npNextBuf = npIOBuf;
        }

      npBufPoolFoot = npIOBuf;
    }

  ENABLE
}

