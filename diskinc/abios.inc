ABIOS_EP_START	EQU	00H 
ABIOS_EP_INTERRUPT	EQU	01H 
ABIOS_EP_TIMEOUT	EQU	02H 
ABFC_DEFAULT_INT_HANDLER	EQU	00H 
ABFC_RET_LID_PARMS	EQU	01H 
ABFC_READ_DEVICE_PARMS	EQU	03H 
ABFC_SET_DEVICE_PARMS	EQU	04H 
ABFC_RESET_DEVICE	EQU	05H 
ABFC_DSKT_RESET_INTERRUPT	EQU	07H 
ABFC_DSKT_READ	EQU	08H 
ABFC_DSKT_WRITE	EQU	09H 
ABFC_DKST_FORMAT	EQU	0AH 
ABFC_DSKT_VERIFY	EQU	0BH 
ABFC_DSKT_READ_MEDIA_PARMS	EQU	0CH 
ABFC_DSKT_SET_MEDIA_TYPE	EQU	0DH 
ABFC_DSKT_READ_CHGSIGNAL	EQU	0EH 
ABFC_DSKT_TURN_OFF_MOTOR	EQU	0FH 
ABFC_DKST_INT_STATUS	EQU	10H 
ABFC_DSKT_GET_MEDIA_TYPE	EQU	11H 
ABFC_DISK_READ	EQU	08H 
ABFC_DISK_WRITE	EQU	09H 
ABFC_DISK_WRITE_VERIFY	EQU	0AH 
ABFC_DISK_VERIFY	EQU	0BH 
ABFC_DISK_INTERRUPT_STATUS	EQU	0CH 
ABFC_DISK_SET_DMA_PACING	EQU	10H 
ABFC_DISK_RET_DMA_PACING	EQU	11H 
ABFC_DISK_TRANSFER_SCB	EQU	12H 
ABFC_DISK_DEALLOC_LID	EQU	14H 
ABFC_DISK_GET_SCSI_PARMS	EQU	1AH 
ABFC_SCSIA_RET_DEVICE_CONFIG	EQU	0BH 
ABFC_SCSIA_RET_INTERRUPT_LID	EQU	0CH 
AFCB_SCSIA_ENABLE_CACHE	EQU	0DH        
ABFC_SCSIA_DISABLE_CACHE	EQU	0EH        
ABFC_SCSIA_CACHE_STATUS	EQU	0FH        
ABFC_SCSIA_SET_DMA_PACING	EQU	10H 
ABFC_SCSIA_RET_DMA_PACING	EQU	11H 
ABFC_SCSIA_TRANSFER_SCB	EQU	12H 
ABFC_SCSIP_SET_DEV_TIMEOUT	EQU	10H 
ABFC_SCSIP_READ_DEV_TIMEOUT	EQU	11H 
ABFC_SCSIP_TRANSFER_SCB	EQU	12H 
ABFC_SCSIP_DEALLOC_SCSI_DEV	EQU	14H 
ABFC_SCSIP_ALLOC_SCSI_DEV	EQU	15H 
ABFC_SCSIP_RET_TYPE_COUNT	EQU	16H 
ABFC_SCSIP_ABORT	EQU	17H 

ABRBH	STRUC
ABH_Length	DW	?
ABH_LID	DW	?
ABH_Unit	DW	?
ABH_Function	DW	?
ABH_Reserved_1	DD	?
ABH_RC	DW	?
ABH_Timeout	DW	?
ABRBH	ENDS

ABRB_RETLIDPARMS	STRUC
ABRLP_abrbh	DB	SIZE ABRBH DUP (?)
ABRLP_HwIntLevel	DB	?
ABRLP_ArbLevel	DB	?
ABRLP_DeviceID	DW	?
ABRLP_cUnits	DW	?
ABRLP_LIDFlags	DW	?
ABRLP_RBLength	DW	?
ABRLP_SecDeviceID	DB	?
ABRLP_Revision	DB	?
ABRLP_Reserved_1	DD	?
ABRB_RETLIDPARMS	ENDS
LF_16MB_SUPPORT	EQU	0020H   
LF_CONCURRENT	EQU	0008H 
LF_PHYSICAL_PTRS	EQU	0002H 
LF_LOGICAL_PTRS	EQU	0001H 

ABRBDIAGS	STRUC
ABD_Reserved_5	DB	?
ABD_ErrorLogLength	DB	?
ABD_IntLevel	DB	?
ABD_ArbLevel	DB	?
ABD_DeviceID	DW	?
ABD_CmdtoCtrlr	DB	?
ABD_St0	DB	?
ABD_St1	DB	?
ABD_St2	DB	?
ABD_Cylinder	DB	?
ABD_Head	DB	?
ABD_Sector	DB	?
ABD_SectorSize	DB	?
ABRBDIAGS	ENDS

ABRB_DSKT_READDEVPARMS	STRUC
AB1RDP_abrbh	DB	SIZE ABRBH DUP (?)
AB1RDP_SectorsPerTrack	DW	?
AB1RDP_BlockSize	DW	?
AB1RDP_DevCtrlFlags	DW	?
AB1RDP_UnitType	DW	?
AB1RDP_Reserved_1	DW	?
AB1RDP_Undefined_1	DW	?
AB1RDP_MotorOffTime	DD	?
AB1RDP_MotorStartTime	DD	?
AB1RDP_Undefined_2	DW	?
AB1RDP_cCylinders	DW	?
AB1RDP_Undefined_3	DW	?
AB1RDP_cHeads	DB	?
AB1RDP_RetryCount	DB	?
AB1RDP_FillByte	DB	?
AB1RDP_HeadSettleTime	DD	?
AB1RDP_ReadGap	DB	?
AB1RDP_FormatGap	DB	?
AB1RDP_DataLen	DB	?
ABRB_DSKT_READDEVPARMS	ENDS
DP_BLOCKSIZE_256	EQU	01H     
DP_BLOCKSIZE_512	EQU	02H     
DP_ABDEFGAPLEN	EQU	0040H   
DP_RECALREQUIRED	EQU	0008H   
DP_CONCURRENT_DSKT	EQU	0004H   
DP_FORMATSUPPORTED	EQU	0002H   
DP_CHANGELINE_AVAIL	EQU	0001H   
DP_DRIVENOTPRES	EQU	00H     
DP_DRIVETYPE_360KB	EQU	01H     
DP_DRIVETYPE_12OOKB	EQU	02H     
DP_DRIVETYPE_720KB	EQU	03H     
DP_DRIVETYPE_144OKB	EQU	04H     
DP_DRIVETYPE_2880KB	EQU	06H     

ABRB_DSKT_SETDEVPARMS	STRUC
AB1SDP_abrbh	DB	SIZE ABRBH DUP (?)
AB1SDP_Reserved_1	DW	?
AB1SDP_BlockSize	DW	?
AB1SDP_Undefined_1	DB	29 DUP (?)
AB1SDP_ReadGap	DB	?
AB1SDP_Undefined_2	DB	?
AB1SDP_DataLen	DB	?
ABRB_DSKT_SETDEVPARMS	ENDS

ABRB_DSKT_RESET	STRUC
AB1RS_abrbh	DB	SIZE ABRBH DUP (?)
AB1RS_Reserved_1	DW	?
ABRB_DSKT_RESET	ENDS

ABRB_DISABLE	STRUC
AB1DI_abrbh	DB	SIZE ABRBH DUP (?)
AB1DI_Undefined_1	DB	8 DUP (?)
AB1DI_Reserved_1	DW	?
ABRB_DISABLE	ENDS

ABRB_DSKT_RWV	STRUC
AB1RWV_abrbh	DB	SIZE ABRBH DUP (?)
AB1RWV_Reserved_1	DW	?
AB1RWV_pIObuffer	DD	?
AB1RWV_Reserved_2	DD	?
AB1RWV_ppIObuffer	DD	?
AB1RWV_Reserved_3	DW	?
AB1RWV_WaitTime	DD	?
AB1RWV_cSectors	DW	?
AB1RWV_Cylinder	DW	?
AB1RWV_Undefined_1	DW	?
AB1RWV_Head	DB	?
AB1RWV_Undefined_2	DB	6 DUP (?)
AB1RWV_Sector	DW	?
AB1RWV_DiagS	DB	SIZE ABRBDIAGS DUP (?)
ABRB_DSKT_RWV	ENDS

ABRB_DSKT_FORMAT	STRUC
AB1FMT_abrbh	DB	SIZE ABRBH DUP (?)
AB1FMT_Reserved_1	DW	?
AB1FMT_pFormatTable	DD	?
AB1FMT_Reserved_2	DD	?
AB1FMT_ppFormatTable	DD	?
AB1FMT_Reserved_3	DW	?
AB1FMT_WaitTime	DD	?
AB1FMT_Subfunction	DW	?
AB1FMT_Cylinder	DW	?
AB1FMT_Undefined_1	DW	?
AB1FMT_Head	DB	?
ABRB_DSKT_FORMAT	ENDS

ABRB_DSKT_READMEDIAPARMS	STRUC
AB1RMP_abrbh	DB	SIZE ABRBH DUP (?)
AB1RMP_SectorsPerTrack	DW	?
AB1RMP_BlockSize	DW	?
AB1RMP_Undefined_1	DW	?
AB1RMP_Reserved_1	DW	?
AB1RMP_Undefined_2	DB	14 DUP (?)
AB1RMP_cCylinders	DW	?
AB1RMP_Undefined_3	DW	?
AB1RMP_cHeads	DB	?
AB1RMP_Undefined_4	DB	6 DUP (?)
AB1RMP_ReadGap	DB	?
AB1RMP_FormatGap	DB	?
AB1RMP_DataLen	DB	?
ABRB_DSKT_READMEDIAPARMS	ENDS

ABRB_DSKT_SETMEDIATYPE	STRUC
AB1SMT_abrbh	DB	SIZE ABRBH DUP (?)
AB1SMT_SectorsPerTrack	DW	?
AB1SMT_BlockSize	DW	?
AB1SMT_Undefined_1	DW	?
AB1SMT_Reserved_1	DW	?
AB1SMT_Undefined_2	DB	8 DUP (?)
AB1SMT_WaitTime	DD	?
AB1SMT_Undefined_3	DW	?
AB1SMT_cTracks	DW	?
AB1SMT_Undefined_4	DD	?
AB1SMT_FillByte	DB	?
AB1SMT_Undefined_5	DB	5 DUP (?)
AB1SMT_FormatGap	DB	?
ABRB_DSKT_SETMEDIATYPE	ENDS

ABRB_DSKT_READCHGLINE	STRUC
AB1RCL_abrbh	DB	SIZE ABRBH DUP (?)
AB1RCL_ChangeLineStatus	DB	?
AB1RCL_Undefined_1	DB	5 DUP (?)
AB1RCL_Reserved_1	DW	?
ABRB_DSKT_READCHGLINE	ENDS
CHANGELINE_INACTIVE	EQU	00H     
CHANGELINE_ACTIVE	EQU	06H     

ABRB_DSKT_MOTOROFF	STRUC
AB1MO_abrbh	DB	SIZE ABRBH DUP (?)
AB1MO_Undefined_1	DB	6 DUP (?)
AB1MO_Reserved_1	DW	?
ABRB_DSKT_MOTOROFF	ENDS

ABRB_DSKT_INTRSTATUS	STRUC
AB1IS_abrbh	DB	SIZE ABRBH DUP (?)
AB1IS_InterruptStatus	DB	?
AB1IS_Undefined_1	DB	5 DUP (?)
AB1IS_Reserved_1	DW	?
ABRB_DSKT_INTRSTATUS	ENDS
NO_INTERRUPT	EQU	00H     
INTERRUPT_PENDING	EQU	01H     

ABRB_DSKT_GETMEDIATYPE	STRUC
AB1GMT_abrbh	DB	SIZE ABRBH DUP (?)
AB1GMT_MediaType	DB	?
AB1GMT_Undefined_1	DB	5 DUP (?)
AB1GMT_Reserved_1	DW	?
ABRB_DSKT_GETMEDIATYPE	ENDS
MEDIA1MB	EQU	03H 
MEDIA2MB	EQU	04H 
MEDIA4MB	EQU	06H 

ABRB_DISK_READDEVPARMS	STRUC
AB2RDP_abrbh	DB	SIZE ABRBH DUP (?)
AB2RDP_SectorsPerTrack	DW	?
AB2RDP_BlockSize	DW	?
AB2RDP_DevCtrlFlags	DW	?
AB2RDP_LUN	DB	?
AB2RDP_Undefined_1	DB	?
AB2RDP_cCylinders	DD	?
AB2RDP_cHead	DB	?
AB2RDP_RetryCount	DB	?
AB2RDP_Undefined_2	DW	?
AB2RDP_cRBA	DD	?
AB2RDP_Reserved_1	DD	?
AB2RDP_Reserved_2	DW	?
AB2RDP_Undefined_3	DW	?
AB2RDP_MaxXferCount	DW	?
ABRB_DISK_READDEVPARMS	ENDS
DP_SCBXFER	EQU	8000H   
DP_SCSI_DEVICE	EQU	4000H   
DP_FORMAT_UNIT	EQU	1000H   
DP_FORMAT_TRACK	EQU	0800H   
DP_ST506	EQU	0400H   
DP_CONCURRENT_DISK	EQU	0200H   
DP_EJECTABLE	EQU	0100H   
DP_MEDIA_ORGANIZATION	EQU	0080H   
DP_LOCKING_AVAIL	EQU	0040H   
DP_READABLE	EQU	0020H   
DP_CACHE_AVAIL	EQU	0010H   
DP_WRITE_FREQUENCY	EQU	0008H   
DP_CHGLINE_DISK	EQU	0004H   
DP_POWER	EQU	0002H   
DP_LOGDATAPTR	EQU	0001H   

ABRB_DISK_RESET	STRUC
AB2RS_abrbh	DB	SIZE ABRBH DUP (?)
AB2RS_Reserved_1	DW	?
AB2RS_Undefined_1	DB	22 DUP (?)
AB2RS_WaitTime	DD	?
ABRB_DISK_RESET	ENDS

ABRB_DISK_RWV	STRUC
AB2RWV_abrbh	DB	SIZE ABRBH DUP (?)
AB2RWV_Reserved_1	DW	?
AB2RWV_pIObuffer	DD	?
AB2RWV_Reserved_2	DW	?
AB2RWV_Reserved_3	DW	?
AB2RWV_ppIObuffer	DD	?
AB2RWV_Reserved_4	DW	?
AB2RWV_RBA	DD	?
AB2RWV_Reserved_5	DD	?
AB2RWV_WaitTime	DD	?
AB2RWV_cBlocks	DW	?
AB2RWV_Flags	DB	?
AB2RWV_SoftError	DW	?
ABRB_DISK_RWV	ENDS
RW_DONT_CACHE	EQU	01H     

ABRB_DISK_INTRSTATUS	STRUC
AB2IS_abrbh	DB	SIZE ABRBH DUP (?)
AB2IS_InterruptStatus	DB	?
AB2IS_Undefined_1	DB	5 DUP (?)
AB2IS_Reserved_1	DW	?
ABRB_DISK_INTRSTATUS	ENDS
NO_INTERRUPT	EQU	00H     
INTERRUPT_PENDING	EQU	01H     

ABRB_DISK_SETDMAPACING	STRUC
AB2SDMA_abrbh	DB	SIZE ABRBH DUP (?)
AB2SDMA_DMAPacing	DB	?
AB2SDMA_Undefined_1	DB	5 DUP (?)
AB2SDMA_Reserved_1	DW	?
AB2SDMA_Undefined_2	DB	16 DUP (?)
AB2SDMA_WaitTime	DD	?
ABRB_DISK_SETDMAPACING	ENDS

ABRB_DISK_RETDMAPACING	STRUC
AB2RDMA_abrbh	DB	SIZE ABRBH DUP (?)
AB2RDMA_DMAPacing	DB	?
AB2RDMA_Undefined_1	DB	5 DUP (?)
AB2RDMA_Reserved_1	DW	?
ABRB_DISK_RETDMAPACING	ENDS
ifndef INCL_NO_SCB

ABRB_DISK_XFERSCB	STRUC
AB2XSCB_abrbh	DB	SIZE ABRBH DUP (?)
AB2XSCB_ppSCB	DD	?
AB2XSCB_Reserved_1	DW	?
AB2XSCB_pSCBHdr	DB	SIZE PSCBHDR DUP (?)
AB2XSCB_Undefined_1	DW	?
AB2XSCB_Reserved_2	DW	?
AB2XSCB_pLastSCBHdr	DB	SIZE PSCBHDR DUP (?)
AB2XSCB_Undefined_2	DD	?
AB2XSCB_Reserved_3	DW	?
AB2XSCB_WaitTime	DD	?
AB2XSCB_Reserved_4	DW	?
AB2XSCB_Flags	DB	?
AB2XSCB_SoftError	DW	?
AB2XSCB_Undefined_3	DB	?
AB2XSCB_Status	DB	?
ABRB_DISK_XFERSCB	ENDS
endif

ABRB_DISK_DEALLOCLID	STRUC
AB2DL_abrbh	DB	SIZE ABRBH DUP (?)
AB2DL_Undefined_1	DW	?
AB2DL_SCSIdisknum	DW	?
AB2DL_Undefined_2	DW	?
AB2DL_Reserved_1	DW	?
ABRB_DISK_DEALLOCLID	ENDS

ABRB_DISK_GETSCSIPARMS	STRUC
AB2GSP_abrbh	DB	SIZE ABRBH DUP (?)
AB2GSP_Reserved_1	DW	?
AB2GSP_PUN	DB	?
AB2GSP_LUN	DB	?
AB2GSP_LDN	DB	?
AB2GSP_AdapterIndex	DB	?
AB2GSP_Port	DW	?
ABRB_DISK_GETSCSIPARMS	ENDS

ABRB_SCSIA_RETDEVICECFGTBL	STRUC
AB17RDCT_abrbh	DB	SIZE ABRBH DUP (?)
AB17RDCT_Reserved_1	DW	?
AB17RDCT_pDevCfgTbl	DB	SIZE PBYTE DUP (?)
AB17RDCT_Reserved_2	DW	?
ABRB_SCSIA_RETDEVICECFGTBL	ENDS

ABRB_RETINTRLID	STRUC
AB17RIL_abrbh	DB	SIZE ABRBH DUP (?)
AB17RIL_IntStatus	DB	?
AB17RIL_Undefined_1	DB	?
AB17RIL_IntLID	DW	?
AB17RIL_Undefined_2	DW	?
AB17RIL_Reserved_1	DW	?
ABRB_RETINTRLID	ENDS

ABRB_SCSIP_READDEVPARMS	STRUC
AB18RDP_abrbh	DB	SIZE ABRBH DUP (?)
AB18RDP_Reserved_1	DW	?
AB18RDP_SCB_Level	DB	?
AB18RDP_AdapterIndex	DB	?
AB18RDP_DeviceFlags	DW	?
AB18RDP_LUN	DB	?
AB18RDP_PUN	DB	?
AB18RDP_Undefined_1	DB	16 DUP (?)
AB18RDP_Reserved_2	DW	?
ABRB_SCSIP_READDEVPARMS	ENDS
DEVDEFECTIVE	EQU	0001H     
POWEROFF	EQU	0002H     
CACHESUPPORT	EQU	0010H     

ABRB_SCSIP_RESET	STRUC
AB18RS_abrbh	DB	SIZE ABRBH DUP (?)
AB18RS_Undefined_1	DB	6 DUP (?)
AB18RS_Reserved_1	DW	?
AB18RS_Undefined_2	DB	16 DUP (?)
AB18RS_WaitTime	DD	?
AB18RS_Undefined_3	DB	6 DUP (?)
AB18RS_Status	DB	?
ABRB_SCSIP_RESET	ENDS

ABRB_SCSIP_SETTIMEOUT	STRUC
AB18STO_abrbh	DB	SIZE ABRBH DUP (?)
AB18STO_Timeout	DB	?
AB18STO_Reserved_1	DB	?
AB18STO_Undefined_1	DB	4 DUP (?)
AB18STO_Reserved_2	DW	?
AB18STO_Undefined_2	DB	16 DUP (?)
AB18STO_WaitTime	DD	?
AB18STO_Undefined_3	DB	6 DUP (?)
AB18STO_Status	DB	?
ABRB_SCSIP_SETTIMEOUT	ENDS

ABRB_SCSIP_READTIMEOUT	STRUC
AB18RTO_abrbh	DB	SIZE ABRBH DUP (?)
AB18RTO_Timeout	DB	?
AB18RTO_Reserved_1	DB	?
AB18RTO_Undefined_1	DB	4 DUP (?)
AB18RTO_Reserved_2	DW	?
ABRB_SCSIP_READTIMEOUT	ENDS

ABRB_SCSIP_TRANSFERSCB	STRUC
AB18TSCB_abrbh	DB	SIZE ABRBH DUP (?)
AB18TSCB_ppSCB	DD	?
AB18TSCB_Reserved_1	DW	?
AB18TSCB_pSCBHdr	DB	SIZE PSCBHDR DUP (?)
AB18TSCB_Undefined_1	DW	?
AB18TSCB_Reserved_2	DW	?
AB18TSCB_Undefined_2	DB	8 DUP (?)
AB18TSCB_Reserved_3	DW	?
AB18TSCB_WaitTime	DD	?
AB18TSCB_Reserved_4	DW	?
AB18TSCB_Flags	DB	?
AB18TSCB_Undefined_3	DB	3 DUP (?)
AB18TSCB_Status	DB	?
ABRB_SCSIP_TRANSFERSCB	ENDS
NORMAL_SCB	EQU	00H                
LONG_SCB	EQU	01H                
TSB_NOT_REQD	EQU	00H             
TSB_NEEDED	EQU	02H             

ABRB_SCSIP_DEALLOC	STRUC
AB18DPD_abrbh	DB	SIZE ABRBH DUP (?)
AB18DPD_Undefined_1	DB	6 DUP (?)
AB18DPD_Reserved_1	DW	?
ABRB_SCSIP_DEALLOC	ENDS

ABRB_SCSIP_ALLOC	STRUC
AB18APD_abrbh	DB	SIZE ABRBH DUP (?)
AB18APD_DevType	DB	?
AB18APD_DevFlags	DB	?
AB18APD_DevUnit	DW	?
AB18APD_Undefined_1	DW	?
AB18APD_Reserved_1	DW	?
ABRB_SCSIP_ALLOC	ENDS
DEVTYPE_CDROM	EQU	05H 
DEVTYPE_REMOVABLE	EQU	80H     

ABRB_SCSIP_RETCOUNT	STRUC
AB18RC_abrbh	DB	SIZE ABRBH DUP (?)
AB18RC_DevType	DB	?
AB18RC_DevFlags	DB	?
AB18RC_Undefined_1	DW	?
AB18RC_DevCount	DW	?
AB18RC_Reserved_1	DW	?
ABRB_SCSIP_RETCOUNT	ENDS

ABRB_SCSIP_ABORT	STRUC
AB18AB_abrbh	DB	SIZE ABRBH DUP (?)
AB18AB_Undefined_1	DB	6 DUP (?)
AB18AB_Reserved_1	DW	?
AB18AB_Undefined_2	DB	16 DUP (?)
AB18AB_WaitTime	DD	?
AB18AB_Undefined_3	DB	6 DUP (?)
AB18AB_Status	DB	?
ABRB_SCSIP_ABORT	ENDS
GENERIC_ABRB_SIZE	EQU	128 
ABRC_COMPLETEOK	EQU	0000H 
ABRC_STAGEONINTERRUPT	EQU	0001H 
ABRC_STAGEONTIME	EQU	0002H 
ABRC_NOTMYINTERRUPT	EQU	0005H 
ABRC_ATTENTION	EQU	0009H 
ABRC_SPURIOUSINTERRUPT	EQU	0081H 
ABRC_BUSY	EQU	8000H 
ABRC_START	EQU	0FFFFH 
ABRC_UNSUPPORTED_LID	EQU	0C000H 
ABRC_UNSUPPORTED_FUNCTION	EQU	0C001H 
ABRC_UNSUPPORRTED_UNIT	EQU	0C003H 
ABRC_UNSUPPORTED_RB_LEN	EQU	0C004H 
ABRC_INVALID_PARM	EQU	0C005H 
ABRC_ERRORBIT	EQU	8000H 
ABRC_RETRYBIT	EQU	0100H 
ABRC_ERRORMSK	EQU	00FFH 
ABRC_DSKT_WRITE_PROTECT	EQU	8003H 
ABRC_DSKT_MEDIA_CHANGED	EQU	8006H 
ABRC_DSKT_MEDIA_NOT_PRESENT	EQU	800DH 
ABRC_DSKT_NOCHGSIG	EQU	800EH 
ABRC_DSKT_INVALID_VALUE	EQU	800FH 
ABRC_DSKT_MEDIA_NOTSUPPORTED	EQU	8010H 
ABRC_DSKT_NO_MEDIA_SENSE	EQU	8011H 
ABRC_DSKT_RESET_FAIL	EQU	9009H 
ABRC_DSKT_ADDRMARK_NOTFND	EQU	9102H 
ABRC_DSKT_SECTOR_NOTFND	EQU	9104H 
ABRC_DSKT_DMA_IN_PROGRESS	EQU	9107H 
ABRC_DSKT_DMA_OVERRUN	EQU	9108H 
ABRC_DSKT_BAD_CRC	EQU	9110H 
ABRC_DSKT_BAD_CONTROLLER	EQU	9120H    
ABRC_DSKT_BAD_SEEK	EQU	9140H 
ABRC_DSKT_GENERAL_ERROR	EQU	9180H 
ABRC_DSKT_UNKNOWN_MEDIA	EQU	0C00CH 
ABRC_DISK_DEV_NOT_POWERED_ON	EQU	8001H    
ABRC_DISK_DEV_BLOCK_INIT_ERR	EQU	8002H    
ABRC_DISK_DEV_NOT_ALLOCATED	EQU	8003H    
ABRC_DISK_DMA_ARB_INVALID	EQU	800FH    
ABRC_DISK_BAD_COMMAND	EQU	9001H    
ABRC_DISK_ADDRMARK_NOTFND	EQU	9002H    
ABRC_DISK_WRITE_PROTECT	EQU	9003H    
ABRC_DISK_RECORD_NOTFND	EQU	9004H    
ABRC_DISK_RESET_FAIL	EQU	9005H    
ABRC_DISK_MEDIA_CHANGED	EQU	9006H    
ABRC_DISK_CTRL_PARM_FAIL	EQU	9007H    
ABRC_DISK_DMA_FAIL	EQU	9008H    
ABRC_DISK_DEFECTIVE_SECTOR	EQU	900AH    
ABRC_DISK_BAD_TRACK	EQU	900BH    
ABRC_DISK_FORMAT_ERROR	EQU	900DH    
ABRC_DISK_CAM_RV	EQU	900EH    
ABRC_DISK_CRC	EQU	9010H    
ABRC_DISK_DEVICE_FAILED	EQU	9014H    
ABRC_DISK_BUS_FAULT	EQU	9015H    
ABRC_DISK_BAD_CONTROLLER	EQU	9020H    
ABRC_DISK_EQUIP_CHECK	EQU	9021H    
ABRC_DISK_BAD_SEEK	EQU	9040H    
ABRC_DISK_DEVICE_NORESPONSE	EQU	9080H    
ABRC_DISK_DRIVE_NOTREADY	EQU	90AAH    
ABRC_DISK_UNDEFINED_ERROR	EQU	90BBH    
ABRC_DISK_WRITE_FAULT	EQU	90CCH    
ABRC_DISK_STATUS_ERROR	EQU	90E0H    
ABRC_DISK_INCOMPLETE_SENSE	EQU	90FFH    
ABRC_DISK_ECC_CORRECTED	EQU	0A011H    
ABNEC	EQU	020Q      
NEC_DSKT_READ	EQU	0E6H 
NEC_DSKT_WRITE	EQU	0C5H 
NEC_DSKT_FORMAT	EQU	4DH 
ABDISKETTELID	EQU	0001H 
HW_CHANGELINE_ACTIVE	EQU	80H 
ABCHGLINEACTIVE	EQU	00FCH 
ABRB_NOCACHE	EQU	01H   
ABRETRYCOUNT	EQU	05H   
DEVID_INTERNAL	EQU	00H 
DEVID_DISKETTE	EQU	01H 
DEVID_DISK	EQU	02H 
DEVID_VIDEO	EQU	03H 
DEVID_KEYBOARD	EQU	04H 
DEVID_PARALLEL	EQU	05H 
DEVID_ASYNC	EQU	06H 
DEVID_SYSTEMTIMER	EQU	07H 
DEVID_RTC	EQU	08H 
DEVID_SYSTEMSVCS	EQU	09H 
DEVID_NMI	EQU	0AH 
DEVID_MOUSE	EQU	0BH 
DEVID_NVRAM	EQU	0EH 
DEVID_DMA	EQU	0FH 
DEVID_POS	EQU	10H 
DEVID_SECURITY	EQU	16H 
DEVID_SCSIADAPTER	EQU	17H 
DEVID_GENERICSCSI	EQU	18H 
