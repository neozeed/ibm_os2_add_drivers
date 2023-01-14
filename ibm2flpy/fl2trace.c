/*static char *SCCSID = "@(#)fl2trace.c	6.2 92/01/17";*/
/*****************************************************************************/
/*                                                                           */
/*   Component   : IBM PS/2 Floppy Adapter Device Driver  (IBM2FLPY)         */
/*                                                                           */
/*   Module      : FL2TRACE.C                                                */
/*                                                                           */
/*   Description :                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

#define INCL_NOPMAPI
#define INCL_NOBASEAPI
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
#include "fl2trace.h"


/*****************************************************************************/
/*                                                                           */
/*   Routine     : CreateTraceBuff                                           */
/*                                                                           */
/*   Description :                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

VOID NEAR CreateTraceBuff()
{
   SEL   TraceBuffSel;
   ULONG PhysAddr;

   DevHelp_AllocGDTSelector( &TraceBuffSel, 1 );

   DevHelp_AllocPhys( 65536, 0, &PhysAddr );

   DevHelp_PhysToGDTSelector( PhysAddr, 0, TraceBuffSel );

   TraceBuffPtr = MAKEP( TraceBuffSel, 0 );

   ClearTraceBuff();
}


/*****************************************************************************/
/*                                                                           */
/*   Routine     : ClearTraceBuff                                            */
/*                                                                           */
/*   Description :                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

VOID NEAR ClearTraceBuff()
{
   OFFSETOF( TraceBuffPtr ) = 0;

   do { *((PUSHORT)TraceBuffPtr)++ = 0; } while( OFFSETOF(TraceBuffPtr) != 0 );
}


