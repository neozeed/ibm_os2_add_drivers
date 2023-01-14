/*static char *SCCSID = "@(#)s506sub.c	6.1 92/01/08";*/

 #define INCL_NOBASEAPI
 #define INCL_NOPMAPI
 #define INCL_NO_SCB
 #define INCL_INITRP_ONLY
 #include "os2.h"                  // C:\DRV6\H
 #include "dos.h"                  // C:\DRV6\H
 #include "infoseg.h"              // C:\DRV6\H

 #include "iorb.h"                 // C:\DRV6\SRC\DEV\DASD\DISKH
 #include "reqpkt.h"               // C:\DRV6\SRC\DEV\DASD\DISKH
 #include "dhcalls.h"

 #include "s506hdr.h"
 #include "s506ext.h"
 #include "s506pro.h"

 #include "addcalls.h"

/*****************************************************************************
 *                                                                           *
 *   Setup - Set request parameters into local structure.                    *
 *                                                                           *
 *       Setup sets the Unit, First, Addr, Count and Flags fields in the     *
 *       device structure which are use to drive the I/O.  The following     *
 *       flags are affected:                                                 *
 *               FWrite  This is a write request, not a read                 *
 *               FVerify This is a write with verify (verify when write is   *
 *                       cleared)                                            *
 *       Other fields are copied from the DOS request packet                 *
 *                                                                           *
 *       ENTRY   DS:SI   Pointer to device variables IOStruc                 *
 *               ES:BX   Current request                                     *
 *                                                                           *
 *       EXIT    The following variables are set                             *
 *               [SI].Unit                                                   *
 *               [SI].First              The hidden sectors are added.       *
 *               [SI].RealAddr                                               *
 *               [SI].CurrAddr                                               *
 *               [SI].Count                                                  *
 *               [SI].Flags                                                  *
 *               [SI].Flags2            Flags for HPFS386 and Fault Tolerance*
 *               [SI].StartSec                                               *
 *                                                                           *
 *               DI = returns BDS address                                    *
 *                                                                           *
 *       USES AX, FLAGS                                                      *
 *                                                                           *
 *****************************************************************************/
VOID NEAR Setup(NPACB npACB)
{
  PIORB_EXECUTEIO pIORB = (PIORB_EXECUTEIO)npACB->pHeadIORB;
  USHORT        LastCurrent;
  USHORT        i;

  npACB->Flags &= FACTIVE + (~FMULTIIO);        // Set initial state of Flags
  LastCurrent = npACB->Current;         // Save last request (used only on
                                        // format commands)

  npACB->Active = 1;                    // Set active !FIX!
  npACB->Current = pIORB->iorbh.UnitHandle;   // Set drive needed
  i = pIORB->iorbh.UnitHandle;

  npACB->PhysDrv = npACB->unit[i].DriveNum;  // Save physical drive number
  npACB->drivebit = npACB->unit[i].ContUnit; // get the controller unit number
                                             // and setup the current disk bit

// Note: need to check on the above code dealing with drive and unit numbers

  npACB->Count = (ULONG)pIORB->BlockCount;      // Set number of sectors to do.
  npACB->SecToGo = pIORB->BlockCount;       // Number of sectors left in request

  if ( npACB->unit[i].DriveFlags & FMULTIPLEMODE )
    {

//  Compute Number of blocks left in request for read/write multiple commands

    npACB->BlocksToGo = pIORB->BlockCount / npACB->unit[i].id.NumSectorsPerInt;

    npACB->LastBlockNumSec = pIORB->BlockCount % npACB->unit[i].id.NumSectorsPerInt;
    if ( !(npACB->LastBlockNumSec) )
      npACB->LastBlockNumSec = npACB->unit[i].id.NumSectorsPerInt;

    if ( npACB->LastBlockNumSec )
      ++npACB->BlocksToGo;                // account for partial block
    }

/*----------------------------------------------------------------------------*
 * Now set up starting sector, transfer address and sector count in IOStruc.  *
 * The state machine runs off this structure.                                 *
 *----------------------------------------------------------------------------*/
  npACB->SGCurrentDesc = 1;             // initialize the count of sg's.

  if ( pIORB->iorbh.CommandModifier != IOCM_READ_VERIFY )
    {
    npACB->SGBytesToGo = pIORB->pSGList->XferBufLen; // save number of bytes in this sg
    npACB->CurrAddr = pIORB->pSGList->ppXferBuf; // save data buffer address from the sg
    }

  npACB->SectorSize = 2;        // set sector size to two 256 byte blocks.

  npACB->First = pIORB->RBA;    // save the RBA Starting Address in the acb.
// NOTE - disk01.sys stored the starting sector in the ios. Check on the
//          useage of ios.first

  npACB->StartSec = 0;          // set sector offset to 0. Used for non-standard
                                //   IOCTL track layouts

// Determine if a read or a write command in order to set the acb FLAGS field.

  if ( pIORB->iorbh.CommandModifier == IOCM_READ )
    npACB->Flags |= FREAD;
  else if ( pIORB->iorbh.CommandModifier == IOCM_READ_VERIFY )
    npACB->Flags |= FVERIFY;
  else if ( pIORB->iorbh.CommandModifier == IOCM_WRITE )
    npACB->Flags |= FWRITE;
  else if ( pIORB->iorbh.CommandModifier == IOCM_WRITE_VERIFY )
    npACB->Flags |= FWRITE + FVERIFY;
}


/*****************************************************************************
 *                                                                           *
 * Subroutinee: PB_FxExRead                                                  *
 *                                                                           *
 * Descriptiion: called while in state FXEXREAD to read one physical         *
 *               record of data into the data buffers pointed to by          *
 *               the scatter/gather descriptors (SG's)                       *
 *                                                                           *
 * Notes: entered at interrupt time.                                         *
 *                                                                           *
 * Linkage:  Near call from FxExRead                                         *
 *                                                                           *
 * Input:    si = address of IOS                                             *
 *           dx = i/o port address                                           *
 *                                                                           *
 *           CurrAddr (in the IOS)is assumed to contain the physical         *
 *           address of the next available byte in the i/o buffer            *
 *                                                                           *
 *           SGBytesToGo (in the IOS) is assumed to contain the              *
 *           number of bytes remaining to be filled in the buffer            *
 *                                                                           *
 *                                                                           *
 * output:   updated values in CurrAddr and SGBytesToGo                      *
 *                                                                           *
 *           eax, bx, ecx, di, es are all destroyed                          *
 *                                                                           *
 * Exit-Normal: Near return                                                  *
 *                                                                           *
 *****************************************************************************/
VOID NEAR PB_FxExRead(NPACB npACB)
{
  ULONG PhysRecBytesToGo;
  ULONG NumBytesToRead;
  UCHAR DataHigh;
  UCHAR DataLow;
  PUCHAR pData;
  UCHAR i = npACB->pHeadIORB->UnitHandle;

// set up the adapter's physical record size.

  if ( npACB->unit[i].DriveFlags & FMULTIPLEMODE )
    {
    if ( (npACB->BlocksToGo-1) )    // BlocksToGo gets updated in State Machine
      PhysRecBytesToGo = (ULONG) (npACB->unit[i].id.NumSectorsPerInt * 512);
    else
      PhysRecBytesToGo = (ULONG) (npACB->LastBlockNumSec * 512);
    }
  else // interrupt for every sector
    {
    PhysRecBytesToGo = 512;
    }

  do
    {

    NumBytesToRead = npACB->SGBytesToGo;  // get number of bytes left in current buffer.

//  Is the space left in the current buffer larger than the bytes remaining
//  in the current physical record?

    if (NumBytesToRead > PhysRecBytesToGo)
      {
      // reduce the number of bytes to read to the number still un-read
      // from the current physical record.

      NumBytesToRead = PhysRecBytesToGo;
      }

    if (NumBytesToRead == 1)    // is this the one byte left special case?
      {
      // process the case where there is only one byte left in the
      // current data buffer

      DevHelp_PhysToGDTSelector( npACB->CurrAddr, 1, gdt_selector_IO );
      pData = (PUCHAR) MAKEP(gdt_selector_IO, 0);

      _asm
        {
        push    ax
        push    dx
        mov     dx,FX_PDAT
        cld
        DISABLE
        in      ax,dx                   ; get a word from the adaptor.
        mov     DataHigh, ah
        mov     DataLow,  al
        pop     dx
        pop     ax
        }

      *(pData) = DataLow;   // save the first byte gotten in the last byte of
                            // the data buffer.

      PB_Get_SG(npACB);     // get the address and length from the
                            // next SG's data buffer.

      DevHelp_PhysToGDTSelector( npACB->CurrAddr, 1, gdt_selector_IO );
      pData = (PUCHAR) MAKEP(gdt_selector_IO, 0);

      *(pData) = DataHigh;          // store the second byte in the new buffer.

      ++npACB->CurrAddr;                // move down to the next available
                                        // byte in the data buffer.

      --npACB->SGBytesToGo;             // dec the available size of the data
                                        // buffer by one byte.

      if ( !(npACB->SGBytesToGo) )      // was this just a one byte buffer?
        PB_Get_SG(npACB);               // yes, get the address and length from
                                        // the next SG's data buffer.

      PhysRecBytesToGo -= 2;            // reduce count of bytes left in the
                                        // physical record by two.
      }
    else
      {
      // process the case where there are at lease two bytes left in the
      // current data buffer

      NumBytesToRead &= (ULONG) ~1;     // truncate the count of remaining byte
                                        // to an even number.

      PhysRecBytesToGo -= NumBytesToRead; // compute bytes remaining in this
                                          // physical record.

      DevHelp_PhysToGDTSelector( npACB->CurrAddr, (USHORT) NumBytesToRead,
                                 gdt_selector_IO );

// Note: The following assembler code does NOT account for NumBytesToRead
//       being a double word. The compiler does not recognize ecx or
//       allow db 66h. The code will work as long as NumBytesToRead does
//       not exceed a word value. The instruction mov cx,NumBytesToRead
//       loads the low word. This should be okay since a physical record
//       is currently 512 bytes.

      _asm
        {
        push    ax
        push    cx
        push    dx
        push    di
        push    es

        mov     cx,NumBytesToRead       // pick up the count of bytes to be read.
        shr     cx,1                    // compute the number of words to read.
        mov     dx,FX_PDAT
        xor     di, di                       // get pointer to data location
        mov     es, word ptr gdt_selector_IO // ES:DI is 32-bit address of data
        cld                             // ensure the direction flag is "up".
        DISABLE                         // ensure that interrupts are inhibited.
        rep     ins word ptr es:[di],dx // read the data in.

        pop     es
        pop     di
        pop     dx
        pop     cx
        pop     ax
        }

      npACB->CurrAddr += NumBytesToRead;  // compute next buffer address to use.
      npACB->SGBytesToGo -= NumBytesToRead; // compute bytes remaining in this SG.

      if ( !(npACB->SGBytesToGo) )      // is this SG all done?
        PB_Get_SG(npACB);               // yes, get the address and length from
                                        // the next SG's data buffer.
      }
    }
  while ( PhysRecBytesToGo );

// return to our caller with interrupts disabled.

}


/******************************************************************************
 *                                                                            *
 * Subroutine Name: PB_FDWrite                                                *
 *                                                                            *
 * Descriptine Name: Does HPFS386 processing in FXEXWRITE state               *
 *                       of fixed disk state machine. This allows             *
 *                       the processing of scatter/gather                     *
 *                       descriptors.                                         *
 *                                                                            *
 * Notes: entered at interrupt time.                                          *
 *                                                                            *
 * Linkage:  Near call from FxExWrite                                         *
 *                                                                            *
 * Input:                                                                     *
 *           ES    - FSD_DataSeg                                              *
 *           DS    - BioData                                                  *
 *           SI    - pointer to IOStruc                                       *
 *           BX    - Request Header                                           *
 *                                                                            *
 * Exit-Normal: Near return                                                   *
 *                                                                            *
 ******************************************************************************/
VOID NEAR PB_FDWrite(NPACB npACB)
{
  ULONG PhysRecBytesToGo;
  ULONG NumBytesToWrite;
  UCHAR DataHigh;
  UCHAR DataLow;
  PUCHAR pData;
  UCHAR i = npACB->pHeadIORB->UnitHandle;


  npACB->SGStartDesc = npACB->SGCurrentDesc;
  npACB->SGStartByte = npACB->SGBytesToGo;
  npACB->RealAddr = npACB->CurrAddr;

// set up the adapter's physical record size.

  if ( npACB->unit[i].DriveFlags & FMULTIPLEMODE )
    {
    if ( (npACB->BlocksToGo-1) )    // BlocksToGo gets updated in State Machine
      PhysRecBytesToGo = (ULONG) (npACB->unit[i].id.NumSectorsPerInt * 512);
    else
      PhysRecBytesToGo = (ULONG) (npACB->LastBlockNumSec * 512);
    }
  else // interrupt for every sector
    {
    PhysRecBytesToGo = 512;
    }

  do
    {

    NumBytesToWrite = npACB->SGBytesToGo;  // get number of bytes left in current buffer.

//  Is the space left in the current buffer larger than the bytes remaining
//  in the current physical record?

    if (NumBytesToWrite > PhysRecBytesToGo)
      {
      // reduce the number of bytes to write to the number still un-written
      // from the current scatter/gather descriptor.

      NumBytesToWrite = PhysRecBytesToGo;
      }

    if (NumBytesToWrite == 1)    // is this the one byte left special case?
      {
      // process the case where there is only one byte left in the
      // current data buffer

      DevHelp_PhysToGDTSelector( npACB->CurrAddr, 1, gdt_selector_IO );
      pData = (PUCHAR) MAKEP(gdt_selector_IO, 0);

      DataLow = *(pData);   // pick up the odd byte and save it

      PB_Get_SG(npACB);     // get the address and length from the
                            // next SG's data buffer.

      DevHelp_PhysToGDTSelector( npACB->CurrAddr, 1, gdt_selector_IO );
      pData = (PUCHAR) MAKEP(gdt_selector_IO, 0);

      DataHigh = *(pData);  // get the second byte to write out from
                            // the first byte of the new buffer.

      _asm
        {
        push    ax
        push    dx
        mov     ah,DataHigh
        mov     al,DataLow
        mov     dx,FX_PDAT
        DISABLE
        out     dx,ax                   ; send the word to the adaptor.
        pop     dx
        pop     ax
        }

      ++npACB->CurrAddr;                // move down to the next available
                                        // byte in the data buffer.

      --npACB->SGBytesToGo;             // dec the available size of the data
                                        // buffer by one byte.

      if ( !(npACB->SGBytesToGo) )      // was this just a one byte buffer?
        PB_Get_SG(npACB);               // yes, get the address and length from
                                        // the next SG's data buffer.

      PhysRecBytesToGo -= 2;            // reduce count of bytes left in the
                                        // physical record by two.
      }
    else
      {
      // process the case where there are at lease two bytes left in the
      // current data buffer

      NumBytesToWrite &= (ULONG) ~1;    // truncate the count of remaining byte
                                        // to an even number.

      PhysRecBytesToGo -= NumBytesToWrite; // compute bytes remaining in this
                                          // physical record.

      DevHelp_PhysToGDTSelector( npACB->CurrAddr, (USHORT) NumBytesToWrite,
                                 gdt_selector_IO );

// Note: The following assembler code does NOT account for NumBytesToWrite
//       being a double word. The compiler does not recognize ecx or
//       allow db 66h. The code will work as long as NumBytesToWrite does
//       not exceed a word value. The instruction mov cx,NumBytesToWrite
//       loads the low word. This should be okay since a physical record
//       is currently 512 bytes.

      _asm
        {
        push    ax
        push    cx
        push    dx
        push    si
        push    ds

        mov     cx,NumBytesToWrite       // pick up the count of bytes to be written.
        shr     cx,1                    // compute the number of words to write.
        mov     dx,FX_PDAT
        xor     si, si                       // get pointer to data location
        mov     ds, word ptr gdt_selector_IO // DS:SI is 32-bit address of data
        cld                             // ensure the direction flag is "up".
        DISABLE                         // ensure that interrupts are inhibited.
        rep     outs dx, word ptr ds:[si] ; write the data out.

        pop     ds
        pop     si
        pop     dx
        pop     cx
        pop     ax
        }

      npACB->CurrAddr += NumBytesToWrite;  // compute next buffer address to use.
      npACB->SGBytesToGo -= NumBytesToWrite; // compute bytes remaining in this SG.

      if ( !(npACB->SGBytesToGo) )      // is this SG all done?
        PB_Get_SG(npACB);               // yes, get the address and length from
                                        // the next SG's data buffer.
      }
    }
  while ( PhysRecBytesToGo );

// return to our caller with interrupts enabled.

  ENABLE
}


/*****************************************************************************
 *                                                                           *
 * Subroutine Name: PB_Get_SG                                                *
 *                                                                           *
 * Descriptine Name: Sets up IOStruc for current scatter/gather              *
 *                                                                           *
 * Notes: entered at interrupt time and kernal time                          *
 *                                                                           *
 * Input:    si = address of IOS                                             *
 *                                                                           *
 * output:                                                                   *
 *                                                                           *
 *                                                                           *
 *                                                                           *
 * Exit-Normal: Near return                                                  *
 *                                                                           *
 *                                                                           *
 *****************************************************************************/
VOID NEAR PB_Get_SG(NPACB npACB)
{
  PIORB_EXECUTEIO pIORB = (PIORB_EXECUTEIO)npACB->pHeadIORB;
  USHORT i = npACB->SGCurrentDesc;
  PSCATGATENTRY pSG = (PSCATGATENTRY) pIORB->pSGList+i;

  if ( i < pIORB->cSGList )               // have all SG's been processed?
    {
    npACB->SGBytesToGo = pSG->XferBufLen; // save number of bytes in this sg
    npACB->CurrAddr = pSG->ppXferBuf;     // save data buffer address from the sg
    ++npACB->SGCurrentDesc;               // bump the count of SG's processed.
    }
}


/*****************************************************************************
 *                                                                           *
 *   TraceIt                                                                 *
 *                                                                           *
 *****************************************************************************/
 VOID NEAR TraceIt(NPACB npACB, USHORT Trace_MinorCode)
 {
  PBYTE pTraceBuffer;

  DISABLE

  if ( pGlobalInfoSeg->SIS_mec_table[(TRACE_MAJOR>>3)] &
       (0x0080>>(TRACE_MAJOR & 0x0007)) )
    {
    npACB->TraceBuffer[0] = (USHORT) npACB->NestedInts;
    pTraceBuffer = (PBYTE) &(npACB->TraceBuffer);

    DevHelp_RAS ( (USHORT)TRACE_MAJOR, (USHORT) Trace_MinorCode,
                  (USHORT) 16, pTraceBuffer );
    }

  ENABLE

}
