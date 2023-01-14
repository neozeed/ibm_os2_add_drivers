/*static char *SCCSID = "@(#)fl2data.c	6.4 92/01/30";*/
/*****************************************************************************/
/*                                                                           */
/*   Component   : IBM PS/2 Floppy Adapter Device Driver  (IBM2FLPY)         */
/*                                                                           */
/*   Module      : FL2DATA.C                                                 */
/*                                                                           */
/*   Description : A device driver data segment needs to be organized        */
/*                 so that the device header is the first thing in the       */
/*                 segment and data, used only at initialization time,       */
/*                 is at the very end of the segment.  To ensure that        */
/*                 initialization data is placed at the very end, all        */
/*                 global variables are defined in a single file             */
/*                 (this one) and all the global variables are initialized.  */
/*                 Initalizing the global variables forces C to allocate     */
/*                 them in the same order that they are defined.             */
/*                                                                           */
/*****************************************************************************/

#define INCL_NOBASEAPI
#define INCL_NOPMAPI
#include <os2.h>
#include <strat2.h>     /* needed to keep reqpkt.h happy */
#include <reqpkt.h>
#include <scb.h>        /* needed to keey abios.h happy */
#include <abios.h>
#include <iorb.h>
#include <addcalls.h>
#include "fl2def.h"
#include "fl2proto.h"
#include "fl2data.h"


PFN    Device_Help = NULL;


/* ABIOS request blocks */

BYTE   RequestBlock[GENERIC_ABRB_SIZE]   = {0};
BYTE   MotorOffReqBlk[GENERIC_ABRB_SIZE] = {0};

PDEV_BLK pDeviceBlock = NULL;


/* Controller specific information */

USHORT LID       = 0;    /* ABIOS Logical ID for this controller */
USHORT IntLevel  = 0;    /* Interrupt Level for this controller  */
USHORT UnitCnt   = 0;    /* Number of units on this controller   */
UCHAR  Retry     = 0;    /* Current retry attempt                */
USHORT SectorCnt = 0;    /* Current number of sectors being rwv  */
USHORT Stage     = 0;    /* Specifies which ABIOS entry point    */
GLBLFLGS GFlags  = {0};  /* Global Flags                         */
USHORT ADDHandle = 0;    /* ADD Handle returned by RegisterADD   */
ULONG  HookHandle = 0L;  /* Context hook handle                  */
ULONG  LockHandle = 0L;  /* Lock handle for SwapCode segment     */

PCOMPFN CompletionRoutine = NULL;

UCHAR AdapterName[] = "IBM2FLPY";

PIORBH pHeadIORB    = NULL;     /* Start of IORB linked list      */
PIORB_DEVICE_CONTROL pSuspendIORB = NULL; /* Pointer to pending Suspend IORB */
PIORBH pResumeIORB  = NULL;     /* Pointer to a Resume IORB being executed */

PBYTE   pDMABuffer       = NULL;  /* GDT address of DMA buffer            */
ULONG   ppDMABuffer      = NULL;  /* Physical address of DMA buffer       */
PBYTE   pReadBackBuffer  = NULL;  /* GDT address of read back buffer      */
ULONG   ppReadBackBuffer = NULL;  /* Physical address of read back buffer */
ULONG   MaxBuffSize      = 0L;    /* Maximum size needed for DMA buffer   */
ULONG   BuffSize         = 0L;    /* Actual size of the DMA buffer        */

ADD_XFER_DATA XferData = {0};   /* Used by ADD_XferBuffData       */

/* Timer Variables */

USHORT MSPerTick       = 0;     /* Milliseconds per timer tick */
TIMER  Timer[TIMERCNT] = {0};   /* Timer table                 */


DRIVE  Drive[MAXUNITS] = {0};   /* Drive specific information */


/* The following data is used only at initialzation time */

USHORT InitData   = 0;    /* Marks the start of initialization data */


