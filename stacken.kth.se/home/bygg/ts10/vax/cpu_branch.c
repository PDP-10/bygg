// cpu_branch.c - VAX Branch Instructions
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

// *******************************************************
// *********** Condition branch instructions *************
// *******************************************************

void vax_Opcode_AOBLEQ(void)
{
	int32 limit = OP0;
	int32 index = OP1;
#ifdef DEBUG
	int32 old_PC = PC;
#endif DEBUG
	boolean flgov;

	index++;
	if (OP2 >= 0)
		RN(OP2) = index;
	else
		vax11mem_vWrite(OP3, (uint8 *)&index, OP_LONG);

	if (index == LMIN) {
		flgov = 1;
		if (PSW & PSW_IV)
			SET_TRAP(TRAP_INFOVF);
	} else
		flgov = 0;

#ifdef DEBUG
	if (index <= limit) {
		PC += SXTB(vax11_brDisp);
		if (dbg_Check(DBG_TRACE|DBG_DATA)) {
			dbg_Printf("AOBLEQ: PC %08X + %02X => %08X\n",
				old_PC, vax11_brDisp, PC);
			dbg_Printf("AOBLEQ: Jump into location %08X\n", PC);
		}
	}
#else DEBUG
	if (index <= limit)
		PC += SXTB(vax11_brDisp);
#endif DEBUG

	PSB.n = (index < 0);
	PSB.z = (index == 0);
	PSB.v = flgov;
}

void vax_Opcode_AOBLSS(void)
{
	int32 limit = OP0;
	int32 index = OP1;
#ifdef DEBUG
	int32 old_PC = PC;
#endif DEBUG
	boolean flgov;

	index++;
	if (OP2 >= 0)
		RN(OP2) = index;
	else
		vax11mem_vWrite(OP3, (uint8 *)&index, OP_LONG);

	if (index == LMIN) {
		flgov = 1;
		if (PSW & PSW_IV)
			SET_TRAP(TRAP_INFOVF);
	} else
		flgov = 0;

#ifdef DEBUG
	if (index < limit) {
		PC += SXTB(vax11_brDisp);
		if (dbg_Check(DBG_TRACE|DBG_DATA)) {
			dbg_Printf("AOBLSS: PC %08X + %02X => %08X\n",
				old_PC, vax11_brDisp, PC);
			dbg_Printf("AOBLSS: Jump into location %08X\n", PC);
		}
	}
#else DEBUG
	if (index < limit)
		PC += SXTB(vax11_brDisp);
#endif DEBUG

	PSB.n = (index < 0);
	PSB.z = (index == 0);
	PSB.v = flgov;
}

// Bcc - Branch on (condition)
//
// Format:
//   opcode displ.bb
//
// Operation:
//   if condition then PC <- PC + SEXT(displ);
//
// Condition Codes:
//   None affected.
//
// Expections:
//   None
//
// Opcodes: Condition
//   12  Z EQL 0         BNEQ  Branch on Not Equal (signed)
//   12  Z EQL 0         BNEQU Branch on Not Equal Unsigned
//   13  Z EQL 1         BEQL  Branch on Equal (signed)
//   13  Z EQL 1         BEQLU Branch on Equal Unsigned
//   14  (N OR Z) EQL 0  BGTR  Branch on Greater Than (signed)
//   15  (N OR Z) EQL 1  BLEQ  Branch on Less Than or Equal (signed)
//   18  N EQL 0         BGEQ  Branch on Greater Than or Equal (signed)
//   19  N EQL 1         BLSS  Branch on Less Than (signed)
//   1A  (C OR Z) EQL 0  BGTRU Branch on Greater Than Unsigned
//   1B  (C OR Z) EQL 1  BLEQU Branch on Less Than or Equal Unsigned
//   1C  V EQL 0         BVC   Branch on Overflow Clear
//   1D  V EQL 1         BVS   Branch on Overflow Set
//   1E  C EQL 0         BGEQU Branch on Greater Than or Equal Unsigned
//   1E  C EQL 0         BCC   Branch on Carry Clear
//   1F  C EQL 1         BLSSU Branch on Less Than Unsigned
//   1F  C EQL 1         BCS   Branch on Carry Set

void vax_Opcode_BGTR(void)
{
#ifdef DEBUG
	int old_pc = PC;

	if ((PSL & (PSW_N|PSW_Z)) == 0) {
		PC += SXTB(vax11_brDisp);
		if (dbg_Check(DBG_TRACE|DBG_DATA)) {
			dbg_Printf("VAX(CPU): PC %08X + %02X => %08X\n",
				old_pc, vax11_brDisp, PC);
			dbg_Printf("VAX(CPU): Jump into location %08X\n", PC);
		}
	}

#else DEBUG
	if ((PSL & (PSW_N|PSW_Z)) == 0)
		PC += SXTB(vax11_brDisp);
#endif DEBUG
}

void vax_Opcode_BLEQ(void)
{
#ifdef DEBUG
	int old_pc = PC;

	if (PSL & (PSW_N|PSW_Z)) {
		PC += SXTB(vax11_brDisp);
		if (dbg_Check(DBG_TRACE|DBG_DATA)) {
			dbg_Printf("VAX(CPU): PC %08X + %02X => %08X\n",
				old_pc, vax11_brDisp, PC);
			dbg_Printf("VAX(CPU): Jump into location %08X\n", PC);
		}
	}

#else DEBUG
	if (PSL & (PSW_N|PSW_Z))
		PC += SXTB(vax11_brDisp);
#endif DEBUG
}

void vax_Opcode_BNEQ(void)
{
#ifdef DEBUG
	int old_pc = PC;

	if ((PSL & PSW_Z) == 0) {
		PC += SXTB(vax11_brDisp);
		if (dbg_Check(DBG_TRACE|DBG_DATA)) {
			dbg_Printf("VAX(CPU): PC %08X + %02X => %08X\n",
				old_pc, vax11_brDisp, PC);
			dbg_Printf("VAX(CPU): Jump into location %08X\n", PC);
		}
	}

#else DEBUG
	if ((PSL & PSW_Z) == 0)
		PC += SXTB(vax11_brDisp);
#endif DEBUG
}

void vax_Opcode_BEQL(void)
{
#ifdef DEBUG
	int old_pc = PC;

	if (PSL & PSW_Z) {
		PC += SXTB(vax11_brDisp);
		if (dbg_Check(DBG_TRACE|DBG_DATA)) {
			dbg_Printf("VAX(CPU): PC %08X + %02X => %08X\n",
				old_pc, vax11_brDisp, PC);
			dbg_Printf("VAX(CPU): Jump into location %08X\n", PC);
		}
	}

#else DEBUG
	if (PSL & PSW_Z)
		PC += SXTB(vax11_brDisp);
#endif DEBUG
}

void vax_Opcode_BGEQ(void)
{
#ifdef DEBUG
	int old_pc = PC;

	if ((PSL & PSW_N) == 0) {
		PC += SXTB(vax11_brDisp);
		if (dbg_Check(DBG_TRACE|DBG_DATA)) {
			dbg_Printf("VAX(CPU): PC %08X + %02X => %08X\n",
				old_pc, vax11_brDisp, PC);
			dbg_Printf("VAX(CPU): Jump into location %08X\n", PC);
		}
	}

#else DEBUG
	if ((PSL & PSW_N) == 0)
		PC += SXTB(vax11_brDisp);
#endif DEBUG
}

void vax_Opcode_BLSS(void)
{
#ifdef DEBUG
	int old_pc = PC;

	if (PSL & PSW_N) {
		PC += SXTB(vax11_brDisp);
		if (dbg_Check(DBG_TRACE|DBG_DATA)) {
			dbg_Printf("VAX(CPU): PC %08X + %02X => %08X\n",
				old_pc, vax11_brDisp, PC);
			dbg_Printf("VAX(CPU): Jump into location %08X\n", PC);
		}
	}

#else DEBUG
	if (PSL & PSW_N)
		PC += SXTB(vax11_brDisp);
#endif DEBUG
}

void vax_Opcode_BGTRU(void)
{
#ifdef DEBUG
	int old_pc = PC;

	if ((PSL & (PSW_C|PSW_Z)) == 0) {
		PC += SXTB(vax11_brDisp);
		if (dbg_Check(DBG_TRACE|DBG_DATA)) {
			dbg_Printf("VAX(CPU): PC %08X + %02X => %08X\n",
				old_pc, vax11_brDisp, PC);
			dbg_Printf("VAX(CPU): Jump into location %08X\n", PC);
		}
	}

#else DEBUG
	if ((PSL & (PSW_C|PSW_Z)) == 0)
		PC += SXTB(vax11_brDisp);
#endif DEBUG
}

void vax_Opcode_BLEQU(void)
{
#ifdef DEBUG
	int old_pc = PC;

	if (PSL & (PSW_C|PSW_Z)) {
		PC += SXTB(vax11_brDisp);
		if (dbg_Check(DBG_TRACE|DBG_DATA)) {
			dbg_Printf("VAX(CPU): PC %08X + %02X => %08X\n",
				old_pc, vax11_brDisp, PC);
			dbg_Printf("VAX(CPU): Jump into location %08X\n", PC);
		}
	}

#else DEBUG
	if (PSL & (PSW_C|PSW_Z))
		PC += SXTB(vax11_brDisp);
#endif DEBUG
}

void vax_Opcode_BVC(void)
{
#ifdef DEBUG
	int old_pc = PC;

	if ((PSL & PSW_V) == 0) {
		PC += SXTB(vax11_brDisp);
		if (dbg_Check(DBG_TRACE|DBG_DATA)) {
			dbg_Printf("VAX(CPU): PC %08X + %02X => %08X\n",
				old_pc, vax11_brDisp, PC);
			dbg_Printf("VAX(CPU): Jump into location %08X\n", PC);
		}
	}

#else DEBUG
	if ((PSL & PSW_V) == 0)
		PC += SXTB(vax11_brDisp);
#endif DEBUG
}

void vax_Opcode_BVS(void)
{
#ifdef DEBUG
	int old_pc = PC;

	if (PSL & PSW_V) {
		PC += SXTB(vax11_brDisp);
		if (dbg_Check(DBG_TRACE|DBG_DATA)) {
			dbg_Printf("VAX(CPU): PC %08X + %02X => %08X\n",
				old_pc, vax11_brDisp, PC);
			dbg_Printf("VAX(CPU): Jump into location %08X\n", PC);
		}
	}

#else DEBUG
	if (PSL & PSW_V)
		PC += SXTB(vax11_brDisp);
#endif DEBUG
}

void vax_Opcode_BCC(void)
{
#ifdef DEBUG
	int old_pc = PC;

	if ((PSL & PSW_C) == 0) {
		PC += SXTB(vax11_brDisp);
		if (dbg_Check(DBG_TRACE|DBG_DATA)) {
			dbg_Printf("VAX(CPU): PC %08X + %02X => %08X\n",
				old_pc, vax11_brDisp, PC);
			dbg_Printf("VAX(CPU): Jump into location %08X\n", PC);
		}
	}

#else DEBUG
	if ((PSL & PSW_C) == 0)
		PC += SXTB(vax11_brDisp);
#endif DEBUG
}

void vax_Opcode_BCS(void)
{
#ifdef DEBUG
	int old_pc = PC;

	if (PSL & PSW_C) {
		PC += SXTB(vax11_brDisp);
		if (dbg_Check(DBG_TRACE|DBG_DATA)) {
			dbg_Printf("VAX(CPU): PC %08X + %02X => %08X\n",
				old_pc, vax11_brDisp, PC);
			dbg_Printf("VAX(CPU): Jump into location %08X\n", PC);
		}
	}

#else DEBUG
	if (PSL & PSW_C)
		PC += SXTB(vax11_brDisp);
#endif DEBUG
}

// BB - Branch on Bit
//
// Format:
//   opcode pos.rl, base.vb, displ.bb
//
// Operation:
//   teststate = if {BBS} then 1 else 0;
//   if FIELD(pos, 1, base) EQL teststate then
//     PC = PC + SEXT(displ);
//
// Condtion Codes:
//   None
//
// Execption:
//   Reserved Operand Fault
//
// Opcodes:
//   E0  BBS  Branch on Bit Set
//   E1  BBC  Branch on Bit Clear
//
// Operand Registers:
//   pos.rl       OP0 = value of operand
//   base.vb      OP1 = register/memory flag
//                OP2 = content/memory address
//   displ.bb

int vax_TestBit(int32 *opnd)
{
	uint32 pos  = opnd[0];
	int32  nReg = opnd[1];
	uint32 src;

	if (nReg >= 0) {
		if (pos > 31)
			RSRVD_OPND_FAULT;
		src = RN(nReg);
	} else {
		src = 0;
		vax11mem_vbRead(opnd[2] + (pos >> 3), (uint8 *)&src);
		pos &= 0x07;
	}

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA))
		dbg_Printf("BBx: %08X<%d> => %d\n", src, pos, (src >> pos) & 1);
#endif DEBUG

	return (src >> pos) & 1;
}

void vax_Opcode_BBS(void)
{
	if (vax_TestBit(&OP0))
		PC += SXTB(vax11_brDisp);
}

void vax_Opcode_BBC(void)
{
	if (vax_TestBit(&OP0) == 0)
		PC += SXTB(vax11_brDisp);
}

// BB - Branch on Bit (and modify without Interlock)
//
// Format:
//   opcode pos.rl, base.vb, displ.bb
//
// Operation:
//   teststate = if {BBSS or BBSC} then 1 else 0;
//   newstate = if {BBSS or BBCS} then 1 else 0;
//   tmp = FIELD(pos, 1, base);
//   FIELD(pos, 1, base) = newstate;
//   if tmp EQL teststate then
//     PC = PC + SEXT(displ);
//
// Condition Codes:
//   None
//
// Execption:
//   Reserved Operand Fault
//
// Opcodes:
//   E2  BBSS  Branch on Bit Set and Set
//   E3  BBCS  Branch on Bit Clear and Set
//   E4  BBSC  Branch on Bit Set and Clear
//   E5  BBCC  Branch on Bit Clear and Clear
//
// Operand Registers:
//   pos.rl       OP0 = value of operand
//   base.vb      OP1 = register/memory flag
//                OP2 = content/memory address
//   displ.bb

int vax_SetBit(int32 *opnd, int32 newBit)
{
	uint32 Pos  = opnd[0];
	int32  nReg = opnd[1];
	int32  eAddr;
	uint8  curBit;
	int    oldBit;

	if (nReg >= 0) {
		if (Pos > 31)
			RSRVD_OPND_FAULT;
		oldBit = (RN(nReg) >> Pos) & 1;
		RN(nReg) = newBit ? (RN(nReg) | (1 << Pos)) :
		                    (RN(nReg) & ~(1 << Pos));
	} else {
		eAddr = opnd[2] + (Pos >> 3);
		Pos &= 0x07;
		vax11mem_vbRead(eAddr, &curBit);
		oldBit = (curBit >> Pos) & 1;
		curBit = newBit ? (curBit | (1 << Pos)) :
		                  (curBit & ~(1 << Pos));
		vax11mem_vbWrite(eAddr, curBit);
	}

	return oldBit;
}

void vax_Opcode_BBCC(void)
{
	if (vax_SetBit(&OP0, 0) == 0)
		PC += SXTB(vax11_brDisp);
}

void vax_Opcode_BBCS(void)
{
	if (vax_SetBit(&OP0, 1) == 0)
		PC += SXTB(vax11_brDisp);
}

void vax_Opcode_BBSC(void)
{
	if (vax_SetBit(&OP0, 0))
		PC += SXTB(vax11_brDisp);
}

void vax_Opcode_BBSS(void)
{
	if (vax_SetBit(&OP0, 1))
		PC += SXTB(vax11_brDisp);
}

// BLB  Branch on Low Bit
//
// Format:
//   opcode src.rl, displ.bb
//
// Operation:
//   teststate = if {BLBS} then 1 else 0;
//   if src<0> EQL teststate then
//     PC = PC + SEXT(displ);
//
// Condition Codes:
//   None
//
// Execptions:
//   None
//
// Opcodes:
//   E8  BLBS  Branch on Low Bit Set
//   E9  BLBC  Branch on Low Bit Clear
//
// Operand Registers:
//   src.rl    OP0 = value of operand
//   displ.bb

void vax_Opcode_BLBS(void)
{
	int32 src = OP0;

	if (src & 1)
		PC += SXTB(vax11_brDisp);
}

void vax_Opcode_BLBC(void)
{
	int32 src = OP0;

	if (!(src & 1))
		PC += SXTB(vax11_brDisp);
}

// CASE  Case
//
// Format:
//   opcode selector.rx, base.rx, limit.rx,
//     displ[0].bw, .... ,displ[limit].bw
//
// Operation:
//   tmp = selector - base;
//   PC = PC + if tmp LEQU limit then
//     SEXT(displ[tmp])
//   else
//     {2 + 2 * ZEXT(limit)};
//
// Condition Codes:
//   N = tmp LSS limit;
//   Z = tmp EQL limit;
//   V = 0;
//   C = tmp LSSU limit;
//
// Exceptions:
//   None
//
// Opcodes:
//   8F  CASEB  Case Byte
//   AF  CASEW  Case Word
//   CF  CASEL  Case Long
//
// Operand Registers:
//   selctor.rx   OP0 = value of operand
//   base.rx      OP1 = value of operand
//   limit.rx     OP2 = value of operand

void vax_Opcode_CASEB(void)
{
	uint8  selector = OP0;
	uint8  base     = OP1;
	uint8  limit    = OP2;
	uint32 tmp      = selector - base;
	int16  disp;

	if (tmp <= limit) {
		vax11mem_vRead(PC + (tmp << 1), (uint8 *)&disp, OP_WORD);
		PC += disp;
	} else
		PC += (limit << 1) + 2;

	PSB.n = ((signed)tmp < (signed)limit);
	PSB.z = ((signed)tmp == (signed)limit);
	PSB.v = 0;
	PSB.c = (tmp < limit);
}

void vax_Opcode_CASEW(void)
{
	uint16 selector = OP0;
	uint16 base     = OP1;
	uint16 limit    = OP2;
	uint32 tmp      = selector - base;
	int16  disp;

	if (tmp <= limit) {
		vax11mem_vRead(PC + (tmp << 1), (uint8 *)&disp, OP_WORD);
		PC += disp;
	} else
		PC += (limit << 1) + 2;

	PSB.n = ((signed)tmp < (signed)limit);
	PSB.z = ((signed)tmp == (signed)limit);
	PSB.v = 0;
	PSB.c = (tmp < limit);
}

void vax_Opcode_CASEL(void)
{
	uint32 selector = OP0;
	uint32 base     = OP1;
	uint32 limit    = OP2;
	uint32 tmp      = selector - base;
	int16  disp;

	if (tmp <= limit) {
		vax11mem_vRead(PC + (tmp << 1), (uint8 *)&disp, OP_WORD);
		PC += disp;
	} else
		PC += (limit << 1) + 2;

	PSB.n = ((signed)tmp < (signed)limit);
	PSB.z = ((signed)tmp == (signed)limit);
	PSB.v = 0;
	PSB.c = (tmp < limit);
}

// *******************************************************
// **************** Branch instructions ******************
// *******************************************************

// BR - Branch
//
// Format:
//   opcode displ.bx
//
// Operation:
//   PC <- PC + SEXT(displ);
//
// Conditions:
//   None affected.
//
// Expections:
//   None
//
// Opcodes:
//   11  BRB  Branch With Byte Displacement
//   31  BRW  Branch With Word Displacement

void vax_Opcode_BRB(void)
{
#ifdef DEBUG
	int old_pc = PC;
#endif DEBUG

	PC += SXTB(vax11_brDisp);

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		dbg_Printf("VAX(CPU): PC %08X + %02X => %08X\n",
			old_pc, vax11_brDisp, PC);
		dbg_Printf("VAX(CPU): Jump into location %08X\n", PC);
	}
#endif DEBUG
}

void vax_Opcode_BRW(void)
{
#ifdef DEBUG
	int old_pc = PC;
#endif DEBUG

	PC += SXTW(vax11_brDisp);

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		dbg_Printf("VAX(CPU): PC %08X + %04X => %08X\n",
			old_pc, vax11_brDisp, PC);
		dbg_Printf("VAX(CPU): Jump into location %08X\n", PC);
	}
#endif DEBUG
}

// JMP - Jump
//
// Format:
//   opcode dst.ab
//
// Operation:
//   PC <- dst
//
// Condition Codes:
//   None affected.
//
// Expections:
//   None
//
// Opcode:
//   17  JMP  Jump
//
// Description:
//   PC is replaced by the destination operand.

void vax_Opcode_JMP(void)
{
	PC = OP0;

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA))
		dbg_Printf("VAX(CPU): Jump into location %08X\n", PC);
#endif DEBUG
}

// *******************************************************
// ************** Subroutine instructions ****************
// *******************************************************

// BSB - Branch to Subroutine
//
// Format:
//   opcode displ.bx
//
// Operation:
//   -(SP) <- PC;
//   PC <- PC + SEXT(displ);
//
// Condition Codes:
//   None affected.
//
// Execptions:
//   None
//
// Opcodes:
//   10  BSBB  Branch to Subroutine With Byte Displacement
//   30  BSBW  Branch to Subroutine With Word Displacement

void vax_Opcode_BSBB(void)
{
#ifdef DEBUG
	int old_pc = PC;
#endif DEBUG

	vax11mem_vWrite(SP - 4, (uint8 *)&PC, 4);
	SP -= 4;
	PC += SXTB(vax11_brDisp);

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		dbg_Printf("VAX(CPU): PC %08X + %02X => %08X\n",
			old_pc, vax11_brDisp, PC);
		dbg_Printf("VAX(CPU): Subroutine into location %08X\n", PC);
	}
#endif DEBUG
}

void vax_Opcode_BSBW(void)
{
#ifdef DEBUG
	int old_pc = PC;
#endif DEBUG

	vax11mem_vWrite(SP - 4, (uint8 *)&PC, 4);
	SP -= 4;
	PC += SXTW(vax11_brDisp);

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA)) {
		dbg_Printf("VAX(CPU): PC %08X + %04X => %08X\n",
			old_pc, vax11_brDisp, PC);
		dbg_Printf("VAX(CPU): Subroutine into location %08X\n", PC);
	}
#endif DEBUG
}

// JSB - Jump to Subroutine
//
// Format:
//   opcode dst.ab
//
// Operations:
//   -(SP) <- PC;
//   PC <- dst;
//
// Condition Codes:
//   None affected.
//
// Exceptions:
//   None
//
// Opcodes:
//   16  JSB  Jump to Subroutine

void vax_Opcode_JSB(void)
{
	vax11mem_vWrite(SP - 4, (uint8 *)&PC, 4);
	SP -= 4;
	PC = OP0;
}

// RSB - Return from Subroutine
//
// Format:
//   opcode
//
// Operations:
//   PC <- (SP)+;
//
// Condition Codes:
//   None affected.
//
// Execptions:
//   None
//
// Opcodes:
//   05  RSB  Return From Subroutine

void vax_Opcode_RSB(void)
{
	vax11mem_vRead(SP, (uint8 *)&PC, 4);
	SP += 4;
}

void vax_Opcode_SOBGEQ(void)
{
	int32 index = OP0;
#ifdef DEBUG
	int32 old_PC = PC;
#endif DEBUG
	boolean flgov;

	index++;
	if (OP1 >= 0)
		RN(OP1) = index;
	else
		vax11mem_vWrite(OP2, (uint8 *)&index, OP_LONG);

	if (index == LMAX) {
		flgov = 1;
		if (PSW & PSW_IV)
			SET_TRAP(TRAP_INFOVF);
	} else
		flgov = 0;

#ifdef DEBUG
	if (index >= 0) {
		PC += SXTB(vax11_brDisp);
		if (dbg_Check(DBG_TRACE|DBG_DATA)) {
			dbg_Printf("SOBGEQ: PC %08X + %02X => %08X\n",
				old_PC, vax11_brDisp, PC);
			dbg_Printf("SOBGEQ: Jump into location %08X\n", PC);
		}
	}
#else DEBUG
	if (index >= 0)
		PC += SXTB(vax11_brDisp);
#endif DEBUG

	PSB.n = (index < 0);
	PSB.z = (index == 0);
	PSB.v = flgov;
}

void vax_Opcode_SOBGTR(void)
{
	int32 index = OP0;
#ifdef DEBUG
	int32 old_PC = PC;
#endif DEBUG
	boolean flgov;

	index--;
	if (OP1 >= 0)
		RN(OP1) = index;
	else
		vax11mem_vWrite(OP2, (uint8 *)&index, OP_LONG);

	if (index == LMAX) {
		flgov = 1;
		if (PSW & PSW_IV)
			SET_TRAP(TRAP_INFOVF);
	} else
		flgov = 0;

#ifdef DEBUG
	if (index > 0) {
		PC += SXTB(vax11_brDisp);
		if (dbg_Check(DBG_TRACE|DBG_DATA)) {
			dbg_Printf("SOBGTR: PC %08X + %02X => %08X\n",
				old_PC, vax11_brDisp, PC);
			dbg_Printf("SOBGTR: Jump into location %08X\n", PC);
		}
	}
#else DEBUG
	if (index > 0)
		PC += SXTB(vax11_brDisp);
#endif DEBUG

	PSB.n = (index < 0);
	PSB.z = (index == 0);
	PSB.v = flgov;
}
