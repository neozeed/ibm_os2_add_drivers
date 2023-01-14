/*static char *SCCSID = "@(#)adskextn.h	6.2 92/01/17";*/

/***********************************************************************/
/*                                                                     */
/* Driver Name: ABIOS Adapter Device Driver for DASD Devices           */
/*              --------------------------------------------           */
/*                                                                     */
/* Source File Name: ADSKEXTN.H                                        */
/*                                                                     */
/* Descriptive Name: ABIOS Disk Driver - Static/Initialization Externs */
/*                                                                     */
/* Function: Maps internal data used by this ADD in ADSKDATA.C         */
/*                                                                     */
/*           When editing this file, try to keep the text aligned with */
/*           ADSKDATA.C. This will make tracking EXTERNS for data      */
/*           items a log easier!                                       */
/*                                                                     */
/***********************************************************************/

/*--------------*/
/* Static Data  */
/*--------------*/

extern PFN     Device_Help;

extern USHORT  InitComplete;
extern USHORT  TotalLids;
extern USHORT  TotalLCBs;

extern USHORT  ADDHandle;

extern NPUCB   npUCBAnchor;

extern NPLCB   npLCBCmpQHead;
extern NPLCB   npLCBCmpQFoot;

extern NPLCB   npLCBBusyQHead;

extern USHORT  MaxSGBuffers;
extern USHORT  ABIOSMaxXfer;

extern USHORT  CompletionProcessActive;

extern USHORT  LevelsInUse;
extern INTCB   IntLevelCB[MAX_HW_INT_LEVELS];




extern NPLCB   npLCBBufQHead;
extern NPLCB   npLCBBufQFoot;

extern NPIOBUF_POOL    npBufPoolHead;
extern NPIOBUF_POOL    npBufPoolFoot;
extern IOBUF_POOL      IOBufPool[];

extern UCHAR           LidIOCount[MAX_LIDS];



/* Refer to ADSKERRT.H for the following data items */

extern UCHAR   AB_8xxx_To_Index[];
extern USHORT  AB_8xxx_Max_Index;
extern USHORT  AB_8xxx_Index_To_IORBErr[];

extern UCHAR   AB_9xxx_To_Index[];
extern USHORT  AB_9xxx_Max_Index;
extern USHORT  AB_9xxx_Index_To_IORBErr[];
extern USHORT  AB_Axxx_Index_To_IORBErr[];

extern UCHAR   AB_Cxxx_To_Index[];
extern USHORT  AB_Cxxx_Max_Index;
extern USHORT  AB_Cxxx_Index_To_IORBErr[];



extern UCHAR AdapterName_ST506[17];
extern UCHAR AdapterName_ESDI [17];
extern UCHAR AdapterName_SCSI [17];


/*--------------------*/
/* Configuration Data */
/*--------------------*/

extern  BYTE   ConfigPool[MAX_CONFIG_DATA];

/*---------------------*/
/* Initialization Data */
/*---------------------*/

extern BYTE    InitDataStart;
extern USHORT  ConfigPoolAvail;
extern NPBYTE  npConfigPool;
extern NPUCB   npUCBPrevLID;
extern NPLCB   npLCBPrevLID;
extern BYTE    InitABRB1[GENERIC_ABRB_SIZE];
extern BYTE    InitABRB2[GENERIC_ABRB_SIZE];

extern USHORT  InitLidTable[];


