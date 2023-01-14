/*static char *SCCSID = "@(#)adskinit.c	6.4 92/01/17";*/

/***********************************************************************/
/*                                                                     */
/* Driver Name: ABIOS Adapter Device Driver for DASD Devices           */
/*              --------------------------------------------           */
/*                                                                     */
/* Source File Name: ADSKINIT.C                                        */
/*                                                                     */
/* Descriptive Name: ABIOS Disk Driver - Initialization                */
/*                                                                     */
/* Function: Builds configuration dependent data in response to OS/2   */
/*           KERNEL initialization request.                            */
/*                                                                     */
/***********************************************************************/

#define INCL_NOBASEAPI
#define INCL_NOPMAPI
#include <os2.h>

#include <devcmd.h>

#define INCL_INITRP_ONLY
#include <reqpkt.h>

#include <scb.h>
#include <abios.h>

#include <iorb.h>
#include <addcalls.h>

#include <dhcalls.h>

#include <adskcons.h>
#include <adsktype.h>
#include <adskpro.h>
#include <adskextn.h>


/*--------------------------------------------------*/
/*                                                  */
/* Process Base Initialization Request Packet       */
/* ------------------------------------------       */
/*                                                  */
/*                                                  */
/*--------------------------------------------------*/

USHORT ADSKInit( pRPI )

PRPINITIN pRPI;
{

  NPABRB_RETLIDPARMS npRLP;
  USHORT             Lid;
  USHORT             LidIndex;
  USHORT             Rc;
  USHORT             i;
  PVOID              TimerPool;
  USHORT             TimerPoolSize;
  PRPINITOUT         pRPO = (PRPINITOUT) pRPI;

  InitComplete = 1;

  Device_Help = pRPI->DevHlpEP;

  npRLP               = (NPABRB_RETLIDPARMS) &InitABRB1;
  npRLP->abrbh.Length = GENERIC_ABRB_SIZE;

  LidIndex   = 1;


  /*------------------------------------------------------*/
  /* Request Disk Logical IDs until no more are available */
  /*------------------------------------------------------*/

  while ( !(Rc = DevHelp_GetLIDEntry( DEVID_DISK, LidIndex, 0, &Lid )) )
    {
      LidIndex++;

      /*------------------------*/
      /* Record Lids for Abort  */
      /* processing             */
      /*------------------------*/
      InitLidTable[TotalLids++] = Lid;

      /*-----------------------*/
      /* Obtain LID Parameters */
      /*-----------------------*/
      npRLP->abrbh.LID       = Lid;
      npRLP->abrbh.Unit      = 0;
      npRLP->abrbh.RC        = ABRC_START;
      npRLP->abrbh.Function  = ABFC_RET_LID_PARMS;
      Rc = DevHelp_ABIOSCall( Lid, (NPBYTE) npRLP, ABIOS_EP_START );

      /*-------------------------------------*/
      /* In case of error deallocate the LID */
      /*-------------------------------------*/
      if ( Rc || (npRLP->abrbh.RC != ABRC_COMPLETEOK) )
        {
          DevHelp_FreeLIDEntry( Lid );
          continue;
        }

      Rc = BuildUCBs( npRLP );

      /*----------------------------------------------------*/
      /* If we ran out of Control Block Space -- SHUTDOWN!  */
      /*----------------------------------------------------*/
      if ( Rc == 0xffff )
        {
          for ( i = 0; i < TotalLids; i++ )
            {
              DevHelp_FreeLIDEntry( InitLidTable[i] );
              InitLidTable[i] = 0;
            }
          TotalLids = 0;
          goto ADSKInit_Failed;
        }

      /*-------------------------------------------------------*/
      /* If we found no eligible devices, then release the LID */
      /*-------------------------------------------------------*/
      if ( Rc )
        {
          DevHelp_FreeLIDEntry( Lid );
          InitLidTable[--TotalLids] = 0;
          continue;
        }
    }

  if ( !TotalLids )
    {
      goto ADSKInit_Failed;
    }


  if ( Rc = InitSGBufferPool( (TotalLCBs > MAX_SG_BUFFERS)
                                               ? MAX_SG_BUFFERS : TotalLCBs ) )
    {
      goto ADSKInit_Failed;
    }

  TimerPoolSize = sizeof(ADD_TIMER_POOL)
                               +  (2 * TotalLCBs - 1) * sizeof(ADD_TIMER_DATA);

  if ( !(TimerPool=(PVOID) InitAllocate( TimerPoolSize )) )
    {
      goto ADSKInit_Failed;
    }

  if ( ADD_InitTimer( TimerPool, TimerPoolSize ) )
    {
      goto ADSKInit_Failed;
    }


  if ( DevHelp_RegisterDeviceClass( (NPSZ)    ADDNAME,
                                    (PFN)     ADSKIORBEntr,
                                    (USHORT)  ADDFLAGS,
                                    (USHORT)  DRIVERCLASS_ADD,
                                    (PUSHORT) &ADDHandle       ) )
    {
      goto ADSKInit_Failed;
    }

  pRPO->Unit   = 0;
  pRPO->CodeEnd = ((USHORT) &ADSKInit) - 1;
  pRPO->DataEnd = ((USHORT) npConfigPool ) - 1;

  return ( STDON );

ADSKInit_Failed: ;

  pRPO->Unit   = 0;
  pRPO->CodeEnd = 0;
  pRPO->DataEnd = 0;

  return( 0x8115 /* ERROR_I24_QUIET_INITFAIL */ );

}

/*--------------------------------------------------*/
/*                                                  */
/* Allocate/Initialize Unit Control Blocks (UCBs)   */
/* ----------------------------------------------   */
/*                                                  */
/*                                                  */
/*--------------------------------------------------*/

USHORT BuildUCBs( npRLP )

NPABRB_RETLIDPARMS      npRLP;
{
  NPUCB                         npUCB;
  NPUCB                         npUCBFirst;
  NPUCB                         npUCBLast;
  NPABRB_DISK_READDEVPARMS      npRDP;

  USHORT                        i;
  USHORT                        Rc;
  USHORT                        Rc2;
  USHORT                        cUnits;
  USHORT                        LidFlags;
  USHORT                        UnitIndex;

  Rc2        = 0;
  npUCBFirst = 0;
  npUCB      = 0;
  UnitIndex  = 0;

  cUnits   = npRLP->cUnits;
  LidFlags = npRLP->LIDFlags;

  npRDP                 = (NPABRB_DISK_READDEVPARMS) &InitABRB2;
  npRDP->abrbh.Function = ABFC_READ_DEVICE_PARMS;
  npRDP->abrbh.Length   = GENERIC_ABRB_SIZE;
  npRDP->abrbh.LID      = npRLP->abrbh.LID;

  /*-----------------------------------------*/
  /* For each unit, obtain device parameters */
  /*-----------------------------------------*/
  for (i=0; i < cUnits; i++)
    {
      /* May want to support multiple stages on get Read Device Parms */
      npRDP->abrbh.Unit = i;
      npRDP->abrbh.RC   = ABRC_START;
      Rc = DevHelp_ABIOSCall( npRDP->abrbh.LID, (NPBYTE) npRDP, ABIOS_EP_START );

      /*----------------------------------------*/
      /* There was an error obtaining LID parms */
      /*              -- or --                  */
      /* If the device supports SCB transfer    */
      /* Skip the device.                       */
      /*----------------------------------------*/
      if ( Rc || (npRDP->abrbh.RC != ABRC_COMPLETEOK)
                                    || (npRDP->DevCtrlFlags & DP_SCBXFER) )
        {
          Rc2 = 1;
          continue;
        }

      /*----------------------------------------------*/
      /* Allocate a Unit Control Block for the device */
      /*----------------------------------------------*/
      if (!(npUCB = (NPUCB)InitAllocate( sizeof(UCB) ) ))
        {
          Rc2 = -1;
          goto BuildUCB_Failed;
        }

      npUCB->ABIOSUnit    = i;
      npUCB->ABIOSFlags   = npRDP->DevCtrlFlags;
      npUCB->ABIOSRetry   = (USHORT) npRDP->RetryCount;
      npUCB->HwMaxXfer    = npRDP->MaxXferCount;
      if (ABIOSMaxXfer < npUCB->HwMaxXfer )
        {
          ABIOSMaxXfer = npUCB->HwMaxXfer;
        }


      /*---------------------*/
      /* Setup UnitInfo Data */
      /*---------------------*/
      npUCB->UnitInfo.AdapterIndex = TotalLids;
      npUCB->UnitInfo.UnitIndex    = UnitIndex++;
      npUCB->UnitInfo.UnitHandle   = (USHORT) npUCB;
      npUCB->UnitInfo.UnitType     = UIB_TYPE_DISK;
      npUCB->UnitInfo.QueuingCount = 2;
      npUCB->UnitInfo.UnitFlags    =
             ((npUCB->ABIOSFlags & DP_EJECTABLE)    ? UF_REMOVABLE  : 0) |
             ((npUCB->ABIOSFlags & DP_CHGLINE_DISK) ? UF_CHANGELINE : 0);

      /*---------------------*/
      /* Setup Geometry Data */
      /*---------------------*/
      npUCB->Geometry.TotalSectors    = npRDP->cRBA;
      npUCB->Geometry.BytesPerSector  = npRDP->BlockSize;
      npUCB->Geometry.NumHeads        = npRDP->cHead;
      npUCB->Geometry.TotalCylinders  = npRDP->cCylinders;
      npUCB->Geometry.SectorsPerTrack = npRDP->SectorsPerTrack;

      /*----------------------------------------------------*/
      /* Either start UCB chain or link into existing chain */
      /*----------------------------------------------------*/
      (!npUCBFirst) ? (npUCBFirst           = npUCB)
                    : (npUCBLast->npNextUCB = npUCB);

      npUCBLast = npUCB;

      /*-----------------------------------------------------*/
      /* If this is a CONCURRENT Logical ID, an LCB is built */
      /* for each independently programmable device.         */
      /*-----------------------------------------------------*/
      if (LidFlags & LF_CONCURRENT)
        {
          npUCB->Flags |= (UCBF_FIRST | UCBF_LAST);
          if ( (Rc2 = BuildLCB( npRLP, npUCBFirst, npUCBLast )) )
            {
              goto BuildUCB_Failed;
            }
          npUCBFirst= 0;
        }
    }

  /*-----------------------------------------------------*/
  /* For a non-CONCURRENT Logical ID, a single LCB is    */
  /* built for all devices attached to the LID.          */
  /*-----------------------------------------------------*/
  if (!(LidFlags & LF_CONCURRENT))
    {
       npUCBLast->Flags  |= UCBF_LAST;
       npUCBFirst->Flags |= UCBF_FIRST;
       Rc2 = BuildLCB( npRLP, npUCBFirst, npUCBLast );
    }

BuildUCB_Failed: ;

  return( Rc2 );
}


/*--------------------------------------------------*/
/*                                                  */
/* Allocate/Initialize LID Control Blocks (LCBs)    */
/* ----------------------------------------------   */
/*                                                  */
/*                                                  */
/*--------------------------------------------------*/


USHORT BuildLCB( npRLP, npUCBFirst, npUCBLast )

NPABRB_RETLIDPARMS      npRLP;
NPUCB                   npUCBFirst;
NPUCB                   npUCBLast;
{

  USHORT             Rc;
  USHORT             i;
  NPUCB              npUCB;
  NPLCB              npLCB;
  NPINTCB            npIntCB;
  USHORT             HwIntLevel;
  NPABRB_RETLIDPARMS npRLP2;

  Rc = 0;

  if ( !(npLCB = InitAllocate( sizeof( LCB )) ) )
    {
      Rc = -1;
      goto BuildLCB_Failed;
    }

  HwIntLevel = npRLP->HwIntLevel;


  /*------------------------------------------------*/
  /* Initialize LCB pointer fields and ABIOS RB     */
  /*------------------------------------------------*/
  npLCB->npFirstUCB    = npUCBFirst;

  npLCB->ABIOSLidFlags = npRLP->LIDFlags;

  npLCB->LidIndex      = TotalLids-1;

  npRLP2               = (NPABRB_RETLIDPARMS) &npLCB->ABIOSReq;
  npRLP2->abrbh.LID    = npRLP->abrbh.LID;
  npRLP2->abrbh.Length = GENERIC_ABRB_SIZE;


  /*------------------------------------------------*/
  /* If the LID requires Logical Data Pointers,     */
  /* allocate a GDT Selector                        */
  /*------------------------------------------------*/
  if ( npLCB->ABIOSLidFlags & LF_LOGICAL_PTRS )
    {
      if (DevHelp_AllocGDTSelector( (PSEL) &npLCB->LogXferSel, 1 ) )
        {
          Rc = -1;
          goto BuildLCB_Failed;
        }
    }

  /*------------------------------------------------*/
  /* Check if requested IRQ level is already in use */
  /*------------------------------------------------*/
  for (i = 0, npIntCB = &IntLevelCB[0]; i < LevelsInUse; i++, npIntCB++ )
    {
      if ( HwIntLevel == npIntCB->HwIntLevel )
        {
          break;
        }
    }

  /*----------------------------------------------------*/
  /* If the level was not found, then do initial set-up */
  /* Else add new LCB to existing level                 */
  /*----------------------------------------------------*/
  if ( LevelsInUse == MAX_HW_INT_LEVELS )
    {
      _asm { int 3 }
    }

  if ( !npIntCB->npFirstLCB )
    {
      npIntCB->npFirstLCB = npLCB;
      npIntCB->HwIntLevel = HwIntLevel;
      DevHelp_SetIRQ( (NPFN)   OFFSETOF(npIntCB->IntHandlerRtn),
                      (USHORT) npIntCB->HwIntLevel,
                      (USHORT) 1                                 );
      LevelsInUse++;

      ((NPABRBH) npIntCB->DefaultABIOSReq)->Length =
                                           sizeof(npIntCB->DefaultABIOSReq);

    }
  else
    {
      (npIntCB->npLastLCB)->npNextIntLCB = npLCB;
    }
  npIntCB->npLastLCB = npLCB;

  /*-----------------------------------------------------*/
  /* Retain first UCB built in npUCBAnchor.              */
  /*-----------------------------------------------------*/
  if ( !npUCBAnchor )
    {
      npUCBAnchor = npUCBFirst;
    }

  /*-----------------------------------------------------*/
  /* Link previously built UCBs to first UCB for this    */
  /* LCB.                                                */
  /*-----------------------------------------------------*/
  if ( npUCBPrevLID )
    {
      npUCBPrevLID->npNextUCB = npUCBFirst;
    }

  npUCBPrevLID = npUCBLast;

  /*-----------------------------------------------------*/
  /* Link set of UCBs passed in to this LCB              */
  /* LCB.                                                */
  /*-----------------------------------------------------*/
  npUCB = npUCBFirst;
  do
    {
      npUCB->npLCB = npLCB;
    }
  while (npUCB = npUCB->npNextUCB );

  /*-----------------------------------------------------*/
  /* Set LCB's CurrentUCB field to last LCB passed       */
  /*-----------------------------------------------------*/
  npLCB->npCurUCB = npUCBLast;


  TotalLCBs++;


BuildLCB_Failed: ;

  return (Rc);

}

/*--------------------------------------------------*/
/*                                                  */
/* Allocate Storage from Control Block Pool         */
/* ----------------------------------------         */
/*                                                  */
/*                                                  */
/*--------------------------------------------------*/

NPVOID NEAR InitAllocate( USHORT Size )
{
  NPVOID      PoolElement = 0;

  if (ConfigPoolAvail >= Size );
    {
      PoolElement      = (NPVOID) npConfigPool;
      ConfigPoolAvail -= Size;
      npConfigPool    += Size;
    }

  return( PoolElement );
}

/*--------------------------------------------------*/
/*                                                  */
/* Allocate/Initialize Scatter/Gather Buffer Pool   */
/* ----------------------------------------------   */
/*                                                  */
/*                                                  */
/*--------------------------------------------------*/

USHORT InitSGBufferPool( cBuffers )

USHORT cBuffers;
{
  NPIOBUF_POOL  npIOBuf;
  ULONG         MaxXferLen;
  USHORT        i;


  MaxXferLen = ((ULONG) ABIOSMaxXfer) << SECTOR_SHIFT;

  if ( MaxXferLen > 0x10000l )
    {
      MaxXferLen = 0x10000l;
    }

  if ( cBuffers > MAX_SG_BUFFERS )
    {
      cBuffers = MAX_SG_BUFFERS;
    }

  for ( npIOBuf = IOBufPool, i=0; i < cBuffers; npIOBuf++, i++ )
    {
      if ( DevHelp_AllocGDTSelector( (PSEL) &SELECTOROF(npIOBuf->pBuf), 1) )
        {
          goto InitSGBufferPool_Error;
        }

      if ( DevHelp_AllocPhys( (ULONG)  MaxXferLen,
                              (USHORT) MEMTYPE_ABOVE_1M,
                              (PULONG) &npIOBuf->ppBuf   ) )
        {
          goto InitSGBufferPool_Error;
        }

      if ( DevHelp_PhysToGDTSelector( (ULONG)  npIOBuf->ppBuf,
                                      (USHORT) MaxXferLen,
                                      (SEL)    SELECTOROF(npIOBuf->pBuf) ) )
        {
          goto InitSGBufferPool_Error;
        }

      npIOBuf->BufSize = MaxXferLen;
      npIOBuf->BufSec  = (USHORT) (MaxXferLen >> SECTOR_SHIFT);

      (!npBufPoolHead) ? (npBufPoolHead = npIOBuf)
                       : (npBufPoolHead->npNextBuf = npIOBuf);

      npBufPoolFoot = npIOBuf;
    }

  return ( 0 );


InitSGBufferPool_Error:

  if ( npIOBuf->pBuf )
    {
      DevHelp_FreeGDTSelector( SELECTOROF(npIOBuf->pBuf) );
      npIOBuf->pBuf = 0;
    }

  if ( npIOBuf->ppBuf )
    {
      DevHelp_FreePhys( npIOBuf->ppBuf );
      npIOBuf->ppBuf = 0;
    }

  return ( (i) ? 0 : -1 );
}

