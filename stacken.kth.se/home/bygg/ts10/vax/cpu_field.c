// cpu_field.c - Variable-length Bit Field Instructions
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

// Operand Registers:
//   pos.rl   OP0 = value of operand
//   size.rb  OP1 = value of operand
//   base.vb  OP2 = register/memory flag
//            OP3 = content/memory address

int32 vax_GetField(int32 *opnd, int32 sign)
{
	int32 Pos   = opnd[0];
	int32 Size  = opnd[1];
	int32 nReg  = opnd[2];
	int32 Word1 = opnd[3];
	int32 Word2, Base;

	// If size is zero, do nothing but return zero.
	if (Size == 0)
		return 0;

	// If size is more than 32,
	// initiate reserved operand fault.
	if (Size > 32)
		RSRVD_OPND_FAULT;

	// Extract a field from one or two longwords.
	if (nReg >= 0) {
		if (Pos > 31)
			RSRVD_OPND_FAULT;
		if (Pos)
			Word1 = (Word1 >> Pos) | (RN(nReg+1) << (32 - Pos));
	} else {
		Base = Word1 + (Pos >> 3);
		Pos = (Pos & 0x07) | ((Base & 0x03) << 3);
		Base &= ~0x03;
		vax11mem_vRead(Base, (uint8 *)&Word1, OP_LONG);
		if ((Pos + Size) > 32)
			vax11mem_vRead(Base+4, (uint8 *)&Word2, OP_LONG);
		if (Pos)
			Word1 = (Word1 >> Pos) | (Word2 << (32 - Pos));
	}

	return Word1 & ((1 << Size) - 1);
}

// Operand Registers:
//   pos.rl   OP0 = value of operand
//   size.rb  OP1 = value of operand
//   base.vb  OP2 = register/memory flag
//            OP3 = content/memory address
//   src.rl   OP4 = value of operand

void vax_Opcode_CMPV(void)
{
	int32 tmp = vax_GetField(&OP0, 1);
	int32 src = OP4;

	PSB.n = (tmp < src);
	PSB.z = (tmp == src);
	PSB.v = 0;
	PSB.c = ((unsigned)tmp < (unsigned)src);
}

void vax_Opcode_CMPZV(void)
{
	int32 tmp = vax_GetField(&OP0, 0);
	int32 src = OP4;

	PSB.n = (tmp < src);
	PSB.z = (tmp == src);
	PSB.v = 0;
	PSB.c = ((unsigned)tmp < (unsigned)src);
}

// Operand Registers:
//   pos.rl   OP0 = value of operand
//   size.rb  OP1 = value of operand
//   base.vb  OP2 = register/memory flag
//            OP3 = content/memory address
//   src.wl   OP4 = register/memory flag
//            OP5 = content/memory address

void vax_Opcode_EXTV(void)
{
	int32 dst = vax_GetField(&OP0, 1);

	if (OP4 >= 0)
		RN(OP4) = dst;
	else
		vax11mem_vWrite(OP5, (uint8 *)&dst, OP_LONG);

	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.v = 0;
	PSB.c = 0;
}

void vax_Opcode_EXTZV(void)
{
	int32 dst = vax_GetField(&OP0, 0);

	if (OP4 >= 0)
		RN(OP4) = dst;
	else
		vax11mem_vWrite(OP5, (uint8 *)&dst, OP_LONG);

	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.v = 0;
	PSB.c = 0;
}

