/*static char *SCCSID = "@(#)dmdata.h	6.6 92/02/06";*/
/*--------------------------------------------*/
/* External data declarations in DDDATA.C     */
/*--------------------------------------------*/
extern UCHAR       ScratchBuffer[512];  /* Scratch buffer for I/O         */
extern PFN         Device_Help;         /* far ptr to devhelp function    */
extern PBYTE       pSysInfoSeg;         /* Pointer to sys info seg        */
extern PBYTE       pSIS_mec_table;      /* Pointer to RAS MEC Trace Table */
extern PVOID       pDataSeg;            /* virt ptr of our data segment   */
extern ULONG       ppDataSeg;           /* phys addr of our data segment  */
extern ULONG       plDataSeg;           /* linear addr of our data seg    */
extern ULONG       ppScratchBuffer;     /* Phys addr of ScratchBuffer     */
extern USHORT      ScratchBufSem;       /* Semaphore for ScratchBuffer    */
extern USHORT      DDFlags;             /* Global driver flags            */
extern NPUNITCB    UnitCB_Head;         /* near ptr to first UnitCB       */
extern NPVOLCB     VolCB_Head;          /* near ptr to first VolCB        */
extern NPVOLCB     pVolCB_DriveA;       /* Pointer to A: VolCB            */
extern NPVOLCB     pVolCB_DriveB;       /* Pointer to B: VolCB            */
extern NPVOLCB     pVolCB_DriveC;       /* Pointer to C: VolCB            */
extern NPVOLCB     pVolCB_80;           /* Pointer to VolCB for drive 80H */
extern USHORT      NextLogDriveNum;     /* next logical drive number      */
extern NPBYTE      pNextFreeCB;         /* ptr to next free control blk   */
extern NPVOLCB     pLastLogVolCB;
extern USHORT      NextLogDriveNum;
extern USHORT      NumDrivers;          /* number of adapter drivers      */
extern USHORT      NumUnitCBs;          /* number of unit control blocks  */
extern USHORT      NumVolCBs;           /* number of volume control blocks*/
extern USHORT      NumLogDrives;        /* number of logical drives       */
extern USHORT      NumRemovableDisks;   /* number of removable disk drives*/
extern USHORT      NumFixedDisks;       /* number of fixed disk drives    */
extern USHORT      NumPartitions;       /* number of fixed disk partitions*/
extern USHORT      NumFTPartitions;     /* number of fault tol. partitions*/
extern NPVOLCB     pExtraVolCBs;        /* pointer to extra volcbs        */
extern USHORT      NumExtraVolCBs;      /* number of extra volcbs         */
extern USHORT      NumAdapters;         /* number of adapters             */
extern USHORT      NumReqsInProgress;   /* num requests in progress       */
extern USHORT      NumReqsWaiting;      /* num requests on waiting queues */
extern USHORT      TraceFlags;          /* Trace Flags                    */
extern NPBYTE      pDMTraceBuf;         /* pointer to internal trace buffer*/
extern NPBYTE      pDMTraceHead;        /* pointer to head of trace buffer */
extern NPBYTE      pDMTraceEnd;         /* pointer to end of trace buffer */
extern NPBYTE      CB_FreeList;         /* Control Block Free List for    */
                                        /*  IORBs and CWAs                */
extern USHORT      PoolSem;             /* Pool semaphore                 */
extern USHORT      PoolSize;            /* Size of control block pool     */
extern USHORT      FreePoolSpace;       /* Free space left in data segment*/
extern UCHAR       fBigFat;             /* flags for drives               */
extern UCHAR       XActPDrv;
extern PVOID       pFSD_EndofInt;       /* FSD's End of interrupt routine */
extern PVOID       pFSD_AccValidate;    /* FSD's Access Validation routine*/
extern PVOID       pDiskFT_Request;     /* DISKFT's Request routine       */
extern PVOID       pDiskFT_Done;        /* DISKFT's Done routine          */
extern USHORT      DiskFT_DS;           /* DISKFT's DS selector           */


extern DriverCaps  DriverCapabilities;  /* Driver Capabilities structure  */
extern DISKTABLE_ENTRY DiskTable[];

extern BPB         BPB_Minimum;
extern BPB         BPB_160KB;
extern BPB         BPB_180KB;
extern BPB         BPB_320KB;
extern BPB         BPB_360KB;
extern BPB         BPB_12MB;
extern BPB         BPB_720KB;
extern BPB         BPB_144MB;
extern BPB         BPB_288MB;

extern PBPB        DummyBPB;            /* Pointer to BPB for drive aliasing*/
extern NPBPB       InitBPBArray[30];    /* BPB array returned in INIT packet */
extern UNITCB      FirstUnitCB[1];      /* First UnitCB allocated here    */
extern UCHAR       CBPool[];            /* Start of Pool                  */
extern USHORT      InitData;            /* Start of Init data             */
