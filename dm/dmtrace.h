/*static char *SCCSID = "@(#)dmtrace.h	6.3 92/01/30";*/
/*--------------------------------------------------------------------*/
/*   RAS/DEKKO/PERFVIEW trace header file for OS2DASD.SYS             */
/*--------------------------------------------------------------------*/
#include "pvwxport.h"

#define TEST_RAS_TRACING(RMT,Maj)   \
         (*(RMT+(Maj>>3)) & (0x80>>(Maj&7)))

#define TEST_TRACING(RMT,Maj)   \
        (*RMT & 0x40) && (*(RMT+(Maj>>3)) & (0x80>>(Maj&7)))

/* Trace Event flags */
#define TRACE_STRAT1            0x0001
#define TRACE_STRAT2            0x0002
#define TRACE_IORB              0x0004
#define TRACE_ENTRY             0x0008
#define TRACE_ASYNCSTART        0x0010
#define TRACE_ASYNCDONE         0x0020
#define TRACE_EXIT              0x0040
#define TRACE_READ              0x0080
#define TRACE_WRITE             0x0100
#define TRACE_FORMAT            0x0200
#define TRACE_VERIFY            0x0400
#define TRACE_PREFETCH          0x0800
#define TRACE_IOCTL             0x1000

/* Global TraceFlags */
#define TF_INTERNAL             0x0001
#define TF_PERFVIEW             0x0002
#define TF_RAS                  0x0004
#define TF_DEKKO                0x0008


/* RAS Trace Major/Minor Codes */
#define DEKKO_MAJOR_DISK        0x68

#define RAS_MAJOR_DISK          0x07
#define RAS_MINOR_STRAT1_RWV    0x08
#define RAS_MINOR_IOCTL_RWVF    0x09
#define RAS_MINOR_STRAT2_RLH    0x0A
#define RAS_MINOR_STRAT2_RLE    0x0B
#define RAS_MINOR_IORB          0x0C

/* Trace Control Block */

typedef struct _TCB {

   PBYTE        pRequest;
   USHORT       Unit;
   UCHAR        Drive[2];
   UCHAR        CommandCode;
   UCHAR        CommandModifier;
   UCHAR        CmdString[3];
   UCHAR        Reserved_1;
   USHORT       RequestControl;
   UCHAR        Priority;
   UCHAR        Flags;
   USHORT       cSGList;
   ULONG        RBA;
   ULONG        BlockCount;
   USHORT       pRLH;
   USHORT       Reserved_2;

} TCB, FAR *PTCB;


typedef struct _TCBD {

   PBYTE        pRequest;
   USHORT       Status;
   USHORT       ErrorCode;
   ULONG        BlocksXferred;

} TCBD, FAR *PTCBD;


typedef struct _TRLHS {

   PBYTE        pRLH;
   USHORT       Count;
   USHORT       Unit;
   UCHAR        Drive[2];
   USHORT       Request_Control;

} TRLHS, FAR *PTRLHS;

typedef struct _TRLHD {

   PBYTE        pRLH;
   USHORT       DoneCount;
   USHORT       Status;

} TRLHD, FAR *PTRLHD;

/* Internal Trace Control Block */

#define TRACEBUF_SIZE 256

typedef struct _ITCB {

   UCHAR        Unit;
   UCHAR        Event;
   USHORT       pIORB;
   UCHAR        CommandModifier;
   UCHAR        CommandCode;
   USHORT       Status;
   USHORT       ErrorCode;
   ULONG        rba;
   USHORT       BlockCount;

} ITCB, FAR *PITCB, NEAR *NPITCB;




typedef struct _PVDB {

   DBH          pfdbh;
   CNT          NumReads;               /* Read Counter */
   TIMR         ReadTime;               /* Read Timer   */
   CNT          ReadBytes;              /* Read Byte Count */
   CNT          NumWrites;              /* Write Counter */
   TIMR         WriteTime;              /* Write Timer   */
   CNT          WriteBytes;             /* Write Byte Count */
} PVDB;




