/* $NoKeywords:$ */
/**
 * @file
 *
 * AMD Library
 *
 * Contains interface to the AMD AGESA library
 *
 * @xrefitem bom "File Content Label" "Release Content"
 * @e project:      AGESA
 * @e sub-project:  Lib
 * @e \$Revision: 48409 $   @e \$Date: 2011-03-08 11:19:40 -0600 (Tue, 08 Mar 2011) $
 *
 */
/*
 ******************************************************************************
 *
 * Copyright (c) 2008 - 2011, Advanced Micro Devices, Inc.
 *               2013 - 2014, Sage Electronic Engineering, LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Advanced Micro Devices, Inc. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ADVANCED MICRO DEVICES, INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************
 */

#include <AGESA.h>
#include <cpuRegisters.h>
#include <Filecode.h>
#include <Ids.h>
#include <Porting.h>
#include "amdlib.h"
CODE_GROUP (G1_PEICC)
RDATA_GROUP (G1_PEICC)

#if !defined(AMDLIB_OPTIMIZE)
	#define AMDLIB_OPTIMIZE
#endif

#define FILECODE LIB_AMDLIB_FILECODE

BOOLEAN
STATIC
GetPciMmioAddress (
     OUT   UINT64            *MmioAddress,
     OUT   UINT32            *MmioSize,
  IN       AMD_CONFIG_PARAMS *StdHeader
  );

VOID
STATIC
LibAmdGetDataFromPtr (
  IN       ACCESS_WIDTH AccessWidth,
  IN       CONST VOID   *Data,
  IN       CONST VOID   *DataMask,
     OUT   UINT32       *TemData,
     OUT   UINT32       *TempDataMask
  );
VOID
IdsOutPort (
  IN       UINT32 Addr,
  IN       UINT32 Value,
  IN       UINT32 Flag
  );

VOID
CpuidRead (
  IN        UINT32      CpuidFcnAddress,
  OUT       CPUID_DATA  *Value
  );

UINT8
ReadNumberOfCpuCores(
  void
  );

AMDLIB_OPTIMIZE
UINT8
ReadIo8 (
  IN       UINT16 Address
  )
{
  return __inbyte (Address);
}

AMDLIB_OPTIMIZE
UINT16
ReadIo16 (
  IN       UINT16 Address
  )
{
  return __inword (Address);
}

AMDLIB_OPTIMIZE
UINT32
ReadIo32 (
  IN       UINT16 Address
  )
{
  return __indword (Address);
}

AMDLIB_OPTIMIZE
VOID
WriteIo8 (
  IN       UINT16 Address,
  IN       UINT8 Data
  )
{
  __outbyte (Address, Data);
}

AMDLIB_OPTIMIZE
VOID
WriteIo16 (
  IN       UINT16 Address,
  IN       UINT16 Data
  )
{
  __outword (Address, Data);
}

AMDLIB_OPTIMIZE
VOID
WriteIo32 (
  IN       UINT16 Address,
  IN       UINT32 Data
  )
{
   __outdword (Address, Data);
}

AMDLIB_OPTIMIZE
STATIC
UINT64 SetFsBase (
  UINT64 address
  )
{
  UINT64 hwcr;
  hwcr = __readmsr (0xC0010015);
  __writemsr (0xC0010015, hwcr | 1 << 17);
  __writemsr (0xC0000100, address);
  return hwcr;
}

AMDLIB_OPTIMIZE
STATIC
VOID
RestoreHwcr (
  UINT64
  value
  )
{
  __writemsr (0xC0010015, value);
}

AMDLIB_OPTIMIZE
UINT8
Read64Mem8 (
  IN       UINT64 Address
  )
{
  UINT8 dataRead;
  UINT64 hwcrSave;
  if ((Address >> 32) == 0) {
    return *(volatile UINT8 *) (UINTN) Address;
  }
  hwcrSave = SetFsBase (Address);
  dataRead = __readfsbyte (0);
  RestoreHwcr (hwcrSave);
  return dataRead;
}

AMDLIB_OPTIMIZE
UINT16
Read64Mem16 (
  IN       UINT64 Address
  )
{
  UINT16 dataRead;
  UINT64 hwcrSave;
  if ((Address >> 32) == 0) {
    return *(volatile UINT16 *) (UINTN) Address;
  }
  hwcrSave = SetFsBase (Address);
  dataRead = __readfsword (0);
  RestoreHwcr (hwcrSave);
  return dataRead;
}

AMDLIB_OPTIMIZE
UINT32
Read64Mem32 (
  IN       UINT64 Address
  )
{
  UINT32 dataRead;
  UINT64 hwcrSave;
  if ((Address >> 32) == 0) {
    return *(volatile UINT32 *) (UINTN) Address;
  }
  hwcrSave = SetFsBase (Address);
  dataRead = __readfsdword (0);
  RestoreHwcr (hwcrSave);
  return dataRead;
  }

AMDLIB_OPTIMIZE
VOID
Write64Mem8 (
  IN       UINT64 Address,
  IN       UINT8 Data
  )
{
  if ((Address >> 32) == 0){
    *(volatile UINT8 *) (UINTN) Address = Data;
  }
  else {
    UINT64 hwcrSave;
    hwcrSave = SetFsBase (Address);
    __writefsbyte (0, Data);
    RestoreHwcr (hwcrSave);
  }
}

AMDLIB_OPTIMIZE
VOID
Write64Mem16 (
  IN       UINT64 Address,
  IN       UINT16 Data
  )
{
 if ((Address >> 32) == 0){
   *(volatile UINT16 *) (UINTN) Address = Data;
 }
 else {
   UINT64 hwcrSave;
   hwcrSave = SetFsBase (Address);
   __writefsword (0, Data);
   RestoreHwcr (hwcrSave);
 }
}

AMDLIB_OPTIMIZE
VOID
Write64Mem32 (
  IN       UINT64 Address,
  IN       UINT32 Data
  )
{
  if ((Address >> 32) == 0){
    *(volatile UINT32 *) (UINTN) Address = Data;
  }
  else {
    UINT64 hwcrSave;
    hwcrSave = SetFsBase (Address);
    __writefsdword (0, Data);
    RestoreHwcr (hwcrSave);
  }
}

AMDLIB_OPTIMIZE
VOID
LibAmdReadCpuReg (
  IN       UINT8 RegNum,
     OUT   UINT32 *Value
  )
{
    *Value = 0;
    switch (RegNum){
    case CR4_REG:
      *Value = __readcr4 ();
      break;
    case DR0_REG:
      *Value = __readdr (0);
      break;
    case DR1_REG:
      *Value = __readdr (1);
      break;
    case DR2_REG:
      *Value = __readdr (2);
      break;
    case DR3_REG:
      *Value = __readdr (3);
      break;
    case DR7_REG:
      *Value = __readdr (7);
      break;
    default:
      *Value = -1;
      break;
  }
}

AMDLIB_OPTIMIZE
VOID
LibAmdWriteCpuReg (
  IN       UINT8 RegNum,
  IN       UINT32 Value
  )
{
   switch (RegNum){
    case CR4_REG:
      __writecr4 (Value);
      break;
    case DR0_REG:
      __writedr (0, Value);
      break;
    case DR1_REG:
      __writedr (1, Value);
      break;
    case DR2_REG:
      __writedr (2, Value);
      break;
    case DR3_REG:
      __writedr (3, Value);
      break;
    case DR7_REG:
      __writedr (7, Value);
      break;
    default:
      break;
  }
}

AMDLIB_OPTIMIZE
VOID
LibAmdWriteBackInvalidateCache (
  void
  )
{
  __wbinvd ();
}

AMDLIB_OPTIMIZE
VOID
LibAmdHDTBreakPoint (
  void
  )
{
  __writemsr (0xC001100A, __readmsr (0xC001100A) | 1);
  __debugbreak (); // do you really need icebp? If so, go back to asm code
}

AMDLIB_OPTIMIZE
UINT8
LibAmdBitScanForward (
  IN       UINT32 value
  )
{
  UINTN Index;
  for (Index = 0; Index < 32; Index++){
    if (value & (1 << Index)) break;
  }
  return (UINT8) Index;
}

AMDLIB_OPTIMIZE
UINT8
LibAmdBitScanReverse (
  IN       UINT32 value
)
{
  uint8_t bit = 31;
  do {
    if (value & (1 << 31))
      return bit;

    value <<= 1;
    bit--;

  } while (value != 0);

  return 0xFF; /* Error code indicating no bit found */
}

AMDLIB_OPTIMIZE
VOID
LibAmdMsrRead (
  IN       UINT32 MsrAddress,
     OUT   UINT64 *Value,
  IN OUT   AMD_CONFIG_PARAMS *ConfigPtr
  )
{
  if ((MsrAddress == 0xFFFFFFFF) || (MsrAddress == 0x00000000)) {
	  IdsErrorStop(MsrAddress);
  }
  *Value = __readmsr (MsrAddress);
}

AMDLIB_OPTIMIZE
VOID
LibAmdMsrWrite (
  IN       UINT32 MsrAddress,
  IN       UINT64 *Value,
  IN OUT   AMD_CONFIG_PARAMS *ConfigPtr
  )
{
  __writemsr (MsrAddress, *Value);
}

AMDLIB_OPTIMIZE
void LibAmdCpuidRead (
  IN       UINT32 CpuidFcnAddress,
     OUT   CPUID_DATA* Value,
  IN OUT   AMD_CONFIG_PARAMS *ConfigPtr
  )
{
  __cpuid ((int *)Value, CpuidFcnAddress);
}

AMDLIB_OPTIMIZE
UINT64
ReadTSC (
  void
  )
{
  return __rdtsc ();
}

AMDLIB_OPTIMIZE
VOID
LibAmdSimNowEnterDebugger (
  void
  )
{
  STATIC CONST UINT8 opcode [] = {0x60,                             // pushad
                                   0xBB, 0x02, 0x00, 0x00, 0x00,     // mov ebx, 2
                                   0xB8, 0x0B, 0xD0, 0xCC, 0xBA,     // mov eax, 0xBACCD00B
                                   0x0F, 0xA2,                       // cpuid
                                   0x61,                             // popad
                                   0xC3                              // ret
                                 };
  ((VOID (*)(VOID)) (size_t) opcode) (); // call the function
}

AMDLIB_OPTIMIZE
VOID
IdsOutPort (
  IN       UINT32 Addr,
  IN       UINT32 Value,
  IN       UINT32 Flag
  )
{
  __outdword ((UINT16) Addr, Value);
}

AMDLIB_OPTIMIZE
VOID
StopHere (
  void
  )
{
  VOLATILE UINTN x = 1;
  while (x);
}

AMDLIB_OPTIMIZE
VOID
LibAmdCLFlush (
  IN       UINT64 Address,
  IN       UINT8  Count
  )
{
  UINT64 hwcrSave;
  UINT8  *address32;
  UINTN  Index;
  address32 = 0;
  hwcrSave = SetFsBase (Address);
  for (Index = 0; Index < Count; Index++){
    _mm_mfence ();
    _mm_clflush_fs (&address32 [Index * 64]);
  }
  RestoreHwcr (hwcrSave);
}


AMDLIB_OPTIMIZE
VOID
LibAmdFinit(
  void
  )
{
	/* TODO: finit */
	__asm__ volatile ("finit");
}
/*---------------------------------------------------------------------------------------*/
/**
 * Read IO port
 *
 *
 * @param[in] AccessWidth   Access width
 * @param[in] IoAddress     IO port address
 * @param[in] Value         Pointer to save data
 * @param[in] StdHeader     Standard configuration header
 *
 */
VOID
LibAmdIoRead (
  IN       ACCESS_WIDTH AccessWidth,
  IN       UINT16 IoAddress,
     OUT   VOID *Value,
  IN OUT   AMD_CONFIG_PARAMS *StdHeader
  )
{
  switch (AccessWidth) {
  case AccessWidth8:
  case AccessS3SaveWidth8:
    *(UINT8 *) Value = ReadIo8 (IoAddress);
    break;
  case AccessWidth16:
  case AccessS3SaveWidth16:
    *(UINT16 *) Value = ReadIo16 (IoAddress);
    break;
  case AccessWidth32:
  case AccessS3SaveWidth32:
    *(UINT32 *) Value = ReadIo32 (IoAddress);
    break;
  default:
    ASSERT (FALSE);
    break;
  }
}

/*---------------------------------------------------------------------------------------*/
/**
 * Write IO port
 *
 *
 * @param[in] AccessWidth   Access width
 * @param[in] IoAddress     IO port address
 * @param[in] Value         Pointer to data
 * @param[in] StdHeader     Standard configuration header
 *
 */
VOID
LibAmdIoWrite (
  IN       ACCESS_WIDTH AccessWidth,
  IN       UINT16 IoAddress,
  IN       CONST VOID *Value,
  IN OUT   AMD_CONFIG_PARAMS *StdHeader
  )
{
  switch (AccessWidth) {
  case AccessWidth8:
  case AccessS3SaveWidth8:
    WriteIo8 (IoAddress, *(UINT8 *) Value);
    break;
  case AccessWidth16:
  case AccessS3SaveWidth16:
    WriteIo16 (IoAddress, *(UINT16 *) Value);
    break;
  case AccessWidth32:
  case AccessS3SaveWidth32:
    WriteIo32 (IoAddress, *(UINT32 *) Value);
    break;
  default:
    ASSERT (FALSE);
    break;
  }
}

/*---------------------------------------------------------------------------------------*/
/**
 * IO read modify write
 *
 *
 * @param[in] AccessWidth   Access width
 * @param[in] IoAddress     IO address
 * @param[in] Data          OR data
 * @param[in] DataMask      Mask to be used before data write back to register.
 * @param[in] StdHeader     Standard configuration header
 *
 */
VOID
LibAmdIoRMW (
  IN       ACCESS_WIDTH AccessWidth,
  IN       UINT16 IoAddress,
  IN       CONST VOID *Data,
  IN       CONST VOID *DataMask,
  IN OUT   AMD_CONFIG_PARAMS *StdHeader
  )
{
  UINT32  TempData;
  UINT32  TempMask;
  UINT32  Value;
  LibAmdGetDataFromPtr (AccessWidth, Data,  DataMask, &TempData, &TempMask);
  LibAmdIoRead (AccessWidth, IoAddress, &Value, StdHeader);
  Value = (Value & (~TempMask)) | TempData;
  LibAmdIoWrite (AccessWidth, IoAddress, &Value, StdHeader);
}

/*---------------------------------------------------------------------------------------*/
/**
 * Poll IO register
 *
 *  Poll register until (RegisterValue & DataMask) ==  Data
 *
 * @param[in] AccessWidth   Access width
 * @param[in] IoAddress     IO address
 * @param[in] Data          Data to compare
 * @param[in] DataMask      And mask
 * @param[in] Delay         Poll for time in 100ns (not supported)
 * @param[in] StdHeader     Standard configuration header
 *
 */
VOID
LibAmdIoPoll (
  IN       ACCESS_WIDTH AccessWidth,
  IN       UINT16 IoAddress,
  IN       CONST VOID *Data,
  IN       CONST VOID *DataMask,
  IN       UINT64 Delay,
  IN OUT   AMD_CONFIG_PARAMS *StdHeader
  )
{
  UINT32  TempData;
  UINT32  TempMask;
  UINT32  Value;
  LibAmdGetDataFromPtr (AccessWidth, Data,  DataMask, &TempData, &TempMask);
  do {
    LibAmdIoRead (AccessWidth, IoAddress, &Value, StdHeader);
  } while (TempData != (Value & TempMask));
}

/*---------------------------------------------------------------------------------------*/
/**
 * Read memory/MMIO
 *
 *
 * @param[in] AccessWidth   Access width
 * @param[in] MemAddress    Memory address
 * @param[in] Value         Pointer to data
 * @param[in] StdHeader     Standard configuration header
 *
 */
VOID
LibAmdMemRead (
  IN       ACCESS_WIDTH AccessWidth,
  IN       UINT64 MemAddress,
     OUT   VOID *Value,
  IN OUT   AMD_CONFIG_PARAMS *StdHeader
  )
{
  switch (AccessWidth) {
  case AccessWidth8:
  case AccessS3SaveWidth8:
    *(UINT8 *) Value = Read64Mem8 (MemAddress);
    break;
  case AccessWidth16:
  case AccessS3SaveWidth16:
    *(UINT16 *) Value = Read64Mem16 (MemAddress);
    break;
  case AccessWidth32:
  case AccessS3SaveWidth32:
    *(UINT32 *) Value = Read64Mem32 (MemAddress);
    break;
  default:
    ASSERT (FALSE);
    break;
  }
}

/*---------------------------------------------------------------------------------------*/
/**
 * Write memory/MMIO
 *
 *
 * @param[in] AccessWidth   Access width
 * @param[in] MemAddress    Memory address
 * @param[in] Value         Pointer to data
 * @param[in] StdHeader     Standard configuration header
 *
 */
VOID
LibAmdMemWrite (
  IN       ACCESS_WIDTH AccessWidth,
  IN       UINT64 MemAddress,
  IN       CONST VOID *Value,
  IN OUT   AMD_CONFIG_PARAMS *StdHeader
  )
{

  switch (AccessWidth) {
  case AccessWidth8:
  case AccessS3SaveWidth8:
    Write64Mem8 (MemAddress, *((UINT8 *) Value));
    break;
  case AccessWidth16:
  case AccessS3SaveWidth16:
    Write64Mem16 (MemAddress, *((UINT16 *) Value));
    break;
  case AccessWidth32:
  case AccessS3SaveWidth32:
    Write64Mem32 (MemAddress, *((UINT32 *) Value));
    break;
  default:
    ASSERT (FALSE);
    break;
  }
}
/*---------------------------------------------------------------------------------------*/
/**
 * Memory/MMIO read modify write
 *
 *
 * @param[in] AccessWidth   Access width
 * @param[in] MemAddress    Memory address
 * @param[in] Data          OR data
 * @param[in] DataMask      Mask to be used before data write back to register.
 * @param[in] StdHeader     Standard configuration header
 *
 */
VOID
LibAmdMemRMW (
  IN       ACCESS_WIDTH AccessWidth,
  IN       UINT64 MemAddress,
  IN       CONST VOID *Data,
  IN       CONST VOID *DataMask,
  IN OUT   AMD_CONFIG_PARAMS *StdHeader
  )
{
  UINT32  TempData;
  UINT32  TempMask;
  UINT32  Value;
  LibAmdGetDataFromPtr (AccessWidth, Data,  DataMask, &TempData, &TempMask);
  LibAmdMemRead (AccessWidth, MemAddress, &Value, StdHeader);
  Value = (Value & (~TempMask)) | TempData;
  LibAmdMemWrite (AccessWidth, MemAddress, &Value, StdHeader);
}

/*---------------------------------------------------------------------------------------*/
/**
 * Poll Mmio
 *
 *  Poll register until (RegisterValue & DataMask) ==  Data
 *
 * @param[in] AccessWidth   Access width
 * @param[in] MemAddress    Memory address
 * @param[in] Data          Data to compare
 * @param[in] DataMask      AND mask
 * @param[in] Delay         Poll for time in 100ns (not supported)
 * @param[in] StdHeader     Standard configuration header
 *
 */
VOID
LibAmdMemPoll (
  IN       ACCESS_WIDTH AccessWidth,
  IN       UINT64 MemAddress,
  IN       CONST VOID *Data,
  IN       CONST VOID *DataMask,
  IN       UINT64 Delay,
  IN OUT   AMD_CONFIG_PARAMS *StdHeader
  )
{
  UINT32  TempData = 0;
  UINT32  TempMask = 0;
  UINT32  Value;
  LibAmdGetDataFromPtr (AccessWidth, Data,  DataMask, &TempData, &TempMask);
  do {
    LibAmdMemRead (AccessWidth, MemAddress, &Value, StdHeader);
  } while (TempData != (Value & TempMask));
}

/*---------------------------------------------------------------------------------------*/
/**
 * Read PCI config space
 *
 *
 * @param[in] AccessWidth   Access width
 * @param[in] PciAddress    Pci address
 * @param[in] Value         Pointer to data
 * @param[in] StdHeader     Standard configuration header
 *
 */
VOID
LibAmdPciRead (
  IN       ACCESS_WIDTH AccessWidth,
  IN       PCI_ADDR PciAddress,
     OUT   VOID *Value,
  IN OUT   AMD_CONFIG_PARAMS *StdHeader
  )
{
  UINT32 LegacyPciAccess;
  UINT32 MMIOSize;
  UINT64 RMWrite;
  UINT64 RMWritePrevious;
  UINT64 MMIOAddress;

  ASSERT (StdHeader != NULL);
  ASSERT (PciAddress.AddressValue != ILLEGAL_SBDFO);
  if (!GetPciMmioAddress (&MMIOAddress, &MMIOSize, StdHeader)) {
    // We need to convert our "portable" PCI address into a "real" PCI access
    LegacyPciAccess = ((1 << 31) + (PciAddress.Address.Register & 0xFC) + (PciAddress.Address.Function << 8) + (PciAddress.Address.Device << 11) + (PciAddress.Address.Bus << 16) + ((PciAddress.Address.Register & 0xF00) << (24 - 8)));
    if (PciAddress.Address.Register <= 0xFF) {
      LibAmdIoWrite (AccessWidth32, IOCF8, &LegacyPciAccess, StdHeader);
      LibAmdIoRead (AccessWidth, IOCFC + (UINT16) (PciAddress.Address.Register & 0x3), Value, StdHeader);
    } else {
      LibAmdMsrRead  (NB_CFG, &RMWritePrevious, StdHeader);
      RMWrite = RMWritePrevious | 0x0000400000000000;
      LibAmdMsrWrite (NB_CFG, &RMWrite, StdHeader);
      LibAmdIoWrite (AccessWidth32, IOCF8, &LegacyPciAccess, StdHeader);
      LibAmdIoRead (AccessWidth, IOCFC + (UINT16) (PciAddress.Address.Register & 0x3), Value, StdHeader);
      LibAmdMsrWrite (NB_CFG, &RMWritePrevious, StdHeader);
    }
    //IDS_HDT_CONSOLE (LIB_PCI_RD, "~PCI RD %08x = %08x\n", LegacyPciAccess, *(UINT32 *)Value);
  } else {
    // Setup the MMIO address
    ASSERT ((MMIOAddress + MMIOSize) > (MMIOAddress + (PciAddress.AddressValue & 0x0FFFFFFF)));
    MMIOAddress += (PciAddress.AddressValue & 0x0FFFFFFF);
    LibAmdMemRead (AccessWidth, MMIOAddress, Value, StdHeader);
    //IDS_HDT_CONSOLE (LIB_PCI_RD, "~MMIO RD %08x = %08x\n", (UINT32) MMIOAddress, *(UINT32 *)Value);
  }
}

/*---------------------------------------------------------------------------------------*/
/**
 * Write PCI config space
 *
 *
 * @param[in] AccessWidth   Access width
 * @param[in] PciAddress    Pci address
 * @param[in] Value         Pointer to data
 * @param[in] StdHeader     Standard configuration header
 *
 */
VOID
LibAmdPciWrite (
  IN       ACCESS_WIDTH AccessWidth,
  IN       PCI_ADDR PciAddress,
  IN       CONST VOID *Value,
  IN OUT   AMD_CONFIG_PARAMS *StdHeader
  )
{
  UINT32 LegacyPciAccess;
  UINT32 MMIOSize;
  UINT64 RMWrite;
  UINT64 RMWritePrevious;
  UINT64 MMIOAddress;

  ASSERT (StdHeader != NULL);
  ASSERT (PciAddress.AddressValue != ILLEGAL_SBDFO);
  if (!GetPciMmioAddress (&MMIOAddress, &MMIOSize, StdHeader)) {
    // We need to convert our "portable" PCI address into a "real" PCI access
    LegacyPciAccess = ((1 << 31) + (PciAddress.Address.Register & 0xFC) + (PciAddress.Address.Function << 8) + (PciAddress.Address.Device << 11) + (PciAddress.Address.Bus << 16) + ((PciAddress.Address.Register & 0xF00) << (24 - 8)));
    if (PciAddress.Address.Register <= 0xFF) {
      LibAmdIoWrite (AccessWidth32, IOCF8, &LegacyPciAccess, StdHeader);
      LibAmdIoWrite (AccessWidth, IOCFC + (UINT16) (PciAddress.Address.Register & 0x3), Value, StdHeader);
    } else {
      LibAmdMsrRead  (NB_CFG, &RMWritePrevious, StdHeader);
      RMWrite = RMWritePrevious | 0x0000400000000000;
      LibAmdMsrWrite (NB_CFG, &RMWrite, StdHeader);
      LibAmdIoWrite (AccessWidth32, IOCF8, &LegacyPciAccess, StdHeader);
      LibAmdIoWrite (AccessWidth, IOCFC + (UINT16) (PciAddress.Address.Register & 0x3), Value, StdHeader);
      LibAmdMsrWrite (NB_CFG, &RMWritePrevious, StdHeader);
    }
    //IDS_HDT_CONSOLE (LIB_PCI_WR, "~PCI WR %08x = %08x\n", LegacyPciAccess, *(UINT32 *)Value);
    //printk(BIOS_DEBUG, "~PCI WR %08x = %08x\n", LegacyPciAccess, *(UINT32 *)Value);
    //printk(BIOS_DEBUG, "LibAmdPciWrite\n");
  } else {
    // Setup the MMIO address
    ASSERT ((MMIOAddress + MMIOSize) > (MMIOAddress + (PciAddress.AddressValue & 0x0FFFFFFF)));
    MMIOAddress += (PciAddress.AddressValue & 0x0FFFFFFF);
    LibAmdMemWrite (AccessWidth, MMIOAddress, Value, StdHeader);
    //IDS_HDT_CONSOLE (LIB_PCI_WR, "~MMIO WR %08x = %08x\n", (UINT32) MMIOAddress, *(UINT32 *)Value);
    //printk(BIOS_DEBUG, "~MMIO WR %08x = %08x\n", (UINT32) MMIOAddress, *(UINT32 *)Value);
    //printk(BIOS_DEBUG, "LibAmdPciWrite mmio\n");
  }
}

/*---------------------------------------------------------------------------------------*/
/**
 * PCI read modify write
 *
 *
 * @param[in] AccessWidth   Access width
 * @param[in] PciAddress    Pci address
 * @param[in] Data          OR Data
 * @param[in] DataMask      Mask to be used before data write back to register.
 * @param[in] StdHeader     Standard configuration header
 *
 */
VOID
LibAmdPciRMW (
  IN       ACCESS_WIDTH AccessWidth,
  IN       PCI_ADDR PciAddress,
  IN       CONST VOID *Data,
  IN       CONST VOID *DataMask,
  IN OUT   AMD_CONFIG_PARAMS *StdHeader
  )
{
  UINT32  TempData = 0;
  UINT32  TempMask = 0;
  UINT32  Value;
  LibAmdGetDataFromPtr (AccessWidth, Data,  DataMask, &TempData, &TempMask);
  LibAmdPciRead (AccessWidth, PciAddress, &Value, StdHeader);
  Value = (Value & (~TempMask)) | TempData;
  LibAmdPciWrite (AccessWidth, PciAddress, &Value, StdHeader);
}

/*---------------------------------------------------------------------------------------*/
/**
 * Poll PCI config space register
 *
 *  Poll register until (RegisterValue & DataMask) ==  Data
 *
 * @param[in] AccessWidth   Access width
 * @param[in] PciAddress    Pci address
 * @param[in] Data          Data to compare
 * @param[in] DataMask      AND mask
 * @param[in] Delay         Poll for time in 100ns (not supported)
 * @param[in] StdHeader     Standard configuration header
 *
 */
VOID
LibAmdPciPoll (
  IN       ACCESS_WIDTH AccessWidth,
  IN       PCI_ADDR PciAddress,
  IN       CONST VOID *Data,
  IN       CONST VOID *DataMask,
  IN       UINT64 Delay,
  IN OUT   AMD_CONFIG_PARAMS *StdHeader
  )
{
  UINT32  TempData = 0;
  UINT32  TempMask = 0;
  UINT32  Value;
  LibAmdGetDataFromPtr (AccessWidth, Data,  DataMask, &TempData, &TempMask);
  do {
    LibAmdPciRead (AccessWidth, PciAddress, &Value, StdHeader);
  } while (TempData != (Value & TempMask));
}

/*---------------------------------------------------------------------------------------*/
/**
 * Get MMIO base address for PCI accesses
 *
 * @param[out] MmioAddress   PCI MMIO base address
 * @param[out] MmioSize      Size of region in bytes
 * @param[in]  StdHeader     Standard configuration header
 *
 * @retval    TRUE          MmioAddress/MmioSize are valid
 */
BOOLEAN
STATIC
GetPciMmioAddress (
     OUT   UINT64            *MmioAddress,
     OUT   UINT32            *MmioSize,
  IN       AMD_CONFIG_PARAMS *StdHeader
  )
{
  BOOLEAN MmioIsEnabled;
  UINT32  EncodedSize;
  UINT64  LocalMsrRegister;

  ASSERT (StdHeader != NULL);

  MmioIsEnabled = FALSE;
  LibAmdMsrRead (MSR_MMIO_Cfg_Base, &LocalMsrRegister, StdHeader);
  if ((LocalMsrRegister & BIT0) != 0) {
    *MmioAddress = LocalMsrRegister & 0xFFFFFFFFFFF00000;
    EncodedSize = (UINT32) ((LocalMsrRegister & 0x3C) >> 2);
    *MmioSize = ((1 << EncodedSize) * 0x100000);
    MmioIsEnabled = TRUE;
  }
  return MmioIsEnabled;
}

/*---------------------------------------------------------------------------------------*/
/**
 * Read field of PCI config register.
 *
 *
 *
 * @param[in]   Address       Pci address  (register must be DWORD aligned)
 * @param[in]   Highbit       High bit position of the field in DWORD
 * @param[in]   Lowbit        Low bit position of the field in DWORD
 * @param[out]  Value         Pointer to data
 * @param[in]   StdHeader     Standard configuration header
 */
VOID
LibAmdPciReadBits (
  IN       PCI_ADDR Address,
  IN       UINT8 Highbit,
  IN       UINT8 Lowbit,
     OUT   UINT32 *Value,
  IN       AMD_CONFIG_PARAMS *StdHeader
  )
{
  ASSERT (Highbit < 32 && Lowbit < 32 && Highbit >= Lowbit && (Address.AddressValue & 3) == 0);

  LibAmdPciRead (AccessWidth32, Address, Value, StdHeader);
  *Value >>= Lowbit;  // Shift

  // A 1 << 32 == 1 << 0 due to x86 SHL instruction, so skip if that is the case

  if ((Highbit - Lowbit) != 31) {
    *Value &= (((UINT32) 1 << (Highbit - Lowbit + 1)) - 1);
  }
}

/*---------------------------------------------------------------------------------------*/
/**
 * Write field of PCI config register.
 *
 *
 *
 * @param[in]   Address       Pci address  (register must be DWORD aligned)
 * @param[in]   Highbit       High bit position of the field in DWORD
 * @param[in]   Lowbit        Low bit position of the field in DWORD
 * @param[in]   Value         Pointer to data
 * @param[in]   StdHeader     Standard configuration header
 */
VOID
LibAmdPciWriteBits (
  IN       PCI_ADDR Address,
  IN       UINT8 Highbit,
  IN       UINT8 Lowbit,
  IN       CONST UINT32 *Value,
  IN       AMD_CONFIG_PARAMS *StdHeader
  )
{
  UINT32 Temp;
  UINT32 Mask;

  ASSERT (Highbit < 32 && Lowbit < 32 && Highbit >= Lowbit && (Address.AddressValue & 3) == 0);

  // A 1<<32 == 1<<0 due to x86 SHL instruction, so skip if that is the case

  if ((Highbit - Lowbit) != 31) {
    Mask = (((UINT32) 1 << (Highbit - Lowbit + 1)) - 1);
  } else {
    Mask = (UINT32) 0xFFFFFFFF;
  }

  LibAmdPciRead (AccessWidth32, Address, &Temp, StdHeader);
  Temp &= ~(Mask << Lowbit);
  Temp |= (*Value & Mask) << Lowbit;
  LibAmdPciWrite (AccessWidth32, Address, &Temp, StdHeader);
}

/*---------------------------------------------------------------------------------------*/
/**
 * Locate next capability pointer
 *
 *  Given a SBDFO this routine will find the next PCI capabilities list entry.
 *  if the end of the list is reached, or if a problem is detected, then ILLEGAL_SBDFO is
 *  returned.
 *  To start a new search from the head of the list, specify a SBDFO with an offset of zero.
 *
 * @param[in,out] Address       Pci address
 * @param[in]     StdHeader     Standard configuration header
 */

VOID
LibAmdPciFindNextCap (
  IN OUT   PCI_ADDR *Address,
  IN       AMD_CONFIG_PARAMS *StdHeader
  )
{
  PCI_ADDR Base;
  UINT32 Offset;
  UINT32 Temp;
  PCI_ADDR TempAddress;

  ASSERT (Address != NULL);
  ASSERT (*(UINT32 *) Address != ILLEGAL_SBDFO);

  Base.AddressValue = Address->AddressValue;
  Offset = Base.Address.Register;
  Base.Address.Register = 0;

  Address->AddressValue = (UINT32) ILLEGAL_SBDFO;

  // Verify that the SBDFO points to a valid PCI device SANITY CHECK
  LibAmdPciRead (AccessWidth32, Base, &Temp, StdHeader);
  if (Temp == 0xFFFFFFFF) {
    ASSERT (FALSE);
    return; // There is no device at this address
  }

  // Verify that the device supports a capability list
  TempAddress.AddressValue = Base.AddressValue + 0x04;
  LibAmdPciReadBits (TempAddress, 20, 20, &Temp, StdHeader);
  if (Temp == 0) {
    return; // This PCI device does not support capability lists
  }

  if (Offset != 0) {
    // If we are continuing on an existing list
    TempAddress.AddressValue = Base.AddressValue + Offset;
    LibAmdPciReadBits (TempAddress, 15, 8, &Temp, StdHeader);
  } else {
    // We are starting on a new list
    TempAddress.AddressValue = Base.AddressValue + 0x34;
    LibAmdPciReadBits (TempAddress, 7, 0, &Temp, StdHeader);
  }

  if (Temp == 0) {
    return; // We have reached the end of the capabilities list
  }

  // Error detection and recovery- The statement below protects against
  //   PCI devices with broken PCI capabilities lists.  Detect a pointer
  //   that is not uint32 aligned, points into the first 64 reserved DWORDs
  //   or points back to itself.
  if (((Temp & 3) != 0) || (Temp == Offset) || (Temp < 0x40)) {
    ASSERT (FALSE);
    return;
  }

  Address->AddressValue = Base.AddressValue + Temp;
  return;
}

/*---------------------------------------------------------------------------------------*/
/**
 * Set memory with value
 *
 *
 * @param[in,out] Destination   Pointer to memory range
 * @param[in]     Value         Value to set memory with
 * @param[in]     FillLength    Size of the memory range
 * @param[in]     StdHeader     Standard configuration header (Optional)
 */
VOID
LibAmdMemFill (
  IN       VOID *Destination,
  IN       UINT8 Value,
  IN       UINTN FillLength,
  IN OUT   AMD_CONFIG_PARAMS *StdHeader
  )
{
  UINT8 *Dest;
  ASSERT (StdHeader != NULL);
  Dest = Destination;
  while ((FillLength--) != 0) {
    *Dest++ = Value;
  }
}

/*---------------------------------------------------------------------------------------*/
/**
 * Copy memory
 *
 *
 * @param[in,out] Destination   Pointer to destination buffer
 * @param[in]     Source        Pointer to source buffer
 * @param[in]     CopyLength    buffer length
 * @param[in]     StdHeader     Standard configuration header (Optional)
 */
VOID
LibAmdMemCopy (
  IN       VOID *Destination,
  IN       CONST VOID *Source,
  IN       UINTN CopyLength,
  IN OUT   AMD_CONFIG_PARAMS *StdHeader
  )
{
  UINT8 *Dest;
  CONST UINT8 *SourcePtr;
  ASSERT (StdHeader != NULL);
  Dest = Destination;
  SourcePtr = Source;
  while ((CopyLength--) != 0) {
    *Dest++ = *SourcePtr++;
  }
}

/*---------------------------------------------------------------------------------------*/
/**
 * Verify checksum of binary image (B1/B2/B3)
 *
 *
 * @param[in]   ImagePtr      Pointer to image  start
 * @retval      TRUE          Checksum valid
 * @retval      FALSE         Checksum invalid
 */
BOOLEAN
LibAmdVerifyImageChecksum (
  IN       CONST VOID *ImagePtr
  )
{
  // Assume ImagePtr points to the binary start ($AMD)
  // Checksum is on an even boundary in AMD_IMAGE_HEADER

  UINT16 Sum;
  UINT32 i;

  Sum = 0;

  i = ((AMD_IMAGE_HEADER*) ImagePtr)->ImageSize;

  while (i > 1) {
    Sum = Sum + *((UINT16 *)ImagePtr);
    ImagePtr = (VOID *) ((UINT8 *)ImagePtr + 2);
    i = i - 2;
  }
  if (i > 0) {
    Sum = Sum + *((UINT8 *) ImagePtr);
  }

  return (Sum == 0)?TRUE:FALSE;
}

/*---------------------------------------------------------------------------------------*/
/**
 * Locate AMD binary image that contain specific module
 *
 *
 * @param[in]   StartAddress    Pointer to start range
 * @param[in]   EndAddress      Pointer to end range
 * @param[in]   Alignment       Image address alignment
 * @param[in]   ModuleSignature Module signature.
 * @retval     NULL  if image not found
 * @retval     pointer to image header
 */
CONST VOID *
LibAmdLocateImage (
  IN       CONST VOID *StartAddress,
  IN       CONST VOID *EndAddress,
  IN       UINT32 Alignment,
  IN       CONST CHAR8 ModuleSignature[8]
  )

{
  CONST UINT8 *CurrentPtr = StartAddress;
  AMD_MODULE_HEADER *ModuleHeaderPtr;
  CONST UINT64 SearchStr = *((UINT64*)ModuleSignature);

  // Search from start to end incrementing by alignment
  while ((CurrentPtr >= (UINT8 *) StartAddress) && (CurrentPtr < (UINT8 *) EndAddress)) {
    // First find a binary image
    if (IMAGE_SIGNATURE == *((UINT32 *) CurrentPtr)) {
      // TODO Figure out a way to fix the AGESA binary checksum
//    if (LibAmdVerifyImageChecksum (CurrentPtr)) {
        // If we have a valid image, search module linked list for a match
        ModuleHeaderPtr = (AMD_MODULE_HEADER*)(((AMD_IMAGE_HEADER *) CurrentPtr)->ModuleInfoOffset);
        while ((ModuleHeaderPtr != NULL) &&
            (MODULE_SIGNATURE == *((UINT32*)&(ModuleHeaderPtr->ModuleHeaderSignature)))) {
          if (SearchStr == *((UINT64*)&(ModuleHeaderPtr->ModuleIdentifier))) {
            return  CurrentPtr;
          }
          ModuleHeaderPtr = (AMD_MODULE_HEADER *)ModuleHeaderPtr->NextBlock;
        }
//      }
    }
    CurrentPtr += Alignment;
  }
  return NULL;
}

/*---------------------------------------------------------------------------------------*/
/**
 * Returns the package type mask for the processor
 *
 *
 * @param[in]     StdHeader     Standard configuration header (Optional)
 */

//  Returns the package type mask for the processor
UINT32
LibAmdGetPackageType (
  IN       AMD_CONFIG_PARAMS *StdHeader
  )
{
  UINT32      ProcessorPackageType;
  CPUID_DATA  CpuId;

  LibAmdCpuidRead (0x80000001, &CpuId, StdHeader);
  ProcessorPackageType = (UINT32) (CpuId.EBX_Reg >> 28) & 0xF; // bit 31:28
  return (UINT32) (1 << ProcessorPackageType);
}

/*---------------------------------------------------------------------------------------*/
/**
 * Returns the package type mask for the processor
 *
 *
 * @param[in]     AccessWidth     Access width
 * @param[in]     Data            data
 * @param[in]     DataMask        data
 * @param[out]    TemData         typecast data
 * @param[out]    TempDataMask    typecast data
 */

VOID
STATIC
LibAmdGetDataFromPtr (
  IN       ACCESS_WIDTH AccessWidth,
  IN       CONST VOID   *Data,
  IN       CONST VOID   *DataMask,
     OUT   UINT32       *TemData,
     OUT   UINT32       *TempDataMask
  )
{
  switch (AccessWidth) {
  case AccessWidth8:
  case AccessS3SaveWidth8:
    *TemData = (UINT32)*(UINT8 *) Data;
    *TempDataMask = (UINT32)*(UINT8 *) DataMask;
    break;
  case AccessWidth16:
  case AccessS3SaveWidth16:
    *TemData = (UINT32)*(UINT16 *) Data;
    *TempDataMask = (UINT32)*(UINT16 *) DataMask;
    break;
  case AccessWidth32:
  case AccessS3SaveWidth32:
    *TemData = *(UINT32 *) Data;
    *TempDataMask = *(UINT32 *) DataMask;
    break;
  default:
    IDS_ERROR_TRAP;
    break;
  }
}

/*---------------------------------------------------------------------------------------*/
/**
 * Returns the package type mask for the processor
 *
 *
 * @param[in]     AccessWidth     Access width
 * @retval        Width in number of bytes
 */

UINT8
LibAmdAccessWidth (
  IN       ACCESS_WIDTH AccessWidth
  )
{
  UINT8 Width;

  switch (AccessWidth) {
  case AccessWidth8:
  case AccessS3SaveWidth8:
    Width = 1;
    break;
  case AccessWidth16:
  case AccessS3SaveWidth16:
    Width = 2;
    break;
  case AccessWidth32:
  case AccessS3SaveWidth32:
    Width = 4;
    break;
  case AccessWidth64:
  case AccessS3SaveWidth64:
    Width = 8;
    break;
  default:
    Width = 0;
    IDS_ERROR_TRAP;
    break;
  }
  return Width;
}

AMDLIB_OPTIMIZE
VOID
CpuidRead (
  IN        UINT32      CpuidFcnAddress,
  OUT       CPUID_DATA  *Value
  )
{
  __cpuid ((int *)Value, CpuidFcnAddress);
}

AMDLIB_OPTIMIZE
UINT8
ReadNumberOfCpuCores(
  void
  )
{
  CPUID_DATA  Value;
  CpuidRead (0x80000008, &Value);
  return   Value.ECX_Reg & 0xff;
}

BOOLEAN
IdsErrorStop (
  IN      UINT32 FileCode
  )
{
	struct POST {
		UINT16 deadlo;
		UINT32 messagelo;
		UINT16 deadhi;
		UINT32 messagehi;
	} post = {0xDEAD, FileCode, 0xDEAD, FileCode};
	UINT16 offset = 0;
	UINT16 j;

	while(1) {
		offset %= sizeof(struct POST) / 2;
		WriteIo16(80, *((UINT16 *)&post)+offset);
		++offset;
		for (j=0; j<250; ++j) {
			ReadIo8(80);
		}
	}
}
