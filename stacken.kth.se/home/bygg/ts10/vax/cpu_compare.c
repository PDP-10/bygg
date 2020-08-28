// cpu_compare.c - Compare instructions.
//
// Written by
//  Timothy Stark <sword7@speakeasy.org>
//
// This file is part of the TS-10 Emulator.
// See README for copyright notice.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#include "vax/defs.h"

// CMP - Compare
//
// Format:
//   opcode src1.rx, src2.rx
//
// Operations:
//   src1 - src2;
//
// Condition Codes:
//   N <- src1 LSS src2;
//   Z <- src1 EQL src2;
//   V <- 0;
//   C <- src1 LSSU src2;
//
// Execptions:
//   None
//
// Opcodes:
//   91  CMPB  Compare Byte
//   B1  CMPW  Compare Word
//   D1  CMPL  Compare Long

void vax_Opcode_CMPB(void)
{
	int8 src1 = OP0;
	int8 src2 = OP1;

	// Set system flags for results from comparsion.
	PSB.n = (src1 < src2);
	PSB.z = (src1 == src2);
	PSB.v = 0;
	PSB.c = ((unsigned)src1 < (unsigned)src2);

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA))
		dbg_Printf("VAX(CPU): Compare Byte - OP0=%02X OP1=%02X Result: %s\n",
			src1, src2, vax_DisplayCondition());
#endif DEBUG
}

void vax_Opcode_CMPW(void)
{
	int16 src1 = OP0;
	int16 src2 = OP1;

	// Set system flags for results from comparsion.
	PSB.n = (src1 < src2);
	PSB.z = (src1 == src2);
	PSB.v = 0;
	PSB.c = ((unsigned)src1 < (unsigned)src2);

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA))
		dbg_Printf("VAX(CPU): Compare Word - OP0=%04X OP1=%04X Result: %s\n",
			src1, src2, vax_DisplayCondition());
#endif DEBUG
}

void vax_Opcode_CMPL(void)
{
	int32 src1 = OP0;
	int32 src2 = OP1;

	// Set system flags for results from comparsion.
	PSB.n = (src1 < src2);
	PSB.z = (src1 == src2);
	PSB.v = 0;
	PSB.c = ((unsigned)src1 < (unsigned)src2);

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA))
		dbg_Printf("VAX(CPU): Compare Long - OP0=%08X OP1=%08X Result: %s\n",
			src1, src2, vax_DisplayCondition());
#endif DEBUG
}

// TST - Test
//
// Format:
//   opcode src.rx
//
// Operations:
//   src - 0;
//
// Condition Codes:
//   N <- src LSS 0;
//   Z <- src EQL 0;
//   V <- 0;
//   C <- 0;
//
// Execptions:
//   None
//
// Opcodes:
//   95  TSTB  Test Byte
//   B5  TSTW  Test Word
//   D5  TSTL  Test Long

void vax_Opcode_TSTB(void)
{
	int8 src = OP0;

	// Set system flags for results from comparsion.
	PSB.n = (src < 0);
	PSB.z = (src == 0);
	PSB.v = 0;
	PSB.c = 0;

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA))
		dbg_Printf("VAX(CPU): Test Byte - OP0=%02X Result: %s\n",
			src, vax_DisplayCondition());
#endif DEBUG
}

void vax_Opcode_TSTW(void)
{
	int16 src = OP0;

	// Set system flags for results from comparsion.
	PSB.n = (src < 0);
	PSB.z = (src == 0);
	PSB.v = 0;
	PSB.c = 0;

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA))
		dbg_Printf("VAX(CPU): Test Word - OP0=%04X Result: %s\n",
			src, vax_DisplayCondition());
#endif DEBUG
}

void vax_Opcode_TSTL(void)
{
	int32 src = OP0;

	// Set system flags for results from comparsion.
	PSB.n = (src < 0);
	PSB.z = (src == 0);
	PSB.v = 0;
	PSB.c = 0;

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA))
		dbg_Printf("VAX(CPU): Test Long - OP0=%08X Result: %s\n",
			src, vax_DisplayCondition());
#endif DEBUG
}
