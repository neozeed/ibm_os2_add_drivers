/*static char *SCCSID = "@(#)addcalls.h	6.2 92/01/10";*/
/*--------------------------------------------------------
 *       IBM Confidential                                *
 *                                                       *
 *       (c) Copyright IBM Corporation 1981, 1990        *
 *                                                       *
 *       All Rights Reserved                             *
 *-------------------------------------------------------*/

/*****************************************************************************
*                                                                            *
*  ADD Service Routine Header File                                           *
*                                                                            *
*****************************************************************************/

/*******************************/
/* Xfer Buffer Data Structure  */
/*******************************/

typedef struct _ADD_XFER_DATA   {             /* ADDX */

  USHORT        Mode;                         /* Direction of xferdata      */
  USHORT        cSGList;                      /* Count of S/G list elements */
  PSCATGATENTRY pSGList;                      /* Far pointer to S/G List    */
  PBYTE         pBuffer;                      /* Far pointer to buffer      */
  ULONG         numTotalBytes;                /* Total bytes to copy        */
  USHORT        iSGList;                      /* Current index of S/G List  */
  ULONG         SGOffset;                     /* Current offset             */
} ADD_XFER_DATA, FAR *PADD_XFER_DATA;


#define SGLIST_TO_BUFFER     1                /* From S/G list to buffer    */
#define BUFFER_TO_SGLIST     2                /* From buffer to S/G list    */

/*******************************/
/* Timer Data Structure        */
/*******************************/

typedef struct _ADD_TIMER_DATA   {            /* ADDT */

  ULONG        Interval;                      /* Interval value in millisecond */
  ULONG        BackupInterval;                /* Interval value for backup     */
  PFN          NotifyEntry;                   /* Notify address                */
  PVOID        Parm_1;                        /* parameter which ADD wants     */
  PVOID        Parm_2;                        /* parameter which ADD wants     */
} ADD_TIMER_DATA;


/*******************************/
/* Timer Pool Structure        */
/*******************************/

typedef struct _ADD_TIMER_POOL   {            /* ADDT */

  USHORT         MTick;                       /* Milliseconds per timer tick   */
  ADD_TIMER_DATA TimerData[1];                /* Interval value for backup     */
} ADD_TIMER_POOL;

/*                                                                          */
/* If the caller wants "n" timer elements, the size of data pool is         */
/*                                                                          */
/*    sizeof(ADD_TIMER_POOL) + (n-1)*(ADD_TIMER_DATA).                      */
/*                                                                          */
/*                                                                          */

/*******************************/
/* I/O Instruction macro       */
/*******************************/
                                              /* OUT                        */
#define outp(port, data) _asm{ \
      _asm    push ax          \
      _asm    push dx          \
      _asm    mov  ax,data     \
      _asm    mov  dx,port     \
      _asm    out  dx,al       \
      _asm    pop  dx          \
      _asm    pop  ax          \
}
                                             /* IN                         */
#define inp(port, data) _asm{  \
      _asm    push ax          \
      _asm    push dx          \
      _asm    xor  ax,ax       \
      _asm    mov  dx,port     \
      _asm    in   al,dx       \
      _asm    mov  data,ax     \
      _asm    pop  dx          \
      _asm    pop  ax          \
}
                                             /* OUTSW                       */
#define outswp(port, pdata) _asm{  \
          _asm   push ds         \
          _asm   push si         \
          _asm   push dx         \
          _asm   lds  di,pdata   \
          _asm   mov  dx,port    \
          _asm   outsw           \
          _asm   pop  dx         \
          _asm   pop  si         \
          _asm   pop  ds         \
}
                                             /* INSW                        */
#define inswp(port, pdata) _asm{ \
          _asm   push es         \
          _asm   push di         \
          _asm   push dx         \
          _asm   les  di,pdata   \
          _asm   mov  dx,port    \
          _asm   insw            \
          _asm   pop  dx         \
          _asm   pop  di         \
          _asm   pop  es         \
}

/*******************************/
/* ADD Common Services         */
/*******************************/


BOOL APIENTRY f_ADD_XferBuffData(PADD_XFER_DATA);
BOOL APIENTRY f_ADD_DMASetup(USHORT, USHORT, USHORT, ULONG);
BOOL APIENTRY f_ADD_ConvRBAtoCHS(ULONG, PGEOMETRY, PCHS_ADDR);

BOOL APIENTRY f_ADD_InitTimer(PBYTE, USHORT);
BOOL APIENTRY f_ADD_StartTimerMS(PULONG, ULONG, PFN, PVOID, PVOID);
BOOL APIENTRY f_ADD_CancelTimer(ULONG);


BOOL PASCAL NEAR ADD_XferBuffData(PADD_XFER_DATA);
BOOL PASCAL NEAR ADD_DMASetup(USHORT, USHORT, USHORT, ULONG);
BOOL PASCAL NEAR ADD_ConvRBAtoCHS(ULONG, PGEOMETRY, PCHS_ADDR);

BOOL PASCAL NEAR ADD_InitTimer(PBYTE, USHORT);
BOOL PASCAL NEAR ADD_StartTimerMS(PULONG, ULONG, PFN, PVOID, PVOID);
BOOL PASCAL NEAR ADD_CancelTimer(ULONG);


/*******************************/
/* ADD Common Services R / C   */
/*******************************/

#define ADD_SUCCESS   0
#define ADD_ERROR     1


