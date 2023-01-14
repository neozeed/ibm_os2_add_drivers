/*static char *SCCSID = "@(#)fl2proto.h	6.5 92/01/30";*/
/*****************************************************************************/
/*                                                                           */
/*   Component   : IBM PS/2 Floppy Adapter Device Driver  (IBM2FLPY)         */
/*                                                                           */
/*   Module      : FL2PROTO.H                                                */
/*                                                                           */
/*   Description : Function prototypes and segment assignments               */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/


/*** Functions which reside in Static Code segment ***/

   /* Functions which reside in fl2entry.c module  */

      VOID   _loadds FAR ADDEntryPoint( PIORBH );
      VOID   FAR  IORBDone( VOID );
      VOID   FAR  ShutDown( VOID );
      VOID   FAR  HookHandler( VOID );
      VOID   NEAR SuspendResume( PIORBH );
      VOID   FAR  Reset( VOID );
      VOID   FAR  ResetComplete( VOID );
      BOOL   FAR  CreateDMABuffer( VOID );

      VOID   NEAR Strategy( VOID );

   /* Functions which reside in fl2intr.c module */

      BOOL   FAR  NextStage( VOID );
      VOID   NEAR IntHandler( VOID );

      VOID   FAR  StartTimer( USHORT, USHORT, PPOSTFN );
      VOID   NEAR CancelTimer( USHORT );
      VOID   NEAR TimerHandler( VOID );

   /* Assign functions to Static Code segment */

      #pragma alloc_text( StaticCode, ADDEntryPoint, IORBDone )
      #pragma alloc_text( StaticCode, ShutDown, HookHandler, SuspendResume )
      #pragma alloc_text( StaticCode, Reset, ResetComplete, CreateDMABuffer )
      #pragma alloc_text( StaticCode, Strategy )
      #pragma alloc_text( StaticCode, NextStage, IntHandler )
      #pragma alloc_text( StaticCode, StartTimer, CancelTimer, TimerHandler )


/*** Functions which reside in Init Code segment ***/

   /* Functions which reside in fl2init.c module */

      VOID   NEAR InitCode( VOID );
      VOID   FAR  InitFloppy( PRPINITIN );
      BOOL   NEAR GetParameters( VOID );
      BOOL   NEAR InitTimer( VOID );
      BOOL   NEAR SetupDMABuffer( VOID );

   /* Assign functions to Init Code segment */

      #pragma alloc_text( InitCode, InitCode, InitFloppy, GetParameters )
      #pragma alloc_text( InitCode, InitTimer, SetupDMABuffer )


/*** Functions which reside in Swap Code segment ***/

   /* Functions which reside in fl2iorb.c module */

      VOID   FAR  NextIORB( VOID );
      VOID   NEAR GetDeviceTable( VOID );
      VOID   NEAR AllocateUnit( VOID );
      VOID   NEAR DeallocateUnit( VOID );
      VOID   NEAR ChangeUnitInfo( VOID );
      VOID   NEAR GetUnitStatus( VOID );
      VOID   NEAR GetChangelineState( VOID );
      VOID   FAR  GetChangelineStateComplete( VOID );
      VOID   NEAR GetMediaSense( VOID );
      VOID   FAR  GetMediaSenseComplete( VOID );
      VOID   NEAR CompleteInit( VOID );
      VOID   NEAR CmdNotSupported( VOID );

      USHORT NEAR TranslateErrorCode( USHORT );
      VOID   FAR  TurnOffMotor( USHORT );

   /* Functions which reside in fl2geo.c module  */

      VOID   NEAR GetMediaGeometry( VOID );
      VOID   FAR  GetMediaGeometryComplete( VOID );
      VOID   NEAR SetMediaGeometry( VOID );
      VOID   FAR  SetMediaGeometryComplete( VOID );
      VOID   NEAR GetDeviceGeometry( VOID );
      VOID   NEAR SetLogicalGeometry( VOID );

   /* Functions which reside in fl2io.c module   */

      VOID   NEAR IO( VOID );
      VOID   FAR  VerifyComplete( VOID );
      VOID   FAR  ReadMediaParamsComplete( VOID );
      VOID   NEAR RWV( VOID );
      VOID   NEAR NextRWV( VOID );
      VOID   NEAR NextRWVStep( VOID );
      VOID   FAR  RWVComplete( VOID );

   /* Functions which reside in fl2fmt.c module  */

      VOID   NEAR Format( VOID );
      VOID   NEAR NextFormatStep( VOID );
      VOID   FAR  FormatComplete( VOID );

   /* Assign functions to Swap Code segment */

      #pragma alloc_text( SwapCode, NextIORB, GetDeviceTable, GetUnitStatus )
      #pragma alloc_text( SwapCode, AllocateUnit, DeallocateUnit )
      #pragma alloc_text( SwapCode, ChangeUnitInfo )
      #pragma alloc_text( SwapCode, GetChangelineState )
      #pragma alloc_text( SwapCode, GetChangelineStateComplete )
      #pragma alloc_text( SwapCode, GetMediaSense, GetMediaSenseComplete )
      #pragma alloc_text( SwapCode, CompleteInit )
      #pragma alloc_text( SwapCode, CmdNotSupported, TranslateErrorCode )
      #pragma alloc_text( SwapCode, TurnOffMotor )
      #pragma alloc_text( SwapCode, GetMediaGeometry, GetMediaGeometryComplete )
      #pragma alloc_text( SwapCode, SetMediaGeometry, SetMediaGeometryComplete )
      #pragma alloc_text( SwapCode, GetDeviceGeometry, SetLogicalGeometry )
      #pragma alloc_text( SwapCode, IO,VerifyComplete, ReadMediaParamsComplete )
      #pragma alloc_text( SwapCode, RWV, NextRWV, NextRWVStep, RWVComplete )
      #pragma alloc_text( SwapCode, Format, NextFormatStep, FormatComplete )


