/*static char *SCCSID = "@(#)reqpkt.h	6.2 92/01/19";*/

/*---------------------------------*/
/* Misc constants                  */
/*---------------------------------*/
#define MAX_DISKDD_CMD          29

/*******************************/
/* Device Driver Header        */
/*******************************/

typedef struct _DDHDR  {             /* DDH */

  PVOID  NextHeader;
  USHORT DevAttr;
  USHORT StrategyEP;
  USHORT InterruptEP;
  UCHAR  DevName[8];
  USHORT ProtModeCS;
  USHORT ProtModeDS;
  USHORT RealModeCS;
  USHORT RealModeDS;
  ULONG  SDevCaps;                   /* bit map of DD /MM restrictions */
} DDHDR;

/*******************************/
/* BIOS Parameter Block        */
/*******************************/

typedef struct _BPB  {                  /* BPB */

  USHORT        BytesPerSector;
  UCHAR         SectorsPerCluster;
  USHORT        ReservedSectors;
  UCHAR         NumFATs;
  USHORT        MaxDirEntries;
  USHORT        TotalSectors;
  UCHAR         MediaType;
  USHORT        NumFATSectors;
  USHORT        SectorsPerTrack;
  USHORT        NumHeads;
  ULONG         HiddenSectors;
  ULONG         BigTotalSectors;
  UCHAR         Reserved_1[6];
} BPB, FAR *PBPB, *NPBPB;

typedef BPB    near *BPBS[];         /* An array of NEAR BPB Pointers */
typedef BPBS   far  *PBPBS;          /* A pointer to the above array  */

/***************************/
/*  Request Packet Header  */
/***************************/
typedef struct _RPH  RPH;
typedef struct _RPH  FAR *PRPH;
typedef struct _RPH  *NPRPH;

typedef struct _RPH  {                  /* RPH */

  UCHAR         Len;
  UCHAR         Unit;
  UCHAR         Cmd;
  USHORT        Status;
  UCHAR         Flags;
  UCHAR         Reserved_1[3];
  PRPH          Link;
} RPH;


/* Status word in RPH */
#define STERR        0x8000           // Bit 15 - Error
#define STINTER      0x0400           // Bit 10 - Interim character
#define STBUI        0x0200           // Bit  9 - Busy
#define STDON        0x0100           // Bit  8 - Done
#define STECODE      0x00FF           // Error code
#define WRECODE      0x0000

#define STATUS_DONE       0x0100
#define STATUS_ERR_UNKCMD 0x8003

/* Bit definitions for Flags field in RPH */
#define RPF_Int13RP         0x01        /* Int 13 Request Packet           */
#define RPF_CallOutDone     0x02        /* Int 13 Callout completed        */
#define RPF_PktDiskIOTchd   0x04        /* Disk_IO has touched this packet */
#define RPF_CHS_ADDRESSING  0x08        /* CHS Addressing used in RBA field*/
#define RPF_Internal        0x10        /* Internal request packet command */
#define RPF_TraceComplete   0x20        /* Trace completion flag           */

/************************/
/*  Init Request Packet */
/************************/

typedef struct _RPINIT  {               /* RPINI */

  RPH           rph;
  UCHAR         Unit;
  PFN           DevHlpEP;
  PSZ           InitArgs;
  UCHAR         DriveNum;
} RPINITIN, FAR *PRPINITIN;

typedef struct _RPINITOUT  {            /* RPINO */

  RPH           rph;
  UCHAR         Unit;
  USHORT        CodeEnd;
  USHORT        DataEnd;
  PBPBS         BPBArray;
  USHORT        Status;
} RPINITOUT, FAR *PRPINITOUT;


typedef struct _DDD_Parm_List {     /* DDPL */
  USHORT        cache_parm_list;    /* address of InitCache_Parameter List */
  USHORT        disk_config_table;  /* address of disk_configuration_table */
  USHORT        int_req_vec_table;  /* address of IRQ_Vector_Table         */
  USHORT        cmd_line_args;      /* address of command line parms       */
} DDD_Parm_List, FAR *PDDD_Parm_List;


#ifndef INCL_INITRP_ONLY
/*******************************/
/*  Media Check Request Packet */
/*******************************/

typedef struct _RP_MEDIACHECK  {        /* RPMC */

  RPH           rph;
  UCHAR         MediaDescr;
  UCHAR         rc;
  PSZ           PrevVolID;
} RP_MEDIACHECK, FAR *PRP_MEDIACHECK;

/*******************************/
/*  Build BPB                  */
/*******************************/
typedef struct _RP_BUILDBPB  {          /* RPBPB */

  RPH           rph;
  UCHAR         MediaDescr;
  ULONG         XferAddr;
  PBPB          bpb;
  UCHAR         DriveNum;
} RP_BUILDBPB, FAR *PRP_BUILDBPB;


/*******************************/
/*  Read, Write, Write Verify  */
/*******************************/
typedef struct _RP_RWV  {               /* RPRWV */

  RPH           rph;
  UCHAR         MediaDescr;
  ULONG         XferAddr;
  USHORT        NumSectors;
  ULONG         rba;
  USHORT        sfn;
} RP_RWV, FAR *PRP_RWV;


/*******************************/
/*  Nondestructive Read        */
/*******************************/
typedef struct _RP_NONDESTRUCREAD  {    /* RPNDR */

  RPH           rph;
  UCHAR         character;
} RP_NONDESTRUCREAD, *RPR_NONDESTRUCREAD;


/*******************************/
/*  Open/Close Device          */
/*******************************/
typedef struct _RP_OPENCLOSE  {         /* RPOC */

  RPH           rph;
  USHORT        sfn;
} RP_OPENCLOSE, FAR *PRP_OPENCLOSE;


/*************************/
/*  IOCTL Request Packet */
/*************************/

typedef struct _RP_GENIOCTL  {          /* RPGIO */

  RPH           rph;
  UCHAR         Category;
  UCHAR         Function;
  PUCHAR        ParmPacket;
  PUCHAR        DataPacket;
  USHORT        sfn;
} RP_GENIOCTL, FAR *PRP_GENIOCTL;


/*******************************/
/*  Partitionable Fixed Disks  */
/*******************************/
typedef struct _RP_PARTFIXEDDISKS  {    /* RPFD */

  RPH           rph;
  UCHAR         NumFixedDisks;
} RP_PARTFIXEDDISKS, FAR *PRP_PARTFIXEDDISKS;

/*******************************/
/*  Get Unit Map               */
/*******************************/
typedef struct _RP_GETUNITMAP  {        /* RPUM */

  RPH           rph;
  ULONG         UnitMap;
} RP_GETUNITMAP, FAR *PRP_GETUNITMAP;


/**********************************/
/*  Get Driver Capabilities  0x1D */
/**********************************/
typedef struct _RP_GETDRIVERCAPS  {     /* RPDC */

  RPH           rph;
  UCHAR         Reserved[3];
  P_DriverCaps  pDCS;
  P_VolChars    pVCS;

} RP_GETDRIVERCAPS, FAR *PRP_GETDRIVERCAPS;


#endif
