/*static char *SCCSID = "@(#)dmdefs.h	6.5 92/02/06";*/
/*------------------------------*/
/* Common typedefs              */
/*------------------------------*/
typedef struct _VolumeControlBlock *NPVOLCB, FAR *PVOLCB;
typedef struct _UnitControlBlock   UNITCB, *NPUNITCB;
typedef struct _FaultTolCB         *NPFTCB;
typedef struct _VolChars           *NPVOLCHARS;
typedef struct _BPB                BPB;
typedef struct _DOSBOOTREC         DOSBOOTREC, _far *PDOSBOOTREC, *NPDOSBOOTREC;
typedef struct _PB_Read_Write      PB_Read_Write, FAR *PPB_Read_Write;
typedef struct _Req_List_Header    Req_List_Header, FAR *PReq_List_Header;
typedef struct _PVDB               PVDB, *NPPVDB;

typedef struct _INITDATA     INITDATA;
typedef struct _PARTITIONTABLE PARTITIONTABLE;
typedef struct _DISKTABLE_ENTRY DISKTABLE_ENTRY;

#define MAX_FIXED_DISKS         24
#define MAX_PARTITIONS          24
#define MAX_DRIVE_LETTERS       26
#define MAX_QUEUING_COUNT       16
#define NUM_DEFAULT_CWAS         4
#define MAX_ADAPTERS_PER_ADD    16
#define MAX_UNITS_PER_ADD       32
#define INIT_POOL_SIZE          32 * 1024

/*** BPB Media Descriptor constants ***/
#define MEDIA_FIXED_DISK  0xF8      /* Fixed Disk                        */
#define MEDIA_720KB       0xF9      /* 3.5 inch / 720KB             2/9  */
#define MEDIA_144MB       0xF0      /* 3.5 inch / 1.44 MB          2/18  */
#define MEDIA_288MB       0xF0      /* 3.5 inch / 2.88 MB          2/36  */
#define MEDIA_12MB        0xF9      /* 96TPI hi density drive      2/15  */
#define MEDIA_180KB       0xFC      /* 48TPI 9 sector single sided  1/9  */
#define MEDIA_360KB       0xFD      /* 48TPI 9 sector double sided  2/9  */
#define MEDIA_160KB       0xFE      /* 48TPI 8 sector single sided  1/8  */
#define MEDIA_320KB       0xFF      /* 48TPI 8 sector               2/8  */

/*** Device Types returned in Get Device Parms IOCTL */
#define TYPE_360KB        0x00
#define TYPE_12MB         0x01
#define TYPE_720KB        0x02
#define TYPE_FIXED_DISK   0x05
#define TYPE_144MB        0x07
#define TYPE_OTHER        0x08
#define TYPE_288MB        0x09


/* Change Media Flags */
#define MEDIA_CHANGED           0xFF
#define MEDIA_UNSURE_CHANGED    0x00
#define MEDIA_UNCHANGED         0x01


/*** Partition type constants ***/
#define PARTITION_16M         0x01
#define PARTITION_16Mto32M    0x04
#define PARTITION_EBR         0x05
#define PARTITION_32M         0x06
#define PARTITION_IFS         0x07
#define PARTITION_FTACTIVE    0x87
#define PARTITION_FTINACTIVE  0xC7

/** Indices for CurIndex field **/

#define INDEX99      0x00           /* 96tpi in 96tpi drive   (5.25") */
#define INDEX49      0x01           /* 48tpi in 96tpi drive   (5.25") */
#define INDEX44      0x02           /* 48tpi in 48tpi drive   (5.25") */
#define INDEX77      0x03           /* 720 KB in 720KB        (3.5")  */
#define INDEX71      0x04           /* 720 KB in 1.44 MB      (3.5")  */
#define INDEX11      0x05           /* 1.44 MB in 1.44 MB     (3.5")  */

#define MAXINDEX     INDEX11        /* Max. index supported for AT or 7552 */


#define DISKTABLECOUNT          9


/*----------------------------------------------------*/
/* Global DDFlags defines                             */
/*----------------------------------------------------*/
#define DDF_NO_MEDIA            0x00000001    /* Medialess system */
#define DDF_INIT_TIME           0x00000002    /* Init time flag           */
#define DDF_FT_ENABLED          0x00000004    /* Fault Tolerance enabled  */
#define DDF_DMAReadBack         0x00000008    /* DMA Readback 1=on, 0=off */
#define DDF_DsktSuspended       0x00000010    /* 1=dskt suspended, 0=resumed */
#define DDF_ELEVATOR_DISABLED   0x00000020    /* Elevator sort disabled      */
#define DDF_PRTYQ_DISABLED      0x00000040    /* Priority queuing disabled   */


/* MACROS */

#define ENABLE  _asm {sti}
#define DISABLE _asm {cli}
#define PUSHFLAGS _asm {pushf}
#define POPFLAGS _asm {popf}

