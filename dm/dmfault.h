/*static char *SCCSID = "@(#)dmfault.h	6.1 92/01/08";*/
/*static char *SCCSID = "@(#)dmfault.h    6.4 91/08/28";*/

/*--------------------------------------------------------
 *       (c) Copyright IBM Corporation 1981, 1990        *
 *                                                       *
 *       All Rights Reserved                             *
 *-------------------------------------------------------*/

/*-------------------------------------------------------*/
/*   Header file for fault tolerance support.            */
/*-------------------------------------------------------*/

#define FT_IOCTL_Cat         0x88    /* FT IOCTL Category     */
#define FT_IOCTL_Func        0x51    /* FT IOCTL Function     */
#define FT_ActivePartition   0x87    /* Active FT partition   */
#define FT_InactivePartition 0xC7    /* Inactive FT partition */

#define I24_MIN_RECOV_ERROR  0x1A    /* Error Codes > this are recoverable */

/*-------------------------------------------------------*/
/*   Partition descriptor table header                   */
/*-------------------------------------------------------*/
typedef struct _PDTH {
   UCHAR        NumControllers;         /* Number of controllers    */
   UCHAR        NumPhysDisks;           /* Number of physical disks */
   UCHAR        NumPartitions;          /* Number of partitions     */
   UCHAR        NumLogUnits;            /* Number of logical units  */
   UCHAR        PartNumOffset;          /* Parition Number offset   */
   UCHAR        Reserved_1;             /* Reserved, MBZ            */
   USHORT       Reserved_2;             /* Reserved, MBZ            */
   ULONG        Reserved_3;             /* Reserved, MBZ            */
   ULONG        Reserved_4;             /* Reserved, MBZ            */

} PDTH, FAR *PPDTH;


/*-------------------------------------------------------*/
/*   Partition descriptor table entries                  */
/*-------------------------------------------------------*/
typedef struct _PDT {

   UCHAR        Controller;             /* Number of the Controller         */
   UCHAR        PhysDisk;               /* Physical disk number (0x80...)   */
   UCHAR        PartitionType;          /* partition type                   */
   UCHAR        LogUnit;                /* Logical unit number of partition */
   ULONG        StartSec;               /* Starting Sector Number           */
   ULONG        EndSec;                 /* Ending sector number             */
   ULONG        Reserved;               /* Reserved                         */

} PDT, FAR *PPDT;

/*-------------------------------------------------------*/
/*   FT_IOCTL parameter packet filled in by DISKFT.SYS   */
/*-------------------------------------------------------*/
typedef struct _FT_IOCTL_param {

  USHORT        FT_Version;
  USHORT        FT_SigFT;
  UCHAR         Command;
  UCHAR         PartNumOffset;
  USHORT        FT_ProtDS;
  PVOID         (FAR *FT_Request) ( );
  PVOID         (FAR *FT_Done) ( );
  PPDT          pPDT;

} FT_IOCTL_param, FAR *PFT_IOCTL_param;

#define FT_VERSION      0x0002          /* FT Version Number     */
#define FT_SIG          0xf589          /* FT Signature Number   */

/* Command codes */
#define FT_IDENTIFY     0x00            /* validate enable struc */
#define FT_ENABLE       0x01            /* enable FT processing  */
#define FT_DISABLE      0x02            /* disable FT processing */

/*-------------------------------------------------------*/
/*   FT_IOCTL Data packet filled in by DASD Manager      */
/*-------------------------------------------------------*/
typedef struct _FT_IOCTL_data {

   USHORT       SupportCode;            /* Support level    */
   USHORT       FT_SigDD;               /* return signature */

} FT_IOCTL_data, FAR *PFT_IOCTL_data;

#define FT_SUPPORTED            0x0000    /* FT interface fully supported */
#define FT_VERSION_INCOMPAT     0x0001    /* FT version incompatibility   */
#define FT_NOT_SUPPORTED        0x0002    /* FT not supported             */
#define FT_INVALID_SIGNATURE    0x0003    /* invalid signature            */
#define FT_NO_FT_PARTITIONS     0x8000    /* no FT partitions detected    */

#define FT_SIG_DD               0xf58a    /* return signature value       */



/*-------------------------------------------------------*/
/*   FT_REQUEST return data                              */
/*-------------------------------------------------------*/
typedef struct _FT_RESULTS {

   USHORT       Reserved;
   USHORT       RequestHandle;
   PBYTE        pShadowReq;
   UCHAR        SecPartNum;

} FT_RESULTS;

#define ACT_PRIMARY          0x00    /* Access primary drive only     */
#define ACT_PRIMARY_FIRST    0x01    /* Access primary drive first    */
#define ACT_SECONDARY        0x02    /* Access secondary drive only   */
#define ACT_SECONDARY_FIRST  0x03    /* Access secondary drive first  */
#define ACT_BOTH             0x05    /* Acess both drives             */
#define ACT_EITHER           0x06    /* Use either drive for access   */


/*-------------------------------------------------------*/
/*   FT_DONE return codes                                */
/*-------------------------------------------------------*/
#define FT_DONE             0x00    /* Done */
#define FT_NOTDONE          0x01    /* Try again later, not finished */
#define I24_MIN_RECOV_ERROR 0x1A    /* Minimum recoverable error     */


/* Data areas used in Request Packet and Request List Entries for  */
/* tracking fault tolerant requests                                */

typedef struct _FTORIG {

   USHORT       RequestHandle;
   USHORT       Reserved;
} FTORIG;


typedef struct _FTDB {

   UCHAR        FT_Flags;
   UCHAR        AltLogDriveNum;
   union
   {
      FTORIG   OrigRequest;
      PBYTE    pOrigRequest;
   };

} FTDB, FAR *PFTDB;

/* FT_Flags defines */

#define FTF_FT_REQUEST       0x01
#define FTF_ACT_BOTH         0x02
#define FTF_SHADOW_REQUEST   0x04


#define MAXRPSIZE 32

typedef struct _RPFT {

   UCHAR        Dummy[MAXRPSIZE-6];
   FTDB         ftdb;

} RPFT, FAR *PRPFT;


typedef struct _RHFT {

   UCHAR        Dummy[sizeof(Req_Header)-8];
   FTDB         ftdb;
   UCHAR        Block_Dev_Unit;
   UCHAR        Reserved;

} RHFT, FAR *PRHFT;





