/*static char *SCCSID = "@(#)s506pro.h	6.3 92/01/21";*/
/*----------------------------------------------------------------------------*
 *      S506RTE.C Procedures                                                  *
 *----------------------------------------------------------------------------*/

/*--------------------*/
/* Strategy 1 Router  */
/*--------------------*/

 VOID   NEAR S506Str1();
 VOID   NEAR StatusError(PRPH pRPH, USHORT ErrorCode);

/*----------------------------------------------------------------------------*
 * S506IORB.C - IORB Queueing Entry Point                                     *
 *----------------------------------------------------------------------------*/
VOID NEAR GetDeviceTable(NPACB npACB);
VOID NEAR CompleteInit(NPACB npACB);
VOID NEAR AllocateUnit(NPACB npACB);
VOID NEAR DeallocateUnit(NPACB npACB);
VOID NEAR ChangeUnitInfo( NPACB npACB );
VOID NEAR GetMediaGeometry(NPACB npACB);
VOID NEAR GetDeviceGeometry(NPACB npACB);
VOID NEAR Read(NPACB npACB);
VOID NEAR Write(NPACB npACB);
VOID NEAR Verify(NPACB npACB);
VOID NEAR GetUnitStatus( NPACB npACB );
VOID NEAR CmdNotSupported(NPACB npACB);
VOID NEAR NextIORB(NPACB npACB);
VOID FAR  _loadds ADDEntryPoint( PIORBH pNewIORB );
VOID NEAR IORBDone(NPACB npACB);


/*------------------------*/
/* Interrupt Entry Points */
/*------------------------*/

VOID NEAR FixedInterrupt(VOID);          // located in S506SM.C


/*----------------------------------------------------------------------------*
 *  S506TIMR.C Timer Routines                                                 *
 *----------------------------------------------------------------------------*/
VOID NEAR DDDTimer(VOID);
VOID NEAR FTimer(NPACB npACB);
VOID NEAR SetTimeoutTimer(NPACB npACB);

/*----------------------------------------------------------------------------*
 *      S506SM.C Procedures (State Machine)                                   *
 *----------------------------------------------------------------------------*/
VOID NEAR FixedExecute(NPACB npACB);
VOID NEAR STARTState(NPACB npACB);
VOID NEAR CALCState(NPACB npACB);
VOID NEAR VERIFYState(NPACB npACB);
VOID NEAR DONEState(NPACB npACB);
VOID NEAR IDLEState(NPACB npACB);
VOID NEAR SERRORState(NPACB npACB);
VOID NEAR RECALState(NPACB npACB);
VOID NEAR READState(NPACB npACB);
VOID NEAR WRITEState(NPACB npACB);
VOID NEAR SETPARAMState(NPACB npACB);
VOID NEAR Do_Verify(NPACB npACB);
VOID NEAR Do_Retry(NPACB npACB);

/*----------------------------------------------------------------------------*
 *      S506HW.C Routines that interface to hard disk controller              *
 *----------------------------------------------------------------------------*/
USHORT NEAR MapHardError(NPACB npACB);
UCHAR NEAR FDGetStat(NPACB npACB);
BOOL NEAR FDWrite(NPACB npACB);
BOOL NEAR FDParam(NPACB npACB);
BOOL NEAR FDCommand(NPACB npACB);
BOOL NEAR WaitRdy(NPACB npACB);
BOOL NEAR SendCmdPkt(NPACB npACB);
VOID NEAR Initialize_hardfile();
VOID NEAR Enable_hardfile();
VOID NEAR Disable_hardfile();

/*----------------------------------------------------------------------------*
 *      ST506SUB.C Utility Functions                                          *
 *----------------------------------------------------------------------------*/
VOID NEAR Setup(NPACB npACB);
VOID NEAR PB_FxExRead(NPACB npACB);
VOID NEAR PB_FDWrite(NPACB npACB);
VOID NEAR PB_Get_SG(NPACB npACB);
VOID NEAR TraceIt(NPACB npACB, USHORT Trace_MinorCode);

/*----------------------------------------------------------------------------*
 *      S506INIT.C Procedures                                                 *
 *----------------------------------------------------------------------------*/

VOID NEAR DriveInit(PRPINITIN pRPH );
VOID NEAR Initialize_Vars( NPACB npACB );
BOOL NEAR TimerInit();
VOID NEAR SetROMCfg( PRPINITIN pRPH, NPACB npACB );
VOID NEAR Fixed_Init(NPACB npACB);
VOID NEAR Undo_Fixed_Init(NPACB npACB);
VOID NEAR NotifyOS2DASD();
VOID NEAR get_xlate_parms( NPACB npACB, UCHAR i );
BOOL NEAR Set_Multiple_Mode( NPACB npACB, UCHAR i );
BOOL NEAR ST506Ctrl( NPACB npACB, UCHAR i );
VOID NEAR ResetST506Ctrl( NPACB npACB, UCHAR i );
