/*static char *SCCSID = "@(#)fl2data.h	6.4 92/01/30";*/
/*****************************************************************************/
/*                                                                           */
/*   Component   : IBM PS/2 Floppy Adapter Device Driver  (IBM2FLPY)         */
/*                                                                           */
/*   Module      : FL2DATA.H                                                 */
/*                                                                           */
/*   Description : External references to variables in the data segment      */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

extern PFN      Device_Help;
extern BYTE     RequestBlock[];
extern BYTE     MotorOffReqBlk[];
extern PDEV_BLK pDeviceBlock;
extern USHORT   LID;
extern USHORT   IntLevel;
extern USHORT   UnitCnt;
extern UCHAR    Retry;
extern USHORT   SectorCnt;
extern USHORT   Stage;
extern GLBLFLGS GFlags;
extern USHORT   ADDHandle;
extern ULONG    HookHandle;
extern ULONG    LockHandle;

extern PCOMPFN  CompletionRoutine;

extern UCHAR    AdapterName[];

extern PIORBH   pHeadIORB;
extern PIORB_DEVICE_CONTROL pSuspendIORB;
extern PIORBH   pResumeIORB;

extern PBYTE    pDMABuffer;
extern ULONG    ppDMABuffer;
extern PBYTE    pReadBackBuffer;
extern ULONG    ppReadBackBuffer;
extern ULONG    MaxBuffSize;
extern ULONG    BuffSize;

extern ADD_XFER_DATA XferData;

extern USHORT   MSPerTick;
extern TIMER    Timer[];
extern DRIVE    Drive[];

extern USHORT   InitData;

