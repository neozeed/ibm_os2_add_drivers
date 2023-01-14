/*static char *SCCSID = "@(#)dmioctl.h	6.3 92/01/30";*/
/*     SCCSID = @(#)dmioctl.h	6.3 92/01/30                                */
/***************************************************************************
*                                                                          *
*                This file is IBM unique                                   *
*                (c) Copyright  IBM Corporation  1981, 1991                *
*                           All Rights Reserved                            *
*                                                                          *
****************************************************************************/


/********************************************************/
/* Internal Request Packet generated for IOCTL requests */
/********************************************************/

typedef struct _RP_INTERNAL
{
   RPH          rph;                    /* Request Packet Header          */
   UCHAR        Function;               /* Internal Function Code         */
   ULONG        XferAddr;               /* Data Transfer Address          */
   USHORT       NumSectors;             /* Number of sectors to transfer  */
   ULONG        rba;                    /* Starting rba for request       */
   USHORT       SectorSize;             /* Size of sector in bytes        */
   USHORT       RetStatus;              /* Return status word             */

} RP_INTERNAL, FAR *PRP_INTERNAL;


/* Internal Function codes passed in RP_INTERNAL */
#define CMDInternal                   0x30    /* Request Packet command code */
                                              /* to identify RP_INTERNAL     */
#define DISKOP_READ_VERIFY            0x00
#define DISKOP_FORMAT                 0x01
#define DISKOP_FORMAT_VERIFY          0x02
#define DISKOP_GET_CHANGELINE_STATE   0x03
#define DISKOP_GET_MEDIA_SENSE        0x04
#define DISKOP_RESUME                 0x05
#define DISKOP_SUSPEND_DEFERRED       0x06
#define DISKOP_SET_MEDIA_GEOMETRY     0x07


/*------------------------------------*/
/* Change partition IOCTL             */
/*------------------------------------*/
#define IODC_CP         0x6C            /* Change Partition type           */
                                        /* move to ioctl.h                 */

/*****************************************************/
/* Common Work Area (CWA) used for IOCTL processing  */
/*****************************************************/

typedef struct _CWA
{
UCHAR           Flags;                  /* CWA Flags                       */
PBYTE           pParmPkt;               /* Parameter Packet Virtual Addr   */
ULONG           ppParmPkt;              /* Parameter Packet Physical Addr  */
ULONG           Reserved_1;             /* Reserved for DevHelp_VMLock call*/
UCHAR           hLockParmPkt[12];       /* Parameter Packet Lock Handle    */
PBYTE           pDataPkt;               /* Data Packet Virtual  Addr       */
ULONG           ppDataPkt;              /* Data Packet Physical Addr       */
ULONG           Reserved_2;             /* Reserved for DevHelp_VMLock call*/
UCHAR           hLockDataPkt[12];       /* Data Packet Lock Handle         */

UCHAR           GIO_Type;               /* IOCTL Type                      */
USHORT          TTSectors;              /* Track Table Sector Count        */
USHORT          Head;                   /* Head from IOCTL RP              */
USHORT          Cylinder;               /* Cylinder from IOCTL RP          */
USHORT          StartSector;            /* Start Sector Num from IOCTL RP  */
USHORT          NumSectors;             /* Sector Count from IOCTL RP      */

NPVOLCB         pVolCB;                 /* ptr to VolCB                    */
RP_GENIOCTL     far *pRP;               /* ptr to RP                       */
RP_INTERNAL     far *pIRP;              /* ptr to internal RP              */

} CWA, *NPCWA;

/*----------------*/
/* Flags in CWA   */
/*----------------*/
#define LOCKED_PARMPKT          0x01    /* Parm Pkt Locked                 */
#define LOCKED_DATAPKT          0x02    /* Data pkt locked                 */
#define TT_NOT_CONSEC           0x04    /* Non-consecutive Sectors         */
#define TT_NOT_SAME_SIZE        0x08    /* More than one sector size       */
#define TT_NON_STD_SECTOR       0x10    /* Non-standard sector size (!512) */
#define TT_START_NOT_SECTOR_ONE 0x20    /* Does not start at sector 1.     */

/*----------------------------------------------------*/
/* Flag defines for call to LockUserPacket routine    */
/*----------------------------------------------------*/
#define LOCK_PARMPKT    0x01            /* Lock parameter packet           */
#define LOCK_DATAPKT    0x02            /* Lock data packet                */
#define LOCK_VERIFYONLY 0x04            /* Verify only lock                */
#define LOCK_WRITE      0x08            /* Lock for write access           */

#define VMDHL_NOBLOCK           0x0001
#define VMDHL_CONTIGUOUS        0x0002
#define VMDHL_16M               0x0004
#define VMDHL_WRITE             0x0008
#define VMDHL_LONG              0x0010
#define VMDHL_VERIFY            0x0020



#define RWV_COMPLEX_TRACK_TABLE         0x00
#define RWV_SIMPLE_TRACK_TABLE          0x01
#define FP_COMMAND_COMPLEX              0x00
#define FP_COMMAND_SIMPLE               0x01
#define FP_COMMAND_MULTITRACK           0x03
;
#define MTF_SUCCESSFUL                  0x8000
#define ERROR_MTF_NOT_SUPPORTED         0x4000
#define ERROR_MTF_FORMAT_FAILURE        0x2000
#define ERROR_MTF_VERIFY_FAILURE        0x1000

#define GDP_RETURN_REC_BPB              0x0000
#define GDP_RETURN_MEDIA_BPB            0x0001
;
#define SDP_USE_MEDIA_BPB               0x0000
#define SDP_TEMP_BPB_CHG                0x0001
#define SDP_PERM_BPB_CHG                0x0002
;
#define DP_DEVICEATTR_NON_REMOVABLE     0x0001
#define DP_DEVICEATTR_CHANGELINE        0x0002
#define DP_DEVICEATTR_GT16MBSUPPORT     0x0004
;
#define DMA_SET_READBACK_FLAG           0x0000
#define DMA_RESET_READBACK_FLAG         0x0001
#define DMA_QUERY_READ_STATS            0x0002
#define DMA_RESET_READ_STATS            0x0003

#define DSKT_RESUME                     0x0000
#define DSKT_SUSPEND                    0x0001
#define DSKT_QUERY                      0x0002

/*------------------------------------------------*/
/* Function declarations and pragma statements    */
/*------------------------------------------------*/
USHORT NEAR GIO_RWVTrack (NPCWA);
#pragma alloc_text(SwapCode, GIO_RWVTrack)

USHORT NEAR CheckRWVPacket (NPCWA);
#pragma alloc_text(SwapCode, CheckRWVPacket)

USHORT NEAR ExecRWVOnePass (NPCWA);
#pragma alloc_text(SwapCode, ExecRWVOnePass)

USHORT NEAR ExecRWVMultiPass (NPCWA);
#pragma alloc_text(SwapCode, ExecRWVMultiPass)

USHORT NEAR GIO_FormatVerify (NPCWA);
#pragma alloc_text(SwapCode, GIO_FormatVerify)

USHORT NEAR CheckFormatPacket (NPCWA);
#pragma alloc_text(SwapCode, CheckFormatPacket)

USHORT NEAR ExecDisketteFormat (NPCWA);
#pragma alloc_text(SwapCode, ExecDisketteFormat)

USHORT NEAR ExecCheckTrack (NPCWA);
#pragma alloc_text(SwapCode, ExecCheckTrack)

USHORT NEAR ExecMultiTrkVerify (NPCWA);
#pragma alloc_text(SwapCode, ExecMultiTrkVerify)

USHORT NEAR GIO_GetDeviceParms8 (NPCWA);
#pragma alloc_text(SwapCode, GIO_GetDeviceParms8)

USHORT NEAR BuildNewBPB (NPCWA);
#pragma alloc_text(SwapCode, BuildNewBPB)

USHORT NEAR GIO_SetDeviceParms (NPCWA);
#pragma alloc_text(SwapCode, GIO_SetDeviceParms)

USHORT NEAR GIO_GetDeviceParms9 (NPCWA);
#pragma alloc_text(SwapCode, GIO_GetDeviceParms9)

USHORT NEAR GIO_ReadBack (NPCWA);
#pragma alloc_text(SwapCode, GIO_ReadBack)

USHORT NEAR GIO_MediaSense (NPCWA);
#pragma alloc_text(SwapCode, GIO_MediaSense)

USHORT NEAR GIO_AliasDrive (NPCWA);
#pragma alloc_text(SwapCode, GIO_AliasDrive)

USHORT NEAR GIO_DsktControl (NPCWA);
#pragma alloc_text(SwapCode, GIO_DsktControl)

USHORT NEAR GIO_ChangePartition (NPCWA);
#pragma alloc_text(SwapCode, GIO_ChangePartition)

USHORT NEAR GIO_FaultTolerance (NPCWA);
#pragma alloc_text(SwapCode, GIO_FaultTolerance)

USHORT NEAR CheckFTPacket (NPCWA);
#pragma alloc_text(SwapCode, CheckFTPacket)

USHORT NEAR CheckFloppy (NPCWA);
#pragma alloc_text(SwapCode, CheckFloppy)

USHORT NEAR BuildCWA (PRP_GENIOCTL, NPVOLCB, NPCWA FAR *);
#pragma alloc_text(SwapCode, BuildCWA)

VOID NEAR ReleaseCWA (NPCWA);
#pragma alloc_text(SwapCode, ReleaseCWA)

VOID NEAR RBA_to_CHS (NPVOLCB, ULONG, CHS_ADDR FAR *);
#pragma alloc_text(SwapCode, RBA_to_CHS)

USHORT NEAR LockUserPacket (NPCWA, USHORT, ULONG);
#pragma alloc_text(SwapCode, LockUserPacket)

VOID NEAR AllocCWA_Wait (NPCWA FAR *);
#pragma alloc_text(SwapCode, AllocCWA_Wait)

VOID NEAR FreeCWA (NPCWA);
#pragma alloc_text(SwapCode, FreeCWA)

UCHAR  NEAR SectorSizeToSectorIndex (USHORT);
#pragma alloc_text(SwapCode, SectorSizeToSectorIndex)

USHORT NEAR SectorIndexToSectorSize (UCHAR);
#pragma alloc_text(SwapCode, SectorIndexToSectorSize)

USHORT NEAR IOCTL_IO (PBYTE, NPVOLCB);
#pragma alloc_text(SwapCode, IOCTL_IO)

USHORT NEAR Chk_AccValidate (PRP_RWV, NPVOLCB);
#pragma alloc_text(SwapCode, Chk_AccValidate)



