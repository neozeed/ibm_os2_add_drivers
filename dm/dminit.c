/*static char *SCCSID = "@(#)dminit.c	6.10 92/02/06";*/
#define SCCSID  "@(#)dminit.c	6.10 92/02/06"

/*--------------------------------------------------------
 *       (c) Copyright IBM Corporation 1981, 1990        *
 *                                                       *
 *       All Rights Reserved                             *
 *-------------------------------------------------------*/

#include "infoseg.h"
#include "dmh.h"
#include "devclass.h"

VOID   NEAR Build_UnitCBs (void);
VOID   NEAR Build_VolCBs (void);
VOID   NEAR InitDCS_VCS (void);
VOID   NEAR InitCBPool (void);
VOID   FAR  InitPost (PIORBH);
VOID   NEAR Setup_Partition_VolCBs (void);
USHORT NEAR Build_Next_VolCB (NPVOLCB, ULONG);
VOID   NEAR Setup_Extended_Volumes (void);
USHORT NEAR Read_Sector (NPVOLCB, ULONG);
VOID   NEAR Init_Trace (void);
VOID   NEAR GetInitParms (PRPINITIN);
VOID   NEAR InitBPBfromGeom (NPVOLCB, NPBPB, PGEOMETRY);

typedef struct InfoSegGDT FAR *PInfoSegGDT;

/*--------------------------------------------------------------*/
/* Init data allocated at the end of the data segment.          */
/*--------------------------------------------------------------*/


USHORT          InitData=0;
UCHAR           InitTimeIORB[MAX_IORB_SIZE]={0};       /* Init time IORB   */
RP_RWV          InitTimeRP={0};                        /* Init time RP     */
PARTITIONTABLE  PartitionTables[MAX_FIXED_DISKS]={0};  /* Partition tables */

/* Perfview init time data */
#define NumTimerCounters  6

UCHAR    GroupName[] = "DISK 01";
UCHAR    str1[] = "ctRD";        /* Read Counter     */
UCHAR    str2[] = "tmRD";        /* Read Time        */
UCHAR    str3[] = "byRD";        /* Read Byte Count  */
UCHAR    str4[] = "ctWR";        /* Write Counter    */
UCHAR    str5[] = "tmWR";        /* Write Time       */
UCHAR    str6[] = "byWR";        /* Write Byte Count */


TBN      NameBlock[]  =
{
   {PVW_CT_CNT, sizeof(CNT), 0 , str1},
   {PVW_CT_TIMR, sizeof(TIMR), 0 , str2},
   {PVW_CT_CNT, sizeof(CNT), 0 , str3},
   {PVW_CT_CNT, sizeof(CNT), 0 , str4},
   {PVW_CT_TIMR, sizeof(TIMR), 0 , str5},
   {PVW_CT_CNT, sizeof(CNT), 0 , str6},
};

TBH PerfViewTB = {
   TBH_VER_2_0_0_0,             /* Version Number */
   0,0,                         /* Block Instance ID, Block Group ID */
   0,0,0,GroupName,             /* Text Block Group Name */
   0,0,0,0,                     /* Text Block Instance Name */
   0,                           /* Message File Name */
   0,                           /* Help file name */
   NumTimerCounters,            /* Number of Timers + Counters */
   &NameBlock[0],               /* Pointer to array of Name Blocks */
};

//#define MAX_DEVICE_TABLE_SIZE 1024
#define MAX_DT_ADAPTERS  8
#define MAX_DT_UNITS     56
#define MAX_DT_SIZE (sizeof(DEVICETABLE) + ((MAX_DT_ADAPTERS-1) * 2) +        \
              (MAX_DT_ADAPTERS * (sizeof(ADAPTERINFO)-sizeof(UNITINFO))) +    \
              (MAX_DT_UNITS * sizeof(UNITINFO))  )

UCHAR           ScratchBuffer2[MAX_DT_SIZE]={0};       /*Scratch buffer */

/*-----------------------------------------------------*/
/* DriveInit  - Initialization for OS2DASD.SYS         */
/*-----------------------------------------------------*/
USHORT near DriveInit(pRP, pVolCB)

PRPINITIN  pRP;
NPVOLCB    pVolCB;
{
  USHORT     rc, i, NumBPBArrayEntries, NumExtraVolCBs;
  NPVOLCB    pVolCBx;
  PRPINITOUT pRPO = (PRPINITOUT) pRP;   /* Output for Init RP           */
  PVOID      pScratchBuffer;

  extern USHORT (near *Strat1Near[])();

//_asm {int 3};

  /* Initialize various variables */

  DDFlags |= DDF_INIT_TIME;            /* Turn off init time flag */

  Device_Help = pRP->DevHlpEP;         /* Save ptr to devhelp function   */

  pDataSeg = (PVOID) &pDataSeg;        /* Set up pointer to data segment */
  OFFSETOF(pDataSeg) = 0;

  /* Get the init parms specified on the BASEDEV= command line */

  GetInitParms(pRP);

  /* Save away the physical and linear addresses of our data segment */

  rc = DevHelp_VirtToPhys(pDataSeg, (PULONG) &ppDataSeg);

  rc = DevHelp_VirtToLin((USHORT) (SELECTOROF(pDataSeg)),
                         (ULONG) (OFFSETOF(pDataSeg)),
                         (PLIN) &plDataSeg);  /* Save lin addr of data seg    */

  /* Save away the physical address of the scratch read buffer      */

  SELECTOROF(pScratchBuffer) = SELECTOROF(pDataSeg);
  OFFSETOF(pScratchBuffer) = (USHORT) &ScratchBuffer[0];

  rc = DevHelp_VirtToPhys(pScratchBuffer, (PULONG) &ppScratchBuffer);


  /* Put init data at end of _DATA since it'll get discarded later     */

  PoolSize = INIT_POOL_SIZE;

  FreePoolSpace = INIT_POOL_SIZE;

  Build_UnitCBs();                      /* Build unit control blocks   */

  if (NumUnitCBs == 0)                  /* Handle no_media case */
  {
     /* Setup the return parameters for the no media case */

     pRPO->Unit = 2;
     pRPO->BPBArray = 0;
     pRPO->CodeEnd = (USHORT) DriveInit;
     pRPO->DataEnd = (USHORT) CBPool;
  }
  else  /* Media exists which is the typical case */
  {
     Build_VolCBs();                    /* Build Volume control blocks */

     InitDCS_VCS();                     /* Initialize the DCS and VCS strucs */

     InitCBPool ();                     /* Initialize the Control Block pool */

     Init_Trace();                      /* Initialize tracing */

     /* Set up the return parameters for the INIT packet. */

     NumLogDrives = NumRemovableDisks + NumPartitions;
     if (pVolCB_DriveA->Flags & vf_AmMult)
        NumLogDrives++;                    /* Add the psuedo drive */
     else if (NumRemovableDisks == 0)
        NumLogDrives += 2;                 /* No floppy case */

     pRP->Unit = (CHAR) NumLogDrives;      /* Return number of logical drives */


     /* Setup the return BPB array */

     NumExtraVolCBs = NumRemovableDisks;

     if (NumRemovableDisks == 0)
        NumExtraVolCBs++;

     NumBPBArrayEntries = NumLogDrives + NumExtraVolCBs;

     if (NumBPBArrayEntries > MAX_DRIVE_LETTERS)
       NumBPBArrayEntries = MAX_DRIVE_LETTERS;

     pRPO->BPBArray = (PVOID) InitBPBArray;

     InitBPBArray[0] = &(pVolCB_DriveA->MediaBPB);
     InitBPBArray[1] = &(pVolCB_DriveB->MediaBPB);

     pVolCBx = pVolCB_DriveC;
//???for (i = 2; i < NumBPBArrayEntries; i++, pVolCBx = pVolCBx->pNextVolCB)
     for (i = 2; i < NumLogDrives; i++, pVolCBx = pVolCBx->pNextVolCB)
        InitBPBArray[i] = &(pVolCBx->MediaBPB);

     /* Return the end of the code and data segments */

     pRPO->CodeEnd = (USHORT) DriveInit;
     pRPO->DataEnd = (USHORT) pNextFreeCB;

  }
  /* Dont allow another INIT command to come in */

  Strat1Near[CMDInitBase] = CmdErr;   /* Patch strat1 table to disable inits */
  DDFlags &= ~DDF_INIT_TIME;          /* Turn off init time flag */

  return(STDON);                      /* Done with init, so return */

}


/********************** START OF SPECIFICATIONS *****************************
*                                                                           *
* SUBROUTINE NAME: Build_UnitCBs                                            *
*                                                                           *
* DESCRIPTIVE NAME: Build Unit Control Blocks  (UnitCBs)                    *
*                                                                           *
* FUNCTION:  This routine issues the GetAdapterDeviceTable command          *
*            to each Adapter Device Driver and builds the unit control      *
*            blocks from each Adapter Device Table returned.  Each unit     *
*            control block (UnitCB) represents a physical device unit       *
*            (i.e. drive numbers 0x80,0x81, etc.) which the adapter         *
*            device driver manages.                                         *
*                                                                           *
* ENTRY POINT: Build_UnitCBs                                                *
*                                                                           *
* LINKAGE: Call Near                                                        *
*                                                                           *
* INPUT:                                                                    *
*                                                                           *
* EXIT-NORMAL: UnitCBs built for each physical unit.                        *
*              NumUnitCBs = Number of UnitCBs built                         *
*                                                                           *
* EXIT-ERROR: None                                                          *
*                                                                           *
*********************** END OF SPECIFICATIONS *******************************/

void Build_UnitCBs()

{
  USHORT                rc;
  NPUNITCB              pUnitCB;         /* Pointer to current UnitCB    */
  NPIORB_CONFIGURATION  pIORB;           /* ptr to IORB                  */
  DEVICETABLE           *pDeviceTable;   /* ptr to device table          */
  NPADAPTERINFO         pAdapterInfo;    /* near ptr to AdapterInfo      */
  USHORT                i, j, k;         /* Index pointers               */
  USHORT                FilterADDHandle; /* Filter Handle                */
  struct DevClassTableStruc far *pDriverTable;  /*  ptr to registered ADD EPs*/
  VOID (FAR * DriverEP) ();                  /* Driver entry point           */

  (NPUNITCB)pNextFreeCB = FirstUnitCB;  /* Next free is first unit CB   */
  UnitCB_Head = FirstUnitCB;            /* Init UnitCB head pointer     */
  pUnitCB = UnitCB_Head;                /* Point to first UnitCB        */
  NumUnitCBs = 0;                       /* Init UnitCB count            */

  /*--------------------------------------------------------------------*/
  /* Get the adapter device tables for each adapter driver and create   */
  /* the unit control blocks (UnitCBs) from the returned tables.        */
  /*--------------------------------------------------------------------*/

  rc = DevHelp_GetDOSVar((USHORT) DHGETDOSV_DEVICECLASSTABLE, 1,
                         (PPVOID) &pDriverTable);

  NumDrivers = pDriverTable->DCCount;

  pDeviceTable = (DEVICETABLE *) ScratchBuffer2;

  for (i = 0; i < NumDrivers; i++)
  {
     pIORB = (NPIORB_CONFIGURATION) InitTimeIORB;
     pIORB->iorbh.Length = sizeof(IORB_CONFIGURATION);
     pIORB->iorbh.CommandCode = IOCC_CONFIGURATION;
     pIORB->iorbh.CommandModifier = IOCM_GET_DEVICE_TABLE;
     pIORB->iorbh.Status = 0;
     pIORB->iorbh.RequestControl = IORB_ASYNC_POST;
     pIORB->iorbh.NotifyAddress = &InitPost;
     pIORB->pDeviceTable = (PVOID) pDeviceTable;
     pIORB->DeviceTableLen = sizeof(ScratchBuffer2);

     OFFSETOF(DriverEP) =  pDriverTable->DCTableEntries[i].DCOffset;
     SELECTOROF(DriverEP) = pDriverTable->DCTableEntries[i].DCSelector;

     f_ZeroCB((PBYTE)pDeviceTable, sizeof(ScratchBuffer2));

     (*DriverEP) ((PVOID)(pIORB));

     while ( !(pIORB->iorbh.Status & IORB_DONE) )  /* Wait till done */
     ;

     for (j = 0; j < pDeviceTable->TotalAdapters; j++)
     {
        pAdapterInfo =  pDeviceTable->pAdapter[j];

        for (k = 0; k < pAdapterInfo->AdapterUnits; k++)
        {

           /* Only allocate DISK type devices which are not defective */
           /* and which dont suppress DASD manager support.           */

           if ( (pAdapterInfo->UnitInfo[k].UnitType == UIB_TYPE_DISK) &&
                ! (pAdapterInfo->UnitInfo[k].UnitFlags &
                        (UF_NODASD_SUPT | UF_DEFECTIVE)) )
           {
              /* Allocate the unit if it's not already allocated */
              /* Wait until the request comes back from ADD.     */

              pIORB->iorbh.Length = sizeof(IORB_UNIT_CONTROL);
              pIORB->iorbh.CommandCode = IOCC_UNIT_CONTROL;
              pIORB->iorbh.CommandModifier = IOCM_ALLOCATE_UNIT;
              pIORB->iorbh.Status = 0;
              pIORB->iorbh.UnitHandle = pAdapterInfo->UnitInfo[k].UnitHandle;
              pIORB->iorbh.RequestControl = IORB_ASYNC_POST;
              pIORB->iorbh.NotifyAddress = &InitPost;
              ((NPIORB_UNIT_CONTROL)pIORB)->Flags = 0;
              ((NPIORB_UNIT_CONTROL)pIORB)->pUnitInfo = 0;
              ((NPIORB_UNIT_CONTROL)pIORB)->UnitInfoLen = 0;

              (*DriverEP) ((PVOID)(pIORB));

              while ( !(pIORB->iorbh.Status & IORB_DONE) )  /* Wait till done */
              ;

              /* If allocation succeeded then add unit to unit tables */

              if ( !(pIORB->iorbh.Status & IORB_ERROR) )
              {
                 NumUnitCBs ++;                        /* update unit count */
                 pUnitCB->UnitInfo = pAdapterInfo->UnitInfo[k];

                 /* Save the callable entry point of the adapter driver. If */
                 /* the unit is being filtered, use the entry point of the  */
                 /* filter driver.                                          */

                 FilterADDHandle = pUnitCB->UnitInfo.FilterADDHandle;
                 if (FilterADDHandle == 0)
                     pUnitCB->AdapterDriverEP = DriverEP;
                 else
                 {
                    OFFSETOF(pUnitCB->AdapterDriverEP) =
                      pDriverTable->DCTableEntries[FilterADDHandle].DCOffset;

                    SELECTOROF(pUnitCB->AdapterDriverEP) =
                      pDriverTable->DCTableEntries[FilterADDHandle].DCSelector;
                 }

                 pUnitCB->MaxHWSGList = pAdapterInfo->MaxHWSGList;

                 if (pAdapterInfo->AdapterFlags & AF_HW_SCATGAT)
                    pUnitCB->Flags |= UCF_HW_SCATGAT;

                 if (pAdapterInfo->AdapterFlags & AF_16M)
                    pUnitCB->Flags |= UCF_16M;

                 if (pAdapterInfo->AdapterFlags & AF_CHS_ADDRESSING)
                    pUnitCB->Flags |= UCF_CHS_ADDRESSING;

//               if (pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE)
//                  if ((pAdapterInfo->AdapterDevBus & 0x00FF)
//                                                     != AI_DEVBUS_FLOPPY)
//                   pUnitCB->UnitInfo.UnitFlags &= ~UF_REMOVABLE;

                 if (pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE)
                 {
                    pUnitCB->PhysDriveNum = NumRemovableDisks;
                    NumRemovableDisks++;
                    if ((pAdapterInfo->AdapterDevBus & 0x00FF)
                                                       != AI_DEVBUS_FLOPPY)

                       pUnitCB->Flags |= UCF_REMOVABLE_NON_FLOPPY;
                 }
                 else
                 {
                    pUnitCB->PhysDriveNum = NumFixedDisks + 0x80;
                    NumFixedDisks++;
                 }
                 pUnitCB->pNextUnitCB = pUnitCB + 1;
                 pUnitCB++;
              }
           }
        }  /* end unit loop */
     }  /* end adapter loop */
  }  /* end driver loop */

  (NPUNITCB) pNextFreeCB = pUnitCB;     /* Update next free control blk ptr */
  (pUnitCB-1)->pNextUnitCB = 0;
}


/********************** START OF SPECIFICATIONS *****************************
*                                                                           *
* SUBROUTINE NAME: Build_VolCBs                                             *
*                                                                           *
* DESCRIPTIVE NAME: Build Volume Control Blocks  (VolCBs)                   *
*                                                                           *
* FUNCTION:  This routine builds the physical and logical volume            *
*            control blocks for each volume.                                *
*                                                                           *
* ENTRY POINT: Build_VolCBs                                                 *
*                                                                           *
* LINKAGE: Call Near                                                        *
*                                                                           *
* INPUT:                                                                    *
*                                                                           *
* EXIT-NORMAL: VolCBs built for each volume.                                *
*              NumVolCBs = Number of VolCBs built                           *
*                                                                           *
* EXIT-ERROR: None                                                          *
*                                                                           *
*********************** END OF SPECIFICATIONS *******************************/

void Build_VolCBs()

{
  NPVOLCB         pVolCB;
  NPUNITCB        pUnitCB;
  USHORT          iUnit;
  USHORT          iVol;
  NPIORB_GEOMETRY pIORB;                /* ptr to IORB                    */

  BOOL A_Found = FALSE;                 /* A: drive found                  */
  BOOL B_Found = FALSE;                 /* B: drive found                  */
  UCHAR PseudoB = NO;


  /*-----------------------------------------------*/
  /* Create Volume Controls Blocks for units       */
  /* managing A: and B:                            */
  /*-----------------------------------------------*/

  /* The VolCB chain starts with Drive A: and Drive B:  */
  /* and are placed right after the UnitCBs             */

  VolCB_Head = (NPVOLCB)pNextFreeCB;

  pVolCB_DriveA = VolCB_Head;
  pVolCB_DriveA->LogDriveNum = 0;          /* LogDriveNum for A: is 0  */
  pVolCB_DriveA->pNextVolCB = pVolCB_DriveA + 1;

  pVolCB_DriveB = VolCB_Head + 1;
  pVolCB_DriveB->LogDriveNum = 1;       /* LogDriveNum for B: is 1  */
  pVolCB_DriveB->pNextVolCB = pVolCB_DriveB + 1;


  pUnitCB = UnitCB_Head;                /* Point back to head UnitCB       */
  for (iUnit=0; iUnit < NumUnitCBs; iUnit++, pUnitCB++)
  {
     if (A_Found && B_Found)
        break;

     if ((pUnitCB->UnitInfo.UnitFlags & UF_A_DRIVE) && !(A_Found))
     {
        A_Found = TRUE;                          /* Indicate A: found        */
        pVolCB_DriveA->pUnitCB = pUnitCB;         /* Link VolCB to UnitCB     */
        pVolCB_DriveA->PhysDriveNum = 0;         /* PhysDriveNum for A: is 0 */
        pVolCB_DriveA->Flags |= vf_OwnPhysical;  /* Owns physical drive      */
        pUnitCB->pCurrentVolCB = pVolCB_DriveA;

        if ((pUnitCB->UnitInfo.UnitFlags & UF_B_DRIVE) && !(B_Found))
        {
           B_Found = TRUE;                       /* Indicate B: found   */
           PseudoB = YES;                        /* Indicate Pseudo B drive */
        }
     }
     else if ((pUnitCB->UnitInfo.UnitFlags & UF_B_DRIVE) && !(B_Found))
     {
        B_Found = TRUE;                          /* Indicate B: found        */
        pVolCB_DriveB->pUnitCB = pUnitCB;         /* Link VolCB to UnitCB     */
        pVolCB_DriveB->PhysDriveNum = 1;         /* PhysDriveNum for B: is 1 */
        pVolCB_DriveB->Flags |= vf_OwnPhysical;  /* Owns physical drive      */
        pUnitCB->pCurrentVolCB = pVolCB_DriveB;
     }
  }

  /* If PseudoB drive found, or found A: but not B: */

  if ( (PseudoB == YES) || (A_Found && !B_Found) )
  {
     pVolCB_DriveA->Flags |= vf_AmMult;      /* Mult VolCBs mapped to unit  */
     pVolCB_DriveB->Flags |= vf_AmMult;      /* Mult VolCBs mapped to unit */
     pVolCB_DriveB->pUnitCB = pVolCB_DriveA->pUnitCB;
     pVolCB_DriveB->PhysDriveNum = 0;
  }

  /* If found B:, but not A: */

  else if (!A_Found && B_Found)
  {
     pVolCB_DriveA->PhysDriveNum = 0;
     pVolCB_DriveA->pUnitCB = pVolCB_DriveB->pUnitCB;
     pVolCB_DriveA->Flags |= vf_AmMult + vf_OwnPhysical;
     pVolCB_DriveA->pUnitCB->pCurrentVolCB = pVolCB_DriveA;

     pVolCB_DriveB->PhysDriveNum = 0;
     pVolCB_DriveB->Flags |= vf_AmMult;      /* Mult VolCBs mapped to unit */
     pVolCB_DriveB->Flags & ~vf_OwnPhysical;
  }

  /* If no A: or B:           */

  else if (!A_Found && !B_Found)
  {
     pVolCB_DriveA->PhysDriveNum = -1;
     pVolCB_DriveA->LogDriveNum = -1;
     pVolCB_DriveA->MediaBPB = BPB_144MB;

     pVolCB_DriveB->PhysDriveNum = -1;
     pVolCB_DriveB->LogDriveNum = -1;
     pVolCB_DriveB->MediaBPB = BPB_144MB;
  }


  NumVolCBs = 2;
  pVolCB = VolCB_Head + 2;

  /*-----------------------------------------------*/
  /* Create volume control blocks for removable    */
  /* disk devices which were not assigned A: or B: */
  /*-----------------------------------------------*/

  for (iUnit=0, pUnitCB = UnitCB_Head; iUnit < NumUnitCBs; iUnit++, pUnitCB++)
  {
     if (pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE &&
         pUnitCB->UnitInfo.UnitType == UIB_TYPE_DISK &&
         !(pUnitCB->UnitInfo.UnitFlags & (UF_A_DRIVE | UF_B_DRIVE) ) )
     {
        pVolCB->pUnitCB = pUnitCB;
        pVolCB->LogDriveNum = -1;
        pVolCB->PhysDriveNum = pUnitCB->PhysDriveNum;
        pVolCB->Flags |= vf_OwnPhysical;         /* Owns physical drive      */
        pUnitCB->pCurrentVolCB = pVolCB;
        pVolCB->pNextVolCB = pVolCB + 1;
        pVolCB++;
        NumVolCBs++;
     }
  }

  pLastLogVolCB = pVolCB-1;

  /*------------------------------------------------*/
  /* Create volume control blocks for non-removable */
  /* disk devices.                                  */
  /*------------------------------------------------*/

  pVolCB_80 = pVolCB;
  for (iUnit=0, pUnitCB = UnitCB_Head; iUnit < NumUnitCBs; iUnit++, pUnitCB++)
  {
     if ( !(pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE) &&
         pUnitCB->UnitInfo.UnitType == UIB_TYPE_DISK)
     {
        pVolCB->pUnitCB = pUnitCB;
        pVolCB->LogDriveNum = pUnitCB->PhysDriveNum;  /* Assign Logical same */
        pVolCB->PhysDriveNum = pUnitCB->PhysDriveNum;  /*  as physical       */
        pVolCB->Flags |= vf_OwnPhysical;         /* Owns physical drive      */
        pUnitCB->pCurrentVolCB = pVolCB;
        pVolCB->pNextVolCB = pVolCB + 1;
        pVolCB++;
        NumVolCBs++;
     }
   }
   (pVolCB-1)->pNextVolCB = (NPVOLCB) NULL;          /* Terminate VolCB chain */


  /*------------------------------------------------*/
  /* We now have all the physical VolCB's created.  */
  /* Fill in the BPB and other various device       */
  /* parameters in the VolCB.                       */
  /*------------------------------------------------*/

  pIORB = (NPIORB_GEOMETRY) InitTimeIORB;

  for (iVol = 0, pVolCB = VolCB_Head; iVol < NumVolCBs; iVol++, pVolCB++)
  {
     if (pVolCB->Flags & vf_OwnPhysical)
     {
        pIORB->iorbh.Length = sizeof(IORB_GEOMETRY);
        pIORB->iorbh.CommandCode = IOCC_GEOMETRY;
        pIORB->iorbh.CommandModifier = IOCM_GET_DEVICE_GEOMETRY;
        pIORB->iorbh.UnitHandle = pVolCB->pUnitCB->UnitInfo.UnitHandle;
        pIORB->iorbh.RequestControl = IORB_ASYNC_POST;
        pIORB->iorbh.Status = 0;
        pIORB->iorbh.NotifyAddress = &InitPost;
        pIORB->pGeometry = (GEOMETRY *) ScratchBuffer2;
        pIORB->GeometryLen = sizeof(struct _GEOMETRY);

        f_ZeroCB((PBYTE)pIORB->pGeometry, pIORB->GeometryLen);

        pUnitCB = pVolCB->pUnitCB;

        (*pUnitCB->AdapterDriverEP) ((PVOID) (pIORB));

        while ( !(pIORB->iorbh.Status & IORB_DONE) )  /* Wait till done */
        ;

        InitBPBfromGeom( pVolCB, &(pVolCB->RecBPB), pIORB->pGeometry);

        pVolCB->MediaBPB = pVolCB->RecBPB;     /* Copy Rec BPB to media BPB */
     }
  }

  (NPVOLCB)pNextFreeCB = pVolCB;

  if (NumFixedDisks > 0)
     Setup_Partition_VolCBs();         /* Setup VolCBs for partitions       */

  /* Go back and fill in the logical drive number for those volumes         */
  /* we assign at the end (i.e. removable drives which are not A: or B:     */

  pVolCB = VolCB_Head;
  for (iVol = 0; iVol < NumVolCBs; iVol++, pVolCB = pVolCB->pNextVolCB)
     if (pVolCB->LogDriveNum == -1)
        pVolCB->LogDriveNum = NextLogDriveNum++;

  /* Reserve 1 extra VolCB for each removable unit so drive aliasing */
  /* via the DEVICE=EXTDSKDD command can be supported.               */

  if (NumRemovableDisks != 0)
  {
     pExtraVolCBs = (NPVOLCB) pNextFreeCB;
     NumExtraVolCBs = NumRemovableDisks;
     pNextFreeCB = pNextFreeCB + (NumRemovableDisks * sizeof(VOLCB));
  }

}

/*--------------------------------------------------------------------------
;
;** InitBPBfromGeom - Initialize BPB using Geometry info
;
;   Initialize the BPB for a VolCB from the Geometry info returned by
;   the adapter driver.
;
;   VOID InitBPBfromGeom (NPVOLCB pVolCB, NPBPB pBPB, PGEOMETRY pGeometry)
;
;   ENTRY:    pVolCB           - input pointer to VolCB
;             pBPB             - pointer to BPB to fill in
;             pGeometry        - pointer to Geometry info
;
;   RETURN:   VOID
;
;   EFFECTS:
;
;----------------------------------------------------------------------------*/
VOID InitBPBfromGeom (pVolCB, pBPB, pGeometry)

NPVOLCB   pVolCB;
NPBPB     pBPB;
PGEOMETRY pGeometry;

{
   ULONG  TotalCylinders, TotalSectors, TotalDataSectors, temp;
   USHORT i;
   NPUNITCB pUnitCB;

   pUnitCB = pVolCB->pUnitCB;
   TotalSectors = pGeometry->TotalSectors;
   pUnitCB->LastRBA = TotalSectors;

   /* If the unit is removable and it's not bigger than a 2.88M drive  */
   /* then use one of the canned BPBs we have for the specified drive. */

   if ((pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE) && (TotalSectors <= 5760))
   {
      pVolCB->NumPhysCylinders = pGeometry->TotalCylinders;
      pVolCB->NumLogCylinders = pGeometry->TotalCylinders;

      switch (TotalSectors)
      {
          case 720:         /* 5.25 inch - 360 KB Drive */
            *pBPB = BPB_360KB;
            break;

          case 1440:        /* 3.5 inch - 720 KB Drive */
            *pBPB = BPB_720KB;
            break;

          case 2400:        /* 5.25 inch - 1.2M Drive */
            *pBPB = BPB_12MB;
            break;

          case 2880:        /* 3.5 inch - 1.44M Drive */
            *pBPB = BPB_144MB;
            break;

          case 5760:        /* 3.5 inch - 2.88M Drive */
            *pBPB = BPB_288MB;
            break;

          default:
            *pBPB = BPB_144MB;
            break;
      }
   }

   /* If it's a fixed disk, or a removable drive we dont have a canned */
   /* BPB for, then calculate the rest of the BPB.                     */

   else
   {
      if (TotalSectors > 0xffff)
         pBPB->BigTotalSectors = TotalSectors;
      else
         pBPB->TotalSectors = TotalSectors;

      /* If the drive doesnt return any Geometry information other than */
      /* TotalSectors, then create a virtual geometry for the drive,    */
      /* else copy the Geometry data into the BPB.                      */

      if (pGeometry->NumHeads != 0)
      {
         pBPB->BytesPerSector = pGeometry->BytesPerSector;
         pBPB->NumHeads = pGeometry->NumHeads;
         pBPB->SectorsPerTrack = pGeometry->SectorsPerTrack;
         pVolCB->NumPhysCylinders = pGeometry->TotalCylinders;
         pVolCB->NumLogCylinders = pGeometry->TotalCylinders;
      }
      else
      {
         pBPB->BytesPerSector = 512;
         pBPB->NumHeads = 64;
         pBPB->SectorsPerTrack = 32;
         TotalCylinders = TotalSectors / (64 * 32);

         pVolCB->NumPhysCylinders = TotalCylinders;
         pVolCB->NumLogCylinders = TotalCylinders;
      }

      /* If it's a removable drive, then calculate the file system fields */
      /* i.e. NumFATSectors, etc. in the BPB.                             */

      if (pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE)
      {
         /* Find appropriate DiskTable entry based on TotalSectors */

         for (i = 0; i < DISKTABLECOUNT; i++)
           if (TotalSectors <= DiskTable[i].NumSectors)
              break;

         fBigFat = DiskTable[i].Flags;
         pBPB->ReservedSectors = 1;
         pBPB->MaxDirEntries = DiskTable[i].MaxDirEntries;
         pBPB->SectorsPerCluster = DiskTable[i].SectorsPerCluster;
         pBPB->NumFATs = 2;
         pBPB->MediaType = 0xF0;

         /* Calculate number of FAT table sectors */

         if (fBigFat & vf_Big)
         {
            temp = (pBPB->SectorsPerCluster * 256) + 2;
            pBPB->NumFATSectors = (TotalSectors -
               ((pBPB->MaxDirEntries / 16) + 1) +        /* Dir + Reserved*/
               temp - 1 + (pBPB->SectorsPerCluster * 2)) / temp;
         }
         else
         {
            TotalDataSectors = TotalSectors + pBPB->SectorsPerCluster - 1;

            TotalDataSectors >>= DiskTable[i].Log2SectorsPerCluster;

            TotalDataSectors = ((TotalDataSectors + 1) & (0xFFFFFFFE)) + 2;

            temp = TotalDataSectors;

            TotalDataSectors >>= 1;

            pBPB->NumFATSectors = (TotalDataSectors + temp + 511) / 512;
         }
      }
   }
}


/********************** START OF SPECIFICATIONS *****************************
*                                                                           *
* SUBROUTINE NAME: Setup_Partition_VolCBs                                   *
*                                                                           *
* DESCRIPTIVE NAME: Installs a VolCB for each OS/2 partition found          *
*                   on every fixed disks.                                   *
*                                                                           *
* FUNCTION:  This routine searches all fixed disks (80H - 86H) looking      *
*            for a primary partition and extended volumes on each           *
*            disk.                                                          *
*                                                                           *
*            A VolCB is installed for every primary partition, then VolCBs  *   *
*            are installed for all extended volumes. If drive 80H did       *
*            not have a primary partition, it will not be searched for      *
*            extended volumes.  All following fixed drives will always      *
*            be searched for extended volumes whether it had a primary      *
*            partition or not.                                              *
*                                                                           *
* NOTES: The VolCBs for all physical fixed drives must be set up            *
*        and NumFixedDisks must be set to the number of fixed disks         *
*        prior to calling this routine.                                     *
*                                                                           *
* ENTRY POINT: Setup_Partition_VolCBs                                       *
*                                                                           *
* LINKAGE: Call Near                                                        *
*                                                                           *
* INPUT: None                                                               *
*                                                                           *
* EXIT-NORMAL: VolCBs installed for all partitions found                    *
*                                                                           *
* EXIT-ERROR: None                                                          *
*                                                                           *
* EFFECTS: Modifies NumPartitions                                           *
*                                                                           *
*********************** END OF SPECIFICATIONS *******************************/


void near Setup_Partition_VolCBs()

{
   NPVOLCB pPhysVolCB, pVolCB;
   ULONG   NumSectors;
   USHORT  iDisk, i;
   MBR     near *pMBR = (MBR near *) ScratchBuffer; /* Ptr to buffer for MBR */
   NPVOLCB pLogVolCB = (NPVOLCB) pNextFreeCB;

   /*-----------------------------------------------*/
   /*  Allocate VolCBs for all Primary Partitions   */
   /*-----------------------------------------------*/

   NextLogDriveNum = 2;
   pVolCB_DriveC = pLogVolCB;

   for (iDisk=0,pPhysVolCB=pVolCB_80; iDisk<NumFixedDisks; iDisk++,pPhysVolCB++)
   {

      /*** added by WKP - start */
      NumSectors = pPhysVolCB->MediaBPB.NumHeads *
                   pPhysVolCB->MediaBPB.SectorsPerTrack *
                   pPhysVolCB->NumPhysCylinders;

      if (NumSectors > 0xFFFF)
      {
         pPhysVolCB->MediaBPB.BigTotalSectors = NumSectors;
         pPhysVolCB->MediaBPB.TotalSectors = 0;
         pPhysVolCB->RecBPB.BigTotalSectors = NumSectors;
         pPhysVolCB->RecBPB.TotalSectors = 0;
      }
      else
      {
         pPhysVolCB->MediaBPB.TotalSectors = (USHORT)NumSectors;
         pPhysVolCB->RecBPB.TotalSectors = (USHORT)NumSectors;
      }
      /*** added by WKP -end */

      /* Read the Master Boot Record (MBR) and copy the partition table */
      /* into a temporary buffer for later use.                         */

      if (Read_Sector(pPhysVolCB,0L) == ERROR) /* Read MBR & check for error*/
          PartitionTables[iDisk].Bad_MBR,1;
      else
      {
         for (i = 0; i < 4; i++)
            PartitionTables[iDisk].PartitionTable[i]
                                             = pMBR->PartitionTable[i];
         if (Build_Next_VolCB(pPhysVolCB,0L) == ERROR)
            PartitionTables[iDisk].No_PrimPart,1;
      }
   }
   /*------------------------------------------------*/
   /*  Allocate VolCBs for all Extended Partitions   */
   /*------------------------------------------------*/

   Setup_Extended_Volumes();

   NumPartitions = NumPartitions - NumFTPartitions;
}

/*--------------------------------------------------------------------------
;
;** BuildNextVolCB - Build a VolCB for the next logical drive
;
;   This routine attempts to build a VolCB for a logical
;   fixed disk partition.  It is passed a pointer to the
;   physical VolCB (80H - 86H) of the disk containing the
;   partition and the offset in sectors of that
;   partition sector from the begining of the disk.
;
;   Process_Partition is then called to validate the
;   partition sector and see if there is a DOS partition.
;   If one is found, this VolCB is added to the chain and
;   initialized. The DOS boot sector for the partition
;   is then read in and Process_Boot is called to examine
;   it and build the BPB in the VolCB.
;
;   If there was no valid DOS partition, the VolCB will
;   remain available.
;
;   USHORT Build_Next_VolCB (NPVOLCB pPhysVolCB, ULONG PartitionOffset)
;
;   ENTRY:    pPhysVolCB       - Pointer to VolCB for Physical drive
;             PartitionOffset  - Partition sector offset
;
;   RETURN:   USHORT           - Result Code (NO_ERROR if valid partition)
;
--------------------------------------------------------------------------*/

USHORT Build_Next_VolCB(pPhysVolCB,PartitionOffset)

NPVOLCB pPhysVolCB;
ULONG   PartitionOffset;
{
   ULONG  SectorsInPartition, rba, VolBootRBA, CylinderSize;
   NPVOLCB pLogVolCB = (NPVOLCB) pNextFreeCB;
   USHORT rc = ERROR;

   if (NumPartitions < MAX_PARTITIONS)
   {
      pLogVolCB->PartitionOffset = PartitionOffset;
      pLogVolCB->MediaBPB.SectorsPerTrack=pPhysVolCB->MediaBPB.SectorsPerTrack;
      pLogVolCB->MediaBPB.NumHeads = pPhysVolCB->MediaBPB.NumHeads;

      /* If a valid partition is found, initialize the rest of the  */
      /* Logical Volume Control Block.                              */

      if (Process_Partition(pLogVolCB, (PULONG) &VolBootRBA,
                           (PULONG) &SectorsInPartition) == NO_ERROR)
      {
         CylinderSize = pLogVolCB->MediaBPB.SectorsPerTrack *
                        pLogVolCB->MediaBPB.NumHeads;

         pLogVolCB->NumLogCylinders = SectorsInPartition  / CylinderSize;

         if ( (SectorsInPartition  % CylinderSize) != 0)
             pLogVolCB->NumLogCylinders++;

         /* Link new VolCB after last logical VolCB  */
         /*  and before physical Drive 80 VolCB      */

         pLogVolCB->pNextVolCB = pVolCB_80;
         pLastLogVolCB->pNextVolCB = pLogVolCB;
         pLastLogVolCB = pLogVolCB;

         /* Copy over applicable fields from the Physical VolCB */

         pLogVolCB->pUnitCB = pPhysVolCB->pUnitCB;
         pLogVolCB->pVolChar = pPhysVolCB->pVolChar;

         if (pLogVolCB->Flags & vf_FTPartition)
           pLogVolCB->LogDriveNum = MAX_DRIVE_LETTERS + NumFTPartitions;
         else
           pLogVolCB->LogDriveNum = NextLogDriveNum++;

         pLogVolCB->PhysDriveNum = pPhysVolCB->PhysDriveNum;
         pLogVolCB->NumPhysCylinders = pPhysVolCB->NumPhysCylinders;

         /* Setup default BPB fields for FORMAT */

         pLogVolCB->MediaBPB.MediaType = MEDIA_FIXED_DISK;
         pLogVolCB->MediaBPB.BytesPerSector = 512;
         pLogVolCB->MediaBPB.ReservedSectors = 1;
         pLogVolCB->MediaBPB.NumFATs = 2;

         pLogVolCB->RecBPB.MediaType = MEDIA_FIXED_DISK;
         pLogVolCB->RecBPB.BytesPerSector = 512;
         pLogVolCB->RecBPB.ReservedSectors = 1;
         pLogVolCB->RecBPB.NumFATs = 2;

         /* Read the DOS boot record into the ScratchBuffer */

         rba = pLogVolCB->PartitionOffset + pLogVolCB->MediaBPB.HiddenSectors;
         rc = Read_Sector (pPhysVolCB, rba);  /* Read DOS boot sector */
         rc = Is_BPB_Boot((VOID _far *)&ScratchBuffer);
         if (!rc)                       /* Is boot sector valid ?      */

            /* copy boot bpb to media bpb */
            BootBPB_To_MediaBPB (pLogVolCB, (DOSBOOTREC FAR *) &ScratchBuffer);

         /* Call Process_Boot to examine the boot sector and build the BPB */

         if (pLogVolCB->MediaBPB.TotalSectors != 0)
            SectorsInPartition = pLogVolCB->MediaBPB.TotalSectors;
         else
            SectorsInPartition = pLogVolCB->MediaBPB.BigTotalSectors;

         Process_Boot (pLogVolCB, SectorsInPartition);

         NumPartitions++;
         pLogVolCB++;
         NumVolCBs++;
         (NPVOLCB) pNextFreeCB = pLogVolCB;
         rc = NO_ERROR;
      }
   }
   return(rc);
}

/*********************** Start of Specifications **********************
*                                                                     *
* Subroutine Name: Setup_Extended_Volumes                             *
*                                                                     *
* Function: Sets up the VolCB entries for all extended volumes        *
*           found on a given fixed disk.                              *
*                                                                     *
* Entry Point: Setup_Extended_Volumes                                 *
*                                                                     *
* Linkage:  CALL near                                                 *
*                                                                     *
* Input: Physical BDS,                                                *
*        ScratchBuffer containing Master Boot Record                  *
*                                                                     *
* Exit-Normal:                                                        *
*                                                                     *
* Exit-Error:                                                         *
*                                                                     *
************************* End of Specifications ***********************/

void Setup_Extended_Volumes ()

{
   ULONG   rba, Nextrba, MBRExtRBA;
   MBR     *pMBR;
   NPVOLCB pPhysVolCB;
   USHORT  i, j, found;
   PARTITIONENTRY *pPartitionEntry;

   pMBR = (MBR *) ScratchBuffer;
   pPhysVolCB = pVolCB_80;


   /* Search all disks for extended partitions, and setup a VolCB */
   /* for each extended partition found.                          */

   for (i = 0; i < NumFixedDisks; i++, pPhysVolCB++)
   {
      if (i == 0 && PartitionTables[i].No_PrimPart == 1)
         continue;

      if (PartitionTables[i].Bad_MBR == 0)
      {
         found = FALSE;
         for  (j = 0; j < 4 && found == FALSE; j++)
         {
           pPartitionEntry = &(PartitionTables[i].PartitionTable[j]);
           if (pPartitionEntry->SysIndicator == PARTITION_EBR)
           {
//            rba = CHS_to_RBA(pPhysVolCB,
//                             pPartitionEntry->BegCylinder,
//                             pPartitionEntry->BegHead,
//                             pPartitionEntry->BegSector);
//
              rba = pPartitionEntry->RelativeSectors;
              MBRExtRBA = rba;
              found = TRUE;
           }
         }

         /* If the Master Boot Record contained an Extended Partition Type, */
         /* then read in the Extended Boot Record                           */

         while (found == TRUE)
         {
            found = FALSE;
            if (Read_Sector(pPhysVolCB, rba) == NO_ERROR)
            {
               for  (j = 0; j < 4 && found == FALSE; j++)
               {
                  pPartitionEntry = &(pMBR->PartitionTable[j]);
                  if (pPartitionEntry->SysIndicator == PARTITION_EBR)
                  {
//                   Nextrba = CHS_to_RBA(pPhysVolCB,
//                                     pPartitionEntry->BegCylinder,
//                                     pPartitionEntry->BegHead,
//                                     pPartitionEntry->BegSector);
                     Nextrba = pPartitionEntry->RelativeSectors + MBRExtRBA;
                     found = TRUE;
                  }
               }
               Build_Next_VolCB(pPhysVolCB, rba);
               rba = Nextrba;
            }
         }
      }
   }
}

/*---------------------------------------------------------------
;
;** Read_Sector - performs disk reads during initialization
;
;   Reads a sector from a fixed disk into ScratchBuffer.
;   This routine sets up a hard coded read request packet
;   and calls the Adapter Driver to perform the read.
;
;   USHORT Read_Sector (NPVOLCB pPhysVolCB, ULONG rba)
;
;   ENTRY:    pPhysVolCB       - Physical VolCB of disk to read
;             rba              - RBA of sector to read
;
;   RETURN:   USHORT           - Result Code (NO_ERROR if successful)
;
;   EFFECTS:  Reads a sector into global variable ScratchBuffer.
;
;   NOTES:    This routine is DISCARDED after init time.
;--------------------------------------------------------------*/

USHORT  Read_Sector(pPhysVolCB, rba)

NPVOLCB  pPhysVolCB;
ULONG    rba;

{
   PRP_RWV  pRP;
   NPIORB   pIORB;
   NPUNITCB pUnitCB;
   USHORT   rc;


   /* Set up the request packet for the read */

   pRP = &InitTimeRP;
   pRP->rph.Unit = pPhysVolCB->PhysDriveNum;
   pRP->rph.Cmd = CMDINPUT;
   pRP->rph.Status = 0;
   pRP->MediaDescr = MEDIA_FIXED_DISK;
   pRP->XferAddr = ppScratchBuffer;      /* Point to scratch buffer */
   pRP->NumSectors = 1;                  /* Read 1 sector */
   pRP->rba = rba;                       /* Store rba */
   pRP->sfn = 512;                       /* Use sfn field for SectorSize */

   pUnitCB = pPhysVolCB->pUnitCB;

   pIORB = (NPIORB) InitTimeIORB;
   f_ZeroCB((PBYTE)pIORB, MAX_IORB_SIZE);

   SetupIORB(pUnitCB, (PBYTE) pRP, pIORB);

   (pUnitCB->AdapterDriverEP) ((PVOID) pIORB);

   DISABLE;
   while ( !(pRP->rph.Status & STDON) )    /* Loop until I/O done   */
   {
      DevHelp_ProcBlock ((ULONG)pRP, -1L, 1);  /* Block: No timeout,non-interruptible*/
      DISABLE;                          /* Block does an enable  */
   }
   ENABLE;

   if (pRP->rph.Status & STERR)          /* Check for error */
      rc = ERROR;
   else
      rc = NO_ERROR;

   return(rc);
}

/*--------------------------------------------------------------------------
;
;** InitDCS_VCS
;
;   Initialize the DCS and VCS control blocks
;
;   This function will initialize the statically allocated
;   DriverCapabilities Structure (DCS). It will also dynamically
;   allocate one Volume Characteristics Structure (VCS) for each unit
;   and initialize it.
;
;   VOID InitDCS_VCS ()
;
;   ENTRY:
;
;   RETURN:   VOID
;---------------------------------------------------------------------------*/

VOID InitDCS_VCS()

{
   VolChars *pVCS;
   NPUNITCB pUnitCB;
   USHORT   i;
   NPVOLCB  pVolCB;

   /* Initialize the Driver Capabilites Structure */

   DriverCapabilities.VerMajor = 1;
   DriverCapabilities.VerMinor = 0;
   DriverCapabilities.Capabilities = GDC_DD_Mirror   | GDC_DD_Duplex |
                                     GDC_DD_No_Block | GDC_DD_16M;
   DriverCapabilities.Strategy2   = (PVOID) DMStrat2;
   DriverCapabilities.SetFSDInfo  = (PVOID) DD_SetFSDInfo;
   DriverCapabilities.ChgPriority = (PVOID) DD_ChgPriority_asm;
   DriverCapabilities.SetRestPos  = 0;
   DriverCapabilities.GetBoundary = 0;


   /* Allocate and initialize a VCS for each volume */

   pVCS = (VolChars *) pNextFreeCB;
   pVolCB = pVolCB_DriveA;

   for (i = 0; i < NumVolCBs; i++)
   {
      pVolCB->pVolChar = pVCS;

      pVCS->VolDescriptor = 0;
      pVCS->AvgSeekTime = -1;
      pVCS->AvgLatency =  -1;
      pVCS->TrackMinBlocks = pVolCB->RecBPB.SectorsPerTrack;
      pVCS->TrackMaxBlocks = pVolCB->RecBPB.SectorsPerTrack;
      pVCS->HeadsPerCylinder = pVolCB->RecBPB.NumHeads;
      pVCS->VolCylinderCount = pVolCB->NumLogCylinders;

      if (pVolCB->pUnitCB != 0)
      {
         pUnitCB = pVolCB->pUnitCB;

         pVCS->VolMedianBlock = pUnitCB->LastRBA / 2;
         pVCS->MaxSGList = pUnitCB->MaxHWSGList;
         if (pUnitCB->MaxHWSGList == 0)
            pVCS->MaxSGList = -1;

         if (pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE)
            pVCS->VolDescriptor |= VC_REMOVABLE_MEDIA;

         if (pUnitCB->UnitInfo.UnitFlags & UF_PREFETCH)
            pVCS->VolDescriptor |= VC_PREFETCH;

         if (pUnitCB->Flags & UCF_HW_SCATGAT)
            pVCS->VolDescriptor |= VC_SCB;

         if (pUnitCB->UnitInfo.UnitType == UIB_TYPE_CDROM)
            pVCS->VolDescriptor |= VC_READ_ONLY;
      }
      pVCS++;
      pVolCB = pVolCB->pNextVolCB;
   }
   pNextFreeCB = (NPBYTE) pVCS;
}


/* Dummy notification callout for ADDs during init processing */

VOID FAR InitPost(pIORB)

PIORB pIORB;
{

}
/*--------------------------------------------------------------------------
;
;** GetInitParms - Get init parms from BASEDEV command line
;
;   VOID GetInitParms (PRPINITIN pRP);
;
;   ENTRY:    pRP              - Pointer to init request packet
;
;   RETURN:   VOID
;
;   EFFECTS:  Turns on Queueing flags in global DDFlags.
;
;---------------------------------------------------------------------------*/
VOID GetInitParms (pRP)

PRPINITIN pRP;

{
   PSZ    pCmdString;
   USHORT i;

   pCmdString = pRP->InitArgs;
   OFFSETOF(pCmdString) = ((PDDD_Parm_List)pRP->InitArgs)->cmd_line_args;

   for (i = 0; *pCmdString != 0 && i < 40; i++, pCmdString++)
   {
      if (*pCmdString == '/')
      {
         if ( ( *(pCmdString+1) == 'Q' || *(pCmdString+1) == 'q' ) &&
              ( *(pCmdString+2) == 'F' || *(pCmdString+2) == 'f' ) )
         {
            if ( *(pCmdString + 3) == ':' )
            {
               switch ( *(pCmdString + 4) )
               {
                  case '1':
                      DDFlags |= DDF_ELEVATOR_DISABLED;
                      break;

                  case '2':
                      DDFlags |= DDF_PRTYQ_DISABLED;
                      break;

                  case '3':
                      DDFlags |= (DDF_ELEVATOR_DISABLED | DDF_PRTYQ_DISABLED);
                      break;
                }
            }
         }
         else
         {
            if ( ( *(pCmdString+1) == 'T' || *(pCmdString+1) == 't' ) &&
                 ( *(pCmdString+2) == 'R' || *(pCmdString+2) == 'r' ) )

            TraceFlags |= TF_INTERNAL;
         }
      }
   }
}

/*--------------------------------------------------------------------------
;
;** Init_Trace -  Initialize RAS, DEKKO, PERFVIEW and Internal tracing
;
;   VOID Init_Trace ()
;
;   ENTRY:
;
;   RETURN:   VOID
;---------------------------------------------------------------------------*/
VOID Init_Trace()
{

   PPVOID   ppSysInfoSeg;
   NPUNITCB pUnitCB;
   USHORT   i;

   /* Get pointer to RAS Major Event Code Table */

   DevHelp_GetDOSVar(DHGETDOSV_SYSINFOSEG, 0, (PPVOID) &ppSysInfoSeg);

   SELECTOROF(pSysInfoSeg) = (USHORT) *ppSysInfoSeg;
   OFFSETOF(pSysInfoSeg) = 0;

   pSIS_mec_table = ((PInfoSegGDT)pSysInfoSeg)->SIS_mec_table;


   /* Register PerfView Data and Text Blocks for each Non-Removable */
   /* physical unit.                                                */

   pUnitCB = UnitCB_Head;
   for (i = 0; i < NumUnitCBs; i++, pUnitCB++)
   {
      if ( !(pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE) )
      {
         pUnitCB->PerfViewDB.pfdbh.dbh_ulTotLen = sizeof(PVDB);
         pUnitCB->PerfViewDB.pfdbh.dbh_flFlags = RPC_FL_16BIT | RPC_FL_DD;

         PerfViewTB.tbh_bidID.bid_usInstance = 0;
         PerfViewTB.tbh_bidID.bid_usGroup = 0;

         DevHelp_RegisterPerfCtrs( (NPBYTE)&pUnitCB->PerfViewDB,
                                   (NPBYTE)&PerfViewTB, RPC_FL_16BIT );

         if (GroupName[6] == '9')
         {
            GroupName[6]='0';
            GroupName[5]++;
         }
         else
            GroupName[6]++;
      }
   }

   /* If internal tracing enabled, then allocate trace buffer  */
   /* Note: Always will be enough since init data is > 1 K.    */

// if (TraceFlags & TF_INTERNAL)
// {
      pDMTraceBuf = pNextFreeCB;
      pNextFreeCB += TRACEBUF_SIZE;
      pDMTraceHead = pDMTraceBuf;
      pDMTraceEnd =  pDMTraceBuf + TRACEBUF_SIZE;
// }
}





