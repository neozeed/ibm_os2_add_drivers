/*static char *SCCSID = "@(#)fl2def.h	6.6 92/01/30";*/
/*****************************************************************************/
/*                                                                           */
/*   Component   : IBM PS/2 Floppy Adapter Device Driver  (IBM2FLPY)         */
/*                                                                           */
/*   Module      : FL2DEF.H                                                  */
/*                                                                           */
/*   Description : Definitions                                               */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

#define MAXUNITS    4           /* PS/2 only supports three floppys          */
#define TIMERCNT    MAXUNITS+1  /* Need an extra timer for shut down delay   */
#define SHUTDNTIMER TIMERCNT-1  /* Shut down timer is the last one           */
#define SHUTDNDELAY 3000        /* Milliseconds to delay before shuting down */
#define MINBUFFSIZE 4096        /* Minimum size that DMA buffer can be       */
#define SUCCESS     FALSE       /* Success means no error                    */
#define FAILURE     TRUE        /* Failure means there is an error           */
#define STARTUP     0L          /* Tells context hook handler to start up    */
#define SHUTDOWN    1L          /* Tells context hook handler to shut down   */

#define STATUS_SUCCESS      0x0000 /* Req Pkt Status of Success           */
#define STATUS_ERR_GENFAIL  0x800C /* Req Pkt Status of General Failure   */
#define STATUS_QUIET_FAIL   0x8015 /* Req Pkt Status of Fail Init Quietly */
#define DP_READBACK_NOT_REQ 0x0020 /* Read Dev Parms dev control flag     */

typedef struct _ABRB_GENERIC       /* Generic ABIOS request block         */
   {                               /* Used for clearing reserved fields   */
      ABRBH     abrbh;
      USHORT    Offset10H;
      USHORT    Offset12H;
      USHORT    Offset14H;
      USHORT    Offset16H;
      USHORT    Offset18H;
      USHORT    Offset1AH;
      USHORT    Offset1CH;
      USHORT    Offset1EH;
      ULONG     WaitTime;
      USHORT    cSectors;
   } ABRB_GENERIC;

typedef ABRB_GENERIC near *NPABRB_GENERIC;


typedef VOID (FAR *PCOMPFN)();        /* Completion routine definition */

typedef VOID (FAR *PPOSTFN)(USHORT);  /* Timer post routine definition */

typedef struct _TIMER           /* Entry in the timer table */
   {
      USHORT   TicksToGo;
      PPOSTFN NotifyRoutine;
   } TIMER;


typedef struct _GLBLFLGS         /* Global Flags */
   {
      USHORT Resetting      : 1; /* Set when reset is in progress      */
      USHORT Unlocking      : 1; /* Set when unlock is in progress     */
      USHORT VideoPauseOn   : 1; /* Set if video pause is on           */
      USHORT ProcDisabled   : 1; /* Set if IORB processing is disabled */
      USHORT ShutdnPending  : 1; /* Set when shut down is in progress  */
      USHORT BootComplete   : 1; /* Set when boot is complete          */
      USHORT StageOnInt     : 1; /* Set when expecting an interrupt    */
      USHORT CtxHookArmed   : 1; /* Set if context hook is armed       */
   } GLBLFLGS;


typedef struct _DRVFLGS         /* Drive Flags */
   {
      USHORT HasChangeLine  : 1; /* Drive has change line capability */
      USHORT ReadBackReq    : 1; /* Drive requires read back         */
      USHORT Allocated      : 1; /* Set if unit is allocated         */
      USHORT LogicalMedia   : 1; /* Media geometry is logical        */
      USHORT UnknownMedia   : 1; /* Media geometry is unknown        */
   } DRVFLGS;

typedef struct _DRIVE           /* Drive structure */
   {
      GEOMETRY  Geometry[2];    /* Device geometry and media geometry       */
      USHORT    MotorOffDelay;  /* Millisecs before turning off drive motor */
      UCHAR     RetryCount;     /* Number of times to retry failed attempt  */
      UCHAR     FormatGap;      /* Gap length for format                    */
      UCHAR     FillByte;       /* Fill byte for format                     */
      PUNITINFO pUnitInfo;      /* Passed in on Change Unit Info            */
      DRVFLGS   Flags;          /* Drive flags                              */
   } DRIVE;

/* Indexes into the Drive[].Geometry[] table */
#define DEVICE      0           /* Max geometry for the drive    */
#define MEDIA       1           /* Geometry of the current media */


#define DisableInts _asm cli    /* Disable Interrupts           */
#define EnableInts  _asm sti    /* Enable Interrupts            */


/* ABIOS Device Block */
typedef struct _DEV_BLK
   {
      UCHAR        Reserved[12];
      ULONG        Port3F0;
      ULONG        Port3F4;
      ULONG        Port3F7;
      PVOID        pPortCMOS;
      PVOID        pPortDMA;
      ULONG        PortDMAMask;
      ULONG        PortDMAPage;
      USHORT       DeviceLength;
      UCHAR        DeviceStatus;
      UCHAR        MotorStatus;
      UCHAR        TimeoutCount;
      UCHAR        LastRate;
      USHORT       DBUnitCount;
      USHORT       UnitLength;
   } DEV_BLK;

typedef DEV_BLK far *PDEV_BLK;


