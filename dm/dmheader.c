/*static char *SCCSID = "@(#)dmheader.c	6.1 92/01/08";*/
/*static char *SCCSID = "@(#)dmdata.c   6.4 91/08/28";*/
#define SCCSID  "@(#)dmdata.c   6.4 91/08/28"

/*--------------------------------------------------------
 *       (c) Copyright IBM Corporation 1981, 1990        *
 *                                                       *
 *       All Rights Reserved                             *
 *-------------------------------------------------------*/

#define INCL_NOBASEAPI
#define INCL_NOPMAPI
#define INCL_ERROR_H

#include "os2.h"
#include "devhdr.h"
/*-------------------------------------------------------------*/
/* Disk Device Driver Header                                   */
/*                                                             */
/* This must be at the beginning of the data segment           */
/* AND MUST NOT BE MOVED.                                      */
/*-------------------------------------------------------------*/

VOID   NEAR DMStrat1 (void);

struct SysDev DiskDDHeader =
{
   -1L,                                 /* Pointer to next DD Header      */
   DEV_NON_IBM | DEVLEV_1,              /* Device attribute               */
   (USHORT) DMStrat1,                   /* Offset to Strategy routine     */
   0,                                   /* Offset to IDC Entry Point      */
   " Disk DD",                          /* Device Name                    */
   0,                                   /* Protect mode CS of strategy EP */
   0,                                   /* Protect mode DS of strategy EP */
   0,
   0
};
