/*static char *SCCSID = "@(#)dmgencb.h	6.5 92/02/06";*/
/*--------------------------------------------------------------------*/
/*                                                                    */
/*      Control Blocks for OS2DASD.SYS                                */
/*                                                                    */
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
/*                                                                    */
/* Volume Control Block                                               */
/* --------------------                                               */
/*                                                                    */
/* This control blocks relates OS/2 logical drive letters (A:, B: C:) */
/* and OS/2 physical drive numbers (0, 1, 80h...9fh) to a unit        */
/* control block.                                                     */
/*                                                                    */
/*--------------------------------------------------------------------*/


typedef struct _VolumeControlBlock
{
  NPVOLCB     pNextVolCB;               /* Next Volume Control Blk           */
  NPUNITCB    pUnitCB;                  /* Assoc. Unit Control Blk           */
  NPVOLCHARS  pVolChar;                 /* Assoc. Volume Characteristics Blk */
  USHORT      LogDriveNum;              /* Logical  Drive Number             */
  USHORT      PhysDriveNum;             /* Physical Drive Number             */
  USHORT      FT_PartitionNumber;       /* Fault Tolerance partition number  */
  USHORT      Flags;                    /*                                   */
  UCHAR       PartitionType;            /* Partition Type                    */
  UCHAR       Reserved_1;               /* Reserved for word alignment       */
  ULONG       PartitionOffset;          /* Partition Tbl Offset (RBA)        */
  ULONG       VolumeOffset;             /* Volume start offset  (RBA)        */
  ULONG       NumPhysCylinders;         /* Number cylinders on device        */
  ULONG       NumLogCylinders;          /* Number cylinders on logical drive */
  BPB         RecBPB;                   /* Recommended BPB for drive         */
  UCHAR       Reserved_2;               /* Reserved for word alignment       */
  BPB         MediaBPB;                 /* Current media BPB in drive        */
  UCHAR       Reserved_3;               /* Reserved for word alignment       */

} VOLCB;


/* Flags defines in VOLCB */
#define vf_NoDOSPartition       0x00000001
#define vf_TooBig               0x00000002
#define vf_Big                  0x00000004
#define vf_InvBigPart           0x00000008
#define vf_BigFat               0x00000010
#define vf_ReturnFakeBPB        0x00000020
#define vf_ForceRdWrt           0x00000040
#define vf_UncertainMedia       0x00000080
#define vf_ChangedByFormat      0x00000100
#define vf_Changed              0x00000200
#define vf_OwnPhysical          0x00000400
#define vf_AmMult               0x00000800
#define vf_FTPartition          0x00001000

/*--------------------------------------------------------------------*/
/*                                                                    */
/* Unit Control Block                                                 */
/* ------------------                                                 */
/*                                                                    */
/* This control blocks relates hardware devices to the Adapter        */
/* Dependent Driver which manages them.                               */
/*                                                                    */
/*--------------------------------------------------------------------*/

typedef struct _PRTYQ
{
  PBYTE    Head;
  PBYTE    Tail;
} PRTYQ;

#define  NUM_RLE_QUEUES        9
#define  NUM_RP_QUEUES         2

typedef struct _UnitControlBlock
{
  NPUNITCB    pNextUnitCB;                /* Pointer to Next UnitCB       */
  NPVOLCB     pCurrentVolCB;              /* Currently attached VolCB     */
  USHORT      Flags;                      /* UnitCB Flags                 */
  USHORT      PhysDriveNum;               /* Physical Drive Number        */
  UNITINFO    UnitInfo;                   /* Additional Driver Info       */
  USHORT      ADDHandle;                  /* ADD handle                   */
  USHORT      AdapterNumber;              /* Logical Adapter Number       */
  ULONG       LastRBA;                    /* Last RBA for this unit       */
  USHORT      MaxHWSGList;                /* Max HW scatter/gather list   */
  NPIORBH     pDedicatedIORB;             /* Dedicated IORB for this unit */
  VOID (FAR * AdapterDriverEP)();         /* ADD Entry Point              */
  USHORT      NumReqsInProgress;          /* # of requests in progress    */
  USHORT      NumReqsWaiting;             /* Number of requests waiting   */
  NPIORBH     InProgressQueue;            /* Queue of in progress requests*/
  PRTYQ       PrtyQRLE[NUM_RLE_QUEUES];   /* Priority Queues for RLE's    */
  PRTYQ       PrtyQRP[NUM_RP_QUEUES];     /* Priority Queues for RP's     */
  PVDB        PerfViewDB;                 /* PerfView data block          */

} UNITCB;

/* Flag definitions for Flags field in UNITCB */
#define UCF_IORB_ALLOCATED  0x0001      /* Dedicated IORB allocated       */
#define UCF_HW_SCATGAT      0x0002      /* Unit supports ScatGat in HW    */
#define UCF_16M             0x0004      /* Unit supports > 16M addressing */
#define UCF_CHS_ADDRESSING  0x0008      /* Unit supports CHS Addressing   */
#define UCF_REMOVABLE_NON_FLOPPY  0x0010  /* Unit supports CHS Addressing */


/*--------------------------------------------------------------------*/
/* IORB DMWorkSpace Structure                                         */
/*--------------------------------------------------------------------*/
typedef struct _IORB_DMWORK
{
   NPUNITCB      pUnitCB;
   USHORT        Reserved_1;
   PBYTE         pRequest;
   SCATGATENTRY  SGList;
   ULONG         Reserved_2;

} IORB_DMWORK, FAR *PIORB_DMWORK, NEAR *NPIORB_DMWORK;

typedef struct _PARTITIONENTRY
{
   UCHAR        BootIndicator;
   UCHAR        BegHead;
   USHORT       BegSector:6;
   USHORT       BegCylinder:10;
   UCHAR        SysIndicator;
   UCHAR        EndHead;
   UCHAR        EndSector;
   UCHAR        EndCylinder;
   ULONG        RelativeSectors;
   ULONG        NumSectors;
} PARTITIONENTRY;


typedef struct _MBR
{
   UCHAR          Pad[0x1BE];
   PARTITIONENTRY PartitionTable[4];
   USHORT         Signature;
} MBR;


typedef struct _PARTITIONTABLE
{
   PARTITIONENTRY   PartitionTable[4];  /* buffer for partition table        */
   USHORT           Signature;
   UCHAR            Bad_MBR;            /* 0 = MBR good, 1 = MBR is no good  */
   UCHAR            No_PrimPart;        /* 0 = PP exits, 1 = no PP exists    */

} PARTITIONTABLE;

typedef struct _DOSBOOTREC
{
   UCHAR          JmpCode;
   UCHAR          Reserved_1;
   UCHAR          nop;
   UCHAR          Name[3];
   UCHAR          Release[5];
   BPB            bpb;
} DOSBOOTREC;


typedef struct _DRIVERENTRY
{
   UCHAR        DriverName[17];
   PFN          AdapterDriverEP;
} DRIVERENTRY;


typedef struct _DRIVERTABLE
{
   USHORT       NumDrivers;
   DRIVERENTRY  DriverEntry[MAX_DRIVERS];
} DRIVERTABLE;


typedef struct _FaultTolCB
{
UCHAR   Dummy;

} FaultTolCB;


typedef struct _DISKTABLE_ENTRY
{
   ULONG   NumSectors;
   UCHAR   Log2SectorsPerCluster;
   UCHAR   SectorsPerCluster;
   USHORT  MaxDirEntries;
   USHORT  Flags;

} DISKTABLE_ENTRY;


/*--------------------------------------------------------------*/
/* Init data allocated at the end of the data segment has the   */
/* following structure:                                         */
/*--------------------------------------------------------------*/

#define MAX_DEVICE_TABLE_SIZE 1024
//      (sizeof(DEVICETABLE) + ((MAX_ADAPTERS_PER_ADD - 1) * 2) +
//      ((sizeof(ADAPTERINFO) - sizeof(UNITINFO)) * MAX_ADAPTERS_PER_ADD) +
//      sizeof(UNITINFO) * MAX_UNITS_PER_ADD)

typedef struct _INITDATA
{
   UCHAR           ScratchBuffer2[MAX_DEVICE_TABLE_SIZE];  /* Scratch buffer  */
   UCHAR           ScratchIORB[MAX_IORB_SIZE]; /* IORB used during init       */
   RP_RWV          InitPkt;                    /* RP used during init         */
   PARTITIONTABLE  PartitionTables[MAX_FIXED_DISKS];  /* Array of partition tables */
} INITDATA;





