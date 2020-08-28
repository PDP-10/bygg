// cpu_call.c - Procedure Call Instructions
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

// Procedure Call Stack Frame
//
// +----------------------------------------------------+
// |            Condition Handler (initially 0)         | :(FP)
// +---+-+-+-----------------+-+--------------+---------+
// |SPA|S|0|    Mask<11:0>   |Z|   PSW<14:5>  | 0 0 0 0 |
// +---+-+-+-----------------+-+--------------+---------+
// |                      saved AP                      |
// +----------------------------------------------------+
// |                      saved FP                      |
// +----------------------------------------------------+
// |                      saved PC                      |
// +----------------------------------------------------+
// |                    saved R0 (...)                  |
// +----------------------------------------------------+
//              :                         :
//              :                         :
// +----------------------------------------------------+
// |                    saved R11 (...)                 |
// +----------------------------------------------------+
//
// (0 to 3 bytes specified by SPA, Stack Pointer Alignment)
// S = Set if CALLS; clear if CALLG.
// Z = Always cleared by CALL.  Can be set by software
//     force a reserved operand fault on a RET.

// Procdure Entry Mask
//
//  15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |DV|IV| MBZ |           Registers               |
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

// Entry Mask definitions
#define CALL_DV   0x8000
#define CALL_IV   0x4000
#define CALL_MBZ  0x3000
#define CALL_MASK 0x0FFF

// Call Stack Frame definitions (2nd longword)
#define CALL_SPA    0x03
#define CALL_S      (1 << CALL_P_S)
#define CALL_PSW    0x7FE0

#define CALL_P_SPA  30
#define CALL_P_S    29
#define CALL_P_MASK 16

void vax_Call(int32 *opnd, int flgStack)
{
	int32 pcAddr = opnd[1];
	int32 mask, tSP;
	int32 idx, tmp, zero = 0;

	vax11mem_vRead(pcAddr, (uint8 *)&mask, OP_WORD);
	if (mask & CALL_MBZ)
		RSRVD_OPND_FAULT;

	if (flgStack) {
		vax11mem_vWrite(SP - 4, (uint8 *)&opnd[0], OP_LONG);
		SP -= 4;
	}

	tmp = ((SP & CALL_SPA) << CALL_P_SPA) |
	      (flgStack << CALL_P_S) | (PSL & CALL_PSW) |
			((mask & CALL_MASK) << CALL_P_MASK);
	tSP = SP & ~CALL_SPA;
	for (idx = 11; idx > 0; idx--) {
		if ((mask >> idx) & 1) {
			vax11mem_vWrite(tSP - OP_LONG, (uint8 *)&RN(idx), OP_LONG);
			tSP -= OP_LONG;
		}
	}
	vax11mem_vWrite(tSP - 4,  (uint8 *)&PC, OP_LONG);
	vax11mem_vWrite(tSP - 8,  (uint8 *)&FP, OP_LONG);
	vax11mem_vWrite(tSP - 12, (uint8 *)&AP, OP_LONG);
	vax11mem_vWrite(tSP - 16, (uint8 *)&tmp, OP_LONG);
	vax11mem_vWrite(tSP - 20, (uint8 *)&zero, OP_LONG);

	AP = flgStack ? SP : opnd[0];
	SP = FP = tSP - 20;
	PC = pcAddr + OP_WORD;
	PSL = (PSL & ~(PSW_DV|PSW_FU|PSW_IV)) |
	      ((mask & CALL_DV) ? PSW_DV : 0) |
	      ((mask & CALL_IV) ? PSW_IV : 0);
}

// CALLG  Call Procedure with General Argument List
//
// Format:
//   opcode arglist.ab, dst.ab
//
// Operation:
//   {align stack};
//   {create stack frame};
//   {set arithmetic exception enables};
//   {set new values of AP, FP, PC};
//
// Condition Codes:
//   N <- 0;
//   Z <- 0;
//   V <- 0;
//   C <- 0;
//
// Exception:
//   Reserved Operand Fault
//
// Opcode
//   FA  CALLG  Call Procedure with General Argument List
//
// Operand Registers:
//   arglist.ab  OP0 = address of operand
//   dst.ab      OP1 = address of operand

void vax_Opcode_CALLG(void)
{
	vax_Call(&OP0, 0);
	PSL &= ~PSW_CC;
}

// CALLS  Call Procedure with Stack Argument List
//
// Format:
//   opcode numarg.rl, dst.ab
//
// Operation:
//   {push arg count};
//   {align stack};
//   {create stack frame};
//   {set arithmetic exception enables};
//   {set new values of AP, FP, PC};
//
// Condition Codes:
//   N <- 0;
//   Z <- 0;
//   V <- 0;
//   C <- 0;
//
// Exception:
//   Reserved Operand Fault
//
// Opcode
//   FB  CALLS  Call Procedure with Stack Argument List
//
// Operand Registers:
//   numarg.rl  OP0 = value of operand
//   dst.ab     OP1 = address of operand

void vax_Opcode_CALLS(void)
{
	vax_Call(&OP0, 1);
	PSL &= ~PSW_CC;
}

// RET  Return From Subroutine
//
// Format:
//   opcode
//
// Operation:
//   {restore SP from FP};
//   {restore registers};
//   {drop stack alignment};
//   {if CALLS then remove arglist};
//   {restore PSW};
//
// Condition Codes:
//   N <- tmp1<3>;
//   Z <- tmp1<2>;
//   V <- tmp1<1>;
//   C <- tmp1<0>;
//
// Exception:
//   Reserved Operand Fault
//
// Opcode:
//   04  RET  Return From Procedure
//
// Operand Register
//   None

void vax_Opcode_RET(void)
{
	int32 tSP = FP;
	int32 newPC, mask;
	int32 idx, tmp;

	// Restore PC, FP, AP, and SP registers from stack
	vax11mem_vRead(tSP+4, (uint8 *)&mask, OP_LONG);
	if (mask & PSW_MBZ)
		RSRVD_OPND_FAULT;
	vax11mem_vRead(tSP+8, (uint8 *)&AP, OP_LONG);
	vax11mem_vRead(tSP+12, (uint8 *)&FP, OP_LONG);
	vax11mem_vRead(tSP+16, (uint8 *)&newPC, OP_LONG);
	tSP += 20;

	// Restore specific registers from stack
	tmp = mask >> CALL_P_MASK;
	for (idx = 0; idx < 11; idx++) {
		if ((tmp >> idx) & 1) {
			vax11mem_vRead(tSP, (uint8 *)&RN(idx), OP_LONG);
			tSP += OP_LONG;
		}
	}

	// Dealign stack pointer
	SP = tSP + ((mask >> CALL_P_SPA) & CALL_SPA);

	// Pop old argument list if CALLS is used.
	if (mask & CALL_S) {
		vax11mem_vRead(SP, (uint8 *)&tmp, OP_LONG);
		SP += ((tmp & BMASK) << 2) + 4;
	}

	// Reset PSW bits
	PSL = (PSL & ~PSW_MASK) | (mask & PSW_MASK);
	PC  = newPC;
}
