/*static char *SCCSID = "@(#)adskpro.h	6.3 92/01/17";*/

/***********************************************************************/
/*                                                                     */
/* Driver Name: ABIOS Adapter Device Driver for DASD Devices           */
/*              --------------------------------------------           */
/*                                                                     */
/* Source File Name: ADSKPRO.H                                         */
/*                                                                     */
/* Descriptive Name: ABIOS Disk Driver - C function prototypes         */
/*                                                                     */
/* Function:                                                           */
/*                                                                     */
/***********************************************************************/

typedef void   near *NPVOID;
typedef USHORT near *NPUSHORT;

/*----------------------*/
/* Module: ADSKSTR1.C   */
/*----------------------*/

VOID NEAR ADSKStr1();
VOID NEAR StatusError(PRPH pRPH, USHORT ErrorCode );

/*----------------------*/
/* Module: ADSKINIT.C   */
/*----------------------*/

USHORT NEAR ADSKInit(PRPINITIN pRPI);
USHORT NEAR BuildUCBs( NPABRB_RETLIDPARMS npRLP );
USHORT NEAR BuildLCB( NPABRB_RETLIDPARMS npRLP, NPUCB npUCBFirst, NPUCB npUCBLast );
NPVOID NEAR InitAllocate( USHORT Size );
USHORT NEAR InitSGBufferPool( USHORT cBuffers );

/*----------------------*/
/* Module: ADSKINTH.C   */
/*----------------------*/

USHORT FAR  IRQEntry0();
USHORT FAR  IRQEntry1();
USHORT FAR  IRQEntry2();
USHORT FAR  IRQEntry3();
USHORT NEAR InterruptHandler( NPINTCB npIntCB );

USHORT NEAR StageABIOSRequest( NPLCB npLCB, USHORT Abios_EP_Type );
VOID   NEAR QueueLCBComplete( NPLCB npLCB );
USHORT NEAR ProcessDefaultInt( NPINTCB npIntCB );
VOID   NEAR ProcessLCBComplete( VOID );

VOID FAR InterruptTimeoutHandler( ULONG TimerHandle, PVOID npLCB, PVOID Unused );
VOID FAR DelayTimeoutHandler    ( ULONG TimerHandle, PVOID npLCB, PVOID Unused );

/*----------------------*/
/* Module: ADSKIORB.C   */
/*----------------------*/

VOID FAR  _loadds ADSKIORBEntr( PIORB               pIORB );
VOID NEAR Get_Device_Table    ( PIORB_CONFIGURATION pIORB );
VOID NEAR Allocate            ( PIORB_UNIT_CONTROL  pIORB );
VOID NEAR DeAllocate          ( PIORB_UNIT_CONTROL  pIORB );
VOID NEAR Change_UnitInfo     ( PIORB_UNIT_CONTROL  pIORB );
VOID NEAR Get_Device_Geometry ( PIORB_GEOMETRY      pIORB );
VOID NEAR Device_IO           ( PIORB               pIORB );

VOID NEAR NotifyIORB          ( PIORB pIORB, USHORT ErrorCode );

/*----------------------*/
/* Module: ADSKSM.C     */
/*----------------------*/
VOID   NEAR StartLCB         ( NPLCB npLCB );
USHORT NEAR StartDeviceIO    ( NPLCB npLCB, NPIOBUF_POOL npIOBuf );
USHORT NEAR ContinueDeviceIO ( NPLCB npLCB );
VOID   NEAR CompleteDeviceIO ( NPLCB npLCB );
VOID   NEAR RetryLCBBusyQ    ( NPLCB npLCB );

PIORB  NEAR GetNextIORB      ( NPLCB npLCB );
USHORT NEAR CalcMaxXfer      ( NPLCB npLCB );
USHORT NEAR ABIOSErrorToRc   ( NPLCB npLCB, USHORT ABIOSRc, USHORT AbiosStage );
USHORT NEAR ABIOSToIORBError ( USHORT ABIOSRc );

/*----------------------*/
/* Module: ADSKSGB.C   */
/*----------------------*/

NPIOBUF_POOL NEAR AllocSGBuffer( NPLCB npLCB, VOID (NEAR *CallBackRtn)() );
VOID         NEAR FreeSGBuffer ( NPIOBUF_POOL npIOBuf );

/*----------------------*/
/* Module: ADSKASUB.ASM */
/*----------------------*/

VOID  NEAR memcpy( PBYTE Dst, PBYTE Src, USHORT cb );
ULONG NEAR ULONGdivUSHORT( ULONG Dividend, USHORT Divisor, NPUSHORT npRemainder );
ULONG NEAR ULONGmulULONG( ULONG Multiplier, ULONG Multiplicand );

