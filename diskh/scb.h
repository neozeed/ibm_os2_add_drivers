/*static char *SCCSID = "@(#)scb.h	6.1 92/01/08";*/
/**********************************************************************/
/* Subsystem Control Block Structure                                  */
/**********************************************************************/

typedef struct _SCB      SCB;
typedef struct _SCB FAR *PSCB;
typedef struct _SCB     *NPSCB;

typedef struct _TSB      TSB;
typedef struct _TSB FAR *PTSB;
typedef struct _TSB     *NPTSB;

typedef struct _SCBHDR      SCBHDR;
typedef struct _SCBHDR     *NPSCBHDR;
typedef struct _SCBHDR FAR *PSCBHDR;
/* ASM
PSCBHDR struc
        dd      ?
PSCBHDR ends
*/

typedef struct _SCBFORMAT      SCBFORMAT;
typedef struct _SCBFORMAT FAR *PSCBFORMAT;
typedef struct _SCBFORMAT     *NPSCBFORMAT;

/*********************************/
/*  ABIOS SCB Header             */
/*********************************/
typedef struct _SCBHDR {                /* ABSCB */

  USHORT        Reserved_1;             /* Reserved                        */
  PSCBHDR       pNextSCBHdr;            /* Logical ptr to next SCB header  */
  USHORT        Reserved_2[2];          /* Reserved                        */
  PTSB          pTSB;                   /* Logical ptr to TSB              */
  USHORT        Reserved_3[1];          /* reserved                        */
} SCBHDR;


/****************************************/
/*  SCSI Subsystem Control Block (SCB)  */
/****************************************/

typedef struct _SCB {                   /* SCB */

  USHORT        Cmd;                    /* SCB Command Code                   */
  USHORT        Enable;                 /* SCB Enable Word                    */
  ULONG         LBA;                    /* Logical Block Addr                 */
  ULONG         ppXferBuf;              /* Physical pointer to transfer buf   */
                                        /*  or scatter/gather list            */
  ULONG         XferBufLen;             /* Length of transfer or addr list    */
  ULONG         ppTSB;                  /* Physical pointer to TSB            */
  ULONG         ppNxtSCB;               /* Physical ptr to next SCB on chain  */
  union {

      struct _BLK {                     /* SCB */

          USHORT        BlockCnt;       /* Block count                        */
          USHORT        BlockSize;      /* Block length                       */
        } BLK;
      struct _CDB {                     /* SCB */

          UCHAR        SCSIcdb[12];
        } CDB;
    } EXT;
} SCB;


/**************************/
/* SCB Variant for FORMAT */
/**************************/

typedef struct _SCBFORMAT {             /* SCBFMT */

  USHORT        Cmd;                    /* SCB Command Code                   */
  USHORT        Enable;                 /* SCB Enable Word                    */
  USHORT        ModBits;                /* Modifier Bits  FD/CL               */
  USHORT        Interleave;             /* Interleave Factor                  */
  ULONG         ppXferBuf;              /* Physical pointer to transfer buf   */
                                        /*  or scatter/gather list            */
  ULONG         XferBufLen;             /* Length of transfer or addr list    */
  ULONG         ppTSB;                  /* Physical pointer to TSB            */
  ULONG         ppNxtSCB;               /* Physical ptr to next SCB on chain  */
  union {

      struct _BLK2 {                    /* SCBFMT */

          USHORT        BlockCnt;       /* Block count                        */
          USHORT        BlockSize;      /* Block length                       */
        } BLK2;
    } EXT;
} SCBFORMAT;

/****************/
/* SCBCmd codes */
/****************/

#define SCBREAD         0x1C01          /* SCB Read                           */
#define SCBWRITE        0x1C02          /* SCB Write                          */
#define SCBREADV        0x1C03          /* SCB Read with Verify               */
#define SCBWRITEV       0x1C04          /* SCB Write with Verify              */
#define SCBCMDSTATUS    0x1C07          /* SCB Get Command Complete Status    */
#define SCBCMDSENSE     0x1C08          /* SCB Req SCSI Sense Command         */
#define SCBDEVICECAP    0x1C09          /* SCB Read Device Capacity           */
#define SCBDEVICEINQ    0x1C0B          /* SCB Device Inquiry                 */
#define SCBREASSIGNBLK  0x1C18          /* SCB Reassign Block                 */
#define SCBMAXLBA       0x1C1A          /* SCB Specify Maximum LBA            */
#define SCBSENDOTHER    0x241F          /* SCB Send Other SCSI Command        */
#define SCBPREFETCH     0x1C31          /* SCB Prefetch                       */
#define SCBFORMATUNIT   0x1C16          /* SCB Format Unit                    */

/****************************************/
/* SCBEnable word bit flag definitions  */
/****************************************/
#define SCBEfRD         0x8000       /* I/O Control:  1=read,                 */
                                     /*               0=write                 */
#define SCBEfES         0x4000       /* Return TSB:   1=only on error,        */
                                     /*               0=always                */
#define SCBEfRE         0x2000       /* Retry enable: 1=enable retries,       */
                                     /*               0=disable retries       */
#define SCBEfPT         0x1000       /* SGList: 1=SCBXferBuf is SGList        */
                                     /*         0=SCBXferBuf is SCBXferBuf    */
#define SCBEfSS         0x0400       /* Suppress except: 1=suppress error     */
                                     /*                  0=don't suppress     */
#define SCBEfBB         0x0200       /* Bypass cache: 1=bypass cache          */
                                     /*               0=don't bypass          */
#define SCBEfCC         0x0001       /* Chain Condition: 1=chain              */
                                     /*                  0=no chain           */
/****************************************/
/* SCBEnable default word definitions   */
/****************************************/

#define SCBEWREAD            SCBEfRD+SCBEfES+SCBEfRE+SCBEfPT
#define SCBEWWRITE           SCBEfES+SCBEfRE+SCBEfPT
#define SCBEWREADV           SCBEfRD+SCBEfES+SCBEfRE+SCBEfBB
#define SCBEWWRITEV          SCBEfES+SCBEfRE+SCBEfPT
#define SCBEWCMDSTATUS       SCBEfRD+SCBEfES+SCBEfRE+SCBEfBB
#define SCBEWCMDSENSE        SCBEfRD+SCBEfES+SCBEfRE+SCBEfSS
#define SCBEWDEVICECAP       SCBEfRD+SCBEfES+SCBEfRE+SCBEfBB
#define SCBEWDEVICEINQ       SCBEfRD+SCBEfES+SCBEfRE+SCBEfSS+SCBEfBB
#define SCBEWREASSIGNBLK     SCBEfES+SCBEfRE+SCBEfBB
#define SCBEWMAXLBA          0
#define SCBEWSENDOTHER       SCBEfES+SCBEfRE+SCBEfPT+SCBEfBB
#define SCBEWPREFETCH        SCBEfRD+SCBEfES+SCBEfRE+SCBEfBB
#define SCBEWFORMATUNIT      SCBEfES+SCBEfRE+SCBEfBB
#define SCBEWDEFAULT         SCBEfES+SCBEfRE


/****************************/
/* Termination Status Block */
/****************************/

typedef struct _TSB {                /* TSB */

  USHORT        Status;              /* Ending status                         */
  USHORT        Retries;             /* Retry count                           */
  ULONG         ResidCnt;            /* Residual byte count                   */
  ULONG         ppResidBuf;          /* Residual physical buffer addr         */
  USHORT        StatusLen;           /* Additional status length              */
  UCHAR         SCSIStatus;          /* SCSI Status                           */
  UCHAR         CmdStatus;           /* Command status                        */
  UCHAR         DevError;            /* Device error code                     */
  UCHAR         CmdError;            /* Command error code                    */
  USHORT        DiagMod;             /* Diagnostic error modifier             */
  USHORT        CacheInfo;           /* Cache info word                       */
  ULONG         ppLastSCB;           /* Physical ptr to last SCB processed    */
} TSB;


/****************************/
/* TSB End Status Bit Flags */
/****************************/

#define TSBSfNOERR      0x0001       /* No error has occured                  */
#define TSBSfSHORT      0x0002       /* Short length record encountered       */
#define TSBSfSPECCHK    0x0010       /* SCB Specification Check               */
#define TSBSfLONG       0x0020       /* Long record encountered               */
#define TSBSfHALT       0x0040       /* SCB Chain halted                      */
#define TSBSfINTREQ     0x0080       /* SCB Interrupt Requested               */
#define TSBSfRESIDOK    0x0100       /* Resid buffer data there               */
#define TSBSfSTATF      0x0200       /* Up to word 0F of TSB valid            */
#define TSBSfSTATX      0x0300       /* Extended TSB Format Stored            */
#define TSBSfOVERRUN    0x0400       /* Device Overrun                        */
#define TSBSfNOTINIT    0x0800       /* Device not initialized                */
#define TSBSfEXCEPT     0x1000       /* Major exception has occurred          */
#define TSBSfCHDIR      0x2000       /* Chain direction bit                   */
#define TSBSfSUSPEND    0x4000       /* SCB Suspended                         */
#define TSBSfXSTAT      0x8000       /* Extend end status word                */




/*******************************/
/* Various equates             */
/*******************************/

