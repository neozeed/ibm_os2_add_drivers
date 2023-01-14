/*static char *SCCSID = "@(#)s506init.c	6.5 92/01/22";*/
/****************************************************************************
 *                                                                          *
 *                (c) Copyright  IBM Corporation  1981, 1990                *
 *                           All Rights Reserved                            *
 *                                                                          *
 ****************************************************************************/

/********************* Start of Specifications ******************************
 *                                                                          *
 *  Source File Name: S506INIT.C                                            *
 *                                                                          *
 *  Descriptive Name: Family 1 Hard Disk Driver Initialization routines     *
 *                                                                          *
 *  Copyright:                                                              *
 *                                                                          *
 *  Status:                                                                 *
 *                                                                          *
 *  Function: Part of S506 device driver for OS/2 family 1                  *
 *                                                                          *
 *                                                                          *
 *  Notes:                                                                  *
 *    Dependencies:                                                         *
 *    Restrictions:                                                         *
 *                                                                          *
 *  Entry Points:  DriveInit                                                *
 *                                                                          *
 *  External References:  See EXTRN statements below                        *
 *                                                                          *
 ********************** End of Specifications *******************************/

 #define INCL_NOBASEAPI
 #define INCL_NOPMAPI
 #define INCL_NO_SCB
 #define INCL_INITRP_ONLY
 #define INCL_DOSERRORS
 #include "os2.h"                  // C:\DRV6\H
 #include "dos.h"                  // C:\DRV6\H
 #include "bseerr.h"               // C:\DRV6\H
 #include "infoseg.h"                  // C:\DRV6\H
 #include <scb.h>        /* needed to keey abios.h happy */
 #include <abios.h>

 #include "iorb.h"                 // C:\DRV6\SRC\DEV\DASD\DISKH
 #include "reqpkt.h"               // C:\DRV6\SRC\DEV\DASD\DISKH
 #include "dhcalls.h"
 #include "addcalls.h"

 #include "s506hdr.h"
 #include "s506ext.h"
 #include "s506pro.h"

/*--------------------- START OF SPECIFICATIONS --------------------------*
 *                                                                        *
 * SUBROUTINE NAME:  DriveInit                                            *
 *                                                                        *
 * DESCRIPTIVE NAME: Hard Disk device driver Initialization               *
 *                                                                        *
 * FUNCTION:                                                              *
 *                                                                        *
 *                                                                        *
 * NOTES: This routine runs in protect mode only as a single thread       *
 *        at CPL = 0.                                                     *
 *                                                                        *
 * ENTRY POINT:                                                           *
 *                                                                        *
 * LINKAGE:                                                               *
 *                                                                        *
 * INPUT: []    -                                                         *
 *                                                                        *
 * EXIT-NORMAL:                                                           *
 *                                                                        *
 * EXIT-ERROR:                                                            *
 *                                                                        *
 *                                                                        *
 * EFFECTS:                                                               *
 *                                                                        *
 * INTERNAL REFERENCES:                                                   *
 *    ROUTINES:  -                                                        *
 *               -                                                        *
 *                                                                        *
 * DATA STRUCTURES:                                                       *
 *                                                                        *
 *                                                                        *
 *                                                                        *
 * EXTERNAL REFERENCES:                                                   *
 *    ROUTINES:                                                           *
 *                                                                        *
 *                                                                        *
 *********************** END OF SPECIFICATIONS ****************************/

/********************** START OF PSEUDO CODE ******************************
 *                                                                        *
 *       Save the DevHlp routine address for the DevHlp macro             *
 *                                                                        *
 *       Initialize disk device driver trace (call TraceInit)             *
 *                                                                        *
 *       Initialize timer (call TimerInit)                                *
 *                                                                        *
 *       Set H/W config data in BDS for all drives (call SetROMCfg)       *
 *                                                                        *
 *       Invoke fixed disk initialization routine (call Fixed_Init)       *
 *                                                                        *
 *       Return pointer to BPBs in request packet                         *
 *                                                                        *
 *       Return offset ending address of code in request packet           *
 *                                                                        *
 * |     Calculate the number of extra BPB entries                        *
 *                                                                        *
 *       Fill in the FDInfo table for all logical drives                  *
 *                                                                        *
 *      Adjust the pointer to the end of the data segment                 *
 *                                                                        *
 *      Deny further init commands and don't spin on I/O anymore          *
 *                                                                        *
 *      Return to SYSINIT                                                 *
 *                                                                        *
 ************************* END OF PSEUDO CODE *****************************/

VOID NEAR DriveInit(PRPINITIN pRPH )
{
  PRPINITOUT                    pRPO = (PRPINITOUT) pRPH;
  NPACB                         npACB;
  BOOL Initialization = SUCCESS;
  USHORT rc=0;

  PIORBH pTestIORB;
  PVOID SelOffset;
  ULONG PhysAddr;
  NPSCATGATENTRY npSG;
  USHORT i;
  PUSHORT pTest;

// Remember to change s506data.c when using CHS addressing mode

//RBACHS addr;


/*------------------------------------------------------------------*
 * Init Request Packet is comming in with the following information *
 *------------------------------------------------------------------*/

  Device_Help = pRPH->DevHlpEP;

  rc = DevHelp_GetLIDEntry( DEVID_DISK, 0, 0, &LID );

  if ( rc == ABIOS_NOT_PRESENT )
    {
    npACB = &acb;                 // get address of adapter control block

    Initialize_Vars(npACB);       // initialize variables

    TimerInit();                  // initialize timer
                                  //   also saves selector for SysInfoSeg

    Fixed_Init(npACB);            // fixed disk initialization

    SetROMCfg(pRPH, npACB);       // set h/w config data for Fix BDS

    if (NumFix == 0)              // If there are no ST506 drives in this system
      {
      Undo_Fixed_Init(npACB);     // undo fixed disk initialization
      Initialization = FAILURE;
      pRPO->rph.Status = STDON + ERROR_I24_QUIET_INIT_FAIL;
      }
    else
      {
      NotifyOS2DASD();            // Register ADD
      }
    }
  else
    {
    if ( rc == 0 )
      DevHelp_FreeLIDEntry( LID );
    Initialization = FAILURE;
    pRPO->rph.Status = STDON + ERROR_I24_QUIET_INIT_FAIL;
    }

/*------------------------------------------------------------------*
 * Clean up INIT Request Packet so we can return to OS/2 kernel     *
 *                                                                  *
 *------------------------------------------------------------------*/

  if ( Initialization == SUCCESS )
    {
    pRPO->CodeEnd = (USHORT) DriveInit;
    pRPO->DataEnd = (USHORT) &TestIORB;
    pRPO->rph.Status = STDON;
    }
  else
    {
    pRPO->CodeEnd = (USHORT) 0;
    pRPO->DataEnd = (USHORT) 0;
//  Note: pRPO->rph.Status is set with appropriate rc above
    }

                                       // Init is done

/*------------------------------------------------------------------*
 * Testing....                                                      *
 *                                                                  *
 *------------------------------------------------------------------*/

//INT3
//TestIORB.pDeviceTable = (PDEVICETABLE) &TestDeviceTable;
//TestIORB.DeviceTableLen = 1024;
//TestIORB2.pUnitInfo = (PUNITINFO) &TestUnitInfo;
//TestIORB3.pGeometry = (PGEOMETRY) &TestGeometry;
//TestIORB4.pUnitInfo = (PUNITINFO) &TestUnitInfo;      // for CHANGE_UNIT_INFO
//TestUnitInfo.AdapterIndex     = 0000;
//TestUnitInfo.UnitIndex        = 1111;
//TestUnitInfo.UnitFlags        = 0xffff;
//TestUnitInfo.UnitHandle       = 2222;
//TestUnitInfo.FilterADDHandle  = 3333;
//TestUnitInfo.UnitType         = 4444;
//TestUnitInfo.QueuingCount     = 5555;
//TestUnitInfo.UnitSCSITargetID = 66;
//TestUnitInfo.UnitSCSILUN      = 77;
//
//---------------Start Test Case 1 ------------------------------------------*/
//TestIORBRead.cSGList       = 16;
//TestIORBRead.pSGList       = (PSCATGATENTRY) &TestSGList00;
//npSG                       = (NPSCATGATENTRY) &TestSGList00;
//
//SelOffset = (PVOID) &(TestBuffer[0]);
//DevHelp_VirtToPhys( SelOffset, &PhysAddr );
//DevHelp_AllocPhys( (ULONG) 65536, 0, &PhysAddr );
//for (i=0; i<16; i++)
//  {
//  npSG->ppXferBuf = PhysAddr;
//  npSG->XferBufLen = (ULONG) 65536;
//  ++npSG;
//  }
//
//DevHelp_PhysToGDTSelector( PhysAddr, 0, gdt_selector_IO );
//pTest = (PUSHORT) MAKEP(gdt_selector_IO, 0);
//
//for (i=0; i<32768; i++, pTest++)
//  *pTest = i;
//
//TestIORBRead.RBA           = 0;             // Read RBA = 0 - boot record
//TestIORBRead.RBA           = (ULONG) 0041565;
//
//addr.chs.Cylinder = 203;
//addr.chs.Head     = 0;
//addr.chs.Sector   = 15;
//TestIORBRead.RBA  = addr.rba;         // Read chs = (0203,0000,0015)
//
//TestIORBRead.BlockCount    = 1279;          // read 1279 sectors
//TestIORBRead.BlocksXferred = 0;
//TestIORBRead.BlockSize     = 512;
//
//
//TestIORBWrite.cSGList       = 16;
//TestIORBWrite.pSGList       = (PSCATGATENTRY) &TestSGList00;
//npSG                        = (NPSCATGATENTRY) &TestSGList00;
//
//SelOffset = (PVOID) &(TestBuffer[0]);
//DevHelp_VirtToPhys( SelOffset, &PhysAddr );
//DevHelp_AllocPhys( (ULONG) 65536, 0, &PhysAddr );
//for (i=0; i<16; i++)
//  {
//  npSG->ppXferBuf = PhysAddr;
//  npSG->XferBufLen = (ULONG) 65536;
//  ++npSG;
//  }
//
//DevHelp_PhysToGDTSelector( PhysAddr, 0, gdt_selector_IO );
//pTest = (PUSHORT) MAKEP(gdt_selector_IO, 0);
//
//for (i=0; i<32768; i++, pTest++)
//  *pTest = 0x7777;
//
//TestIORBWrite.RBA           = (ULONG) 0041565;
//
//addr.chs.Cylinder = 203;
//addr.chs.Head     = 0;
//addr.chs.Sector   = 15;
//TestIORBWrite.RBA  = addr.rba;         // Write chs = (0203,0000,0015)
//
//TestIORBWrite.BlockCount    = 1279;          // read 1279 sectors
//TestIORBWrite.BlocksXferred = 0;
//TestIORBWrite.BlockSize     = 512;
//-----------------End Test Case 1-------------------------------------------*/
//
//
//---------------Start Test Case 2 ------------------------------------------*/
//TestIORBRead.cSGList       = 3;
//TestIORBRead.pSGList       = (PSCATGATENTRY) &TestSGList00;
//npSG                       = (NPSCATGATENTRY) &TestSGList00;
//
//SelOffset = (PVOID) &(TestBuffer[0]);
//DevHelp_VirtToPhys( SelOffset, &PhysAddr );
//npSG->ppXferBuf = PhysAddr;
//npSG->XferBufLen = 17;
//++npSG;
//
//SelOffset = (PVOID) &(TestBuffer[17]);
//DevHelp_VirtToPhys( SelOffset, &PhysAddr );
//npSG->ppXferBuf = PhysAddr;
//npSG->XferBufLen = 1;
//++npSG;
//
//SelOffset = (PVOID) &(TestBuffer[18]);
//DevHelp_VirtToPhys( SelOffset, &PhysAddr );
//npSG->ppXferBuf = PhysAddr;
//npSG->XferBufLen = 494;
//++npSG;
//
//TestIORBRead.RBA           = 0;             // Read chs = 0,0,0 - boot record
//TestIORBRead.BlockCount    = 1;
//TestIORBRead.BlocksXferred = 0;
//TestIORBRead.BlockSize     = 512;
//
//
//
//TestIORBWrite.cSGList       = 3;
//TestIORBWrite.pSGList       = (PSCATGATENTRY) &TestSGList00;
//npSG                       = (NPSCATGATENTRY) &TestSGList00;
//
// already filled in above by readiorb
//
//SelOffset = (PVOID) &(TestBuffer[0]);
//DevHelp_VirtToPhys( SelOffset, &PhysAddr );
//npSG->ppXferBuf = PhysAddr;
//npSG->XferBufLen = 17;
//++npSG;
//
//SelOffset = (PVOID) &(TestBuffer[17]);
//DevHelp_VirtToPhys( SelOffset, &PhysAddr );
//npSG->ppXferBuf = PhysAddr;
//npSG->XferBufLen = 1;
//++npSG;
//
//SelOffset = (PVOID) &(TestBuffer[18]);
//DevHelp_VirtToPhys( SelOffset, &PhysAddr );
//npSG->ppXferBuf = PhysAddr;
//npSG->XferBufLen = 494;
//++npSG;
//
//TestIORBWrite.RBA           = 173;           // write to chs = 1,0,4
//TestIORBWrite.BlockCount    = 1;
//TestIORBWrite.BlocksXferred = 0;
//TestIORBWrite.BlockSize     = 512;
//-----------------End Test Case 2-------------------------------------------*/
//
//
//TestIORBVerify.cSGList       = 1;
//TestIORBVerify.pSGList       = (PSCATGATENTRY) &TestSGList2;
//SelOffset = (PVOID) &TestBuffer;
//DevHelp_VirtToPhys( SelOffset, &PhysAddr );
//TestIORBVerify.pSGList->ppXferBuf = PhysAddr;
//TestIORBVerify.pSGList->XferBufLen = 512;
//TestIORBVerify.RBA           = 0;             // Verify chs = 0,0,0
//TestIORBVerify.BlockCount    = 1279;
//TestIORBVerify.BlocksXferred = 0;
//TestIORBVerify.BlockSize     = 512;
//
//
//pTestIORB = (PIORBH)&TestIORB;                // GET_DEVICE_TABLE
//ADDEntryPoint( (PIORBH)pTestIORB);
//
//pTestIORB = (PIORBH)&TestIORB2;               // ALLOCATE_UNIT
//ADDEntryPoint( (PIORBH)pTestIORB);
//
//pTestIORB = (PIORBH)&TestIORB3;               // GET_DEVICE_GEOMETRY
//ADDEntryPoint( (PIORBH)pTestIORB);
//
//pTestIORB = (PIORBH)&TestIORB4;               // CHANGE_UNITINFO
//ADDEntryPoint( (PIORBH)pTestIORB);
//
//pTestIORB = (PIORBH)&TestIORB;                // GET_DEVICE_TABLE
//ADDEntryPoint( (PIORBH)pTestIORB);
//
//pTestIORB = (PIORBH)&TestIORB5;               // GET_UNIT_STATUS
//ADDEntryPoint( (PIORBH)pTestIORB);
//
//pTestIORB = (PIORBH)&TestIORBRead;            // READ
//ADDEntryPoint( (PIORBH)pTestIORB);
//
//pTestIORB = (PIORBH)&TestIORBRead;            // READ
//ADDEntryPoint( (PIORBH)pTestIORB);
//
//pTestIORB = (PIORBH)&TestIORBVerify;          // READ_VERIFY
//ADDEntryPoint( (PIORBH)pTestIORB);
//
//pTestIORB = (PIORBH)&TestIORBWrite;           // WRITE
//ADDEntryPoint( (PIORBH)pTestIORB);
//
//while (1) {;}

}

/*----------------------------------------------------------------------------*
 * Initialize_Vars                                                            *
 *                                                                            *
 *----------------------------------------------------------------------------*/
VOID NEAR Initialize_Vars( NPACB npACB )
{

// Miscellaneous initializations

  npACB->State = IDLE;
  npACB->NestedInts = -1;
  npACB->Flags = 0;
  npACB->Flags2 = 0;
  npACB->IntLevel = IRQ_FIXED;             // Fixed interrupt IRQ #

  npACB->IOPorts[FI_PDAT]     = FX_PDAT;
  npACB->IOPorts[FI_PWRP]     = FX_PWRP;
  npACB->IOPorts[FI_PERR]     = FX_PERR;
  npACB->IOPorts[FI_PSECCNT]  = FX_PSECCNT;
  npACB->IOPorts[FI_PSECNUM]  = FX_PSECNUM;
  npACB->IOPorts[FI_PCYLL]    = FX_PCYLL;
  npACB->IOPorts[FI_PCYLH]    = FX_PCYLH;
  npACB->IOPorts[FI_PDRHD]    = FX_PDRHD;
  npACB->IOPorts[FI_PCMD]     = FX_PCMD;
  npACB->IOPorts[FI_PSTAT]    = FX_PSTAT;
  npACB->IOPorts[FI_RFDR]     = FX_RFDR;
}

/*----------------------------------------------------------------------------*
 * TimerInit                                                                  *
 *                                                                            *
 *----------------------------------------------------------------------------*/
 BOOL NEAR TimerInit()
 {
   PVOID pDOSVar;

   if ( DevHelp_GetDOSVar( DHGETDOSV_SYSINFOSEG, 0, &pDOSVar ) )
      return FAILURE;

   pGlobalInfoSeg = MAKEP( *((PSEL)pDOSVar), 0 );

   MTick = pGlobalInfoSeg->SIS_ClkIntrvl / 10;  // MTick = MS/(System Tick)

   UTick = MTick * 1000;                // UTick = US/(System Tick)
   TimerConv = MTick * 1000;

//  Start a general purpose timer

   if ( DevHelp_TickCount( (NPFN)DDDTimer, TIMERCOUNT ) )
      return FAILURE;

   return SUCCESS;
}

/********************** START OF SPECIFICATIONS ***************************
 *                                                                        *
 * SetROMCfg                                                              *
 *                                                                        *
 * DESCRIPTIVE NAME: Set up BDS with parameters passed by the ROM.        *
 *                                                                        *
 * FUNCTION: Copy H/W configuration parameters passed in INIT command     *
 *           packet to BDS blocks for each drive. And also, set  # of     *
 *           physical fixed disks (NumFix).                               *
 *           If only one floppy drive exists, copy the physical drive     *
 *                                                                        *
 *           H/W configuration data is passed from System Initialization  *
 *           even if ABIOS machine is used.                               *
 *                                                                        *
 *           If this is a media less system, we will set the No_Media     *
 *           byte to one.  This will be used for Remote IPL.              *
 *                                                                        *
 * NOTES: Called by DriveInit                                             *
 *                                                                        *
 * ENTRY POINT: SetROMCfg                                                 *
 *                                                                        *
 * LINKAGE: Call Near                                                     *
 *                                                                        *
 * INPUT:  ES:BX -> point to INIT Command packet                          *
 *                                                                        *
 * EXIT-NORMAL: The following parameters are set in the BDS               *
 *                                                                        *
 *              cClyn         - # of cylinders                            *
 *              TotalPhysCyln - Total # of cylinders in partition         *
 *              DriveNum      - 0-based floppy dr #,80h-based fixed dr #  *
 *              BPBsecsiz     - # of sectors per track                    *
 *              BPBnHead      - # of heads                                *
 *              WrtPrCmp      - Write precompensation factor              *
 *              DrvTypFlag    - bit 0 : non-removable media               *
 *                              bit 1 : changeline is supported           *
 *              FormFactor    - form factor (fixed disks only)            *
 *                                                                        *
 *              other variables:                                          *
 *                                                                        *
 *              NumFix        - # of fixed drives                         *
 *                                                                        *
 * EXIT-ERROR: None                                                       *
 *                                                                        *
 * EFFECTS: None                                                          *
 *                                                                        *
 * INTERNAL REFERENCES:                                                   *
 *    ROUTINES:  None                                                     *
 *                                                                        *
 * DATA STRUCTURES: DriveStruc                                            *
 *                                                                        *
 * EXTERNAL REFERENCES:                                                   *
 *    ROUTINES:  None                                                     *
 *                                                                        *
 *********************** END OF SPECIFICATIONS ****************************/

 VOID NEAR SetROMCfg( PRPINITIN pRPH, NPACB npACB )
 {
  PSEL         pgdt_selector;

  PDDD_PARM_LIST pDDD_Parm_List;
  PROMCFG pROMCFG;
  PCMD_LINE_ARGS pcmd_line_args;
  UCHAR   i;

  /* allocate two GDT selectors */
  pgdt_selector = &gdt_selector;
  DevHelp_AllocGDTSelector( pgdt_selector, 2 );


// Alternate method for getting hard disk device geometry. Use the data
// passed inside the init request packet.
//
// IMPORTANT - This does NOT handle SCSI drives in the chain - must FIX
//           - also must check for > than MAXUNITS

  pDDD_Parm_List = (PDDD_PARM_LIST) pRPH->InitArgs;
  if (pDDD_Parm_List != NULL)
    {
    pcmd_line_args = (PCMD_LINE_ARGS) MAKEP(SELECTOROF(pDDD_Parm_List),
                                              OFFSETOF(pDDD_Parm_List->cmd_line_args));
    pcmd_line_args += 12;       // skip over IBM1S506.SYS
    while ( *pcmd_line_args != NULL )
      {
      if ( *pcmd_line_args == 'M' )
        npACB->Flags2 |= FSETMULTIPLE;          // attempt to enable multiple mode
      ++pcmd_line_args;
      }

    pROMCFG = (PROMCFG) MAKEP(SELECTOROF(pDDD_Parm_List),
                                OFFSETOF(pDDD_Parm_List->disk_config_table));

    NumFix = 0;                    // count of fixed disk drives
    i =0;
    while (OFFSETOF(pROMCFG) != NULL)
      {
      if (pROMCFG->ROMflags & ROMfixed)
        {

        npACB->unit[i].fDriveSetParam = 1;
        npACB->unit[i].ContUnit = i;
        npACB->unit[i].pUnitInfo = NULL;

// Ctrl is NOT enabled for read/write multiple and no double word I/O capability

        npACB->unit[i].DriveFlags &= ~(FMULTIPLEMODE + FDWORDIO);

        npACB->unit[i].LogGeometry.BytesPerSector = 512;
        npACB->unit[i].PhysGeometry.BytesPerSector = 512;
        npACB->unit[i].LogGeometry.TotalCylinders = pROMCFG->ROMcyls;  // maximum number of cylinders
        npACB->unit[i].PhysGeometry.TotalCylinders = pROMCFG->ROMcyls;
        npACB->unit[i].LogGeometry.SectorsPerTrack = pROMCFG->ROMsecptrk; // number of sectors per track

//          - set physical parameters equal to logical for now.  If this is a
//            translated drive, this will be changed in "get_xlate_parms"

        npACB->unit[i].PhysGeometry.SectorsPerTrack = pROMCFG->ROMsecptrk; // Physical sectors/track
        npACB->unit[i].DriveNum = pROMCFG->ROMdevnbr;
        npACB->unit[i].LogGeometry.NumHeads = pROMCFG->ROMheads; // maximum number of heads

//          - set physical parameters equal to logical for now.  If this is a
//            translated drive, this will be changed in "get_xlate_parms"

        npACB->unit[i].PhysGeometry.NumHeads = pROMCFG->ROMheads; // maximum number of heads

        npACB->unit[i].LogGeometry.TotalSectors =
                npACB->unit[i].LogGeometry.TotalCylinders  *
                npACB->unit[i].LogGeometry.NumHeads        *
                npACB->unit[i].LogGeometry.SectorsPerTrack;

        npACB->unit[i].PhysGeometry.TotalSectors =
                npACB->unit[i].PhysGeometry.TotalCylinders  *
                npACB->unit[i].PhysGeometry.NumHeads        *
                npACB->unit[i].PhysGeometry.SectorsPerTrack;

        npACB->unit[i].WrtPrCmp = pROMCFG->ROMwpf; // starting write precompensation cyl.

//           mov ax,es:[si].ROMflags          ; DrvTypflag
//           or  [di].DrvTypFlag,ax           ; (bit 0:fixed disk, bit 1:changeline)

        npACB->unit[i].FormFactor = ffHardFile;  // set Hard disk FormFactor
        npACB->IntLevel = IRQ_FIXED;             // Fixed interrupt IRQ #

        if ( ST506Ctrl(npACB, i) == SUCCESS )
          {
          get_xlate_parms(npACB, i);      // check for phys translate values

          if ( npACB->Flags2 & FSETMULTIPLE )
            Set_Multiple_Mode(npACB, i);  // attempt to enable multiple mode

          ++NumFix;
          ++i;
          }
        } /*endif*/
      pROMCFG = (PROMCFG) MAKEP(SELECTOROF(pROMCFG), OFFSETOF(pROMCFG->ROMlink));
      } /*endwhile*/
    } /*endif*/
 }

/*----------------------------------------------------------------------------*
 * Fixed_Init                                                                 *
 *                                                                            *
 *            In this routine, enable Fixed Disk controller, and              *
 *            set up actual and recommended BPB for physical                  *
 *            fixed disks.                                                    *
 *                                                                            *
 *----------------------------------------------------------------------------*/
  VOID NEAR Fixed_Init( NPACB npACB )
  {

  DevHelp_SetIRQ( (NPFN) FixedInterrupt, npACB->IntLevel, 0 ); /* not shared */

// Timer masks are also set up here since they are also controller dependent

  npACB->TimerMask = FIXEDT1;

//  - Turn on >16MB addressing supported since AT type disks
//    use PIO to transfer data to the processor.

   npACB->Flags2 |= f16MegSup;      // >16MB addressing allowed

 return;
 }

/*----------------------------------------------------------------------------*
 * Undo_Fixed_Init                                                            *
 *                                                                            *
 *            No ST506 drives in this system, undo Fixed_Init.                *
 *                                                                            *
 *----------------------------------------------------------------------------*/
  VOID NEAR Undo_Fixed_Init( NPACB npACB )
  {

  DevHelp_UnSetIRQ( npACB->IntLevel );

// Timer masks are also reset up here since they are also controller dependent

  npACB->TimerMask &= ~FIXEDT1;

  DevHelp_ResetTimer( (NPFN)DDDTimer );

//  - Turn off >16MB addressing supported since AT type disks
//    use PIO to transfer data to the processor.

   npACB->Flags2 &= ~f16MegSup;      // >16MB addressing allowed

 return;
 }

/************************ START OF SPECIFICATIONS *****************************
 *                                                                            *
 * SUBROUTINE NAME:  NotifyOS2DASD                                            *
 *                                                                            *
 * DESCRIPTIVE NAME:                                                          *
 *                                                                            *
 * FUNCTION:                                                                  *
 *                                                                            *
 *                                                                            *
 *                                                                            *
 *                                                                            *
 *                                                                            *
 *                                                                            *
 * ENTRY POINT:                                                               *
 *                                                                            *
 * LINKAGE:                                                                   *
 *                                                                            *
 * INPUT:                                                                     *
 *                                                                            *
 * EXIT-NORMAL:                                                               *
 *                                                                            *
 *                                                                            *
 * EXIT-ERROR: None                                                           *
 *                                                                            *
 * EFFECTS:                                                                   *
 *                                                                            *
 * CALLED BY:                                                                 *
 *                                                                            *
 *                                                                            *
 ************************* END OF SPECIFICATIONS ******************************/
 VOID NEAR NotifyOS2DASD()
 {
 PFN      DriverEP    = (PFN) &ADDEntryPoint;
 BOOL     Status      = 0;

 Status = DevHelp_RegisterDeviceClass(AdapterName, DriverEP, 0, 1, &ADDHandle);
 return;
 }

/*----------------------------------------------------------------------------*
 * get_xlate_parms                                                            *
 *                                                                            *
 *  Check for a drive type that is stored internally                          *
 *                                                                            *
 *----------------------------------------------------------------------------*/
VOID NEAR get_xlate_parms( NPACB npACB, UCHAR i )
{
  USHORT       offset;
  PULONG       pInt4x;
  ULONG        Int4xRealAddr;
  ULONG        Int4xPhysAddr;
  ULONG        TempPhysAddr;
  PFDPARMTABLE_XLATE pFDPARMTABLE_XLATE;
  UCHAR        signature;
  UCHAR        checksum;
  UCHAR        j;
  UCHAR        far * pTbl;
  NPGEOMETRY   npPhysGeometry;
  NPGEOMETRY   npLogGeometry;


  /* map 1st 64K of physical memory to a GDT selector */
  TempPhysAddr = 0;
  DevHelp_PhysToGDTSelector( TempPhysAddr, 0, gdt_selector );

  offset = ( i )  ?  FDTAB_1 : FDTAB_0;
  pInt4x = MAKEP(gdt_selector, offset ); //point to int vector 41H or 46H
  Int4xRealAddr = *pInt4x;
  TempPhysAddr = (ULONG)SELECTOROF(Int4xRealAddr);
  Int4xPhysAddr = (TempPhysAddr<<4)+(ULONG)(OFFSETOF(Int4xRealAddr));

  /* map int4x contents to a GDT selector */
  DevHelp_PhysToGDTSelector( Int4xPhysAddr, 0, gdt_selector );
  pFDPARMTABLE_XLATE = (PFDPARMTABLE_XLATE) MAKEP(gdt_selector, 0);

  signature = pFDPARMTABLE_XLATE->FDtranslate;          // get the sign byte
  signature &= 0x00F0;                                  // strip lower nibble
  if ( signature == TRANSLATE_A0 )      // if Translate signature present
    {
    pTbl = (UCHAR far *) pFDPARMTABLE_XLATE;
    for ( j=0, checksum=0; j < sizeof(FDPARMTABLE_XLATE); j++, pTbl++)
      checksum +=  *(pTbl);
    if ( checksum == 0 )                // a good checksum == 0
      {
      npACB->unit[i].XlateFlags |= XLATE_ENABLE;  // set the xlate enable flag

      npPhysGeometry = &(npACB->unit[i].PhysGeometry);    // physical geometry

      npPhysGeometry->SectorsPerTrack = pFDPARMTABLE_XLATE->FDpsectors; // Physical sectors per track
      npPhysGeometry->NumHeads = pFDPARMTABLE_XLATE->FDpheads;          // number of Physical heads
      npPhysGeometry->TotalCylinders = pFDPARMTABLE_XLATE->FDpcyls;     // maximum number of cylinders

      npPhysGeometry->TotalSectors = npPhysGeometry->TotalCylinders  *
                                     npPhysGeometry->NumHeads        *
                                     npPhysGeometry->SectorsPerTrack;


      npLogGeometry  = &(npACB->unit[i].LogGeometry);     // logical geometry

      npLogGeometry->NumHeads = pFDPARMTABLE_XLATE->FDMaxHdX;           // number of heads
      npLogGeometry->SectorsPerTrack = pFDPARMTABLE_XLATE->FDSecTrkX;   // sectors per track

      npLogGeometry->TotalSectors = npLogGeometry->TotalCylinders  *
                                    npLogGeometry->NumHeads        *
                                    npLogGeometry->SectorsPerTrack;

      npACB->unit[i].WrtPrCmp = pFDPARMTABLE_XLATE->FDWritePCompX;      // Write precompensation factor
      }
    }
}

/*----------------------------------------------------------------------------*
 * Set_Multiple_Mode                                                          *
 *                                                                            *
 *----------------------------------------------------------------------------*/
BOOL NEAR Set_Multiple_Mode( NPACB npACB, UCHAR i )
{
  USHORT        Port;
  UCHAR         Data;
  UCHAR         Status;
  BOOL          rc = SUCCESS;
  NPGEOMETRY    npPhysGeometry;
  NPGEOMETRY    npLogGeometry;
  NPIDENTIFYDATA npid;
  USHORT        j;
  USHORT        DataOff;
  USHORT        DataSel;
  PIDENTIFYDATA pIdentifyData;

  Initialize_hardfile();  /*  enable hard disk */
  Enable_Hardfile();

  Data = i;             // set Data to unit number (0 or 1)
  Data = (Data << 4);
  Data |= 0x00a0;

  if ( (rc=WaitRdy(npACB)) != SUCCESS )
    {
    return FAILURE;
    }
  Port = npACB->IOPorts[FI_PDRHD];
  outp(Port, Data);                     // select the drive

  if ( (rc=WaitRdy(npACB)) != SUCCESS )
    {
    return FAILURE;
    }
  Port = npACB->IOPorts[FI_PCMD];
  Data = FX_IDENTIFY;
  outp(Port, Data);           // issue identify drive command

  Port = npACB->IOPorts[FI_PSTAT];
  for (j=0; j < 0xffff; j++)
    {
    inp(Port, Status);                  // poll controller
    if ( (Status & FX_DRQ) )
      break;                            // controller identify gave OK
    }

  if ( !(Status & FX_DRQ) )
    {
//  We never received the DRQ.

    ResetST506Ctrl(npACB, i);
    rc = FAILURE;
    }
  else
    {
//  We are now going to read in 256 words of identify data.

    pIdentifyData = &(npACB->unit[i].id);
    DataOff = (USHORT) OFFSETOF(pIdentifyData);
    DataSel = (USHORT) SELECTOROF(pIdentifyData);

    _asm
      {
      push      cx
      push      dx
      push      di
      push      es
      mov       cx,256                  // read in 256 words
      mov       dx,FX_PDAT
      mov       di, word ptr DataOff    // get pointer to data location
      mov       es, word ptr DataSel    // ES:DI is 32-bit address of data
      cld
      DISABLE
      REPE      INSW
      ENABLE
      pop       es
      pop       di
      pop       dx
      pop       cx
      }

    npPhysGeometry = &(npACB->unit[i].PhysGeometry);    // physical geometry
    npLogGeometry  = &(npACB->unit[i].LogGeometry);     // logical geometry
    npid = &(npACB->unit[i].id);                // identify drive command data

    npPhysGeometry->TotalCylinders = npid->TotalCylinders;
    npPhysGeometry->SectorsPerTrack = npid->SectorsPerTrack;
    npPhysGeometry->NumHeads = npid->NumHeads;

    npPhysGeometry->TotalSectors = npPhysGeometry->TotalCylinders  *
                                   npPhysGeometry->NumHeads        *
                                   npPhysGeometry->SectorsPerTrack;

//  if ((npLogGeometry->TotalCylinders  != npPhysGeometry->TotalCylinders)  ||
//      (npLogGeometry->SectorsPerTrack != npPhysGeometry->SectorsPerTrack) ||
//      (npLogGeometry->NumHeads        != npPhysGeometry->NumHeads))
//    {
//    npACB->unit[i].XlateFlags |= XLATE_ENABLE;  // set the xlate enable flag
//    }
//  else
//    {
//    npACB->unit[i].XlateFlags &= ~XLATE_ENABLE; // clear the xlate enable flag
//    }

    npACB->unit[i].XlateFlags &= ~XLATE_ENABLE; // clear the xlate enable flag

    if (npid->DoubleWordIO & 1)
      npACB->unit[i].DriveFlags |= FDWORDIO;    // Ctrl double word I/O capability
    else
      npACB->unit[i].DriveFlags &= ~FDWORDIO;   // Ctrl NOT double word I/O capability

    Data = (UCHAR) npid->NumSectorsPerInt;
    npid->NumSectorsPerInt = (USHORT) 0;        // force NumSectorsPerInt to Byte value
    npid->NumSectorsPerInt = (USHORT) Data;

    if (npid->NumSectorsPerInt > 1)
      {
      Port = npACB->IOPorts[FI_PSECCNT];
      outp(Port, Data);                     // set the sector count register

      if ( (rc=WaitRdy(npACB)) != SUCCESS )
        {
        return FAILURE;
        }

      Port = npACB->IOPorts[FI_PCMD];
      Data = FX_SETMUL;
      outp(Port, Data);           // issue Set Multiple Mode

      npACB->unit[i].DriveFlags |= FMULTIPLEMODE;   // Ctrl is enabled for
                                                    //   read/write multiple
      }
    else
      {
      npACB->unit[i].DriveFlags &= ~FMULTIPLEMODE;  // Ctrl is NOT enabled for
                                                    //   read/write multiple
      }

    rc = SUCCESS;
    }

  return( rc );
}

/*----------------------------------------------------------------------------*
 * ST506Ctrl                                                                  *
 *                                                                            *
 * Determine if controller is a ST506 controller by issueing an IORB Read     *
 * Verify. If the read verify is successful, then we have a ST506 controller  *
 * (or at least a controller that emulates an ST506) ; otherwise reset the    *
 * controller.                                                                *
 *                                                                            *
 *----------------------------------------------------------------------------*/
BOOL NEAR ST506Ctrl( NPACB npACB, UCHAR i )
{
  BOOL          rc = SUCCESS;

  TestIORBVerify.iorbh.Length          = sizeof(IORB_EXECUTEIO);
  TestIORBVerify.iorbh.UnitHandle      = i;
  TestIORBVerify.iorbh.CommandCode     = IOCC_EXECUTE_IO;
  TestIORBVerify.iorbh.CommandModifier = IOCM_READ_VERIFY;
  TestIORBVerify.iorbh.RequestControl  = 0;
  TestIORBVerify.iorbh.Status          = 0;
  TestIORBVerify.iorbh.ErrorCode       = 0;
  TestIORBVerify.iorbh.Timeout         = 15;      // wait for 15 seconds
  TestIORBVerify.iorbh.StatusBlockLen  = 0;
  TestIORBVerify.iorbh.pStatusBlock    = NULL;
  TestIORBVerify.iorbh.pNxtIORB        = NULL;
  TestIORBVerify.iorbh.NotifyAddress   = NULL;

  TestIORBVerify.cSGList       = 1;
  TestIORBVerify.pSGList       = NULL;
  TestIORBVerify.RBA           = 0;     // Read Verify chs = 0,0,0 - boot record
  TestIORBVerify.BlockCount    = 1;
  TestIORBVerify.BlocksXferred = 0;
  TestIORBVerify.BlockSize     = 512;

  npACB->pHeadIORB = (PIORBH)&TestIORBVerify;     // READ VERIFY
  npACB->State = START;

  FixedExecute(npACB);     // call state machine

  while ( !(TestIORBVerify.iorbh.Status & IORB_DONE) ) {;}    // wait for read to complete

  if (   (TestIORBVerify.iorbh.Status & IORB_ERROR) &&
       (!(TestIORBVerify.iorbh.Status & IORB_RECOV_ERROR))  )
    {
    ResetST506Ctrl(npACB, i);
    rc = FAILURE;
    }
  else
    rc = SUCCESS ;

  return( rc );
}

/*----------------------------------------------------------------------------*
 * ResetST506Ctrl                                                             *
 *                                                                            *
 *  Perform a reset on the ST506 controller                                   *
 *                                                                            *
 *----------------------------------------------------------------------------*/
VOID NEAR ResetST506Ctrl( NPACB npACB, UCHAR i )
{
  USHORT        Port;
  UCHAR         Data;
  ULONG         j;

  Port = npACB->IOPorts[FI_RFDR];
  inp(Port, Data);                    // get device control register
  IOWait;
  Data |= FX_SRST;
  outp(Port, Data);                   // Soft Reset the disk controller

  for (j=0; j<MAXWAIT; j++)
    {
//  Soft Reset bit must be held active for minimum of 5 microsecs
//  to reset the drive
    }

  Data &= ~FX_SRST;
  outp(Port, Data);                   // re-enable the disk controller

  for (j=0; j<MAXWAIT; j++)
    {
//  spec says ctrl may take up to max. of 0.5 secs to recover from reset
    }

  npACB->unit[i].DriveFlags &= ~FMULTIPLEMODE;  // Ctrl is NOT enabled for
                                                //   read/write multiple cmds
  npACB->unit[i].DriveFlags &= ~FDWORDIO;   // Ctrl NOT double word I/O capability
  npACB->unit[i].fDriveSetParam = 1;    // Set drive parameters again
}
