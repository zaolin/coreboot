/**
 * @file
 *
 * Southbridge PMIO access common routine
 *
 */
/*
 *****************************************************************************
 *
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2010 Advanced Micro Devices, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 * ***************************************************************************
 *
 */

#include "SBPLATFORM.h"

/*----------------------------------------------------------------------------------------*/
/**
 * Read PMIO
 *
 *
 *
 * @param[in] Address  - PMIO Offset value
 * @param[in] OpFlag   - Access sizes
 * @param[in] Value    - Read Data Buffer
 *
 */
VOID
ReadPMIO (
  IN       UINT8 Address,
  IN       UINT8 OpFlag,
  IN       VOID* Value
  )
{
  UINT8 i;
  OpFlag = OpFlag & 0x7f;

  if ( OpFlag == 0x02 ) {
    OpFlag = 0x03;
  }
  for ( i = 0; i <= OpFlag; i++ ) {
    WriteIO (0xCD6, AccWidthUint8, &Address);                   // SB_IOMAP_REGCD6
    Address++;
    ReadIO (0xCD7, AccWidthUint8, (UINT8 *)Value + i);          // SB_IOMAP_REGCD7
  }
}

/*----------------------------------------------------------------------------------------*/
/**
 * Write PMIO
 *
 *
 *
 * @param[in] Address  - PMIO Offset value
 * @param[in] OpFlag   - Access sizes
 * @param[in] Value    - Write Data Buffer
 *
 */
VOID
WritePMIO (
  IN       UINT8 Address,
  IN       UINT8 OpFlag,
  IN       VOID* Value
  )
{
  UINT8 i;
  OpFlag = OpFlag & 0x7f;

  if ( OpFlag == 0x02 ) {
    OpFlag = 0x03;
  }
  for ( i = 0; i <= OpFlag; i++ ) {
    WriteIO (0xCD6, AccWidthUint8, &Address);                   // SB_IOMAP_REGCD6
    Address++;
    WriteIO (0xCD7, AccWidthUint8, (UINT8 *)Value + i);         // SB_IOMAP_REGCD7
  }
}

/*----------------------------------------------------------------------------------------*/
/**
 * RWPMIO - Read/Write PMIO
 *
 *
 *
 * @param[in] Address    - PMIO Offset value
 * @param[in] OpFlag     - Access sizes
 * @param[in] AndMask    - Data And Mask 32 bits
 * @param[in] OrMask     - Data OR Mask 32 bits
 *
 */
VOID
RWPMIO (
  IN       UINT8 Address,
  IN       UINT8   OpFlag,
  IN       UINT32 AndMask,
  IN       UINT32 OrMask
  )
{
  UINT32 Result;
  OpFlag = OpFlag & 0x7f;
  ReadPMIO (Address, OpFlag, &Result);
  Result = (Result & AndMask) | OrMask;
  WritePMIO (Address, OpFlag, &Result);
}
