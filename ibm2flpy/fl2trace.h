/*static char *SCCSID = "@(#)fl2trace.h	6.2 92/01/17";*/
/*****************************************************************************/
/*                                                                           */
/*   Component   : IBM PS/2 Floppy Adapter Device Driver  (IBM2FLPY)         */
/*                                                                           */
/*   Module      : FL2TRACE.H                                                */
/*                                                                           */
/*   Description : Trace definitions                                         */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

extern PUSHORT TraceBuffPtr;

#define Trace( Marker ) *TraceBuffPtr++ = Marker

VOID NEAR ClearTraceBuff( VOID );
VOID NEAR CreateTraceBuff( VOID );

