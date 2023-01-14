/*static char *SCCSID = "@(#)s506hdr.h	6.4 92/01/21";*/
/****************************************************************************
 *                                                                          *
 *                (c) Copyright  IBM Corporation  1981, 1990                *
 *                           All Rights Reserved                            *
 *                                                                          *
 ****************************************************************************/

/********************* Start of Specifications ******************************
 *                                                                          *
 *  Source File Name: S506HDR.H                                             *
 *                                                                          *
 *  Descriptive Name: Locally defined structures for this driver.           *
 *                                                                          *
 *  Copyright:                                                              *
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
 *  Entry Points:                                                           *
 *                                                                          *
 *  External References:                                                    *
 *                                                                          *
 ********************** End of Specifications *******************************/
#define SUCCESS  FALSE    /* Success means no error            */
#define FAILURE  TRUE     /* Failure means there is an error   */
#define STATUS_ERR_GENFAIL  0x800C /* Req Pkt Status of General Failure */
#define ABIOS_NOT_PRESENT   0x8003

#define INT3 _asm{int 3}
#define DISABLE _asm{cli}
#define ENABLE _asm{sti}
#define IOWait _asm{                            \
  _asm          jmp     short IOWait_1          \
  _asm  IOWait_1:                               \
  _asm          jmp     short IOWait_2          \
  _asm  IOWait_2:                               \
}

// Locations in ROM data area of fixed disk parameter tables on an AT

#define FDTAB_0      0x0041 * 4    // First fixed disk  - Int 41H vector
#define FDTAB_1      0x0046 * 4    // Second fixed disk - Int 46H vector

// Hard disk controller commands

#define FX_NORETRY   0x0001        // do not perform retries on i/o
#define FX_ECC       0x0002        // ECC mode during i/o
#define FX_RECAL     0x0010        // recalibrate
#define FX_CREAD     0x0020        // read
#define FX_CWRITE    0x0030        // write
#define FX_VERIFY    0x0040        // verify track.
#define FX_SETP      0x0091        // Set parameters
#define FX_CPWRMODE  0x00E5        // Check if drive powered on
#define FX_IDENTIFY  0x00EC        // Identify drive
#define FX_CREADMUL  0x00C4        // Read Multiple
#define FX_CWRITEMUL 0x00C5        // Write Multiple
#define FX_SETMUL    0x00C6        // Set Multiple Mode

// Status bits

#define FX_BUSY      0x0080        // Status port busy bit
#define MAXWAIT      0x00400000L   // Maximum loop count for port busy
#define FX_READY     0x0040        // Status port ready bit
#define FX_WRTFLT    0x0020        // Write Fault
#define FX_DRQ       0x0008        // Data Request bit
#define FX_ERROR     0x0001        // Error status

// Device Control Register bits

#define FX_IEN       0x0002        // Interrupt Enable (0 = enable; 1 = disable)
#define FX_SRST      0x0004        // Soft Reset

// Error Register bits

#define FX_AMNF      0x0001        // Address Mark Not Found
#define FX_TRK0      0x0002        // Track 0 was not found during Restore Cmd
#define FX_ABORT     0x0004        // Aborted Command
#define FX_IDNF      0x0010        // ID Not Found
#define FX_ECCERROR  0x0040        // Data ECC Error
#define FX_BADBLK    0x0080        // Bad Block was detected in the ID field

// I/O ports for AT hard drives

#define FX_PDAT      0x01F0        // read/write data
#define FX_PWRP      0x01F1        // write precompensation cylinder register
#define FX_PERR      0x01F1        // error register
#define FX_PSECCNT   0x01F2        // sector count register
#define FX_PSECNUM   0x01F3        // sector number register
#define FX_PCYLL     0x01F4        // cylinder register (low)
#define FX_PCYLH     0x01F5        // cylinder register (high)
#define FX_PDRHD     0x01F6        // drive/head register
#define FX_PCMD      0x01F7        // command register
#define FX_PSTAT     0x01F7        // status register
#define FX_RFDR      0x03F6        // fixed disk register

// Array indices (IOPorts[] and IORegs[]) for I/O port address and contents

#define FI_PDAT      0             // read/write data
#define FI_PWRP      1             // write precompensation cylinder register
#define FI_PERR      1             // error register
#define FI_PSECCNT   2             // sector count register
#define FI_PSECNUM   3             // sector number register
#define FI_PCYLL     4             // cylinder register (low)
#define FI_PCYLH     5             // cylinder register (high)
#define FI_PDRHD     6             // drive/head register
#define FI_PCMD      7             // command register
#define FI_PSTAT     7             // status register
#define FI_RFDR      8             // fixed disk register

#define MAXUNITS     0x02    // number of units per adapter
#define ffHardFile   5
#define ErrLim       5    //  Number of retries on error

// Interrupt control

#define IRQ_FIXED    0x000E  // Fixed interrupt IRQ #

// Timer values

#define TIMERCOUNT   32      // DevHelp_TickCount Reload value (call every sec)
                             // 0.031s x 32 = 0.992s

#define FIXEDT1      1       // first controller timer active

// AT State Machine Definitions

#define START        0       // Starting I/O
#define CALC         1       // Calculate position on disk
#define VERIFY       2       // Start verify portion of write
#define DONE         3       // I/O is done.
#define IDLE         4       // Drive is inactive
#define SERROR       5       // Have an error
#define RECAL        6       // do a recal
#define READ         7       // Read from disk
#define WRITE        8       // Write to disk
#define SETPARAM     9       // Set controller parameters

#define NEXTSTATENOW 20      // Continue state machine processeing
#define WAITSTATE    21      // Wait for an interrupt

typedef struct _WORDCYL {
  USHORT WordCyl;
} WORDCYL;

typedef struct _BYTECYL {
  UCHAR LowCyl;
  UCHAR HighCyl;
} BYTECYL;

typedef union _CYL {
  WORDCYL w;
  BYTECYL b;
} CYL;

typedef struct _IDENTIFYDATA
{
  USHORT           GeneralConfig;     // General configuration bits
  USHORT           TotalCylinders;    // Number of cylinders
  USHORT           Reserved;          // Reserved
  USHORT           NumHeads;          // Number of heads
  USHORT           NumUnformattedbpt; // Number of unformatted bytes per track
  USHORT           NumUnformattedbps; // Number of unformatted bytes per sector
  USHORT           SectorsPerTrack;   // Sectors per track
  USHORT           NumBytesISG;       // Number of bytes in inter-sector gap
  USHORT           NumBytesSync;      // Number of bytes in sync field
  USHORT           NumWordsVUS;       // Number of words of vendor unique status
  CHAR             SN[20];            // Serial number
  USHORT           CtrlType;          // Controller Type 0x0003
  USHORT           CtrlBufferSize;    // Ctrl buffer size in 512 byte increments
  USHORT           NumECCBytes;       // Number of ECC bytes on read/write long
  CHAR             FirmwareRN[8];     // Ctrl firmware revision
  CHAR             ModelNum[40];      // Model number
  USHORT           NumSectorsPerInt;  // Num of sectors/int on read/write multiple
  USHORT           DoubleWordIO;      // Double word I/O capability (1=yes)
  USHORT           Capabilities;      // Capabilities
  USHORT           Reserved2;         // Reserved2
  USHORT           PIOCycleTime;      // PIO data transfer cycle timing mode
  USHORT           DMACycleTime;      // DMA data transfer cycle timing mode
  USHORT           Reserved3[75];     // Reserved3
  USHORT           VendorUnique[7];   // Vendor Unique
  USHORT           BarCodeSN[6];      // IBM Bar Code and Serial Number
  USHORT           Reserved4[5];      // Reserved4
  USHORT           VendorUnique2[14]; // Vendor Unique
  USHORT           Reserved5[96];     // Reserved5

}IDENTIFYDATA;
typedef IDENTIFYDATA NEAR *NPIDENTIFYDATA;      // near pointer to identify drive data
typedef IDENTIFYDATA FAR  *PIDENTIFYDATA;       // far pointer to identify drive data

typedef struct _DRIVE
{
  UCHAR            DriveNum;          //   ?     ; INT 13H drive number
  GEOMETRY         LogGeometry;       // Unit's logical geometry
  GEOMETRY         PhysGeometry;      // Unit's physical geometry
  USHORT           WrtPrCmp;          //   0     ; hard disk write precompensation cyl
  UCHAR            FormFactor;        //   0     ; form factor index

//DISK01 specific fields

  UCHAR            fDriveSetParam;    //   1     ; Drive 80 flag for SetParam(1: yes, 0: no)
  UCHAR            ContUnit;          //   0     ; Unit on the controller (0 or 1)
  UCHAR            XlateFlags;        //   0     ; Is this a translated drive???
  UCHAR            AllocatedFlag;     //   0     ; Is this unit allocated?
  USHORT           DriveFlags;        //   0     ; Drive Characteristics
  PUNITINFO        pUnitInfo;         //   0     ; For: IOCM_CHANGE_UNITINFO
  IDENTIFYDATA     id;                //   0     ; identify drive command data

}DRIVE;
typedef DRIVE NEAR *NPDRIVE;          // near pointer to drive control block

// DriveFlags definitions

#define FMULTIPLEMODE  0x0001   // Ctrl is enabled for read/write multiple commands
#define FDWORDIO       0x0002   // Ctrl has double word I/O capability


/*----------------------------------------------------------------------------*
 *                                                                            *
 *           Adapter Control Block Structure                                  *
 *                                                                            *
 *      The following structure is used to store all information about        *
 *      an AT style disk adapter. This includes adapter specific fields       *
 *      such as i/o addresses, an i/o structure for tracking the current      *
 *      i/o operation, and a drive structure for each unit's physical geometry*
 *      and logical geometry.                                                 *
 *                                                                            *
 *      Note that the "Current" field indicates the drive currently being     *
 *      accessed / most recently accessed via the hardware.  The Unit         *
 *      field indicates the logically active unit.  In the case of single-    *
 *      drive floppy systems Unit indicates the pseudo drive in effect;       *
 *      Current indicates the physical drive in use.                          *
 *----------------------------------------------------------------------------*/

typedef struct _ACB
{
  PIORBH           pHeadIORB;      //   0  head ptr to queue of IORBs
  USHORT           Active;         //   0  semaphore

// I/O port addresses for AT hard drives

  USHORT           IOPorts[10];    // I/O port addresses for disk controller
  UCHAR            IORegs[10];     // register contents for disk controller

  USHORT           IntLevel;       // IRQ_FIXED 0x000E  Fixed interrupt IRQ #

  UCHAR            State;          //   0  driver state
  UCHAR            DrvStateReturn; //   0  driver state loop control

  USHORT           Current;        //   -1 current logical drive #
  USHORT           PhysDrv;        //   -1 current physical drive number
  USHORT           ErrCnt;         //   0  # of errors this request.
  USHORT           Flags;          //   0  various flags
  USHORT           Flags2;         //   0  more flags

// Following values are set by Setup from the request packet and are
// updated after each transfer is completed.

  ULONG            First;          //   0  1st sector of request
  ULONG            RealAddr;       //   0  saved addr when using ScratchBuffer
  ULONG            Count;          //   0  Number of sectors to xfer

// Following used for fixed disks on an AT to keep track of which sector
// has been read in or written out, since we do them one at a time

  ULONG            CurrAddr;       //   0  Pointer to data area/buffer
  USHORT           SecToGo;        //   0  Number of sectors left in request

// read/write multiple tracking

  USHORT           BlocksToGo;     //   0  Number of blocks left in request
  USHORT           LastBlockNumSec; //   0  Number of sectors on last block

// Following values are set by MapSector.

  CHS_ADDR         chs;            // Cylinder, zero based sector and Head
  USHORT           cSectors;       //   0  Number of sectors to do on sub I/O (shadow)
  UCHAR            NumSectors;     //   0  Number of sectors to do on sub I/O

  USHORT           StartSec;       //   0  IOCTL starting sector
  UCHAR            SectorSize;     //   2  Sector size index

// scatter/gather tracking

  ULONG            SGBytesToGo;    //   0  Bytes left to process in this descriptor
  USHORT           SGCurrentDesc;  //   0  index of Scatter/Gather array
  USHORT           SGStartDesc;    //   0  SG desc at start of sector I/O
  ULONG            SGStartByte;    //   0  SG descr bytes left count at sec start


  UCHAR            drivebit;       //   0  0 = first disk on ctrl, 1=second ctrl     $100
  UCHAR            XFlags;         //   0  flags for current request                 $111

  ULONG            TOCnt;          //   0  timer for current request             $150
  UCHAR            RetryCounter;   //   0  retry counter for current request     $150

  SHORT            TimerCall;      //   0  Timer call for current request        $150
  SHORT            NestedInts;     //   -1 Nested interrupt counter              $150
// $150 - We need a timer mask for each IOS in the system so that
// $150   the proper timer can be set in the routine "SetTimeoutTimer" using
// $150   register addressing for the currently active IOS.
// $150
  UCHAR            TimerMask;      //   0  Sets proper bit in TimerActive word.

  DRIVE            unit[MAXUNITS];   // device geometries for units

  USHORT           TraceBuffer[8];   // trace buffer for systrace

} ACB;
typedef ACB NEAR *NPACB;                  // near pointer to ACB control block

// Flags for IOStruc Flags field

#define FACTIVE        0x0001   //
#define FREAD          0x0002   //
#define FWRITE         0x0004   //
#define FVERIFY        0x0008   //
#define FMULTIIO       0x0010   // Multiple I/O for a single IORB request
#define FMULTITRACK    0x1000   // Multitrack Format available on device
#define FCOMPLETEINIT  0x8000   // IOCM_COMPLETE_INIT IORB processed

// Flags2 definitions

#define FSETMULTIPLE   0x0001   // device= /SMS specified (set multiple mode)
#define f16MegSup      0x0008   // >16MB buffer addr supported

// XFlags definitions

#define FTIMERINT        2
#define FINTERRUPTSTATE  4

/* Fixed Disk Parameter Table */

typedef struct hdparmtable_s {
  USHORT   hdp_wMaxCylinders;          // maximum number of cylinders
  UCHAR    hdp_bMaxHeads;              // maximum number of heads
  USHORT   hdp_wReserve1;              // reserved (not used)
  USHORT   hdp_wWritePrecompCyl;       // starting write precompensation cyl.
  UCHAR    hdp_bMaxECCDataBurstLen;    // maximum ECC data burst length
  UCHAR    hdp_bControl;               // control byte
  UCHAR    hdp_abReserve2[3];          // reserved (not used)
  USHORT   hdp_wLandingZone;           // landing zone for head parking
  UCHAR    hdp_bSectorsPerTrack;       // number of sectors per track
  UCHAR    hdp_bReserve3;              // reserved for future use
} HDPARMTABLE;
typedef HDPARMTABLE far *PHDPARMTABLE;  // pointer to hard disk parameter table


/* Hard disk parameter - control byte bit mask */

#define HDPCTRL_DISABLERETRY    0xc0    // disable retries
#define HDPCTRL_EXCEED8HEADS    0x08    // more than 8 heads


#define XLATE_ENABLE   0x0001   // set the translate enable bit
#define INTERNAL_TYPE  0x0002   // set the internal table used

#define TRANSLATE_A0   0x00A0   // Translation signature

typedef struct _FDPARMTABLE_XLATE {
  USHORT   FDMaxCylX;                   // Maximum number of cylinders
  UCHAR    FDMaxHdX;                    // Maximum number of heads
  UCHAR    FDtranslate;                 // Translate signature
  UCHAR    FDpsectors;                  // physical sectors/track
  USHORT   FDWritePCompX;               // Starting write precompensation cyl
  UCHAR    FDBUnUseX;                   // Unused on an AT
  UCHAR    FDControlX;                  // Control byte
  USHORT   FDpcyls;                     // physical cylinders
  UCHAR    FDpheads;                    // physical heads
  USHORT   FDLandZonX;                  // Landing zone
  UCHAR    FDSecTrkX;                   // Number of sectors/track
  UCHAR    FDResX;                      // Reserved
} FDPARMTABLE_XLATE;
typedef FDPARMTABLE_XLATE far *PFDPARMTABLE_XLATE;


// Disk/Diskette Configuration Data Table

typedef struct _ROMCFG {
  USHORT           ROMlink;           //   ?     link to next table entry
  USHORT           ROMcyls;           //   ?     maximum number of cylinders
  USHORT           ROMsecptrk;        //   ?     maximum sectors per track
  UCHAR            ROMdevnbr;         //   ?     device number
  USHORT           ROMheads;          //   ?     number of heads
  USHORT           ROMwpf;            //   ?     write precompensation factor
  USHORT           ROMflags;          //   ?     flag byte
} ROMCFG;
typedef ROMCFG far *PROMCFG;  // pointer to Disk/Diskette Configuration Data Table

// Flag bits for ROMflags

#define f16MegSup      0x0008   // >16MB buffer addr supported
#define ROMfixed       0x0001   // 0=diskette, 1=fixed disk
#define ROMchangeline  0x0002   // change line is supported

#define MAX_DRIVE_NUMBER  12    // Max drives supported by Kernel

// Init args

typedef struct _DDD_PARM_LIST {
  USHORT           cache_parm_list;   // address of InitCache_Parameter_List
  USHORT           disk_config_table; // address of disk_configuration_table
  USHORT           int_req_vec_table; // address of IRQ_Vector_Table
  USHORT           cmd_line_args;     // address of command line parms
} DDD_PARM_LIST;
typedef DDD_PARM_LIST far *PDDD_PARM_LIST;
typedef char far *PCMD_LINE_ARGS;


// Disk Device Driver Trace Equates

#define TRACE_MAJOR  0x0068        // Trace major code

#define TRACE_READ   0x0001        // Trace Read minor code
#define TRACE_WRITE  0x0002        // Trace Write minor code
#define TRACE_WRITEV 0x0003        // Trace Write verify minor code
#define TRACE_CMPLT  0x8000        // Trace Complete mask


// AT State Machine Trace Codes

#define START_MINOR       START      // 0  Starting I/O
#define CALC_MINOR        CALC       // 1  Calculate position on disk
#define VERIFY_MINOR      VERIFY     // 2  Start verify portion of write
#define DONE_MINOR        DONE       // 3  I/O is done.
#define IDLE_MINOR        IDLE       // 4  Drive is inactive
#define SERROR_MINOR      SERROR     // 5  Have an error
#define RECAL_MINOR       RECAL      // 6  do a recal
#define READ_MINOR        READ       // 7  Read from disk
#define WRITE_MINOR       WRITE      // 8  Write to disk
#define SETPARAM_MINOR    SETPARAM   // 9  Set controller parameters

#define SETTIMEOUT_MINOR  0x0010     //    set a timeout
#define INTERRUPT_MINOR   0x0020     //    enter FixedInterrupt
#define SM_MINOR          0x0030     //    enter FixedExecute
#define TIMEOUT_MINOR     0x0040     //    timed out!
#define RETRY_MINOR       0x0090     //    Do_Retry function

#define ADDENTRY_MINOR    0x0050     //
#define NEXTIORB_MINOR    0x0051     //
#define READIORB_MINOR    0x0052     //
#define WRITEIORB_MINOR   0x0053     //
#define VERIFYIORB_MINOR  0x0054     //
#define NOTIFY_MINOR      0x005E     //
#define IORBDONE_MINOR    0x005F     //
