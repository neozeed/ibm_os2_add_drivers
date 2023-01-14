/*static char *SCCSID = "@(#)adskdata.c	6.2 92/01/17";*/

/***********************************************************************/
/*                                                                     */
/* Driver Name: ABIOS Adapter Device Driver for DASD Devices           */
/*              --------------------------------------------           */
/*                                                                     */
/* Source File Name: ADSKPRO.H                                         */
/*                                                                     */
/* Descriptive Name: ABIOS Disk Driver - Static/Initialization Data    */
/*                                                                     */
/* Function: Declares all internal data used by this ADD.              */
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

/*--------------*/
/* Static Data  */
/*--------------*/

PFN     Device_Help     = 0;

USHORT  InitComplete    = 0;
USHORT  TotalLids       = 0;
USHORT  TotalLCBs       = 0;

USHORT  ADDHandle       = 0;

NPUCB   npUCBAnchor     = 0;

NPLCB   npLCBCmpQHead   = 0;
NPLCB   npLCBCmpQFoot   = 0;

NPLCB   npLCBBusyQHead  = 0;

USHORT  MaxSGBuffers    = 0;
USHORT  ABIOSMaxXfer    = 0;

USHORT  CompletionProcessActive = 0;

USHORT  LevelsInUse  = 0;
INTCB   IntLevelCB[MAX_HW_INT_LEVELS] = { { IRQEntry0, -1 },
                                          { IRQEntry1, -1 },
                                          { IRQEntry2, -1 },
                                          { IRQEntry3, -1 }  };

NPLCB           npLCBBufQHead = 0;
NPLCB           npLCBBufQFoot = 0;

NPIOBUF_POOL    npBufPoolHead = 0;
NPIOBUF_POOL    npBufPoolFoot = 0;
IOBUF_POOL      IOBufPool[MAX_SG_BUFFERS] = { 0 };

UCHAR           LidIOCount[MAX_LIDS] = { 0 };

#include "adskerrt.h"

/****** The following arrays/vars are defined in ADSKERRT.H ************

 UCHAR   AB_8xxx_To_Index[];
 USHORT  AB_8xxx_Max_Index;
 USHORT  AB_8xxx_Index_To_IORBErr[];

 UCHAR   AB_9xxx_To_Index[];
 USHORT  AB_9xxx_Max_Index;
 USHORT  AB_9xxx_Index_To_IORBErr[];
 USHORT  AB_Axxx_Index_To_IORBErr[];

 UCHAR   AB_Cxxx_To_Index[];
 USHORT  AB_Cxxx_Max_Index;
 USHORT  AB_Cxxx_Index_To_IORBErr[];

*************************************************************************/

UCHAR AdapterName_ST506[17] = { "ABIOS_ST506_DASD" };
UCHAR AdapterName_ESDI [17] = { "ABIOS_ESDI_DASD " };
UCHAR AdapterName_SCSI [17] = { "ABIOS_SCSI_DASD " };

/*--------------------*/
/* Configuration Data */
/*--------------------*/

BYTE    ConfigPool[MAX_CONFIG_DATA]  = {0};

/*---------------------*/
/* Initialization Data */
/*---------------------*/

BYTE    InitDataStart                = 0;
USHORT  ConfigPoolAvail              = MAX_CONFIG_DATA;
NPBYTE  npConfigPool                 = ConfigPool;
NPUCB   npUCBPrevLID                 = 0;
NPLCB   npLCBPrevLID                 = 0;
BYTE    InitABRB1[GENERIC_ABRB_SIZE] = {0};
BYTE    InitABRB2[GENERIC_ABRB_SIZE] = {0};

USHORT  InitLidTable[MAX_LIDS]       = {0};


