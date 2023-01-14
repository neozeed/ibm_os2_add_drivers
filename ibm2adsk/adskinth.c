/*static char *SCCSID = "@(#)adskinth.c	6.4 92/01/17";*/

/***********************************************************************/
/*                                                                     */
/* Driver Name: ABIOS Adapter Device Driver for DASD Devices           */
/*              --------------------------------------------           */
/*                                                                     */
/* Source File Name: ADSKINTH.C                                        */
/*                                                                     */
/* Descriptive Name: ABIOS Disk Driver - Interrupt Handler             */
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

/*-------------------------------------------------------------------------*/
/*                                                                         */
/* Interrupt Handler Entry Points                                          */
/* ------------------------------                                          */
/*                                                                         */
/* Call the common interrupt handler code with the appropriate interrupt   */
/* control block. On return the interrupt handler will indicate whether    */
/* or not it has handled the interrupt via its return code. The return     */
/* code is right shifted to set or reset the CF flag which the kernel      */
/* tests.                                                                  */
/*                                                                         */
/*-------------------------------------------------------------------------*/


USHORT FAR IRQEntry0()
{
  return (InterruptHandler( &IntLevelCB[0] ) >> 1);
}

USHORT FAR IRQEntry1()
{
  return (InterruptHandler( &IntLevelCB[1] ) >> 1);
}

USHORT FAR IRQEntry2()
{
  return (InterruptHandler( &IntLevelCB[2] ) >> 1);
}

USHORT FAR IRQEntry3()
{
  return (InterruptHandler( &IntLevelCB[3] ) >> 1);
}

USHORT NEAR InterruptHandler( npIntCB )

NPINTCB npIntCB;

{
  NPLCB         npLCB;
  USHORT        Claimed = 0;
  SHORT         ABIOSRc;


  ENABLE

  npLCB    = npIntCB->npFirstLCB;

  while ( npLCB )
    {
      if ( npLCB->IntFlags & LCBF_ACTIVE )
        {
          ABIOSRc = ((NPABRBH) npLCB->ABIOSReq)->RC;
          if ( (ABIOSRc & ABRC_STAGEONINTERRUPT) && (ABIOSRc != ABRC_START) )
            {
              ABIOSRc = StageABIOSRequest( npLCB, ABIOS_EP_INTERRUPT );

              Claimed |= !( ( ABIOSRc == ABRC_SPURIOUSINTERRUPT)
                                      || (ABIOSRc == ABRC_NOTMYINTERRUPT));

              if ( ABIOSRc <= 0 && ABIOSRc != -1 )
                {
                  QueueLCBComplete( npLCB );
                }
            }
        }
      npLCB = npLCB->npNextIntLCB;
    }

  if ( !Claimed )
    {
      Claimed = ProcessDefaultInt( npIntCB );
    }

  if ( Claimed )
    {
      DevHelp_EOI( npIntCB->HwIntLevel );
    }

  ProcessLCBComplete();

  return( ~Claimed );
}


/*------------------------------------------*/
/*                                          */
/*                                          */
/*                                          */
/*                                          */
/*------------------------------------------*/

USHORT NEAR StageABIOSRequest( npLCB, Abios_EP_Type )

NPLCB           npLCB;
USHORT          Abios_EP_Type;
{
  NPABRBH       npABRBH;
  USHORT        ABIOSRc;
  USHORT        IntClaimed;
  ULONG         Timeout;


  npABRBH = (NPABRBH)npLCB->ABIOSReq;

  if ( Abios_EP_Type == ABIOS_EP_START )
    {
      npABRBH->RC = ABRC_START;
      LidIOCount[npLCB->LidIndex]++;
    }
  else
    {
      npLCB->IntFlags &= ~LCBF_STARTPENDING;
    }

  do
    {
      ENABLE

      if ( DevHelp_ABIOSCall( npABRBH->LID, (NPBYTE) npABRBH,
                                                       Abios_EP_Type ) )
        {
          _asm { int 3 }
        }
      DISABLE
      ABIOSRc = npABRBH->RC;
    }
  while ( ABIOSRc == ABRC_STAGEONTIME &&
                             ((NPABRB_DISK_RWV) npABRBH)->WaitTime == 1 );

  /*                         A                                 */
  /*                         |                                 */
  /*-----------------------------------------------------------*/
  /* ABIOS hack for problems with Mod 80 ESDI controllers      */
  /*-----------------------------------------------------------*/


  IntClaimed = !( ( ABIOSRc == ABRC_SPURIOUSINTERRUPT)
                                    || (ABIOSRc == ABRC_NOTMYINTERRUPT));

  if ( IntClaimed )
    {
      if ( npLCB->IntTimerHandle )
        {
          ADD_CancelTimer((ULONG) npLCB->IntTimerHandle);
          npLCB->IntTimerHandle = 0;
        }

      if ( npLCB->DlyTimerHandle )
        {
          _asm { int 3 }
        }
    }

  if ( ABIOSRc == ABRC_STAGEONINTERRUPT )
    {
      /*---------------------------------------------------------*/
      /* Note: ABIOS Interrupt Time Out in (Sec) is in Bits 15-3 */
      /*---------------------------------------------------------*/

      if ( (Timeout=npLCB->Timeout) != -1L )
        {
          Timeout = ULONGmulULONG( 1000L,
                                   ((!Timeout) ? (ULONG)(npABRBH->Timeout >> 3)
                                               : Timeout                     ));

          if ( ADD_StartTimerMS((PULONG) &npLCB->IntTimerHandle,
                                (ULONG)  Timeout,
                                (PFN)    InterruptTimeoutHandler,
                                (PVOID)  npLCB,
                                (PVOID)  0            ) )
            {
              _asm { int 3 }
            }
        }

      ENABLE
    }
  else if ( ABIOSRc == ABRC_STAGEONTIME )
    {
      /*--------------------------------*/
      /* Note: ABIOS WaitTime in (uSec) */
      /*--------------------------------*/
      ENABLE
      if ( ADD_StartTimerMS(
                        (PULONG) &npLCB->IntTimerHandle,
                        (ULONG)  ULONGdivUSHORT(
                                  ((NPABRB_DISK_RWV) npABRBH)->WaitTime, 1000, 0),
                        (PFN)    DelayTimeoutHandler,
                        (PVOID)  npLCB,
                        (PVOID)  0            ) )
        {
          _asm { int 3 }
        }
    }

  ENABLE

  return( ABIOSRc );
}


/*------------------------------------------*/
/*                                          */
/*                                          */
/*                                          */
/*                                          */
/*------------------------------------------*/

USHORT NEAR ProcessDefaultInt( npIntCB )

NPINTCB         npIntCB;
{
  NPLCB         npLCB;
  NPABRBH       npABRBH;
  USHORT        CurrentLid = -1;
  USHORT        ThisLid;
  USHORT        Claimed    =  0;


  npABRBH = (NPABRBH) npIntCB->DefaultABIOSReq;
  npABRBH->Unit     = 0;
  npABRBH->Function = ABFC_DEFAULT_INT_HANDLER;

  npLCB = npIntCB->npFirstLCB;
  while( npLCB )
    {
      ThisLid = ((NPABRBH) npLCB->ABIOSReq)->LID;

      if ( CurrentLid != ThisLid )
        {
          npABRBH->LID      = ThisLid;
          npABRBH->RC       = ABRC_START;
          if ( DevHelp_ABIOSCall( npABRBH->LID, (NPBYTE) npABRBH,
                                                        ABIOS_EP_INTERRUPT ) )
            {
              _asm { int 3 }
            }
          Claimed = ( npABRBH->RC == ABRC_COMPLETEOK ) ? 1 : 0;
        }
      CurrentLid = ThisLid;
      npLCB = npLCB->npNextIntLCB;
    }
  return( Claimed );
}

/*------------------------------------------*/
/*                                          */
/*                                          */
/*                                          */
/*                                          */
/*------------------------------------------*/

VOID NEAR QueueLCBComplete( npLCB )

NPLCB           npLCB;
{
  DISABLE

  if ( npLCB->IntFlags & LCBF_ONCOMPLETEQ )
    {
      _asm { int 3 }
    }

  if ( !npLCBCmpQHead )
    {
      npLCBCmpQHead = npLCB;
    }
  else
    {
      npLCBCmpQFoot->npNextCmpQLCB = npLCB;
    }

  npLCBCmpQFoot        = npLCB;
  npLCB->npNextCmpQLCB = 0;

  npLCB->IntFlags |= LCBF_ONCOMPLETEQ;

  ENABLE
}

/*------------------------------------------*/
/*                                          */
/*                                          */
/*                                          */
/*                                          */
/*------------------------------------------*/

VOID NEAR ProcessLCBComplete( )

{
  NPLCB    npLCB;

  DISABLE
  if ( !CompletionProcessActive )
    {
      CompletionProcessActive = 1;

      while ( npLCB = npLCBCmpQHead )
        {
          if ( !(npLCBCmpQHead = npLCB->npNextCmpQLCB) )
            {
              npLCBCmpQFoot = 0;
            }

          npLCB->IntFlags      &= ~LCBF_ONCOMPLETEQ;
          npLCB->npNextCmpQLCB  =  0;

          ENABLE

          if ( ContinueDeviceIO( npLCB ) == REQUEST_DONE )
            {
              CompleteDeviceIO( npLCB );
              StartLCB( npLCB );
            }

          DISABLE
        }

      CompletionProcessActive = 0;

    }
  ENABLE
}

/*------------------------------------------*/
/*                                          */
/*                                          */
/*                                          */
/*                                          */
/*------------------------------------------*/

VOID FAR InterruptTimeoutHandler( hTimer, LCBAddr, Unused )

ULONG   hTimer;
PVOID   LCBAddr;
PVOID   Unused;
{
  NPLCB    npLCB   = (NPLCB) OFFSETOF( LCBAddr );
  USHORT   ABIOSRc;

  npLCB->IntTimerHandle = 0;
  ADD_CancelTimer( hTimer );

  ABIOSRc = StageABIOSRequest( npLCB, ABIOS_EP_TIMEOUT );

  if ( ((SHORT) ABIOSRc) <= 0 && ABIOSRc != -1 )
    {
      QueueLCBComplete( npLCB );
      ProcessLCBComplete();
    }
}


/*------------------------------------------*/
/*                                          */
/*                                          */
/*                                          */
/*                                          */
/*------------------------------------------*/

VOID FAR DelayTimeoutHandler( hTimer, LCBAddr, Unused )

ULONG   hTimer;
PVOID   LCBAddr;
PVOID   Unused;
{
  NPLCB    npLCB   = (NPLCB) OFFSETOF( LCBAddr );
  USHORT   ABIOSRc;

  npLCB->DlyTimerHandle = 0;
  ADD_CancelTimer( hTimer );

  ABIOSRc = StageABIOSRequest( npLCB, ABIOS_EP_INTERRUPT );

  if (  ((SHORT) ABIOSRc) <= 0 && ABIOSRc != -1 )
    {
      QueueLCBComplete( npLCB );
      ProcessLCBComplete();
    }
}




