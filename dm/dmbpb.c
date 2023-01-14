/*static char *SCCSID = "@(#)dmbpb.c	6.6 92/02/06";*/
#define SCCSID  "@(#)dmbpb.c	6.6 92/02/06"

/*--------------------------------------------------------
 *       (c) Copyright IBM Corporation 1981, 1990        *
 *                                                       *
 *       All Rights Reserved                             *
 *-------------------------------------------------------*/

#include "dmh.h"

/*--------------------------------------------------------------------------
;
;** Process_Partition - Process the partition table of the fixed disk
;
;   Process_Partition processes the partition table obtained from the
;   fixed disk and determines where the DOS boot sector is found on the
;   disk.
;
;   USHORT Process_Partition (NPVOLCB pVolCB, PULONG VolBootRBA,
                                              PULONG NumSectors)
;
;   ENTRY:    pVolCB           - input pointer to VolCB
;             VolBootRBA       - returned RBA of Volume Boot Sector
;             NumSectors       - returned number of sectors in partition
;
;   RETURN:   USHORT           - Result Code (NO_ERROR if valid partition)
;
;   EFFECTS:
;
;   NOTES:    Global variable ScratchBuffer contains boot sector
;             on input.
;
;----------------------------------------------------------------------------*/

USHORT Process_Partition (pVolCB, VolBootRBA, NumSectors)

NPVOLCB pVolCB;
PULONG  VolBootRBA;
PULONG  NumSectors;
{
   BOOL   found;
   USHORT i;
   MBR    *pMBR = (MBR *) ScratchBuffer;
   ULONG  temp;
   fBigFat = 0;

   pVolCB->Flags &= ~vf_NoDOSPartition;         /* Partition ok so far */

   if (pMBR->Signature != 0xAA55)               /* Check for signature word */
      pVolCB->Flags |= vf_NoDOSPartition;       /* Partition invalid */
   else
   {
      found = FALSE;
      for (i = 0; i < 4 && found == FALSE ; i++)
      {
         found = TRUE;
         switch (pMBR->PartitionTable[i].SysIndicator)
         {
            case PARTITION_16M:         /* Partition up to 16M */
                   break;
            case PARTITION_16Mto32M:    /* Partition > 16M and <= 32 M */
                   break;
            case PARTITION_32M:         /* Partition > 32M  */
                   break;
            case PARTITION_IFS:         /* IFS Partition */
                   break;
            case PARTITION_FTACTIVE:    /* Active Fault Tolerant partition */
                   pVolCB->Flags |= vf_FTPartition;
                   break;
            case PARTITION_FTINACTIVE:  /* Inactive Fault Tolerant partition */
                   pVolCB->Flags |= vf_FTPartition;
                   break;
            default:
               found = FALSE;
         }
      }

      /* If invalid partition type or valid found and < 32K in size */
      /* then indicate not a valid partition.                       */

      i--;
      if (!found || pMBR->PartitionTable[i].NumSectors < 64)
         pVolCB->Flags |= vf_NoDOSPartition;    /* Partition invalid */
      else
      {
         if (pVolCB->Flags & vf_FTPartition)
            NumFTPartitions ++;

         pVolCB->PartitionType = pMBR->PartitionTable[i].SysIndicator;

         /* Make sure end of partition within 4G sectors of start of disk */
         if (f_add32(pMBR->PartitionTable[i].RelativeSectors,
             pMBR->PartitionTable[i].NumSectors) == 0)
                 fBigFat |= vf_TooBig;

         pVolCB->MediaBPB.HiddenSectors=pMBR->PartitionTable[i].RelativeSectors;

         if (pMBR->PartitionTable[i].NumSectors <= (ULONG) 0xffff)
            pVolCB->MediaBPB.TotalSectors=(USHORT)pMBR->PartitionTable[i].NumSectors;
         else
         {
            pVolCB->MediaBPB.TotalSectors = 0;
            pVolCB->MediaBPB.BigTotalSectors=pMBR->PartitionTable[i].NumSectors;
         }

         /* Return RBA of Volume Boot Sector and NumSectors in Partition */

         *VolBootRBA = pVolCB->MediaBPB.HiddenSectors;

         *NumSectors = pMBR->PartitionTable[i].NumSectors;
      }
   }
   if (pVolCB->Flags & vf_NoDOSPartition)
      return(ERROR);
   else
      return(NO_ERROR);
}



/*--------------------------------------------------------------------------
;
;** Process_Boot - Process the boot sector of the fixed disk
;
;   Process_Boot examines the boot sector read off the fixed disk
;   and builds a BPB for it in the BDS for the drive. If the boot
;   sector is not valid, it assumes a default BPB from a table.
;   If this is a > 32M partition and the boot sector does not have
;   a valid BPB, 'C' is returned. The caller should then set up a
;   "minimum" BPB and set the fTooBig bit.
;
;   USHORT Process_Boot (NPVOLCB pVolCB, ULONG SectorsInPartition)
;
;   ENTRY:    pVolCB             - input pointer to VolCB
;             SectorsInPartition - Number of sectors in partition
;
;   RETURN:   USHORT           - Result Code (NO_ERROR if valid partition)
;
;   EFFECTS:
;
;   NOTES:    Global variable ScratchBuffer contains boot sector
;             on input.
;
;----------------------------------------------------------------------------*/

USHORT Process_Boot(pVolCB, SectorsInPartition)

NPVOLCB  pVolCB;
ULONG    SectorsInPartition;

{
   USHORT rc, i;
   ULONG  TotalSectors, temp;
   USHORT Unknown = YES;
   PDOSBOOTREC pBootRec = (PDOSBOOTREC) ScratchBuffer;

   /*--------------------------------------------------------*/
   /* Check for a valid boot sector and use the BPB in it to */
   /* build the MediaBPB in the VolCB.  It is assumed that   */
   /* ONLY SectorsPerCluster, NumFatSectors, MaxDirEntries   */
   /* and MediaType need to be set.                          */
   /*--------------------------------------------------------*/

   if (Is_BPB_Boot(pBootRec) == NO_ERROR)
   {
      pVolCB->MediaBPB.MediaType = pBootRec->bpb.MediaType;
      if (pBootRec->bpb.TotalSectors != 0)
          TotalSectors = pBootRec->bpb.TotalSectors;
      else
          TotalSectors = pBootRec->bpb.BigTotalSectors;

      /* Make sure there are enough sectors for the boot sector, */
      /* plus the FAT sectors plus the directory sectors.        */

      if (TotalSectors > (1 + (pBootRec->bpb.NumFATSectors * 2) +
                         (pBootRec->bpb.MaxDirEntries / 16)))
      {
        pVolCB->MediaBPB.NumFATSectors = pBootRec->bpb.NumFATSectors;
        pVolCB->MediaBPB.MaxDirEntries = pBootRec->bpb.MaxDirEntries;
        pVolCB->MediaBPB.SectorsPerCluster = pBootRec->bpb.SectorsPerCluster;

        /* Calculate sectors left for data sectors */

        TotalSectors = TotalSectors - (1 + (pBootRec->bpb.NumFATSectors * 2) +
                                      (pBootRec->bpb.MaxDirEntries / 16));

        if ( (pVolCB->PartitionType != PARTITION_IFS) &
             (TotalSectors / pVolCB->MediaBPB.SectorsPerCluster) > 4096-10 )
           fBigFat |= vf_Big;           /* Set FBIG if 16 bit FAT */

        Unknown = NO;
      }
   }

   if (Unknown == YES)
   {
      /* If IFS, zero out FAT related fields */

      if (pVolCB->PartitionType == PARTITION_IFS)
      {
         pVolCB->MediaBPB.SectorsPerCluster = 0;
         pVolCB->MediaBPB.ReservedSectors = 0;
         pVolCB->MediaBPB.NumFATs = 0;
         pVolCB->MediaBPB.MaxDirEntries = 0;
         if (pVolCB->MediaBPB.TotalSectors != 0)
            pVolCB->MediaBPB.BigTotalSectors = pVolCB->MediaBPB.TotalSectors;
         pVolCB->MediaBPB.TotalSectors = 0;
         pVolCB->MediaBPB.MediaType = 0;
         pVolCB->MediaBPB.NumFATSectors = 0;
      }
      else
      {

         /* Find appropriate DiskTable entry based on SectorsInPartition */

         for (i = 0; i < DISKTABLECOUNT; i++)
           if (SectorsInPartition <= DiskTable[i].NumSectors)
              break;

         fBigFat = DiskTable[i].Flags;
         pVolCB->MediaBPB.MaxDirEntries = DiskTable[i].MaxDirEntries;
         pVolCB->MediaBPB.SectorsPerCluster= DiskTable[i].SectorsPerCluster;
         pVolCB->MediaBPB.MediaType = MEDIA_FIXED_DISK;

         /* Calculate number of FAT table sectors */

         if (fBigFat & vf_Big)
         {
            temp = (pVolCB->MediaBPB.SectorsPerCluster * 256) + 2;
            pVolCB->MediaBPB.NumFATSectors = (SectorsInPartition -
               ((pVolCB->MediaBPB.MaxDirEntries / 16) + 1) + /* Dir + Reserved*/
               temp - 1 +
               (pVolCB->MediaBPB.SectorsPerCluster * 2)) / temp;
         }
         else
         {
            TotalSectors = SectorsInPartition +
                           pVolCB->MediaBPB.SectorsPerCluster - 1;

            TotalSectors >>= DiskTable[i].Log2SectorsPerCluster;

            TotalSectors = ((TotalSectors + 1) & (0xFFFFFFFE)) + 2;

            temp = TotalSectors;

            TotalSectors >>= 1;

            pVolCB->MediaBPB.NumFATSectors = (TotalSectors + temp + 511) / 512;
         }
      }
   }

   pVolCB->Flags |= vf_BigFat;

   pVolCB->RecBPB = pVolCB->MediaBPB;   /* Copy media BPB to recommended BPB */
}


/*--------------------------------------------------------------------------
;
;** BPBFromBoot - Get BPB from the boot sector passed in
;
;   BPBFromBoot builds a BPB for the disk/media type from the boot
;   sector passed to it.
;
;   USHORT BPBFromBoot (NPVOLCB pVolCB, PDOSBOOTREC pBootSector)
;
;   ENTRY:    pVolCB           - VolCB for the drive
;             pBootSector      - DOS Boot Sector
;
;   RETURN:   USHORT           - Result Code (NO_ERROR if valid boot sector)
;
;   EFFECTS:
;
;   NOTES:  This code assumes that the media you are attempting to
;           build on is
;               a)  a DOS disk
;               b)  if it is a >2.x disk, the boot sector
;                   lies at sector 1 on track 0 (head 0).  The first
;                   3 bytes are a JMP, the next 8 bytes are a media
;                   ID ASCII string, and the 11 bytes following that
;                   are the BPB for that disk.
;               c)  if the above conditions (3 byte jump, 8 ascii ID
;                   string) are not met, then we assume the media is
;                   a DOS 1.x disk.  On a 1.x disk sector 2 on
;                   track 0 (head 0) contains the first FAT sector.
;                   The first byte of the first FAT sector contains
;                   the "FAT ID byte" which is the media ID byte for
;                   this disk.
;
;----------------------------------------------------------------------------*/
USHORT FAR f_BPBFromBoot (pVolCB, pBootSector)

NPVOLCB     pVolCB;
PDOSBOOTREC pBootSector;
{
   return(BPBFromBoot (pVolCB, pBootSector));
}

USHORT BPBFromBoot (pVolCB, pBootSector)

NPVOLCB pVolCB;
PDOSBOOTREC pBootSector;
{
   USHORT rc = NO_ERROR;

   if (!(pVolCB->Flags & vf_ReturnFakeBPB))    /* Do nothing if this is set */
   {
      if ( (rc = Is_BPB_Boot(pBootSector)) == NO_ERROR) /* Boot sector valid? */
      {
         BootBPB_To_MediaBPB (pVolCB, pBootSector);

         /*------------------------------------------------------------------
         ; In pre-DOS 3.20 versions of Format, there was a bug that caused
         ; the sectors per cluster field in the BPB in the boot sector to
         ; be set to 2 even when the medium was single-sided. We "fix" this
         ; by setting the field to 1 on the diskettes that may have been
         ; affected. This will ensure that whatever has been recorded on
         ; those media will remain intact.
         ------------------------------------------------------------------*/

        if ((pVolCB->pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE) &&
            ((pVolCB->MediaBPB.MediaType == MEDIA_160KB) ||
             (pVolCB->MediaBPB.MediaType == MEDIA_180KB)) &&
             (GetBootVersion(pBootSector) < 32) )

                pVolCB->MediaBPB.SectorsPerCluster = 1;

      }
   }

   return(rc);
}

/*--------------------------------------------------------------------------
;
;** BPBFromScratch - Build a BPB for the disk
;
;   BPBFromScratch builds a BPB for the drive where the boot sector
;   contains an invalid BPB in it.
;
;   For fixed disks, the partition table is read and from it the
;   location of the boot sector is obtained. The boot sector is then
;   read and the BPB built from there. An error is returned if the
;   partition table is invalid or is too small (< 32K bytes).
;
;   USHORT BPBFromScratch (NPVOLCB pVolCB, PRP pRP)
;
;   ENTRY:    pVolCB           - VolCB for the drive
;             pRP              - Request Packet
;
;   RETURN:   USHORT           - Packet Status
;
;   EFFECTS:
;
;----------------------------------------------------------------------------*/
USHORT FAR f_BPBFromScratch (pVolCB)

NPVOLCB  pVolCB;

{
   return(BPBFromScratch(pVolCB));
}


USHORT BPBFromScratch (pVolCB)

NPVOLCB  pVolCB;

{
   PBYTE   pScratchBuffer;
   UCHAR   MediaType;
   USHORT  rc;
   ULONG   VolBootRBA, NumSectors;

   /* Wait for the ScratchBuffer to be free before doing a read */

   f_SWait (&ScratchBufSem);

   if (pVolCB->pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE)      /* If floppy drive */
   {
      /* Read in the first FAT sector */

      if ((rc = ReadSecInScratch_RBA (pVolCB, 1L, 0)) == NO_ERROR)
      {
         MediaType = (UCHAR) pScratchBuffer;
         if (pVolCB->MediaBPB.MediaType != MediaType)
         {
            switch(MediaType)
            {
               case MEDIA_144MB:                         /* 1.44MB & 2.88MB */
                     if (pVolCB->RecBPB.SectorsPerTrack == 18)
                        pVolCB->MediaBPB = BPB_144MB;    /* 1.44 MB */
                     else
                        pVolCB->MediaBPB = BPB_288MB;    /* 2.88 MB */
                     break;

               case MEDIA_12MB:                          /* 1.2MB & 720 KB */
                     if (pVolCB->RecBPB.SectorsPerTrack == 15)
                        pVolCB->MediaBPB = BPB_12MB;     /* 1.2 MB */
                     else
                        pVolCB->MediaBPB = BPB_720KB;    /* 720 KB */
                     break;

               case MEDIA_360KB:
                        pVolCB->MediaBPB = BPB_360KB;
                        break;

               case MEDIA_320KB:
                        pVolCB->MediaBPB = BPB_320KB;
                        break;

               case MEDIA_180KB:
                        pVolCB->MediaBPB = BPB_180KB;
                        break;

               case MEDIA_160KB:
                        pVolCB->MediaBPB = BPB_160KB;
                        break;

               default:
                        rc = STDON + STERR + ERROR_I24_NOT_DOS_DISK;
            }
         }
      }
   }
   else  /* Fixed disk */
   {
      /* Read RBA 0 - Master Boot Record  */

      if ((rc = ReadSecInScratch_RBA (pVolCB, 0, 1)) == NO_ERROR)
      {
         if ((rc = Process_Partition (pVolCB, &VolBootRBA, &NumSectors))
                                                            == NO_ERROR)
         {
            if ((rc = ReadSecInScratch_RBA (pVolCB, VolBootRBA, 1))
                                                            == NO_ERROR)
               Process_Boot (pVolCB, NumSectors);
         }
         else
         {
            pVolCB->MediaBPB = BPB_Minimum;      /* Setup minimum BPB */
            pVolCB->RecBPB = pVolCB->MediaBPB;   /* Copy MinBPB to recommended*/
            rc = NO_ERROR;
         }
      }
   }

   f_SSig (&ScratchBufSem);     /* Release scratch buffer */

   return (rc);
}

/*--------------------------------------------------------------------------
;
;**  BootBPB_To_MediaBPB
;
;   Copies the BPB from the passed boot sector to the media BPB in the
;   Volume Control Block. If the boot sector is older than DOS 3.2,
;   the 12 byte extended BPB is not copied because it may contain garbage.
;
;   VOID BootBPB_to_MediaBPB (NPVOLCB pVolCB, PDOSBOOTREC pBootSector
;                              USHORT Type)
;
;   ENTRY:    pVolCB           - input pointer to VolCB
;             pBootSector      - Boot Sector
;             Type             - 0 = disk, 1 = diskette
;
;   RETURN:   VOID
;
;   EFFECTS:
;
;   NOTES:
;
--------------------------------------------------------------------------*/

VOID BootBPB_To_MediaBPB (pVolCB, pBootSector)

NPVOLCB    pVolCB;
PDOSBOOTREC pBootSector;
{
   USHORT  i;
   BOOL    done = FALSE;
   USHORT  version = 0;

   version = GetBootVersion(pBootSector);

   if (version < 40)                    /* Boot sector < DOS 4.0 */
      pBootSector->bpb.HiddenSectors &= 0xffffff7f;

   /* Copy over BPB */

   pVolCB->MediaBPB = pBootSector->bpb;


   /* If version < DOS 3.3 or its a diskette and using TotalSectors */
   /* then zero out the extended portion of the BPB                 */

   if ( (version < 33) ||
      ((pVolCB->pUnitCB->UnitInfo.UnitFlags & UF_REMOVABLE) &&
       (pBootSector->bpb.TotalSectors != 0))  )
   {
      pVolCB->MediaBPB.HiddenSectors &= 0x0000FFFF;
      pVolCB->MediaBPB.BigTotalSectors = 0;
      for (i = 0; i < sizeof(pVolCB->MediaBPB.Reserved_1); i++)
         pVolCB->MediaBPB.Reserved_1[i] = 0;
   }
}

/*--------------------------------------------------------------------------
;
;** Is_BPB_Boot - Is this a valid boot sector ?
;
;   ScratchBuffer points to the boot sector.  In theory, the BPB is
;   correct.  We can, therefore, get all the relevant information
;   from the media (those that we cannot get from the partition
;   table) if we can recognize it.
;
;   USHORT Is_BPB_Boot (PDOSBOOTREC pBootSector)
;
;   ENTRY:    pBootSector      - DOS Boot Sector
;
;   RETURN:   USHORT           - Result Code (NO_ERROR if valid boot sector)
;
;   EFFECTS:
;
;   NOTES:
;
--------------------------------------------------------------------------*/
USHORT Is_BPB_Boot (pBootSector)

PDOSBOOTREC pBootSector;
{
   USHORT rc;

   /* 1. Make sure short or near jmp is at start of boot sector */
   /* 2.     and high nibble of MediaType in BPB is 0xF         */
   /* 3.         and SectorsPerCluster in BPB is a power of 2   */

   if ((pBootSector->JmpCode == 0xE9 ||
      (pBootSector->JmpCode == 0xEB && pBootSector->nop == 0x90)) &&
      ((pBootSector->bpb.MediaType & 0xF0) == 0xF0) &&
      (PowerOf2(pBootSector->bpb.SectorsPerCluster)))

            rc = NO_ERROR;
    else
            rc = ERROR;


    return(rc);
}

/*--------------------------------------------------------------------------
;
;** GetBootVersion - Get boot version from boot record
;
;   Get the boot version of the DOS boot sector which appears as a
;   set of characters in the form 'XX.XX' after the 'DOS' or 'IBM'
;   characters.
;
;   USHORT GetBootVersion (PDOSBOOTREC pBootSector)
;
;   ENTRY:    pBootSector      - DOS Boot Sector
;
;   RETURN:   USHORT           - Boot Version
;
;   EFFECTS:
;
;   NOTES:
;
--------------------------------------------------------------------------*/
USHORT GetBootVersion (pBootSector)

PDOSBOOTREC pBootSector;
{
   USHORT i;
   USHORT version = 0;

   for (i = 0; i < sizeof(pBootSector->Release); i++)
   {
      if (pBootSector->Release[i] >= '0' && pBootSector->Release[i] <= '9')
      {
         version = version * 10;
         version = version + (pBootSector->Release[i] - '0');
         if (pBootSector->Release[i+1] == '.')
         {
            version = version * 10;
            version = version + (pBootSector->Release[i+2] - '0');
            break;
         }
      }
   }
   return(version);
}




