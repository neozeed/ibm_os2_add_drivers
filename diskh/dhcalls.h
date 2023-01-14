/*static char *SCCSID = "@(#)dhcalls.h	6.7 92/02/05";*/

/*----------------------*/
/* DevHelp Library Calls */
/*----------------------*/

typedef USHORT NEAR *NPUSHORT;
typedef VOID   NEAR *NPVOID;


/*---------------*/
/* ABIOS Related */
/*---------------*/

BOOL APIENTRY DevHelp_GetLIDEntry ( USHORT DeviceType, USHORT LIDIndex,
                                   USHORT LIDType, PUSHORT LID);
BOOL APIENTRY DevHelp_FreeLIDEntry( USHORT LIDNumber );
BOOL APIENTRY DevHelp_ABIOSCall( USHORT Lid, NPBYTE ReqBlk, USHORT Entry_Type );
BOOL APIENTRY DevHelp_ABIOSCommonEntry(NPBYTE ReqBlk, USHORT Entry_Type );
BOOL APIENTRY DevHelp_ABIOSGetParms( USHORT Lid, NPBYTE ParmsBlk);
BOOL APIENTRY DevHelp_GetDeviceBlock( USHORT Lid, PPVOID DeviceBlockPtr );


/*-------------------*/
/* Memory Management */
/*-------------------*/

BOOL APIENTRY DevHelp_AllocGDTSelector( PSEL Selectors, USHORT Count );
BOOL APIENTRY DevHelp_PhysToGDTSelector( ULONG PhysAddr, USHORT Count,
                                        SEL Selector );
BOOL APIENTRY DevHelp_AllocPhys( ULONG lSize, USHORT MemType, PULONG PhysAddr);
BOOL APIENTRY DevHelp_PhysToUVirt( ULONG PhysAddr, USHORT Length, USHORT Flags,
                                  USHORT TagType, PVOID SelOffset);
BOOL APIENTRY DevHelp_PhysToVirt( ULONG PhysAddr, USHORT usLength, PVOID SelOffset,
                                 PUSHORT ModeFlag );
BOOL APIENTRY DevHelp_UnPhysToVirt( PUSHORT ModeFlag );
BOOL APIENTRY DevHelp_FreePhys( ULONG PhysAddr );
BOOL APIENTRY DevHelp_VirtToPhys( PVOID SelOffset, PULONG PhysAddr );
BOOL APIENTRY DevHelp_Lock( SEL Segment, USHORT LockType, USHORT WaitFlag,
                           PULONG LockHandle );
BOOL APIENTRY DevHelp_UnLock( ULONG LockHandle );
BOOL APIENTRY DevHelp_VerifyAccess( SEL MemSelector, USHORT Length,
                                    USHORT MemOffset, UCHAR AccessFlag);

/* DevHelp_AllocPhys */
#define MEMTYPE_ABOVE_1M 0
#define MEMTYPE_BELOW_1M 1

/* DevHelp_Lock */
#define LOCKTYPE_SHORT_ANYMEM 0x00
#define LOCKTYPE_LONG_ANYMEM  0x01
#define LOCKTYPE_LONG_HIGHMEM 0x03
#define LOCKTYPE_SHORT_VERIFY 0x04

/* DevHelp_PhysToUVirt */
#define SELTYPE_R3CODE  0
#define SELTYPE_R3DATA  1
#define SELTYPE_FREE    2
#define SELTYPE_R2CODE  3
#define SELTYPE_R2DATA  4
#define SELTYPE_R3VIDEO 5


/* DevHelp_VerifyAccess */
#define VERIFY_READONLY    0
#define VERIFY_READWRITE   1

/*---------------------------*/
/* Request Packet Management */
/*---------------------------*/

BOOL APIENTRY DevHelp_AllocReqPacket( USHORT WaitFlag, PBYTE FAR *ReqPktAddr );
BOOL APIENTRY DevHelp_FreeReqPacket( PBYTE ReqPktAddr );
BOOL APIENTRY DevHelp_PullParticular( NPBYTE Queue, PBYTE ReqPktAddr );
BOOL APIENTRY DevHelp_PullRequest( NPBYTE Queue,  PBYTE *ReqPktAddr );
BOOL APIENTRY DevHelp_PushRequest( NPBYTE Queue,  PBYTE ReqPktAddr );
BOOL APIENTRY DevHelp_SortRequest( NPBYTE Queue,  PBYTE ReqPktAddr );


/* DevHelp_AllocRequestPacket */

#define WAIT_NOT_ALLOWED 0
#define WAIT_IS_ALLOWED  1

/*----------------------------*/
/* Character Queue Management */
/*----------------------------*/
BOOL APIENTRY DevHelp_QueueInit( NPBYTE Queue );
BOOL APIENTRY DevHelp_QueueRead( NPBYTE Queue, PBYTE Char );
BOOL APIENTRY DevHelp_QueueWrite( NPBYTE Queue, UCHAR Char );
BOOL APIENTRY DevHelp_QueueFlush( NPBYTE Queue );


/* DevHelp_QueueInit */
typedef struct _QUEUEHDR  {             /* DHQH */

  USHORT   QSize;
  USHORT   QChrOut;
  USHORT   QCount;
  BYTE     Queue[1];
} QUEUEHDR;

typedef QUEUEHDR *PQUEUEHDR;



/*-------------------------------------------------------*/
/* Inter-Device Driver Communications & Kernel Functions */
/*-------------------------------------------------------*/

BOOL APIENTRY DevHelp_ProtToReal();
BOOL APIENTRY DevHelp_RealToProt();
BOOL APIENTRY DevHelp_InternalError( PSZ MsgText, USHORT MsgLength );
BOOL APIENTRY DevHelp_RAS (USHORT Major, USHORT Minor, USHORT Size, PBYTE Data);
BOOL APIENTRY DevHelp_RegisterPerfCtrs (NPBYTE pDataBlock, NPBYTE pTextBlock,
                                        USHORT Flags);
BOOL APIENTRY DevHelp_AttachDD( NPSZ DDName, NPBYTE DDTable );
BOOL APIENTRY DevHelp_GetDOSVar( USHORT VarNumber, USHORT VarMember,
                                 PPVOID KernelVar );

#define DHGETDOSV_SYSINFOSEG            1
#define DHGETDOSV_LOCINFOSEG            2
#define DHGETDOSV_VECTORSDF             4
#define DHGETDOSV_VECTORREBOOT          5
#define DHGETDOSV_VECTORMSATS           6
#define DHGETDOSV_INTERRUPTLEV          13
#define DHGETDOSV_DEVICECLASSTABLE      14

BOOL APIENTRY DevHelp_Save_Message( NPBYTE MsgTable );

typedef struct _MSGTABLE {              /* DHMT */

  USHORT   MsgId;                       /* Message Id #                  */
  USHORT   cMsgStrings;                 /* # of (%) substitution strings */
  PSZ      MsgStrings[1];               /* Substitution string pointers  */
} MSGTABLE;

typedef MSGTABLE *NPMSGTABLE;

/*-----------------------------*/
/* Interrupt/Thread Management */
/*-----------------------------*/

BOOL APIENTRY DevHelp_RegisterStackUsage( PVOID StackUsageData );
BOOL APIENTRY DevHelp_SetIRQ( NPFN IRQHandler, USHORT IRQLevel,
                             USHORT SharedFlag );
BOOL APIENTRY DevHelp_UnSetIRQ( USHORT IRQLevel );
BOOL APIENTRY DevHelp_EOI( USHORT IRQLevel );
BOOL APIENTRY DevHelp_ProcBlock( ULONG EventId, ULONG WaitTime, USHORT IntWaitFlag );
BOOL APIENTRY DevHelp_ProcRun( ULONG EventId, PUSHORT AwakeCount);
BOOL APIENTRY DevHelp_DevDone( PBYTE ReqPktAddr );
BOOL APIENTRY DevHelp_TCYield();
BOOL APIENTRY DevHelp_Yield();
BOOL APIENTRY DevHelp_VideoPause( USHORT OnOff );

/* DevHelp_RegisterStackUsage */
typedef struct _STACKUSAGEDATA  {       /* DHRS */

  USHORT  Size;
  USHORT  Flags;
  USHORT  IRQLevel;
  USHORT  CLIStack;
  USHORT  STIStack;
  USHORT  EOIStack;
  USHORT  NestingLevel;
} STACKUSAGEDATA;

/* DevHelp_Block */
#define WAIT_IS_INTERRUPTABLE      0
#define WAIT_IS_NOT_INTERRUPTABLE  1

/* DevHelp_VideoPause */
#define VIDEO_PAUSE_OFF            0
#define VIDEO_PAUSE_ON             1


/*----------------------*/
/* Semaphore Management */
/*----------------------*/

BOOL APIENTRY DevHelp_SemHandle( ULONG SemKey, USHORT SemUseFlag,
                                PULONG SemHandle );
BOOL APIENTRY DevHelp_SemClear( ULONG SemHandle );
BOOL APIENTRY DevHelp_SemRequest( ULONG SemHandle, ULONG SemTimeout );
BOOL APIENTRY DevHelp_SendEvent( USHORT EventType, USHORT Parm );

/* DevHelp_SemHandle */
#define SEMUSEFLAG_IN_USE       0
#define SEMUSEFLAG_NOT_IN_USE   1

/* DevHelp_SemHandle */

#define EVENT_MOUSEHOTKEY   0
#define EVENT_CTRLBREAK     1
#define EVENT_CTRLC         2
#define EVENT_CTRLNUMLOCK   3
#define EVENT_CTRLPRTSC     4
#define EVENT_SHIFTPRTSC    5
#define EVENT_KBDHOTKEY     6
#define EVENT_KBDREBOOT     7

/*------------------*/
/* Timer Management */
/*------------------*/

BOOL APIENTRY DevHelp_ResetTimer( NPFN TimerHandler );
BOOL APIENTRY DevHelp_SchedClock( NPFN SchedRoutineAddr );
BOOL APIENTRY DevHelp_SetTimer( NPFN TimerHandler );
BOOL APIENTRY DevHelp_TickCount( NPFN TimerHandler, USHORT TickCount );



/*-------------------*/
/* Real Mode Helpers */
/*-------------------*/

BOOL APIENTRY DevHelp_ProtToReal();
BOOL APIENTRY DevHelp_RealToProt();
BOOL APIENTRY DevHelp_ROMCritSection(BOOL EnterExit);
BOOL APIENTRY DevHelp_SetROMVector(NPFN IntHandler, USHORT INTNum,
                                  USHORT SaveDSLoc, PULONG LastHeader );



/*----------*/
/* Monitors */
/*----------*/

BOOL APIENTRY DevHelp_MonFlush( USHORT MonitorHandle );
BOOL APIENTRY DevHelp_Register( USHORT MonitorHandle, USHORT MonitorPID,
                               PBYTE InputBuffer, NPBYTE OutputBuffer,
                               USHORT ChainFlag );
BOOL APIENTRY DevHelp_MonitorCreate( USHORT MonitorHandle, PBYTE FinalBuffer,
                                    NPFN NotifyRtn, PUSHORT MonitorChainHandle);
BOOL APIENTRY DevHelp_DeRegister( USHORT MonitorHandle, PUSHORT MonitorsLeft);
BOOL APIENTRY DevHelp_MonWrite( USHORT MonitorHandle, PBYTE DataRecord,
                               USHORT Count, USHORT WaitFlag );

/* DevHelp_Register */
#define CHAIN_AT_TOP    0
#define CHAIN_AT_BOTTOM 1



/*-------------------*/
/* 32-Bit DevHelps   */
/*-------------------*/

BOOL APIENTRY DevHelp_RegisterPDD( NPSZ PhysDevName, PFN HandlerRoutine );
BOOL APIENTRY DevHelp_RegisterDeviceClass( NPSZ DeviceString, PFN DriverEP,
                                      USHORT DeviceFlags, USHORT DeviceClass,
                                      PUSHORT DeviceHandle);
BOOL APIENTRY DevHelp_CreateInt13VDM( PBYTE VDMInt13CtrlBlk );
BOOL APIENTRY DevHelp_RegisterBeep();
BOOL APIENTRY DevHelp_Beep();
BOOL APIENTRY DevHelp_FreeGDTSelector( SEL Selector );


/*--------------------------*/
/* 32-Bit Memory Management */
/*--------------------------*/

typedef ULONG   LIN;                /* 16:16 Ptr to Linear Addess */
typedef ULONG   _far *PLIN;         /* 16:16 Ptr to Linear Addess */

BOOL APIENTRY DevHelp_VMLock( ULONG Flags, LIN LinearAddr, ULONG Length,
                             LIN pPagelist, LIN pLockHandle,
                             PULONG PageListCount );
BOOL APIENTRY DevHelp_VMUnLock( LIN pLockHandle );
BOOL APIENTRY DevHelp_VMAlloc( ULONG Flags, ULONG Size, ULONG PhysAddr,
                              PLIN LinearAddr, PPVOID SelOffset );
BOOL APIENTRY DevHelp_VMFree( LIN LinearAddr );
BOOL APIENTRY DevHelp_VMProcessToGlobal( ULONG Flags, LIN LinearAddr,
                                        ULONG Length, PLIN GlobalLinearAddr );
BOOL APIENTRY DevHelp_VMGlobalToProcess( ULONG Flags, LIN LinearAddr,
                                        ULONG Length, PLIN ProcessLinearAddr );
BOOL APIENTRY DevHelp_VirtToLin( SEL Selector, ULONG Offset, PLIN LinearAddr );
BOOL APIENTRY DevHelp_LinToGDTSelector( SEL Selector, LIN LinearAddr,
                                       ULONG Size );
BOOL APIENTRY DevHelp_GetDescInfo( SEL Selector, PBYTE SelInfo );
BOOL APIENTRY DevHelp_PageListToLin( ULONG Size, LIN pPageList, PLIN LinearAddr );
BOOL APIENTRY DevHelp_LinToPageList( LIN LinearAddr, ULONG Size, LIN pPageList,
                                    PULONG PageListCount );
BOOL APIENTRY DevHelp_PageListToGDTSelector( SEL Selector, ULONG Size,
                                            USHORT Access, LIN pPageList );


/*----------------------*/
/* 32-Bit Context Hooks */
/*----------------------*/

BOOL APIENTRY DevHelp_AllocateCtxHook( NPFN HookHandler, PULONG HookHandle );
BOOL APIENTRY DevHelp_FreeCtxHook( ULONG HookHandle );
BOOL APIENTRY DevHelp_ArmCtxHook( ULONG HookData, ULONG HookHandle );
BOOL APIENTRY DevHelp_VMSetMem();

/*----------------------------------------*/
/* DevHlp Error Codes  (from ABERROR.INC) */
/*----------------------------------------*/

#define MSG_MEMORY_ALLOCATION_FAILED    0x00
#define ERROR_LID_ALREADY_OWNED         0x01
#define ERROR_LID_DOES_NOT_EXIST        0x02
#define ERROR_ABIOS_NOT_PRESENT         0x03
#define ERROR_NOT_YOUR_LID              0x04
#define ERROR_INVALID_ENTRY_POINT       0x05


