/*static char *SCCSID = "@(#)dmdata.c	6.7 92/02/06";*/
#define SCCSID  "@(#)dmdata.c	6.7 92/02/06"

/*--------------------------------------------------------
 *       (c) Copyright IBM Corporation 1981, 1990        *
 *                                                       *
 *       All Rights Reserved                             *
 *-------------------------------------------------------*/

#define INCL_NOBASEAPI
#define INCL_NOPMAPI
#define INCL_ERROR_H

#include "os2.h"
#include "misc.h"
#include "dmdefs.h"

#include "devhdr.h"
#include "devcmd.h"
#include "strat2.h"
#include "reqpkt.h"
#include "dhcalls.h"
#include "scb.h"
#include "abios.h"
#include "iorb.h"
#include "dmtrace.h"
#include "dmgencb.h"
#include "dmproto.h"
#include "dmioctl.h"
#include "ioctl.h"
#include "dskioctl.h"

/***************************************************************/
/*  GLOBAL DATA                                                */
/*  -----------                                                */
/*  1. Static data                                             */
/*  2. Dynamic data allocated at init time:                    */
/*     - UnitCB                                                */
/*     - VolCB                                                 */
/*     - VCS                                                   */
/*  3. Dynamic data allocated at run time:                     */
/*     - IORB                                                  */
/*  4. Static init data discarded after init time              */
/***************************************************************/

#define MAXADAPTERDRIVERS       32         /* Max adapter device drivers     */

/*  ScratchBuffer should appear after DiskDDHeader and should not be         */
/*  moved to make sure it does not span a 64K boundary.  It wont here        */
/*  since the data segment is page frame aligned.                            */

UCHAR           ScratchBuffer[512] = {0};  /* Scratch buffer for I/O         */

PFN             Device_Help=0L;            /* far ptr to devhelp function    */
PBYTE           pSysInfoSeg=0L;            /* Pointer to sys info seg        */
PBYTE           pSIS_mec_table=0L;         /* Pointer to RAS MEC Trace Table */
PVOID           pDataSeg=0L;               /* virt ptr of our data segment   */
ULONG           ppDataSeg=0L;              /* phys addr of our data segment  */
ULONG           plDataSeg=0L;              /* linear addr of our data seg    */
ULONG           ppScratchBuffer=0L;        /* Phys addr of ScratchBuffer     */
USHORT          ScratchBufSem=0;           /* Semaphore for ScratchBuffer    */
USHORT          DDFlags=0;                 /* Global driver flags            */

NPUNITCB        UnitCB_Head=0;             /* near ptr to first UnitCB       */
NPVOLCB         VolCB_Head=0;              /* near ptr to first VolCB        */
NPVOLCB         pVolCB_DriveA=0;           /* Pointer to A: VolCB            */
NPVOLCB         pVolCB_DriveB=0;           /* Pointer to B: VolCB            */
NPVOLCB         pVolCB_DriveC=0;           /* Pointer to C: VolCB            */
NPVOLCB         pVolCB_80=0;               /* Pointer to VolCB for drive 80H */
USHORT          NextLogDriveNum=0;         /* next logical drive number      */

NPBYTE          pNextFreeCB=0;             /* ptr to next free control blk   */
NPVOLCB         pLastLogVolCB=0;

USHORT          NumDrivers=0;              /* number of adapter drivers      */
USHORT          NumUnitCBs=0;              /* number of unit control blocks  */
USHORT          NumVolCBs=0;               /* number of volume control blocks*/
USHORT          NumLogDrives=0;            /* number of logical drives       */
USHORT          NumRemovableDisks=0;       /* number of removable disk drives*/
USHORT          NumFixedDisks=0;           /* number of fixed disk drives    */
USHORT          NumPartitions=0;           /* number of fixed disk partitions*/
USHORT          NumFTPartitions=0;         /* number of fault tol. partitions*/
NPVOLCB         pExtraVolCBs=0;            /* Pointer to extra volcbs        */
USHORT          NumExtraVolCBs=0;          /* number of extra volcbs         */
USHORT          NumAdapters=0;             /* number of adapters             */
USHORT          NumReqsInProgress=0;       /* num requests in progress       */
USHORT          NumReqsWaiting=0;          /* num requests on waiting queues */
USHORT          TraceFlags=0;              /* Trace Flags                    */
NPBYTE          pDMTraceBuf=0;             /* pointer to internal trace buffer*/
NPBYTE          pDMTraceHead=0;            /* pointer to head of trace buffer */
NPBYTE          pDMTraceEnd=0;             /* pointer to end of trace buffer */

NPBYTE          CB_FreeList=0;             /* Control Block Free List for    */
                                           /*  IORBs and CWAs                */

USHORT          PoolSem=0;                 /* Pool semaphore                 */
USHORT          PoolSize=0;                /* Size of control block pool     */
USHORT          FreePoolSpace=0;           /* Free space left in pool        */

UCHAR           fBigFat=0;                 /* flags for drives               */
UCHAR           XActPDrv=0;

PVOID           pFSD_EndofInt=0L;          /* FSD's End of interrupt routine */
PVOID           pFSD_AccValidate=0L;       /* FSD's Access Validation routine*/
PVOID           pDiskFT_Request=0L;        /* DISKFT's Request routine       */
PVOID           pDiskFT_Done=0L;           /* DISKFT's Done routine          */
USHORT          DiskFT_DS=0L;              /* DISKFT's DS selector           */

DriverCaps      DriverCapabilities= {0};   /* Driver Capabilities structure  */


DISKTABLE_ENTRY  DiskTable[DISKTABLECOUNT] =
                           {
                               { 32l*KB, 3,   8, 512, 0},
                               { 64l*KB, 2,   4, 512, vf_Big},
                               {256l*KB, 2,   4, 512, vf_Big},
                               {512l*KB, 3,   8, 512, vf_Big},
                               {  1l*MB, 4,  16, 512, vf_Big},
                               {  2l*MB, 5,  32, 512, vf_Big},
                               {  4l*MB, 6,  64, 512, vf_Big},
                               {  8l*MB, 7,  64, 512, vf_Big+vf_NoDOSPartition},
                            {0xFFFFFFFF, 7,  64, 512, vf_Big+vf_NoDOSPartition}
                           };



BPB  BPB_Minimum = {512, 1, 1, 1, 16,  4,    0xF0,        1,  0, 0, 0, 0};
BPB  BPB_160KB   = {512, 1, 1, 2, 64,  320,  MEDIA_160KB, 1,  8, 1, 0, 0};
BPB  BPB_180KB   = {512, 1, 1, 2, 64,  360,  MEDIA_180KB, 2,  9, 1, 0, 0};
BPB  BPB_320KB   = {512, 2, 1, 2, 112, 640,  MEDIA_320KB, 2,  8, 2, 0, 0};
BPB  BPB_360KB   = {512, 2, 1, 2, 112, 720,  MEDIA_360KB, 2,  9, 2, 0, 0};
BPB  BPB_12MB    = {512, 1, 1, 2, 224, 2400, MEDIA_12MB,  7, 15, 2, 0, 0};
BPB  BPB_720KB   = {512, 2, 1, 2, 112, 1440, MEDIA_720KB, 3,  9, 2, 0, 0};
BPB  BPB_144MB   = {512, 1, 1, 2, 224, 2880, MEDIA_144MB, 9, 18, 2, 0, 0};
BPB  BPB_288MB   = {512, 2, 1, 2, 240, 5760, MEDIA_288MB, 9, 36, 2, 0, 0};

PBPB DummyBPB = 0L;                     /* Pointer to BPB for drive aliasing*/

NPBPB InitBPBArray[MAX_DRIVE_LETTERS]={0}; /* BPB array returned in INIT packet*/

/*  UnitCBs are allocated after all static data                              */
UNITCB          FirstUnitCB[1]={0};     /* First UnitCB allocated here       */

USHORT          CBPool[24*KB]={0};      /* Pool for Control blocks           */




