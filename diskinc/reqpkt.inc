MAX_DISKDD_CMD	EQU	29 

DDHDR	STRUC
DDH_NextHeader	DB	SIZE PVOID DUP (?)
DDH_DevAttr	DW	?
DDH_StrategyEP	DW	?
DDH_InterruptEP	DW	?
DDH_DevName	DB	8 DUP (?)
DDH_ProtModeCS	DW	?
DDH_ProtModeDS	DW	?
DDH_RealModeCS	DW	?
DDH_RealModeDS	DW	?
DDH_SDevCaps	DD	?
DDHDR	ENDS

BPB	STRUC
BPB_BytesPerSector	DW	?
BPB_SectorsPerCluster	DB	?
BPB_ReservedSectors	DW	?
BPB_NumFATs	DB	?
BPB_MaxDirEntries	DW	?
BPB_TotalSectors	DW	?
BPB_MediaType	DB	?
BPB_NumFATSectors	DW	?
BPB_SectorsPerTrack	DW	?
BPB_NumHeads	DW	?
BPB_HiddenSectors	DD	?
BPB_BigTotalSectors	DD	?
BPB_Reserved_1	DB	6 DUP (?)
BPB	ENDS

RPH	STRUC
RPH_Len	DB	?
RPH_Unit	DB	?
RPH_Cmd	DB	?
RPH_Status	DW	?
RPH_Flags	DB	?
RPH_Reserved_1	DB	3 DUP (?)
RPH_Link	DD	?
RPH	ENDS
STERR	EQU	8000H            
STINTER	EQU	0400H            
STBUI	EQU	0200H            
STDON	EQU	0100H            
STECODE	EQU	00FFH            
WRECODE	EQU	0000H 
STATUS_DONE	EQU	0100H 
STATUS_ERR_UNKCMD	EQU	8003H 
RPF_Int13RP	EQU	01H         
RPF_CallOutDone	EQU	02H         
RPF_PktDiskIOTchd	EQU	04H         
RPF_CHS_ADDRESSING	EQU	08H         
RPF_Internal	EQU	10H         
RPF_TraceComplete	EQU	20H         

RPINIT	STRUC
RPINI_rph	DB	SIZE RPH DUP (?)
RPINI_Unit	DB	?
RPINI_DevHlpEP	DB	SIZE PFN DUP (?)
RPINI_InitArgs	DB	SIZE PSZ DUP (?)
RPINI_DriveNum	DB	?
RPINIT	ENDS

RPINITOUT	STRUC
RPINO_rph	DB	SIZE RPH DUP (?)
RPINO_Unit	DB	?
RPINO_CodeEnd	DW	?
RPINO_DataEnd	DW	?
RPINO_BPBArray	DD	?
RPINO_Status	DW	?
RPINITOUT	ENDS

DDD_Parm_List	STRUC
DDPL_cache_parm_list	DW	?
DDPL_disk_config_table	DW	?
DDPL_int_req_vec_table	DW	?
DDPL_cmd_line_args	DW	?
DDD_Parm_List	ENDS
ifndef INCL_INITRP_ONLY

RP_MEDIACHECK	STRUC
RPMC_rph	DB	SIZE RPH DUP (?)
RPMC_MediaDescr	DB	?
RPMC_rc	DB	?
RPMC_PrevVolID	DB	SIZE PSZ DUP (?)
RP_MEDIACHECK	ENDS

RP_BUILDBPB	STRUC
RPBPB_rph	DB	SIZE RPH DUP (?)
RPBPB_MediaDescr	DB	?
RPBPB_XferAddr	DD	?
RPBPB_bpb	DB	SIZE PBPB DUP (?)
RPBPB_DriveNum	DB	?
RP_BUILDBPB	ENDS

RP_RWV	STRUC
RPRWV_rph	DB	SIZE RPH DUP (?)
RPRWV_MediaDescr	DB	?
RPRWV_XferAddr	DD	?
RPRWV_NumSectors	DW	?
RPRWV_rba	DD	?
RPRWV_sfn	DW	?
RP_RWV	ENDS

RP_NONDESTRUCREAD	STRUC
RPNDR_rph	DB	SIZE RPH DUP (?)
RPNDR_character	DB	?
RP_NONDESTRUCREAD	ENDS

RP_OPENCLOSE	STRUC
RPOC_rph	DB	SIZE RPH DUP (?)
RPOC_sfn	DW	?
RP_OPENCLOSE	ENDS

RP_GENIOCTL	STRUC
RPGIO_rph	DB	SIZE RPH DUP (?)
RPGIO_Category	DB	?
RPGIO_Function	DB	?
RPGIO_ParmPacket	DB	SIZE PUCHAR DUP (?)
RPGIO_DataPacket	DB	SIZE PUCHAR DUP (?)
RPGIO_sfn	DW	?
RP_GENIOCTL	ENDS

RP_PARTFIXEDDISKS	STRUC
RPFD_rph	DB	SIZE RPH DUP (?)
RPFD_NumFixedDisks	DB	?
RP_PARTFIXEDDISKS	ENDS

RP_GETUNITMAP	STRUC
RPUM_rph	DB	SIZE RPH DUP (?)
RPUM_UnitMap	DD	?
RP_GETUNITMAP	ENDS

RP_GETDRIVERCAPS	STRUC
RPDC_rph	DB	SIZE RPH DUP (?)
RPDC_Reserved	DB	3 DUP (?)
RPDC_pDCS	DB	SIZE P_DriverCaps DUP (?)
RPDC_pVCS	DB	SIZE P_VolChars DUP (?)
RP_GETDRIVERCAPS	ENDS
endif