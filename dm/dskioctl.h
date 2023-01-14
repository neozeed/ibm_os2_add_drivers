/*static char *SCCSID = "@(#)dskioctl.h	6.2 92/02/06";*/
/*      SCCSID = @(#)dskioctl.h	6.2 92/02/06 */
/*     SCCSID = @(#)dskioctl.h	6.2 92/02/06 */

/***    DSKIOCTL.H
;
;       These are all the important structures and equates for
;       the disk ioctl.
*/

/* IOC_DC sub-functions - DOS 3.2 codes   */

#define SET_DEVICE_PARAMETERS   0x40
#define WRITE_TRACK             0x41
#define FORMAT_TRACK            0x42
#define GET_DEVICE_PARAMETERS   0x60
#define READ_TRACK              0x61
#define VERIFY_TRACK            0x62

/* Special function for Get Device Parameters */

#define BUILD_DEVICE_BPB        0x01

/* Special Functions for Set Device Parameters - 3.20 version */

#define INSTALL_FAKE_BPB        0x01
#define ONLY_SET_TRACKLAYOUT    0x02
#define TRACKLAYOUT_IS_GOOD     0x04

/* Special function for Format Track - 3.20 version */

#define STATUS_FOR_FORMAT       0x01

/*  Codes returned from Format Status Call */

#define FORMAT_NO_ROM_SUPPORT       0x01
#define FORMAT_COMB_NOT_SUPPORTED   0x02

/* DeviceType values */

#define MAX_SECTORS_IN_TRACK    40      /* Maximum sectors on a disk */
#define DEV_5INCH               0
#define DEV_5INCH96TPI          1
#define DEV_3INCH720KB          2
#define DEV_8INCHSS             3
#define DEV_8INCHDS             4
#define DEV_HARDDISK            5
#define DEV_OTHER               7

#define MAX_DEV_TYPE            7       /* max device type currently supported*/

typedef struct _a_SectorTable
{
  USHORT        SectorNumber;
  USHORT        SectorSize;
} a_SectorTable, TLT;

typedef struct a_SectorTable2
{
  UCHAR         Cylinder;
  UCHAR         Head;
  UCHAR         Sector;
  UCHAR         BytesPerSectorIndex;

} a_SectorTable2, FTT;

/* Structure definitions for structures used for the Int 21H Generic IOCTL */
/* function calls.                                                         */

#define DP_SECTOR_TABLE_SIZE    MAX_SECTORS_IN_TRACK * sizeof(a_SectorTable)

typedef struct _a_DeviceParameters
{
  UCHAR         SpecialFunctions;
  UCHAR         DeviceType;
  USHORT        DeviceAttributes;
  USHORT        Cylinders;
  UCHAR         MediaType;
  BPB           bpb;
  USHORT        TrackTableEntries;
  a_SectorTable SectorTable[MAX_SECTORS_IN_TRACK];

} a_DeviceParameters;


typedef struct _a_TrackReadWritePacket
{
  UCHAR         SpecialFunctions;
  USHORT        Head;
  USHORT        Cylinder;
  USHORT        FirstSector;
  USHORT        SectorsToReadWrite;
  ULONG         TransferAddress;
} a_TrackReadWritePacket;


typedef struct _a_FormatPacket
{
  UCHAR         SpecialFunctions;
  USHORT        Head;
  USHORT        Cylinder;
} a_FormatPacket;


typedef struct _a_VerifyPacket
{
  UCHAR         SpecialFunctions;
  USHORT        Head;
  USHORT        Cylinder;
} a_VerifyPacket;


/*----------------------------------------------------------*/
/* Category 8, functions 43H and 63H                        */
/*   - Set and Get Device Parameters                        */
/*----------------------------------------------------------*/

typedef struct _DDI_DeviceParameters_param
{
   UCHAR        Command;
} DDI_DeviceParameters_param, FAR *PDDI_DeviceParameters_param;

typedef struct DDI_DeviceParameters_data
{
  BPB           bpb;
  USHORT        NumCylinders;
  UCHAR         DeviceType;
  USHORT        DeviceAttr;

} DDI_DeviceParameters_data, FAR *PDDI_DeviceParameters_data;

/* Command equates for Set Device Parameters function */

#define SET_DEVICE_BPB  0x01;           /*  Set BPB for physical device */
#define SET_MEDIA_BPB   0x02;           /*  Set BPB for medium          */

/*----------------------------------------------------------*/
/* Category 8, function 45H, Format and Verify a Track      */
/*----------------------------------------------------------*/

typedef struct _DDI_FormatPacket_param
{
  UCHAR         Command;
  USHORT        Head;
  USHORT        Cylinder;
  USHORT        NumTracks;
  USHORT        NumSectors;
  FTT           FmtTrackTable[1];
} DDI_FormatPacket_param, FAR *PDDI_FormatPacket_param;

typedef struct _DDI_FormatPacket_data
{
UCHAR           StartSector;            /* Starting sector              */
} DDI_FormatPacket_data, FAR *PDDI_FormatPacket_data;

/*----------------------------------------------------------*/
/* Category 8 and 9 functions 44H, 64H, 65H                 */
/*  - Write Track, Read Track, Verify Track                 */
/*----------------------------------------------------------*/

typedef struct _DDI_RWVPacket_param
{
  UCHAR         Command;
  USHORT        Head;
  USHORT        Cylinder;
  USHORT        FirstSector;
  USHORT        NumSectors;
  TLT           TrackTable[1];
} DDI_RWVPacket_param, FAR *PDDI_RWVPacket_param;

/*----------------------------------------------------------*/
/* Category 8, function 60H, get media sense                */
/*----------------------------------------------------------*/

typedef struct _DDI_MediaSense_param
{
  UCHAR        Command;
} DDI_MediaSense_param, FAR *PDDI_MediaSense_param;

typedef struct _DDI_MediaSense_data
{
  UCHAR        MediaSense;
} DDI_MediaSense_data, FAR *PDDI_MediaSense_data;

/*----------------------------------------------------------*/
/* Category 9, function 22H, Install Drive Alias            */
/*----------------------------------------------------------*/
typedef struct _DDI_DriveAlias_param
{
  UCHAR         PhysDriveNum;
  SHORT         cCyln;
  BPB           rbpb;
} DDI_DriveAlias_param, FAR *PDDI_DriveAlias_param;

/*----------------------------------------------------------*/
/* Category 9, function 5EH, ReadBack                       */
/*----------------------------------------------------------*/
typedef struct _DDI_ReadBack_param
{
  UCHAR         Command;
} DDI_ReadBack_param, FAR *PDDI_ReadBack_param;

typedef struct _DDI_ReadBack_data
{
  UCHAR         Action;
  USHORT        DmaOverrunCount;
  USHORT        Count;
  USHORT        Miscompares;
} DDI_ReadBack_data, FAR *PDDI_ReadBack_data;

/*----------------------------------------------------------*/
/* Category 9, function 63H, get physical device parameters */
/*----------------------------------------------------------*/

typedef struct _DDI_PhysDeviceParameters_data
{
  USHORT        Reserved_1;
  USHORT        NumCylinders;
  USHORT        NumHeads;
  USHORT        SectorsPerTrack;
  USHORT        Reserved_2[4];
} DDI_PhysDeviceParameters_data, FAR *PDDI_PhysDeviceParameters_data;


/*----------------------------------------------------------*/
/* Category 9, function 6CH, Change Partition Type          */
/*----------------------------------------------------------*/

typedef struct _DDI_ChangePart_param
{
UCHAR          DDI_PartType_flag;       /* Partition Type flag             */
} DDI_ChangePart_param, FAR *PDDI_ChangePart_param;

#define CP_HPFS         0x01            /* change partition type to HPFS   */

/*----------------------------------------------------------*/
/* Category 8, function 5DH, Diskette Control               */
/*----------------------------------------------------------*/

typedef struct _DDI_DsktControl_param
{
  UCHAR         Command;
} DDI_DsktControl_param, FAR *PDDI_DsktControl_param;


/*----------------------------------------------------------*/
/* Category ?, function ??H, Map Packet                     */
/*----------------------------------------------------------*/

typedef struct _DDI_MapPacket
{
  UCHAR         Drive;
} DDI_MapPacket;

#define GEN_IOCTL_FN_TST        0x20        /* Used to differentiate between */
                                            /* reads and writes.             */




