/*---------------------------------------*/
/* Function declarations - Static Code   */
/*---------------------------------------*/
USHORT NEAR DriveInit (PRPINITIN, NPVOLCB);

VOID   NEAR DMStrat1 (void);
USHORT NEAR MediaCheck (PRP_MEDIACHECK, NPVOLCB);
USHORT NEAR BuildBPB (PRP_BUILDBPB, NPVOLCB);
USHORT NEAR ReadWriteV (PRP_RWV, NPVOLCB);
USHORT NEAR RemovableMedia (PRPH, NPVOLCB);
USHORT NEAR DriveGenIOCTL (PRP_GENIOCTL, NPVOLCB);
USHORT NEAR ResetMedia (PRPH, NPVOLCB);
USHORT NEAR GetLogDriveMap (PRPH, NPVOLCB);
USHORT NEAR SetLogDriveMap (PRPH, NPVOLCB);
USHORT NEAR PartFixedDisks (PRP_PARTFIXEDDISKS, NPVOLCB);
USHORT NEAR GetUnitMap (PRP_GETUNITMAP, NPVOLCB);
USHORT NEAR GetDriverCaps (PRP_GETDRIVERCAPS, NPVOLCB);
USHORT NEAR CmdErr (PRPH, NPVOLCB);
USHORT NEAR StatusDevReady (PRPH, NPVOLCB);
USHORT NEAR StatusComplete (PRPH, NPVOLCB);
USHORT NEAR StatusError (PRPH, USHORT);

VOID   _loadds FAR  DMStrat2 (void);
USHORT _loadds FAR  DD_SetFSDInfo (void);

USHORT NEAR DiskIO (PBYTE, NPVOLCB);
USHORT NEAR DiskIO_Wait (PBYTE, NPVOLCB);
USHORT FAR  f_DiskIO_Wait (PBYTE, NPVOLCB);
USHORT NEAR Get_VolCB_Addr (USHORT, NPVOLCB FAR *);
USHORT FAR  f_Get_VolCB_Addr (USHORT, NPVOLCB FAR *);
VOID   NEAR SetupIORB (NPUNITCB, PBYTE, NPIORBH);
VOID   NEAR SubmitIORB (PIORBH);
ULONG  NEAR CHS_to_RBA (NPVOLCB, USHORT, UCHAR, UCHAR);
ULONG  FAR  f_CHS_to_RBA (NPVOLCB, USHORT, UCHAR, UCHAR);
USHORT NEAR ReadSecInScratch_RBA (NPVOLCB, ULONG, USHORT);
USHORT FAR  f_ReadSecInScratch_RBA (NPVOLCB, ULONG, USHORT);
USHORT NEAR ReadSecInScratch_CHS (NPVOLCB, USHORT, UCHAR, UCHAR);
USHORT FAR  f_ReadSecInScratch_CHS (NPVOLCB, USHORT, UCHAR, UCHAR);
USHORT NEAR CheckPseudoChange (USHORT, NPVOLCB);
USHORT FAR  f_CheckPseudoChange (USHORT, NPVOLCB);
VOID   NEAR Update_Owner (NPVOLCB);
VOID   _loadds FAR  NotifyDoneIORB(PIORB);
VOID   NEAR NotifyRLE (PPB_Read_Write);
USHORT NEAR AllocIORB(NPUNITCB, NPIORBH FAR *);
VOID   NEAR AllocIORB_Wait(NPUNITCB, NPIORBH FAR *);
VOID   NEAR FreeIORB(NPUNITCB, NPIORBH);
UCHAR  NEAR MapADDError(USHORT);
USHORT NEAR GetPrtyQIndex(UCHAR);
VOID   NEAR PutPriorityQueue_RP (NPUNITCB, PBYTE);
VOID   NEAR PutPriorityQueue_RLE (NPUNITCB, PPB_Read_Write);
USHORT NEAR PullPriorityQueue (NPUNITCB, PBYTE FAR *);
USHORT NEAR RemovePriorityQueue (NPUNITCB, PPB_Read_Write);


VOID   NEAR AbortReqList (PReq_List_Header);
VOID   NEAR SubmitRequestsToADD (NPUNITCB);
USHORT NEAR CheckWithinPartition (NPVOLCB, ULONG, ULONG);
ULONG  NEAR VirtToPhys(PBYTE);

USHORT NEAR BPBFromBoot (NPVOLCB, PDOSBOOTREC);
USHORT FAR  f_BPBFromBoot (NPVOLCB, PDOSBOOTREC);
USHORT NEAR BPBFromScratch (NPVOLCB);
USHORT FAR  f_BPBFromScratch (NPVOLCB);
VOID   NEAR BootBPB_To_MediaBPB (NPVOLCB, PDOSBOOTREC);
USHORT NEAR Is_BPB_Boot (PDOSBOOTREC);
USHORT NEAR GetBootVersion (PDOSBOOTREC);
USHORT NEAR Process_Partition (NPVOLCB, PULONG, PULONG);
USHORT NEAR Process_Boot (NPVOLCB, ULONG);
USHORT NEAR w_MediaCheck (UCHAR, NPVOLCB);
USHORT NEAR CheckChangeSignal (USHORT, NPVOLCB);

/*-------------------------------------*/
/* Function declarations - SwapCode    */
/*-------------------------------------*/
USHORT FAR  f_DriveGenIOCTL (PRP_GENIOCTL, NPVOLCB);
#pragma alloc_text(SwapCode, f_DriveGenIOCTL)

/*-----------------------------------------------*/
/* Function declarations for Assembler routines  */
/*-----------------------------------------------*/
USHORT FAR  f_FSD_AccValidate (USHORT);
VOID   FAR  f_ZeroCB (PBYTE, USHORT);
VOID   FAR  f_BlockCopy (PBYTE, PBYTE, USHORT);
ULONG  FAR  f_add32(ULONG, ULONG);
VOID   FAR  f_SWait (PVOID);
VOID   NEAR SWait (PVOID);
VOID   FAR  f_SSig (PVOID);
VOID   NEAR SSig (PVOID);
VOID   NEAR SortPriorityQueue  (PRTYQ *, PBYTE);
VOID   NEAR FSD_Notify (PBYTE, PVOID, USHORT);
VOID   NEAR FSD_EndofInt (VOID);
USHORT FAR  f_FT_Request (USHORT, USHORT, PBYTE FAR *, PBYTE FAR *);
USHORT FAR  f_FT_Done (USHORT, USHORT, ULONG);
VOID   FAR  DD_ChgPriority_asm (VOID);


/*-----------------------------------*/
/* Pragma declarations - DevHelps    */
/*-----------------------------------*/
#pragma alloc_text(Code, DevHelp_VMUnLock)
#pragma alloc_text(Code, DevHelp_VMLock)
#pragma alloc_text(Code, DevHelp_VirtToLin)
#pragma alloc_text(Code, DevHelp_AllocReqPacket)
#pragma alloc_text(Code, DevHelp_FreeReqPacket)






