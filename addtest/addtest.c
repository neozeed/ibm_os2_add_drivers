#pragma title("Adatper Device Driver Tester")
#pragma linesize( 120 )
#pragma pagesize( 60 )

#define INCL_NOPMAPI
#define INCL_DOSDEVICES
#define INCL_DOSPROCESS
#include <os2.h>
#include <strat2.h>     /* needed to keep reqpkt.h happy */
#include <reqpkt.h>
#include <iorb.h>
#include <stdlib.h>
#include <conio.h>


#define DD_NOT_FOUND    110
#define USERDEFCATAGORY 0x80
#define LOCKFUNCTION    0x40
#define UNLOCKFUNCTION  0x41
#define IORBFUNCTION    0x42
#define MAXSTR          100

typedef struct _ADDR
{
   PVOID        pLDT;
   PVOID        pGDT;
   ULONG        Phys;
   USHORT       Length;
} ADDR;

ADDR IORB   = {0};
ADDR SGList = {0};
ADDR Data   = {0};

unsigned Handle = 0;
char NumBuff[MAXSTR] = { MAXSTR + 2, 0 };




void GetInput( char Text[], unsigned far *Variable )
{
   char  Prompt[80];
   char *NumString;
   unsigned i;

   for ( i=0; Text[i]; i++ ) Prompt[i]=Text[i];
   for ( ; i<20; i++ ) Prompt[i]=' ';
   Prompt[i] = 0;

   printf("%s : %d\r%s : ", Prompt, *Variable, Prompt );

   NumString = cgets( NumBuff );
   if ( NumBuff[1] != 0 ) /* If anything was entered */
      *Variable = atoi( NumString );
   printf("\n");
}


void GetLongInput( char Text[], unsigned long far *Variable )
{
   char  Prompt[80];
   char *NumString;
   unsigned i;

   for ( i=0; Text[i]; i++ ) Prompt[i]=Text[i];
   for ( ; i<20; i++ ) Prompt[i]=' ';
   Prompt[i] = 0;

   printf("%s : %ld\r%s : ", Prompt, *Variable, Prompt );

   NumString = cgets( NumBuff );
   if ( NumBuff[1] != 0 ) /* If anything was entered */
      *Variable = atol( NumString );
   printf("\n");
}


void ShowDeviceTable()
{
   PDEVICETABLE pDevTbl = (PDEVICETABLE)Data.pLDT;
   PADAPTERINFO pAdapter;
   PUNITINFO    pUnit;
   USHORT a,u;

   printf("***** Device Table *****\n");
   printf("ADD Level Major      : %d\n", pDevTbl->ADDLevelMajor );
   printf("ADD Level Minor      : %d\n", pDevTbl->ADDLevelMinor );
   printf("ADD Handle           : %d\n", pDevTbl->ADDHandle );
   printf("Total Adapters       : %d\n", pDevTbl->TotalAdapters );

   for ( a=0; a<pDevTbl->TotalAdapters; a++ )
      {
         pAdapter = MAKEP( SELECTOROF(pDevTbl), pDevTbl->pAdapter[a] );

         printf("***** Adapter %d *****\n", a );
         printf("Adapter Name         : %s\n",   pAdapter->AdapterName );
         printf("Adapter Units        : %d\n",   pAdapter->AdapterUnits );
         printf("Adapter Device Bus   : %04X\n", pAdapter->AdapterDevBus );
         printf("Adapter I/O Access   : %02X\n", pAdapter->AdapterIOAccess );
         printf("Adapter Host Bus     : %02X\n", pAdapter->AdapterHostBus );
         printf("Adapter SCSI Trgt ID : %d\n",   pAdapter->AdapterSCSITargetID );
         printf("Adapter SCSI LUN     : %d\n",   pAdapter->AdapterSCSILUN );
         printf("Adapter Flags        : %04X\n", pAdapter->AdapterFlags );
         printf("Max Hardware SG List : %d\n",   pAdapter->MaxHWSGList );
         printf("Max CDB Xfer Length  : %ld\n",  pAdapter->MaxCDBTransferLength );

         for ( u=0; u<pAdapter->AdapterUnits; u++ )
            {
               pUnit = MAKEP( SELECTOROF(pAdapter), &pAdapter->UnitInfo[u] );

               printf("***** Unit %d *****\n", u );
               printf("Adpater Index        : %d\n",   pUnit->AdapterIndex );
               printf("Unit Index           : %d\n",   pUnit->UnitIndex );
               printf("Unit Flags           : %04X\n", pUnit->UnitFlags );
               printf("Unit Handle          : %d\n",   pUnit->UnitHandle );
               printf("Filter ADD Handle    : %d\n",   pUnit->FilterADDHandle );
               printf("Unit Type            : %d\n",   pUnit->UnitType );
               printf("Queuing Count        : %d\n",   pUnit->QueuingCount );
               printf("Unit SCSI Target ID  : %d\n",   pUnit->UnitSCSITargetID );
               printf("Unit SCSI LUN        : %d\n",   pUnit->UnitSCSILUN );
            }
      }
}


void ShowGeometry()
{
   PGEOMETRY pGeometry  = (PGEOMETRY)Data.pLDT;

   printf("Total Sectors        : %ld\n", pGeometry->TotalSectors );
   printf("Bytes Per Sector     : %d\n",  pGeometry->BytesPerSector );
   printf("Number of Heads      : %d\n",  pGeometry->NumHeads );
   printf("Total Cylinders      : %ld\n", pGeometry->TotalCylinders );
   printf("Sectors Per Track    : %d\n",  pGeometry->SectorsPerTrack );
}


void GetGeometry()
{
   PGEOMETRY pGeometry  = (PGEOMETRY)Data.pLDT;

   GetLongInput( "Total Sectors",     &(pGeometry->TotalSectors) );
   GetInput(     "Bytes Per Sector",  &(pGeometry->BytesPerSector) );
   GetInput(     "Number of Heads",   &(pGeometry->NumHeads) );
   GetLongInput( "Total Cylinders",   &(pGeometry->TotalCylinders) );
   GetInput(     "Sectors Per Track", &(pGeometry->SectorsPerTrack) );
}


void GetIO()
{
   PIORB_EXECUTEIO pIORB    = (PIORB_EXECUTEIO)IORB.pLDT;
   PSCATGATENTRY   pSGEntry = (PSCATGATENTRY)SGList.pLDT;
   USHORT x;

   GetLongInput( "RBA",         &(pIORB->RBA)        );
   GetInput(     "Block Count", &(pIORB->BlockCount) );

   pIORB->BlockSize = 512;
   pIORB->cSGList   = 16;
   pIORB->pSGList   = (PSCATGATENTRY)SGList.pGDT;
   pIORB->ppSGList  = SGList.Phys;

   for ( x=0; x<16; x++ )
      {
         pSGEntry = MAKEP( SELECTOROF(SGList.pLDT), x*sizeof(SCATGATENTRY) );
         pSGEntry->ppXferBuf  = Data.Phys;
         pSGEntry->XferBufLen = 65536;
      }
}


void ShowIO()
{
   PIORB_EXECUTEIO pIORB = (PIORB_EXECUTEIO)IORB.pLDT;

   printf("Blocks Transferred   : %d\n",  pIORB->BlocksXferred );
}


void SetupSectors()
{
   PIORB_EXECUTEIO pIORB = (PIORB_EXECUTEIO)IORB.pLDT;
   PUSHORT pSector       = (PUSHORT)Data.pLDT;
   USHORT  Block,Word;

   for ( Block=0; Block<pIORB->BlockCount; Block++ )
      {
         if ( Block == 128 ) break;
         for ( Word=0; Word<256; Word++ )
            {
               *pSector++ = (pIORB->RBA) + Block;
            }
      }
}


void ShowSectors()
{
   PIORB_EXECUTEIO pIORB = (PIORB_EXECUTEIO)IORB.pLDT;
   PUSHORT pSector;
   USHORT  Block,Word;

   printf("\n");
   for ( Block=0; Block<pIORB->BlocksXferred; Block++ )
      {
         if ( Block == 128 ) break;
         pSector = MAKEP( SELECTOROF(Data.pLDT), Block*512 );
         printf("%4d [ ", (pIORB->RBA)+Block );
         for ( Word=0; Word<14; Word++ ) printf("%04X ", *pSector++ );
         printf("]\n");
      }
}


void SetupFormat()
{
   PIORB_FORMAT  pIORB    = (PIORB_FORMAT)IORB.pLDT;
   PSCATGATENTRY pSGEntry = (PSCATGATENTRY)SGList.pLDT;
   PFORMAT_CMD_TRACK pCmd = (PFORMAT_CMD_TRACK)Data.pLDT;
   PBYTE pTblData;
   USHORT x;
   union
      {
         CHS_ADDR CHS;
         ULONG    RBA;
      } Addr;

   /* Setup format command */
   GetInput( "Flags", &(pCmd->Flags) );
   Addr.RBA = pCmd->RBA;
   GetInput( "Cylinder", &(Addr.CHS.Cylinder) );
   x = (USHORT)Addr.CHS.Head;
   GetInput( "Head", &x );
   Addr.CHS.Head   = (UCHAR)x;
   Addr.CHS.Sector = 0;
   pCmd->RBA = Addr.RBA;
   GetInput( "Track Entries", &(pCmd->cTrackEntries) );

   /* Setup IORB */
   pIORB->cSGList      = 1;
   pIORB->pSGList      = (PSCATGATENTRY)SGList.pGDT;
   pIORB->ppSGList     = SGList.Phys;
   pIORB->FormatCmdLen = sizeof(FORMAT_CMD_TRACK);
   pIORB->pFormatCmd   = Data.pGDT;
   pIORB->iorbh.RequestControl |= IORB_CHS_ADDRESSING;

   /* Setup S/G List */
   pSGEntry->ppXferBuf  = Data.Phys + sizeof(FORMAT_CMD_TRACK);
   pSGEntry->XferBufLen = pCmd->cTrackEntries * 4;

   /* Setup track table */
   pTblData = MAKEP( SELECTOROF(Data.pLDT), sizeof(FORMAT_CMD_TRACK) );
   for ( x=1; x<=pCmd->cTrackEntries; x++ )
      {
         *pTblData++ = (UCHAR)Addr.CHS.Cylinder;
         *pTblData++ = Addr.CHS.Head;
         *pTblData++ = x; /* Sector    */
         *pTblData++ = 2; /* 512 bytes */
      }
}


void SpecificInput()
{
   PIORBH               pIORBH   = (PIORBH)              IORB.pLDT;
   PIORB_GETDEVICETABLE pIORBDEV = (PIORB_GETDEVICETABLE)IORB.pLDT;
   PIORB_GEOMETRY       pIORBGEO = (PIORB_GEOMETRY)      IORB.pLDT;

   switch( pIORBH->CommandCode )
      {
         case IOCC_CONFIGURATION:
            switch( pIORBH->CommandModifier )
               {
                  case IOCM_GET_DEVICE_TABLE:
                     pIORBDEV->pDeviceTable   = (PDEVICETABLE)Data.pGDT;
                     pIORBDEV->DeviceTableLen = 65535;
                     break;
               }
            break;

         case IOCC_GEOMETRY:
            pIORBGEO->pGeometry   = (PGEOMETRY)Data.pGDT;
            pIORBGEO->GeometryLen = sizeof(GEOMETRY);
            switch( pIORBH->CommandModifier )
               {
                  case IOCM_SET_MEDIA_GEOMETRY:
                  case IOCM_SET_LOGICAL_GEOMETRY:
                     GetGeometry();
                     break;
               }
            break;

         case IOCC_EXECUTE_IO:
            GetIO();
            switch( pIORBH->CommandModifier )
               {
                  case IOCM_WRITE:
                  case IOCM_WRITE_VERIFY:
                     SetupSectors();
                     break;
               }
            break;

         case IOCC_FORMAT:
            switch( pIORBH->CommandModifier )
               {
                  case IOCM_FORMAT_TRACK:
                     SetupFormat();
                     break;
               }
            break;
      }
}


void SpecificOutput()
{
   PIORBH pIORBH = (PIORBH)IORB.pLDT;

   switch( pIORBH->CommandCode )
      {
         case IOCC_CONFIGURATION:
            switch( pIORBH->CommandModifier )
               {
                  case IOCM_GET_DEVICE_TABLE:
                     ShowDeviceTable();
                     break;
               }
            break;

         case IOCC_GEOMETRY:
            switch( pIORBH->CommandModifier )
               {
                  case IOCM_GET_MEDIA_GEOMETRY:
                  case IOCM_GET_DEVICE_GEOMETRY:
                     ShowGeometry();
                     break;
               }
            break;

         case IOCC_UNIT_STATUS:
            printf("Unit Status          : %04X\n",
                    ((PIORB_UNIT_STATUS)pIORBH)->UnitStatus );
            break;

         case IOCC_EXECUTE_IO:
            ShowIO();
            switch( pIORBH->CommandModifier )
               {
                  case IOCM_READ:
                     ShowSectors();
                     break;
               }
            break;

         case IOCC_FORMAT:
            switch( pIORBH->CommandModifier )
               {
                  case IOCM_FORMAT_TRACK:
                     pIORBH->RequestControl = 0;
                     break;
               }
            break;
      }
}


void main()
{
   unsigned Action;
   unsigned RetCode;
   char    *NumString;
   PIORBH  pIORBH;
   PBYTE   pWorking,pEnd;

   printf("\n");

   RetCode = DosOpen("IBM2FLPY",&Handle,&Action,0L,0,1,0x2042,0L);
   if ( RetCode )
      {
         if ( RetCode == DD_NOT_FOUND )
            printf("The IBM2FLPY.SYS adapter device driver is not installed\n");
         else
            printf("DosOpen Return Code = %d (Dec)\n",RetCode);
         return;
      }

   printf("Allocating IORB seg  : ");
   RetCode = DosAllocSeg( MAX_IORB_SIZE, &(SELECTOROF(IORB.pLDT)), 0 );
   if ( RetCode )
      {
         printf("DosAllocSeg Return Code = %d (Dec)\n",RetCode);
         return;
      }
   else printf("OK\n");

   printf("Allocating SG seg    : ");
   RetCode = DosAllocSeg( MAXSGLISTSIZE, &(SELECTOROF(SGList.pLDT)), 0 );
   if ( RetCode )
      {
         printf("DosAllocSeg Return Code = %d (Dec)\n",RetCode);
         return;
      }
   else printf("OK\n");

   printf("Allocating data seg  : ");
   RetCode = DosAllocSeg( 0, &(SELECTOROF(Data.pLDT)), 0 );  /* 65536 bytes */
   if ( RetCode )
      {
         printf("DosAllocSeg Return Code = %d (Dec)\n",RetCode);
         return;
      }
   else printf("OK\n");

   printf("Locking IORB segment : ");
   IORB.Length = MAX_IORB_SIZE;
   RetCode = DosDevIOCtl(&IORB.pGDT,&IORB,LOCKFUNCTION,USERDEFCATAGORY,Handle);
   if ( RetCode )
      {
         printf("DosDevIOCtl Return Code = %d (Dec)\n",RetCode);
         return;
      }
   else printf("OK\n");

   /* Zero out the IORB segment */
   pWorking = IORB.pLDT;
   pEnd     = MAKEP( SELECTOROF(IORB.pLDT), MAX_IORB_SIZE );
   while ( pWorking < pEnd ) *pWorking++ = 0;

   printf("Locking SG segment   : ");
   SGList.Length = MAXSGLISTSIZE;
   RetCode = DosDevIOCtl(&IORB.pGDT,&SGList,LOCKFUNCTION,USERDEFCATAGORY,Handle);
   if ( RetCode )
      {
         printf("DosDevIOCtl Return Code = %d (Dec)\n",RetCode);

         printf("Unlocking IORB seg   : ");
         RetCode = DosDevIOCtl(&IORB.pGDT,&IORB,UNLOCKFUNCTION,USERDEFCATAGORY,Handle);
         if ( RetCode )
            {
               printf("DosDevIOCtl Return Code = %d (Dec)\n",RetCode);
               return;
            }
         else printf("OK\n");
         return;
      }
   else printf("OK\n");

   /* Zero out the SGList segment */
   pWorking = SGList.pLDT;
   pEnd     = MAKEP( SELECTOROF(SGList.pLDT), MAXSGLISTSIZE );
   while ( pWorking < pEnd ) *pWorking++ = 0;

   printf("Locking data segment : ");
   Data.Length = 0;  /* 0 = 65536 */
   RetCode = DosDevIOCtl(&IORB.pGDT,&Data,LOCKFUNCTION,USERDEFCATAGORY,Handle);
   if ( RetCode )
      {
         printf("DosDevIOCtl Return Code = %d (Dec)\n",RetCode);

         printf("Unlocking SG seg     : ");
         RetCode = DosDevIOCtl(&IORB.pGDT,&SGList,UNLOCKFUNCTION,USERDEFCATAGORY,Handle);
         if ( RetCode )
            {
               printf("DosDevIOCtl Return Code = %d (Dec)\n",RetCode);
               return;
            }
         else printf("OK\n");

         printf("Unlocking IORB seg   : ");
         RetCode = DosDevIOCtl(&IORB.pGDT,&IORB,UNLOCKFUNCTION,USERDEFCATAGORY,Handle);
         if ( RetCode )
            {
               printf("DosDevIOCtl Return Code = %d (Dec)\n",RetCode);
               return;
            }
         else printf("OK\n");
         return;
      }
   else printf("OK\n");

   /* Zero out the data segment */
   pWorking = Data.pLDT;
   pEnd     = MAKEP( SELECTOROF(Data.pLDT), 65535 );
   while ( pWorking < pEnd ) *pWorking++ = 0;
   pWorking = 0;

   pIORBH = (PIORBH)IORB.pLDT;

   pIORBH->Length          = MAX_IORB_SIZE;
   pIORBH->NotifyAddress   = NULL;
   pIORBH->UnitHandle      = 0;
   pIORBH->CommandCode     = IOCC_UNIT_CONTROL;
   pIORBH->CommandModifier = IOCM_ALLOCATE_UNIT;
   pIORBH->RequestControl  = 0;

   /* ---------------------------------------------------------------------- */

   while( TRUE )
      {
         printf("\n\n");

         GetInput( "Unit Handle",     &(pIORBH->UnitHandle) );

         if ( NumBuff[2] == 'q' || NumBuff[2] == 'Q' ) break;

         GetInput("Command Code",     &(pIORBH->CommandCode)     );
         GetInput("Command Modifier", &(pIORBH->CommandModifier) );

         SpecificInput();

         pIORBH->Status    = 0;
         pIORBH->ErrorCode = 0;

         printf("\nCalling ADD          : ");
         RetCode = DosDevIOCtl(&IORB.pGDT,NULL,IORBFUNCTION,USERDEFCATAGORY,Handle);
         if ( RetCode )
            printf("DosDevIOCtl Return Code = %d (Dec)\n",RetCode);
         else
            {
               printf("OK\n");
               printf("Status               : %04X\n", pIORBH->Status    );

               if ( pIORBH->Status & IORB_DONE )
                  {
                     if ( pIORBH->Status & IORB_ERROR )
                        printf("ErrorCode            : %04X\n", pIORBH->ErrorCode );
                     SpecificOutput();
                  }
               else
                  {
                     do { DosSleep(500L); } while( !(pIORBH->Status & IORB_DONE) );
                     printf("\n* Psuedo Asynchronous Post *\n");
                     printf("Status               : %04X\n", pIORBH->Status    );
                     if ( pIORBH->Status & IORB_ERROR )
                        printf("ErrorCode            : %04X\n", pIORBH->ErrorCode );
                     SpecificOutput();
                  }
            }
      }

   /* ---------------------------------------------------------------------- */

   printf("\n");

   printf("Unlocking IORB seg   : ");
   RetCode = DosDevIOCtl(&IORB.pGDT,&IORB,UNLOCKFUNCTION,USERDEFCATAGORY,Handle);
   if ( RetCode )
      {
         printf("DosDevIOCtl Return Code = %d (Dec)\n",RetCode);
      }
   else printf("OK\n");

   printf("Unlocking SG seg     : ");
   RetCode = DosDevIOCtl(&IORB.pGDT,&SGList,UNLOCKFUNCTION,USERDEFCATAGORY,Handle);
   if ( RetCode )
      {
         printf("DosDevIOCtl Return Code = %d (Dec)\n",RetCode);
      }
   else printf("OK\n");

   printf("Unlocking data seg   : ");
   RetCode = DosDevIOCtl(&IORB.pGDT,&Data,UNLOCKFUNCTION,USERDEFCATAGORY,Handle);
   if ( RetCode )
      {
         printf("DosDevIOCtl Return Code = %d (Dec)\n",RetCode);
      }
   else printf("OK\n");

   DosClose(Handle);
}








