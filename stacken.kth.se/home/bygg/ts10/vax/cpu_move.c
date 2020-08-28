// cpu_misc.c - VAX Misc Instructions.
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

// MCOM  Move Complemented
//
// Format:
//   opcode src.rx, dst.wx
//
// Operation:
//   dst <- NOT src;
//
// Condition Codes:
//   N <- dst LSS 0;
//   Z <- dst EQL 0;
//   V <- 0;
//   C <- C;
//
// Exceptions:
//   None
//
// Opcodes:
//   92  MCOMB  Move Complemented Byte
//   B2  MCOMW  Move Complemented Word
//   D2  MCOML  Move COmplemented Long

void vax_Opcode_MCOMB(void)
{
	int8 src = OP0;
	int8 dst;

	dst = ~src;
	if (OP1 >= 0)
		RN(OP1) = (RN(OP1) & ~BMASK) | (dst & BMASK);
	else
		vax11mem_vbWrite(OP2, dst);

	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.v = 0;
}

void vax_Opcode_MCOMW(void)
{
	int16 src = OP0;
	int16 dst;

	dst = ~src;
	if (OP1 >= 0)
		RN(OP1) = (RN(OP1) & ~WMASK) | (dst & WMASK);
	else
		vax11mem_vWrite(OP2, (uint8 *)&dst, OP_WORD);

	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.v = 0;
}

void vax_Opcode_MCOML(void)
{
	int32 src = OP0;
	int32 dst;

	dst = ~src;
	if (OP1 >= 0)
		RN(OP1) = dst;
	else
		vax11mem_vWrite(OP2, (uint8 *)&dst, OP_LONG);

	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.v = 0;
}

// MNEG  Move Negated
//
// Format:
//   opcode src.rx, dst.wx
//
// Operation:
//   dst <- -src;
//
// Condition Codes:
//   N <- dst LSS 0;
//   Z <- dst EQL 0;
//   V <- {integer overflow};
//   C <- dst NEQ 0;
//
// Exceptions:
//   Integer Overflow Trap
//
// Opcodes:
//   8E  MCOMB  Move Negated Byte
//   AE  MCOMW  Move Negated Word
//   CE  MCOML  Move Negated Long

void vax_Opcode_MNEGB(void)
{
	int8 src = OP0;
	int8 dst;

	dst = -src;
	if (OP1 >= 0)
		RN(OP1) = (RN(OP1) & ~BMASK) | (dst & BMASK);
	else
		vax11mem_vbWrite(OP2, dst);

	if (((~src ^ dst) & ~src) & BSIGN) {
		PSB.v = 1;
		if (PSW & PSW_IV)
			SET_TRAP(TRAP_INFOVF);
	} else
		PSB.v = 0;

	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.c = (dst != 0);
}

void vax_Opcode_MNEGW(void)
{
	int16 src = OP0;
	int16 dst;

	dst = -src;
	if (OP1 >= 0)
		RN(OP1) = (RN(OP1) & ~WMASK) | (dst & WMASK);
	else
		vax11mem_vWrite(OP2, (uint8 *)&dst, OP_WORD);

	if (((~src ^ dst) & ~src) & WSIGN) {
		PSB.v = 1;
		if (PSW & PSW_IV)
			SET_TRAP(TRAP_INFOVF);
	} else
		PSB.v = 0;

	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.c = (dst != 0);
}

void vax_Opcode_MNEGL(void)
{
	int32 src = OP0;
	int32 dst;

	dst = -src;
	if (OP1 >= 0)
		RN(OP1) = dst;
	else
		vax11mem_vWrite(OP2, (uint8 *)&dst, OP_LONG);

	if (((~src ^ dst) & ~src) & LSIGN) {
		PSB.v = 1;
		if (PSW & PSW_IV)
			SET_TRAP(TRAP_INFOVF);
	} else
		PSB.v = 0;

	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.c = (dst != 0);
}

// Operand Registers:
//
//   For MOV[BWL] instruction:
//     src.rx: OP0 = value of operand
//     dst.wx: OP1 = memory/register flag
//             OP2 = memory address
//
//   For MOVQ instruction:
//     src.rq: OP0 = value of operand (left half)
//             OP1 = value of operand (right half)
//     dst.wx: OP2 = memory/register flag
//             OP3 = memory address

void vax_Opcode_MOVB(void)
{
	int8 src = OP0;

	if (OP1 >= 0)
		RN(OP1) = (RN(OP1) & ~BMASK) | (src & BMASK);
	else
		vax11mem_vbWrite(OP2, src);

	PSB.n = (src < 0);
	PSB.z = (src == 0);
	PSB.v = 0;

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		char dst[16];

		if (OP1 >= 0)
			sprintf(dst, "R%d", OP1);
		else
			sprintf(dst, "%08X", OP2);
		dbg_Printf("VAX(CPU): Move %02X to %s\n", src, dst);
	}
#endif DEBUG
}

void vax_Opcode_MOVW(void)
{
	int16 src = OP0;

	if (OP1 >= 0)
		RN(OP1) = (RN(OP1) & ~WMASK) | (src & WMASK);
	else
		vax11mem_vWrite(OP2, (uint8 *)&src, 2);

	PSB.n = (src < 0);
	PSB.z = (src == 0);
	PSB.v = 0;
}

void vax_Opcode_MOVL(void)
{
	int32 src = OP0;

	if (OP1 >= 0)
		RN(OP1) = src;
	else
		vax11mem_vWrite(OP2, (uint8 *)&src, 4);

	PSB.n = (src < 0);
	PSB.z = (src == 0);
	PSB.v = 0;
}

void vax_Opcode_MOVQ(void)
{
	int32 src1 = OP0;
	int32 src2 = OP1;

	if (OP2 >= 0) {
		RN0(OP2) = src1;
		RN1(OP2) = src2;
	} else {
		vax11mem_vWrite(OP3+4, (uint8 *)&src2, OP_LONG);
		vax11mem_vWrite(OP3, (uint8 *)&src1, OP_LONG);
	}

	PSB.n = (src2 < 0);
	PSB.z = ((src1|src2) == 0);
	PSB.v = 0;
}

void vax_Opcode_MOVZBW(void)
{
	int16 src = OP0 & BMASK;

	if (OP1 >= 0)
		RN(OP1) = (RN(OP1) & ~WMASK) | (src & WMASK);
	else
		vax11mem_vWrite(OP2, (uint8 *)&src, 2);

	PSB.n = (src < 0);
	PSB.z = (src == 0);
	PSB.v = 0;
}

void vax_Opcode_MOVZBL(void)
{
	int32 src = OP0 & BMASK;

	if (OP1 >= 0)
		RN(OP1) = src;
	else
		vax11mem_vWrite(OP2, (uint8 *)&src, 4);

	PSB.n = (src < 0);
	PSB.z = (src == 0);
	PSB.v = 0;
}

void vax_Opcode_MOVZWL(void)
{
	int32 src = OP0 & WMASK;

	if (OP1 >= 0)
		RN(OP1) = src;
	else
		vax11mem_vWrite(OP2, (uint8 *)&src, 4);

	PSB.n = (src < 0);
	PSB.z = (src == 0);
	PSB.v = 0;
}

// Operand Registers:
//
//   For MOVA[BWL] instruction:
//     src.ax: OP0 = address of operand
//     dst.wx: OP1 = memory/register flag
//             OP2 = memory address
