// cpu_integer.c - Integer/Logical Arithmetic Instructions
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

void vax_Opcode_ADAWI(void)
{
	int16 src = OP0;
	int16 tmp, dst;

	if (OP1 >= 0) {
		tmp = RN(OP1);
		dst = RN(OP1) = (src + tmp) & WMASK;
	} else {
		if (OP2 & 1)
			RSRVD_OPND_FAULT;
		vax11mem_vRead(OP2, (uint8 *)&tmp, OP_WORD);
		dst = src + tmp;
		vax11mem_vWrite(OP2, (uint8 *)&dst, OP_WORD);
	}

	if (((~src ^ tmp) & (src ^ dst)) & WSIGN) {
		PSB.v = 1;
		if (PSW & PSW_IV)
			SET_TRAP(TRAP_INFOVF);
	} else
		PSB.v = 0;

	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.c = ((uint16)dst < (uint16)tmp);

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		dbg_Printf("ADAWI: %04X + %04X => %04X Condition: %s\n",
			src, tmp, dst, vax_DisplayCondition());
	}
#endif DEBUG
}

// **********************************************************
// ********************** ADD  Add **************************
// **********************************************************
//
// Operand Registers:
//
//   opcode w/2 operands
//     add.rx   OP0 = value of operand
//     sum.rx   OP1 = value of operand
//     sum.wx   OP2 = register/memory flag
//              OP3 = memory address
//
//   opcode w/3 operands
//     add1.rx  OP0 = value of operand
//     add2.rx  OP1 = value of operand
//     sum.wx   OP2 = register/memory flag
//              OP3 = memory address

void vax_Opcode_ADDB(void)
{
	int8 add1 = OP0;
	int8 add2 = OP1;
	int8 sum;

	sum = add1 + add2;
	if (OP2 >= 0)
		RN(OP2) = (RN(OP2) & ~BMASK) | (sum & BMASK);
	else
		vax11mem_vbWrite(OP3, sum);

	if (((~add1 ^ add2) & (add1 ^ sum)) & BSIGN) {
		PSB.v = 1;
		if (PSW & PSW_IV)
			SET_TRAP(TRAP_INFOVF);
	} else
		PSB.v = 0;

	PSB.n = (sum < 0);
	PSB.z = (sum == 0);
	PSB.c = ((uint8)sum < (uint8)add2);

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		dbg_Printf("VAX(ADDB): %02X + %02X => %02X Condition: %s\n",
			add1, add2, sum, vax_DisplayCondition());
	}
#endif DEBUG
}

void vax_Opcode_ADDW(void)
{
	int16 add1 = OP0;
	int16 add2 = OP1;
	int16 sum;

	sum = add1 + add2;
	if (OP2 >= 0)
		RN(OP2) = (RN(OP2) & ~WMASK) | (sum & WMASK);
	else
		vax11mem_vWrite(OP3, (uint8 *)&sum, OP_WORD);

	if (((~add1 ^ add2) & (add1 ^ sum)) & WSIGN) {
		PSB.v = 1;
		if (PSW & PSW_IV)
			SET_TRAP(TRAP_INFOVF);
	} else
		PSB.v = 0;

	PSB.n = (sum < 0);
	PSB.z = (sum == 0);
	PSB.c = ((uint16)sum < (uint16)add2);

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		dbg_Printf("VAX(ADDW): %04X + %04X => %04X Condition: %s\n",
			add1, add2, sum, vax_DisplayCondition());
	}
#endif DEBUG
}

void vax_Opcode_ADDL(void)
{
	int32 add1 = OP0;
	int32 add2 = OP1;
	int32 sum;

	sum = add1 + add2;
	if (OP2 >= 0)
		RN(OP2) = sum;
	else
		vax11mem_vWrite(OP3, (uint8 *)&sum, OP_LONG);

	if (((~add1 ^ add2) & (add1 ^ sum)) & LSIGN) {
		PSB.v = 1;
		if (PSW & PSW_IV)
			SET_TRAP(TRAP_INFOVF);
	} else
		PSB.v = 0;

	PSB.n = (sum < 0);
	PSB.z = (sum == 0);
	PSB.c = ((uint32)sum < (uint32)add2);

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		dbg_Printf("VAX(ADDL): %08X + %08X => %08X Condition: %s\n",
			add1, add2, sum, vax_DisplayCondition());
	}
#endif DEBUG
}

// ADWC  Add With Carry
//
// Format:
//   opcode add.rl, sum.ml
//
// Operation:
//   sum <- sum + add + C;
//
// Condition Codes:
//   N <- sum LSS 0;
//   Z <- sum EQL 0;
//   V <- {integer overflow};
//   C <- {carry from most signficant bit};
//
// Exception:
//   Integer Overflow Trap
//
// Opcode:
//   D8  ADWC  Add With Carry
//
// Operand Registers:
//   add.rl   OP0 = value of operand
//   sum.rl   OP1 = value of operand
//   sum.wl   OP2 = register/memory flag
//            OP3 = memory address

void vax_Opcode_ADWC(void)
{
	int32 add1 = OP0;
	int32 add2 = OP1;
	int32 sum;

	sum = add1 + add2 + PSB.c;
	if (OP2 >= 0)
		RN(OP2) = sum;
	else
		vax11mem_vWrite(OP3, (uint8 *)&sum, OP_LONG);

	if (((~add1 ^ add2) & (add1 ^ sum)) & LSIGN) {
		PSB.v = 1;
		if (PSW & PSW_IV)
			SET_TRAP(TRAP_INFOVF);
	} else
		PSB.v = 0;

	PSB.n = (sum < 0);
	PSB.z = (sum == 0);
	PSB.c = ((uint32)sum < (uint32)add2);
	if ((sum == add2) && add1)
		PSB.c = 1;

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		dbg_Printf("VAX(ADWC): %08X + %08X => %08X Condition: %s\n",
			add1, add2, sum, vax_DisplayCondition());
	}
#endif DEBUG
}

// ASH  Arithmetic Shift
//
// Format:
//   opcode cnt.rb, src.rx, dst.wx
//
// Operation:
//   dst = src shifted out bits;
//
// Condition Codes:
//   N = dst < 0;
//   Z = dst == 0;
//   V = {integer overflow};
//   C = 0;
//
// Exception:
//   Integer Overflow Trap
//
// Opcodes:
//   78  ASHL  Arithmetic Shift Long
//   79  ASHQ  Arithmetic Shift Quad
//
// Operand Registers:
//   cnt.rb    OP0 = value of operand
//   src.rx    OP1 = value of operand
//   dst.wx    OP2 = register/memory flag
//             OP3 = memory address

void vax_Opcode_ASHL(void)
{
	int8  cnt = OP0;
	int32 src = OP1;
	int32 dst;
	int   ovflg = 0;

	// Do 32-bit arithmetic shift
	if (cnt == 0) {
		dst = src;
		ovflg = 0;
	} else {
		if (cnt < 0) {
			cnt = -cnt;
			if (cnt < 32)
				dst = src >> cnt;
			else 
				dst = (src < 0) ? -1 : 0;
		} else {
			if (cnt < 32) {
				dst = src << cnt;
				ovflg = (src != (dst >> cnt));
			} else {
				dst = 0;
				ovflg = (src != 0);
			}
		}
	}

	// Write results back.
	if (OP2 >= 0)
		RN(OP2) = dst;
	else
		vax11mem_vWrite(OP3, (uint8 *)&dst, OP_LONG);

	// If overflow occurs, initiate integer overflow trap.
	if (ovflg && (PSW & PSW_IV))
		SET_TRAP(TRAP_INTOVF);

	// Set condition status bits.
	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.v = ovflg;
	PSB.c = 0;

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		dbg_Printf("ASHL: %08X %s %d => %08X Condition: %s\n",
			src, ((cnt < 0) ? ">>" : "<<"), abs(cnt), dst,
			vax_DisplayCondition());
	}
#endif DEBUG
}

void vax_Opcode_ASHQ(void)
{
	int8  cnt = OP0;
	int64 src = ((uint64)OP2 << 32) | (uint64)OP1;
	int64 dst;
	int   ovflg;

	// Do 64-bit arithmetic shift
	if (cnt == 0) {
		dst = src;
		ovflg = 0;
	} else {
		if (cnt < 0) {
			// Shift right
			cnt = -cnt;
			if (cnt < 64)
				dst = src >> cnt;
			else 
				dst = (src < 0) ? -1LL : 0LL;
			ovflg = 0;
		} else {
			// Shift left
			if (cnt < 64) {
				dst = src << cnt;
				ovflg = (src != (dst >> cnt));
			} else {
				dst = 0LL;
				ovflg = (src != 0);
			}
		}
	}

	// Write results back.
	if (OP3 >= 0) {
		RN(OP3)   = (int32)dst;
		RN(OP3+1) = (int32)(dst >> 32);
	} else {
		vax11mem_vWrite(OP4+4, (uint8 *)&dst+4, OP_LONG);
		vax11mem_vWrite(OP4, (uint8 *)&dst, OP_LONG);
	}

	// If overflow occurs, initiate integer overflow trap.
	if (ovflg && (PSW & PSW_IV))
		SET_TRAP(TRAP_INTOVF);

	// Set condition status bits.
	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.v = ovflg;
	PSB.c = 0;

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		dbg_Printf("VAX(ASHQ): %016LX %s %d (%d) => %016LX Condition: %s\n",
			src, ((cnt < 0) ? ">>" : "<<"), abs(cnt)%32, abs(cnt), dst,
			vax_DisplayCondition());
	}
#endif DEBUG
}


void vax_Opcode_BICB(void)
{
	int8 mask = OP0;
	int8 src  = OP1;
	int8 dst;

	dst = src & ~mask;
	if (OP2 >= 0)
		RN(OP2) = (RN(OP2) & ~BMASK) | (dst & BMASK);
	else
		vax11mem_vbWrite(OP3, dst);

	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.v = 0;
}

void vax_Opcode_BICW(void)
{
	int16 mask = OP0;
	int16 src  = OP1;
	int16 dst;

	dst = src & ~mask;
	if (OP2 >= 0)
		RN(OP2) = (RN(OP2) & ~WMASK) | (dst & WMASK);
	else
		vax11mem_vWrite(OP3, (uint8 *)&dst, OP_WORD);

	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.v = 0;
}

void vax_Opcode_BICL(void)
{
	int32 mask = OP0;
	int32 src  = OP1;
	int32 dst;

	dst = src & ~mask;
	if (OP2 >= 0)
		RN(OP2) = dst;
	else
		vax11mem_vWrite(OP3, (uint8 *)&dst, OP_LONG);

	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.v = 0;
}


void vax_Opcode_BISB(void)
{
	int8 mask = OP0;
	int8 src  = OP1;
	int8 dst;

	dst = src | mask;
	if (OP2 >= 0)
		RN(OP2) = (RN(OP2) & ~BMASK) | (dst & BMASK);
	else
		vax11mem_vbWrite(OP3, dst);

	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.v = 0;
}

void vax_Opcode_BISW(void)
{
	int16 mask = OP0;
	int16 src  = OP1;
	int16 dst;

	dst = src | mask;
	if (OP2 >= 0)
		RN(OP2) = (RN(OP2) & ~WMASK) | (dst & WMASK);
	else
		vax11mem_vWrite(OP3, (uint8 *)&dst, OP_WORD);

	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.v = 0;
}

void vax_Opcode_BISL(void)
{
	int32 mask = OP0;
	int32 src  = OP1;
	int32 dst;

	dst = src | mask;
	if (OP2 >= 0)
		RN(OP2) = dst;
	else
		vax11mem_vWrite(OP3, (uint8 *)&dst, OP_LONG);

	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.v = 0;
}


void vax_Opcode_BITB(void)
{
	int8 mask = OP0;
	int8 src  = OP1;
	int8 tmp;

	tmp = src & mask;

	PSB.n = (tmp < 0);
	PSB.z = (tmp == 0);
	PSB.v = 0;
}

void vax_Opcode_BITW(void)
{
	int16 mask = OP0;
	int16 src  = OP1;
	int16 tmp;

	tmp = src & mask;

	PSB.n = (tmp < 0);
	PSB.z = (tmp == 0);
	PSB.v = 0;
}

void vax_Opcode_BITL(void)
{
	int32 mask = OP0;
	int32 src  = OP1;
	int32 tmp;

	tmp = src & mask;

	PSB.n = (tmp < 0);
	PSB.z = (tmp == 0);
	PSB.v = 0;
}

// CLR  Clear
//
// Format:
//   opcode dst.wx
//
// Operation:
//   dst = 0;
//
// Condition Codes:
//   N = 0;
//   Z = 1;
//   V = 0;
//   C = C;
//
// Exceptions:
//   None
//
// Opcodes:
//   94    CLRB  Clear Byte
//   B4    CLRW  Clear Word
//   D4    CLRL  Clear Long
//   7C    CLRQ  Clear Quad
//   7CFD  CLRO  Clear Octa
//
// Operand Registers:
//   dst.wx   OP0 = register/memory flag
//            OP1 = memory address

void vax_Opcode_CLRB(void)
{
	int8 dst = 0;

	if (OP0 >= 0)
		RN(OP0) &= ~BMASK;
	else
		vax11mem_vbWrite(OP1, dst);

	PSB.n = 0;
	PSB.z = 1;
	PSB.v = 0;
}

void vax_Opcode_CLRW(void)
{
	int16 dst = 0;

	if (OP0 >= 0)
		RN(OP0) &= ~WMASK;
	else
		vax11mem_vWrite(OP1, (uint8 *)&dst, OP_WORD);

	PSB.n = 0;
	PSB.z = 1;
	PSB.v = 0;
}

void vax_Opcode_CLRL(void)
{
	int32 dst = 0;

	if (OP0 >= 0) {
		RN(OP0) = 0; 
	} else {
		vax11mem_vWrite(OP1, (uint8 *)&dst, OP_LONG);
	}

	PSB.n = 0;
	PSB.z = 1;
	PSB.v = 0;
}

void vax_Opcode_CLRQ(void)
{
	int64 dst = 0LL;

	if (OP0 >= 0) {
		RN0(OP0) = 0;
		RN1(OP0) = 0;
	} else {
		int64 zero = 0LL;
		vax11mem_vWrite(OP1, (uint8 *)&dst, OP_QUAD);
	}

	PSB.n = 0;
	PSB.z = 1;
	PSB.v = 0;
}

// **********************************************************
// ******************** DEC  Decrement **********************
// **********************************************************

void vax_Opcode_DECB(void)
{
	int8 src = OP0;
	int8 dst;

	dst = src - 1;
	if (OP1 >= 0)
		RN(OP1) = (RN(OP1) & ~BMASK) | (dst & BMASK);
	else
		vax11mem_vbWrite(OP3, dst);

	if (src < dst) {
		PSB.v = 1;
		if (PSW & PSW_IV)
			SET_TRAP(TRAP_INFOVF);
	} else
		PSB.v = 0;

	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.c = (dst == -1);

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		dbg_Printf("DECB: %02X => %02X Condition: %s\n",
			src, dst, vax_DisplayCondition());
	}
#endif DEBUG
}

void vax_Opcode_DECW(void)
{
	int16 src = OP0;
	int16 dst;

	dst = src - 1;
	if (OP1 >= 0)
		RN(OP1) = (RN(OP1) & ~WMASK) | (dst & WMASK);
	else
		vax11mem_vWrite(OP2, (uint8 *)&dst, OP_WORD);

	if (src < dst) {
		PSB.v = 1;
		if (PSW & PSW_IV)
			SET_TRAP(TRAP_INFOVF);
	} else
		PSB.v = 0;

	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.c = (dst == -1);

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		dbg_Printf("DECW: %04X => %04X Condition: %s\n",
			src, dst, vax_DisplayCondition());
	}
#endif DEBUG
}

void vax_Opcode_DECL(void)
{
	int32 src = OP0;
	int32 dst;

	dst = src - 1;
	if (OP1 >= 0)
		RN(OP1) = dst;
	else
		vax11mem_vWrite(OP2, (uint8 *)&dst, OP_LONG);

	if (src < dst) {
		PSB.v = 1;
		if (PSW & PSW_IV)
			SET_TRAP(TRAP_INFOVF);
	} else
		PSB.v = 0;

	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.c = (dst == -1);

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		dbg_Printf("DECL: %08X => %08X Condition: %s\n",
			src, dst, vax_DisplayCondition());
	}
#endif DEBUG
}

// **********************************************************
// ******************** INC  Increment **********************
// **********************************************************

void vax_Opcode_INCB(void)
{
	int8 src = OP0;
	int8 dst;

	dst = src + 1;
	if (OP1 >= 0)
		RN(OP1) = (RN(OP1) & ~BMASK) | (dst & BMASK);
	else
		vax11mem_vbWrite(OP2, dst);

	if (src > dst) {
		PSB.v = 1;
		if (PSW & PSW_IV)
			SET_TRAP(TRAP_INFOVF);
	} else
		PSB.v = 0;

	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.c = (dst == 0);

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		dbg_Printf("INCB: %02X => %02X Condition: %s\n",
			src, dst, vax_DisplayCondition());
	}
#endif DEBUG
}

void vax_Opcode_INCW(void)
{
	int16 src = OP0;
	int16 dst;

	dst = src + 1;
	if (OP1 >= 0)
		RN(OP1) = (RN(OP1) & ~WMASK) | (dst & WMASK);
	else
		vax11mem_vWrite(OP2, (uint8 *)&dst, OP_WORD);

	if (src > dst) {
		PSB.v = 1;
		if (PSW & PSW_IV)
			SET_TRAP(TRAP_INFOVF);
	} else
		PSB.v = 0;

	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.c = (dst == 0);

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		dbg_Printf("INCW: %04X => %04X Condition: %s\n",
			src, dst, vax_DisplayCondition());
	}
#endif DEBUG
}

void vax_Opcode_INCL(void)
{
	int32 src = OP0;
	int32 dst;

	dst = src + 1;
	if (OP1 >= 0)
		RN(OP1) = dst;
	else
		vax11mem_vWrite(OP2, (uint8 *)&dst, OP_LONG);

	if (src > dst) {
		PSB.v = 1;
		if (PSW & PSW_IV)
			SET_TRAP(TRAP_INFOVF);
	} else
		PSB.v = 0;

	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.c = (dst == 0);

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		dbg_Printf("INCL: %08X => %08X Condition: %s\n",
			src, dst, vax_DisplayCondition());
	}
#endif DEBUG
}

// **********************************************************
// ******************** MUL  Multiply ***********************
// **********************************************************

void vax_Opcode_MULB(void)
{
}

void vax_Opcode_MULW(void)
{
}

void vax_Opcode_MULL(void)
{
}

// PUSHL  Push Long
//
// Format:
//   opcode src.rl
//
// Operation:
//   -(SP) <- src;
//
// Condition Codes:
//   N <- src LSS 0;
//   Z <- src EQL 0;
//   V <- 0;
//   C <- C;
//
// Exceptions:
//   None
//
// Opcode:
//   DD  PUSHL  Push Long

void vax_Opcode_PUSHL(void)
{
	int32 src = OP0;

	vax11mem_vWrite(SP - 4, (uint8 *)&src, OP_LONG);
	SP -= 4;

	PSB.n = (src < 0);
	PSB.z = (src == 0);
	PSB.v = 0;
}

// ROTL  Rotate Long
//
// Format:
//   opcode cnt.rb, src.rl, dst.wl
//
// Operation:
//   dst = src rotated cnt bits;
//
// Condition Codes:
//   N = dst LSS 0;
//   Z = dst EQL 0;
//   V = 0;
//   C = C;
//
// Exception:
//   None
//
// Opcode:
//   9C  ROTL  Rotate Long
//
// Operand Registers:
//   cnt.rb   OP0 = value of operand
//   src.rl   OP1 = value of operand
//   dst.wl   OP2 = register/memory flag
//            OP3 = memory address

void vax_Opcode_ROTL(void)
{
	int8   cnt = OP0 % 32;
	uint32 src = OP1;
	int32  dst;

	// Do 32-bit rotate
	if (cnt == 0) {
		dst = src;
	} else {
		if (cnt < 0) {
			// Rotate right
			cnt = -cnt;
			dst = (src >> cnt) | (src << (32 - cnt));
		} else {
			// Rotate left
			dst = (src << cnt) | (src >> (32 - cnt));
		}
	}

	// Write result back
	if (OP2 >= 0)
		RN(OP2) = dst;
	else
		vax11mem_vWrite(OP3, (uint8 *)&dst, OP_LONG);

	// Set condition status bits
	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.v = 0;

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		dbg_Printf("VAX(ROTL): %08X %s %d (%d) => %08X Condition: %s\n",
			src, ((cnt < 0) ? ">>" : "<<"), abs(cnt)%32, abs(cnt), dst,
			vax_DisplayCondition());
	}
#endif DEBUG
}

// SBWC  Subtract With Carry
//
// Format:
//   opcode sub.rl, dif.ml
//
// Operation:
//   dif <- dif - sub - C;
//
// Condition Codes:
//   N <- sum LSS 0;
//   Z <- sum EQL 0;
//   V <- {integer overflow};
//   C <- {borrow into most signficant bit};
//
// Exception:
//   Integer Overflow Trap
//
// Opcode:
//   D9  SBWC  Add With Carry
//
// Operand Registers:
//   sub.rl   OP0 = value of operand
//   dif.rl   OP1 = value of operand
//   dif.wl   OP2 = register/memory flag
//            OP3 = memory address

void vax_Opcode_SBWC(void)
{
	int32 sub = OP0;
	int32 min = OP1;
	int32 dif;

	dif = min - sub - PSB.c;
	if (OP2 >= 0)
		RN(OP2) = dif;
	else
		vax11mem_vWrite(OP3, (uint8 *)&dif, OP_LONG);

	if (((sub ^ min) & (~sub ^ dif)) & LSIGN) {
		PSB.v = 1;
		if (PSW & PSW_IV)
			SET_TRAP(TRAP_INFOVF);
	} else
		PSB.v = 0;

	PSB.n = (dif < 0);
	PSB.z = (dif == 0);
	PSB.c = ((uint32)min < (uint32)sub);
	if ((sub == min) && dif)
		PSB.c = 1;

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		dbg_Printf("VAX(SBWC): %08X - %08X => %08X Condition: %s\n",
			min, sub, dif, vax_DisplayCondition());
	}
#endif DEBUG
}

// SUB  Subtract
//
// Format:
//   opcode sub.rx, dif.mx           2 operands
//   opcode sub.rx, min.rx, dif.wx   3 operands
//
// Operation:
//   dif <- dif - sub;  !2 operands
//   dif <- min - sub;  !3 operands
//
// Condition Codes:
//   N <- dif LSS 0;
//   Z <- dif EQL 0;
//   V <- {integer overflow};
//   C <- {borrow into most significant bit};
//
// Exception:
//   Integer Overflow Trap
//
// Opcodes:
//   82  SUBB2   Subtract Byte 2 Operand
//   83  SUBB3   Subtract Byte 3 Operand
//   A2  SUBW2   Subtract Word 2 Operand
//   A3  SUBW3   Subtract Word 3 Operand
//   C2  SUBL2   Subtract Long 2 Operand
//   C3  SUBL3   Subtract Long 3 Operand
//
// Operand Registers:
//   2 Operand opcode
//     sub.rx   OP0 = value of operand
//     min.rx   OP1 = value of operand
//     min.wx   OP2 = register/memory flag
//              OP3 = memory address
//
//   3 Operand opcode
//     sub.rx   OP0 = value of operand
//     min.rx   OP1 = value of operand
//     dif.wx   OP2 = register/memory flag
//              OP3 = memory address

void vax_Opcode_SUBB(void)
{
	int8 sub = OP0;
	int8 min = OP1;
	int8 dif;

	dif = min - sub;
	if (OP2 >= 0)
		RN(OP2) = (RN(OP2) & ~BMASK) | (dif & BMASK);
	else
		vax11mem_vbWrite(OP3, dif);

	if (((sub ^ min) & (~sub ^ dif)) & BSIGN) {
		PSB.v = 1;
		if (PSW & PSW_IV)
			SET_TRAP(TRAP_INFOVF);
	} else
		PSB.v = 0;

	PSB.n = (dif < 0);
	PSB.z = (dif == 0);
	PSB.c = ((uint8)min < (uint8)sub);

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		dbg_Printf("VAX(SUBB): %02X - %02X => %02X Condition: %s\n",
			min, sub, dif, vax_DisplayCondition());
	}
#endif DEBUG
}

void vax_Opcode_SUBW(void)
{
	int16 sub = OP0;
	int16 min = OP1;
	int16 dif;

	dif = min - sub;
	if (OP2 >= 0)
		RN(OP2) = (RN(OP2) & ~WMASK) | (dif & WMASK);
	else
		vax11mem_vWrite(OP3, (uint8 *)&dif, OP_WORD);

	if (((sub ^ min) & (~sub ^ dif)) & WSIGN) {
		PSB.v = 1;
		if (PSW & PSW_IV)
			SET_TRAP(TRAP_INFOVF);
	} else
		PSB.v = 0;

	PSB.n = (dif < 0);
	PSB.z = (dif == 0);
	PSB.c = ((uint16)min < (uint16)sub);

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		dbg_Printf("VAX(SUBW): %04X - %04X => %04X Condition: %s\n",
			min, sub, dif, vax_DisplayCondition());
	}
#endif DEBUG
}

void vax_Opcode_SUBL(void)
{
	int32 sub = OP0;
	int32 min = OP1;
	int32 dif;

	dif = min - sub;
	if (OP2 >= 0)
		RN(OP2) = dif;
	else
		vax11mem_vWrite(OP3, (uint8 *)&dif, OP_LONG);

	if (((sub ^ min) & (~sub ^ dif)) & LSIGN) {
		PSB.v = 1;
		if (PSW & PSW_IV)
			SET_TRAP(TRAP_INFOVF);
	} else
		PSB.v = 0;

	PSB.n = (dif < 0);
	PSB.z = (dif == 0);
	PSB.c = ((uint32)min < (uint32)sub);

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		dbg_Printf("VAX(SUBL): %08X - %08X => %08X Condition: %s\n",
			min, sub, dif, vax_DisplayCondition());
	}
#endif DEBUG
}

// **********************************************************
// ******************* XOR  Exclusive-OR ********************
// **********************************************************

void vax_Opcode_XORB(void)
{
	int8 mask = OP0;
	int8 src  = OP1;
	int8 dst;

	dst = src ^ mask;
	if (OP2 >= 0)
		RN(OP2) = (RN(OP2) & ~BMASK) | (dst & BMASK);
	else
		vax11mem_vbWrite(OP3, dst);

	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.v = 0;

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		dbg_Printf("XORB: %02X XOR %02X => %02X Condition: %s\n",
			src, mask, dst, vax_DisplayCondition());
	}
#endif DEBUG
}

void vax_Opcode_XORW(void)
{
	int16 mask = OP0;
	int16 src  = OP1;
	int16 dst;

	dst = src ^ mask;
	if (OP2 >= 0)
		RN(OP2) = (RN(OP2) & ~WMASK) | (dst & WMASK);
	else
		vax11mem_vWrite(OP3, (uint8 *)&dst, OP_WORD);

	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.v = 0;

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		dbg_Printf("XORW: %04X XOR %04X => %04X Condition: %s\n",
			src, mask, dst, vax_DisplayCondition());
	}
#endif DEBUG
}

void vax_Opcode_XORL(void)
{
	int32 mask = OP0;
	int32 src  = OP1;
	int32 dst;

	dst = src ^ mask;
	if (OP2 >= 0)
		RN(OP2) = dst;
	else
		vax11mem_vWrite(OP3, (uint8 *)&dst, OP_LONG);

	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.v = 0;

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		dbg_Printf("XORL: %08X XOR %08X => %08X Condition: %s\n",
			src, mask, dst, vax_DisplayCondition());
	}
#endif DEBUG
}

