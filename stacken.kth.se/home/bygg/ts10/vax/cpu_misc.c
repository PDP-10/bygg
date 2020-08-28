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

// BICPSW  Bit Clear PSW
//
// Format:
//   opcode mask.rw
//
// Operation:
//   PSW = PSW AND (NOT mask);
//
// Condition Codes:
//   N = N AND (NOT mask<3>);
//   Z = Z AND (NOT mask<2>);
//   V = V AND (NOT mask<1>);
//   C = C AND (NOT mask<0>);
//
// Exception:
//   Reserved Operand Fault
//
// Opcode:
//   B9  BICPSW  Bit Clear PSW
//
// Operand Register:
//   mask.rw: OP0 = value of operand

void vax_Opcode_BICPSW(void)
{
	int32 mask = OP0 & WMASK;

	if (mask & PSW_MBZ)
		RSRVD_OPND_FAULT;

	// Clear all or any bits with mask bits.
	PSW &= ~mask;
}

// BISPSW  Bit Set PSW
//
// Format:
//   opcode mask.rw
//
// Operation:
//   PSW = PSW OR mask;
//
// Condition Codes:
//   N = N OR mask<3>;
//   Z = Z OR mask<2>;
//   V = V OR mask<1>;
//   C = C OR mask<0>;
//
// Exception:
//   Reserved Operand Fault
//
// Opcode:
//   B8  BISPSW  Bit Clear PSW
//
// Operand Register:
//   mask.rw: OP0 = value of operand

void vax_Opcode_BISPSW(void)
{
	int32 mask = OP0 & WMASK;

	if (mask & PSW_MBZ)
		RSRVD_OPND_FAULT;

	// Set all or any bits with mask bits.
	PSW |= mask;
}

// BPT  Breakpoint
//
// Format:
//   opcode
//
// Operation:
//   PSL<TP> = 0;
//   (initial breakpoint fault);
//
// Condition Codes:
//   N = 0; (Condition codes cleared after BPT fault.)
//   Z = 0;
//   V = 0;
//   C = 0;
//
// Exception:
//   None
//
// Opcode:
//   03  BPT  Breakpoint
//
// Operand Register:
//   None

void vax_Opcode_BPT(void)
{
}

// BUG

void vax_Opcode_BUGL(void)
{
}

void vax_Ocpode_BUGW(void)
{
}

// HALT  Halt
//
// Format:
//   opcode
//
// Operation:
//   If PSL<CUR_MOD> NEQU Kernel then
//     (privileged instruction fault)
//   else
//     (halt the processor);
//
// Condition Codes:
//   N = 0;  When privileged instruction fault occurs,
//   Z = 0;  condition codes are cleared after the fault
//   V = 0;  and PSL is saved on the stack prior the fault.
//   C = 0;
//
//   None    When the processor is halted, all condition
//           codes are not affected.
//
// Exception:
//   Privileged Instruction Fault
//
// Opcode:
//   00  HALT  Halt
//
// Operand Register
//   None

void vax_Opcode_HALT(void)
{
	// If current mode in PSL is not kernel,
	// privileged instruction fault occurs.
	if (PSB.cmod != AM_KERNEL)
		PRIV_INST_FAULT;

	// Return back to console prompt
	vax_Abort(VAX_HALT);
}

// INDEX  Compute Index
//
// Format:
//   opcode subscript.rl, low.rl, high.rl, size.rl,
//     indexin.rl, indexout.wl
//
// Operation:
//   indexout = (indexin * subscript) * size;
//   if (subscript LSS low) or (subscript GTR high) then
//     (subscript range trap);
//
// Condition Codes:
//   N = indexout LSS 0;
//   Z = indexout EQL 0;
//   V = 0;
//   C = 0;
//
// Exception:
//   Subscript Range Trap
//
// Opcode:
//   0A  INDEX  Compute Index
//
// Operand Registers:
//   subscript.rl   OP0 = value of operand
//   low.rl         OP1 = value of operand
//   high.rl        OP2 = value of operand
//   size.rl        OP3 = value of operand
//   indexin.rl     OP4 = value of operand
//   indexout.wl    OP5 = memory/register flag
//                  OP6 = memory address

void vax_Opcode_INDEX(void)
{
	int32 idx;

	// Check subscript for out of range first.
	// If so, initiate subscript range trap.
	if ((OP0 < OP1) || (OP0 > OP2))
		SET_TRAP(TRAP_SUBRNG);

	// Compute Index
 	idx = (OP4 + OP1) * OP3;

	// Write result back to indexout.
	if (OP5 >= 0)
		RN(OP5) = idx;
	else
		vax11mem_vWrite(OP6, (uint8 *)&idx, 4);

	// Set condition flags
	PSB.n = (idx < 0);
	PSB.z = (idx == 0);
	PSB.v = 0;
	PSB.c = 0;
}

// MOVPSL - Move from PSL
//
// Format:
//   opcode dst.wl
//
// Operation:
//   dst <- PSL;
//
// Condition Codes:
//   None
//
// Exceptions:
//   None
//
// Opcode:
//   DC  MOVPSL  Move from PSL
//
// Operand Registers:
//   dst.wl: OP0 = memory/register flag
//           OP1 = memory address

void vax_Opcode_MOVPSL(void)
{
	if (OP0 >= 0)
		RN(OP0) = PSL;
	else
		vax11mem_vWrite(OP1, (uint8 *)&PSL, 4);
}

// NOP  No Operation
//
// Format:
//   opcode
//
// Operation:
//   None
//
// Condition Codes:
//   None
//
// Exception:
//   None
//
// Opcode:
//   01  NOP  No Operation
//
// Operand Register:
//   None

void vax_Opcode_NOP(void)
{
	// Do nothing here...
}

// POPR  Pop Registers
//
// Format:
//   opcode mask.rw
//
// Operation:
//   tmp1 = mask;
//   for tmp2 = 0 step 1 until 14 do
//     if tmp1<tmp2> EQL 1 then
//       R[tmp2] = (SP)+;
//
// Condition Codes:
//   None
//
// Exceptions:
//   None
//
// Opcode:
//   BA  POPR  Pop Registers
//
// Operand Registers:
//   mask.rw  OP0 = value of operand

void vax_Opcode_POPR(void)
{
	int32 mask = OP0 & WMASK;
	int   idx;

	for (idx = 0; idx <= 14; idx++) {
		if ((mask >> idx) & 1) {
			vax11mem_vRead(SP, (uint8 *)&RN(idx), 4);
			SP += 4;
		}
	}
}

// PUSHR  Push Registers
//
// Format:
//   opcode mask.rw
//
// Operation:
//   tmp1 = mask;
//   for tmp2 = 14 step -1 until 0 do
//     if tmp1<tmp2> EQL 1 then
//       -(SP) = R[tmp2];
//
// Condition Codes:
//   None
//
// Exceptions:
//   None
//
// Opcode:
//   BB  PUSHR  Push Registers
//
// Operand Registers:
//   mask.rw  OP0 = value of operand

void vax_Opcode_PUSHR(void)
{
	int32 mask = OP0 & WMASK;
	int   idx;

	for (idx = 14; idx >= 0; idx--) {
		if ((mask >> idx) & 1) {
			vax11mem_vWrite(SP - 4, (uint8 *)&RN(idx), 4);
			SP -= 4;
		}
	}
}

// XFC  Extended Function Call
//
// Format:
//   opcode
//
// Operation:
//   (XFC fault);
//
// Condition Codes:
//   N = 0;
//   Z = 0;
//   V = 0;
//   C = 0;
//
// Exceptions:
//   None
//
// Opcode
//   FC  XFC  Extended Function Call
//
// Operand Register
//   None

void vax_Opcode_XFC(void)
{
	XFC_FAULT;
}

void vax_Opcode_MFPR(void)
{
	int32 preg = OP0;
	int32 dst;

	if (PSL & PSL_CUR)
		RSRVD_INST_FAULT;
	dst = ka780_prRead(preg);

	if (OP1 >= 0)
		RN(OP1) = dst;
	else
		vax11mem_vWrite(OP2, (uint8 *)&dst, OP_LONG);

	PSB.n = (dst < 0);
	PSB.z = (dst == 0);
	PSB.v = 0;
}

void vax_Opcode_MTPR(void)
{
	int32 src  = OP0;
	int32 preg = OP1;

	if (PSL & PSL_CUR)
		RSRVD_INST_FAULT;
	ka780_prWrite(preg, src);

	PSB.n = (src < 0);
	PSB.z = (src == 0);
	PSB.v = 0;
}
