/*static char *SCCSID = "@(#)adsktype.h	6.3 92/01/17";*/

/***********************************************************************/
/*                                                                     */
/* Driver Name: ABIOS Adapter Device Driver for DASD Devices           */
/*              --------------------------------------------           */
/*                                                                     */
/* Source File Name: ADSKTYPE.H                                        */
/*                                                                     */
/* Descriptive Name: ABIOS Disk Driver - Internal Data Structures      */
/*                                                                     */
/* Function: Defines data types used locally within this driver.       */
/*                                                                     */
/***********************************************************************/


typedef struct _UCB UCB, *NPUCB, FAR *PUCB;
typedef struct _LCB LCB, *NPLCB, FAR *PLCB;
typedef struct _IOBUF_POOL IOBUF_POOL, *NPIOBUF_POOL, FAR *PIOBUF_POOL;

/*--------------------------------------------------*/
/*                                                  */
/* Blocking/Deblocking Buffer Element               */
/* ----------------------------------               */
/*                                                  */
/* One control block is allocated per               */
/* Blocking/Deblocking Buffer.                      */
/*                                                  */
/*--------------------------------------------------*/

typedef struct _IOBUF_POOL
{

  NPIOBUF_POOL   npNextBuf;
  PVOID          pBuf;
  ULONG          ppBuf;
  ULONG          BufSize;
  USHORT         BufSec;

} IOBUF_POOL, *NPIOBUF_POOL, FAR *PIOBUF_POOL;


/*--------------------------------------------------*/
/*                                                  */
/* UNIT Control Block                               */
/* ------------------                               */
/*                                                  */
/* One control block is allocated per ABIOS unit.   */
/*                                                  */
/*--------------------------------------------------*/

typedef struct _UCB
{

  /*------------*/
  /* IORB Queue */
  /*------------*/
  PIORB                 pQueueHead;
  PIORB                 pQueueFoot;

  /*---------------------------------------------------------*/
  /* Pointers to next Device CB and owning LID Control Block */
  /*---------------------------------------------------------*/
  NPUCB                 npNextUCB;
  NPLCB                 npLCB;

  /*--------------------*/
  /* ABIOS Data         */
  /*--------------------*/
  USHORT                ABIOSUnit;
  USHORT                ABIOSFlags;
  USHORT                ABIOSRetry;
  USHORT                HwMaxXfer;
  USHORT                LogXferSel;

  /*--------------------*/
  /* UNITINFO Structure */
  /*--------------------*/

  PUNITINFO             pChngUnitInfo;
  UNITINFO              UnitInfo;

  USHORT                Flags;

  #define UCBF_ALLOCATED        0x8000
  #define UCBF_INFOCHANGED      0x4000
  #define UCBF_FIRST            0x0001
  #define UCBF_LAST             0x0002

  /*--------------------*/
  /* GEOMETRY Structure */
  /*--------------------*/

   GEOMETRY             Geometry;

} UCB;



/*---------------------------------------------------*/
/*                                                   */
/* LID Control Block                                 */
/* -----------------                                 */
/*                                                   */
/* One control block allocated per concurrent device */
/* or per set of non-concurrent devices.             */
/*                                                   */
/*---------------------------------------------------*/


typedef struct _LCB
{

  /*-------------------------------------*/
  /* Current IORB and Unit CB pointers   */
  /*-------------------------------------*/
  PIORB                 pIORB;
  NPUCB                 npCurUCB;
  NPUCB                 npFirstUCB;

  NPLCB                 npNextIntLCB;
  NPLCB                 npNextCmpQLCB;
  NPLCB                 npNextBusyQLCB;

  /*-------------------------*/
  /* Request Data from IORB  */
  /*-------------------------*/
  ULONG                 ReqRBA;
  USHORT                ReqSectors;

  /*---------------------------*/
  /* Overall state of this LCB */
  /*---------------------------*/
  USHORT                CmdCode;
  USHORT                CmdModifier;

  USHORT                IntFlags;

  #define  LCBF_ACTIVE            0x0001
  #define  LCBF_ONCOMPLETEQ       0x0002
  #define  LCBF_ONBUFWAITQ        0x0004
  #define  LCBF_STARTPENDING      0x0008
  #define  LCBF_ONBUSYQ           0x0010

  #define  LCBF_WRITE             0x8000
  #define  LCBF_READ              0x4000
  #define  LCBF_DATA_XFER         0x2000
  #define  LCBF_SGBUF_REQUIRED    0x1000

  #define  LCBF_RESET_FLAGS       (LCBF_WRITE | LCBF_READ | LCBF_DATA_XFER | \
                                   LCBF_SGBUF_REQUIRED | LCBF_STARTPENDING)

  /*---------------------------*/
  /* Timer Services            */
  /*---------------------------*/
  ULONG                 IntTimerHandle;
  ULONG                 DlyTimerHandle;

  /*------------------------------*/
  /* ABIOS Data                   */
  /*------------------------------*/
  USHORT                ABIOSLidFlags;
  USHORT                ABIOSDevFlags;
  USHORT                ABIOSRetry;
  USHORT                LogXferSel;
  USHORT                HwMaxXfer;
  USHORT                LidIndex;

  /*------------------------------*/
  /* State of current sub-Request */
  /*------------------------------*/
  ULONG                 CurRBA;
  USHORT                CurSectors;
  USHORT                CurSecRemaining;
  ULONG                 CurBytes;
  USHORT                Retries;
  USHORT                LastABIOSError;
  ULONG                 Timeout;

  /*------------------------------*/
  /* S/G Buffer Management        */
  /*------------------------------*/
  ADD_XFER_DATA         XferData;
  NPIOBUF_POOL          npIOBuf;

  NPLCB                 npNextBufQLCB;
  VOID                 (NEAR *npBufNotifyRtn)(NPLCB        npLCB,
                                              NPIOBUF_POOL npIOBuf );

  /*-----------------------------*/
  /* ABIOS Request Block for LID */
  /*-----------------------------*/
  BYTE                  ABIOSReq[GENERIC_ABRB_SIZE];

} LCB, *NPLCB, FAR *PLCB;


/*---------------------------------------------------*/
/*                                                   */
/* Interrupt Control Block                           */
/* -----------------------                           */
/*                                                   */
/* One control block allocated per IRQ level in use. */
/*                                                   */
/*---------------------------------------------------*/

typedef struct _INTCB
{

  USHORT                (FAR *IntHandlerRtn)();
  USHORT                HwIntLevel;

  NPLCB                 npFirstLCB;
  NPLCB                 npLastLCB;
  BYTE                  DefaultABIOSReq[16];

} INTCB, *NPINTCB, FAR *PINTCB;


