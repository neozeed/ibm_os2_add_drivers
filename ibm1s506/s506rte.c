/*static char *SCCSID = "@(#)s506rte.c	6.2 92/02/06";*/
/****************************************************************************
 *                                                                          *
 *                (c) Copyright  IBM Corporation  1981, 1990                *
 *                           All Rights Reserved                            *
 *                                                                          *
 ****************************************************************************/

/********************* Start of Specifications ******************************
 *                                                                          *
 *  Source File Name: S5061RTE.C                                            *
 *                                                                          *
 *  Descriptive Name: Hard Disk 1 Strategy Routine                          *
 *                                                                          *
 *  Copyright: THE CODE IN THIS MODULE IS IBM UNIQUE CODE.                  *
 *                                                                          *
 *  Status:                                                                 *
 *                                                                          *
 *  Function: Part of Hard Disk device driver for OS/2 family 1             *
 *                                                                          *
 *                                                                          *
 *  Notes:                                                                  *
 *    Dependencies:                                                         *
 *    Restrictions:                                                         *
 *                                                                          *
 *  Entry Points:  S506Str1                                                 *
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
 #include "devcmd.h"               // C:\DRV6\H

 #include "iorb.h"                 // C:\DRV6\SRC\DEV\DASD\DISKH
 #include "reqpkt.h"               // C:\DRV6\SRC\DEV\DASD\DISKH
 #include "dhcalls.h"

 #include "s506hdr.h"
 #include "s506ext.h"
 #include "s506pro.h"

/*--------------------- START OF SPECIFICATIONS --------------------------*
 *                                                                        *
 * SUBROUTINE NAME:  S506Str1.C                                           *
 *                                                                        *
 * DESCRIPTIVE NAME: Disk device driver Sstrategy Routine.                *
 *                                                                        *
 * FUNCTION: The required command is passed to this routine. Then         *
 *           the command is checked if is an initialization command.      *
 *                                                                        *
 * NOTES: This routine runs in protect mode only as a single thread       *
 *        at level 3 with IOPL.                                           *
 *                                                                        *
 * ENTRY POINT:                                                           *
 *                                                                        *
 * LINKAGE: Called from the kernel                                        *
 *                                                                        *
 * INPUT: []    -                                                         *
 *                                                                        *
 * EXIT-NORMAL: Return to the kernel                                      *
 *                                                                        *
 * EXIT-ERROR:                                                            *
 *                                                                        *
 *                                                                        *
 * EFFECTS:                                                               *
 *                                                                        *
 * INTERNAL REFERENCES:                                                   *
 *    ROUTINES:  -                                                        *
 *               -                                                        *
 *                                                                        *
 * DATA STRUCTURES:                                                       *
 *                                                                        *
 *                                                                        *
 * EXTERNAL REFERENCES:                                                   *
 *    ROUTINES:                                                           *
 *                                                                        *
 * Modifications:                                                         *
 * b732626  02/06/92  Kip Harris. Change pRPH->Status value on unsupported*
 *          to permit OPEN and CLOSE commands to be processed.  This allows
 *          us to check for presence of the driver.                       *
 *                                                                        *
 *********************** END OF SPECIFICATIONS ****************************/

/*------------------------------------------------------------------------*/
/* OS/2 Strategy Request Router                                           */
/* ----------------------------                                           */
/* This routine receives the OS/2 Initialization Request Packet. Any      */
/* other request types are rejected.                                      */
/*------------------------------------------------------------------------*/

 VOID NEAR S506Str1()
 {
 PRPH pRPH;                   // Pointer to RPH (Request Packet Header)
 USHORT        Cmd;           // Local variable

 _asm
   {
     mov word ptr pRPH[0], bx       //  pRPH is initialize to
     mov word ptr pRPH[2], es       //  ES:BX passed from the kernel
   }

 pRPH->Status = STATUS_DONE;
 Cmd = pRPH->Cmd;
 if ((Cmd == CMDInitBase) && (Cmd !=Init_Complete ))
   {
   DriveInit( (PRPINITIN) pRPH );
   }
 else
   {
   StatusError( pRPH, STATUS_DONE | STATUS_ERR_UNKCMD );
   }

  _asm
    {
      leave
      retf
    }

 }

/*---------------------------------------------------------------------------*
 * Strategy 1 Error Processing                                               *
 * ---------------------------                                               *
 *                                                                           *
 *                                                                           *
 *---------------------------------------------------------------------------*/
 VOID NEAR StatusError(PRPH pRPH, USHORT ErrorCode )
 {
 pRPH->Status = ErrorCode;
 return;
 }

