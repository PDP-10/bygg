// cpu_main.c - VAX CPU main routines
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
#include <setjmp.h>

jmp_buf vax_SetJump;
int32   vax11_brDisp; // Branch displacement
int32   fault_PC;
int32   idxScale[] = {1, 2, 4, 8, 16};

void vax_Abort(int32 abort)
{
	longjmp(vax_SetJump, abort);
}

char *vax_DisplayCondition(void)
{
	static char ascFlags[5];

	ascFlags[0] = "-N"[PSB.n];
	ascFlags[1] = "-Z"[PSB.z];
	ascFlags[2] = "-V"[PSB.v];
	ascFlags[3] = "-C"[PSB.c];
	ascFlags[4] = '\0';

	return ascFlags;
}

void vax_DisplayRegisters(void)
{
	printf("OP=%08X %08X %08X %08X %08X %08X %08X %08X\n",
		OP0, OP1, OP2, OP3, OP4, OP5, OP6, OP7);
	printf("R0=%08X %08X %08X %08X %08X %08X %08X %08X\n",
		R0, R1, R2, R3, R4, R5, R6, R7);
	printf("R8=%08X %08X %08X %08X %08X %08X %08X %08X\n",
		R8, R9, R10, R11, AP, FP, SP, PC);
	printf("PSL=%08X\n", PSL);
}

// Operand Decoder
//
// Operands and parsed and placed into operand queues.
//
//    r[bwl]    opRegs[idx]         value of operand
//    rq        opRegs[idx:idx+1]   value of operand
//    a[bwlq]   opRegs[idx]         address of operand
//    m[bwl]    opRegs[idx]         value of operand
//    mq        opRegs[idx:idx+1]   value of operand
//    w[bwlq]   opRegs[idx]         register/memory flag
//              opRegs[idx+1]       memory address

void vax_DecodeOperand(INSTRUCTION *Opcode, int32 *pc)
{
	int   opmode, scale;
	uint8 optype;
	uint8 mode, reg;
	int   idx1, idx2;
	int32 iAddr;
	int32 t1, t2;

	// Reset all operand registers first.
	for (idx1 = 0; idx1 < 8; idx1++)
		OPN(idx1) = 0;

	for (idx1 = 0, idx2 = 0; idx1 < Opcode->nOperands; idx1++) {
		opmode = Opcode->opMode[idx1];
		scale  = opmode & OP_SCALE;

		if (opmode & OP_BRANCH) {
			vax11_brDisp = 0;
			vax11mem_vRead(*pc, (uint8 *)&vax11_brDisp, scale);
			*pc += scale;

			// Get out of the operand decoder routine.
			return;
		}	

		vax11mem_vbRead((*pc)++, &optype);
		mode = optype & OP_MMASK;
		reg  = optype & OP_RMASK;

		t1 = t2 = 0;

		switch (mode) {
			case LIT0: case LIT1: // Short Literal
			case LIT2: case LIT3:
				if (opmode & (OP_VADDR|OP_ADDR|OP_MODIFIED|OP_WRITE))
					RSRVD_ADDR_FAULT;
				if (opmode & OP_FLOAT)
					OPN(idx2++) = 0x4000 | (optype << 4);
				else
					OPN(idx2++) = optype;
				if (scale > OP_LONG)
					OPN(idx2++) = 0;
				continue;

			case REG: // Register
				if (reg >= (nPC - (scale > OP_LONG)))
					RSRVD_ADDR_FAULT;
				if (opmode & OP_ADDR)
					RSRVD_ADDR_FAULT;
				if (opmode & (OP_VADDR|OP_WRITE)) {
					OPN(idx2++) = reg;
					OPN(idx2++) = RN(reg);
				} else {
					if (scale <= OP_LONG)
						OPN(idx2++) = RN(reg);
					else {
						OPN(idx2++) = RN0(reg);
						OPN(idx2++) = RN1(reg);
					}
					if (opmode & OP_MODIFIED)
						OPN(idx2++) = reg;
				}
				continue;

			case ADEC: // Autodecrement
				RN(reg) -= scale;

			case REGD: // Register Deferred
				if (reg == nPC)
					RSRVD_ADDR_FAULT;
				iAddr = RN(reg);
				break;

			case AINC: // Autoincrement or Immediate
				iAddr = RN(reg);
				RN(reg) += scale;
				break;

			case AINCD: // Autoincrement Deferred or Absolute
				vax11mem_vRead(RN(reg), (uint8 *)&iAddr, OP_LONG);
				RN(reg) += OP_LONG;
				break;

			case BDP: // Byte Displacement
				vax11mem_vbRead((*pc)++, (uint8 *)&t1);
				iAddr = RN(reg) + (int8)t1;
				break;

			case BDPD: // Byte Displacement Deferred
				vax11mem_vbRead((*pc)++, (uint8 *)&t1);
				vax11mem_vRead(RN(reg) + (int8)t1, (uint8 *)&iAddr, OP_LONG);
				break;

			case WDP: // Word Displacement
				vax11mem_vRead(*pc, (uint8 *)&t1, OP_WORD);
				*pc += OP_WORD;
				iAddr = RN(reg) + (int16)t1;
				break;

			case WDPD: // Word Displacement Deferred
				vax11mem_vRead(*pc, (uint8 *)&t1, OP_WORD);
				*pc += OP_WORD;
				vax11mem_vRead(RN(reg) + (int16)t1, (uint8 *)&iAddr, OP_LONG);
				break;

			case LDP: // Longword Displacement
				vax11mem_vRead(*pc, (uint8 *)&t1, OP_LONG);
				*pc += OP_LONG;
				iAddr = RN(reg) + t1;
				break;

			case LDPD: // Longword Displacement Deferred
				vax11mem_vRead(*pc, (uint8 *)&t1, OP_LONG);
				*pc += OP_LONG;
				vax11mem_vRead(RN(reg) + t1, (uint8 *)&iAddr, OP_LONG);
				break;

			case IDX: // Indexed
				if (reg == nPC)
					RSRVD_ADDR_FAULT;
				iAddr = RN(reg) * scale;

				vax11mem_vbRead((*pc)++, &optype);
				mode = optype & OP_MMASK;
				reg  = optype & OP_RMASK;

				t1 = t2 = 0;

				switch(mode) {
					case ADEC: // Autodecrement
						RN(reg) -= scale;

					case REGD: // Register Deferred
						if (reg == nPC)
							RSRVD_ADDR_FAULT;
						iAddr += RN(reg);
						break;

					case AINC: // Autoincrement or Immediate
						iAddr += RN(reg);
						RN(reg) += scale;
						break;

					case AINCD: // Autoincrement Deferred or Absolute
						vax11mem_vRead(RN(reg), (uint8 *)&t1, OP_LONG);
						RN(reg) += OP_LONG;
						iAddr += t1;
						break;

					case BDP: // Byte Displacement
						vax11mem_vbRead((*pc)++, (uint8 *)&t1);
						iAddr += RN(reg) + (int8)t1;
						break;

					case BDPD: // Byte Displacement Deferred
						vax11mem_vbRead((*pc)++, (uint8 *)&t1);
						vax11mem_vRead(RN(reg) + (int8)t1, (uint8 *)&t2, OP_LONG);
						iAddr += t2;
						break;

					case WDP: // Word Displacement
						vax11mem_vRead(*pc, (uint8 *)&t1, OP_WORD);
						*pc += OP_WORD;
						iAddr += RN(reg) + (int16)t1;
						break;

					case WDPD: // Word Displacement Deferred
						vax11mem_vRead(*pc, (uint8 *)&t1, OP_WORD);
						*pc += OP_WORD;
						vax11mem_vRead(RN(reg) + (int16)t1, (uint8 *)&t2, OP_LONG);
						iAddr += t2;
						break;

					case LDP: // Longword Displacement
						vax11mem_vRead(*pc, (uint8 *)&t1, OP_LONG);
						*pc += OP_LONG;
						iAddr += RN(reg) + t1;
						break;

					case LDPD: // Longword Displacement Deferred
						vax11mem_vRead(*pc, (uint8 *)&t1, OP_LONG);
						*pc += OP_LONG;
						vax11mem_vRead(RN(reg) + t1, (uint8 *)&t2, OP_LONG);
						iAddr += t2;
						break;

					default:
						RSRVD_ADDR_FAULT;
				}
		}

		// Write operand registers
		if (opmode & (OP_VADDR|OP_ADDR|OP_WRITE)) {
			if (opmode & (OP_VADDR|OP_WRITE))
				OPN(idx2++) = OP_MEM;
			OPN(idx2++) = iAddr;
		} else {
			if (scale <= OP_LONG)
				vax11mem_vRead(iAddr, (uint8 *)&OPN(idx2++), scale);
			else {
				vax11mem_vRead(iAddr, (uint8 *)&OPN(idx2++), OP_LONG);
				vax11mem_vRead(iAddr + 4, (uint8 *)&OPN(idx2++), OP_LONG);
			}
			if (opmode & OP_MODIFIED) {
				OPN(idx2++) = OP_MEM;
				OPN(idx2++) = iAddr;
			}
		}
	}
}

int vax_Execute(void)
{
	INSTRUCTION *Opcode;
	uint8 extended, opcode;
	int   abValue;

//	PC = goAddr;

	abValue = setjmp(vax_SetJump);
	if (abValue > 0) {
		// Return back to TS10 Emulator
		if (abValue == VAX_HALT)
			printf("\n\nVAX: HALT Instruction at PC %08X\n", fault_PC);
		return abValue;
	} else if (abValue < 0) {
		printf("Yes, fault trap here.\n");
//		vax_Fault(-abValue);
	}

	// Main loop for every instruction execution.
	while (emu_State == VAX_RUN) {
		// Save current PC for fault/interrupt use.
		fault_PC = PC;

#ifdef DEBUG
		if (dbg_Check(DBG_TRACE)) {
			int32 pc = PC;
			vax11disasm_Opcode(&pc);
		}
#endif DEBUG

		// Fetch instruction opcode from current Program Counter.
		// If opcode is greater than or equal to 0xFD, then
		// opcode is extended opcode.  Fetch another instruction
		// opcode from next byte location.

		extended = 0;
		vax11mem_vbRead(PC++, &opcode);
		if (opcode >= 0xFD) {
			extended = opcode;
			vax11mem_vbRead(PC++, &opcode);
		}

		if (extended)
			Opcode = vax_extOpcode[opcode];
		else
			Opcode = vax_basOpcode[opcode];
 
		// Now execute current instruction. If instruction is not
		// implemented yet, inform operator that.

		vax_DecodeOperand(Opcode, &PC);
		if (Opcode->Execute)
			Opcode->Execute();
		else {
			if (extended) {
				printf("VAX: Unimplemented Instruction - Opcode %02X,%02X\n",
					extended, opcode);
			} else {
				printf("VAX: Unimplemented Instruction - Opcode %02X\n",
					opcode);
			}
		}

#ifdef DEBUG
		if (dbg_Check(DBG_TRACE|DBG_DATA))
			vax_DisplayRegisters();
#endif DEBUG
	}

	return VAX_OK;
}
