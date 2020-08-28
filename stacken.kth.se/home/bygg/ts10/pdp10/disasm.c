// disasm.c - PDP-6/PDP-10 Disassembler
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

#ifdef DEBUG

#include "pdp10/defs.h"
#include "pdp10/proto.h"

extern INSTRUCTION *pdp10_Opcode[];
extern INSTRUCTION *pdp10_OpcodeIO[];
extern INSTRUCTION *pdp10_OpcodeDEV[];
extern INSTRUCTION *pdp10_OpcodeFUNC[];

void p10_Disassemble(int30 Addr, int36 Inst, int mode)
{
	// Fields of Instruction Code
	int18 opDevice;   // (I/O)   Device Code       (DEV)
	int18 opFunction; // (I/O)   Function Code     (FUNC)
	int18 opCode;     // (Basic) Opcode field      (OP)
	int18 opAC;       // (Basic) Accumulator       (AC)
	int18 opIndirect; // (Both)  Indirect          (I)
	int18 opIndex;    // (Both)  Index Register    (X)
	int18 opAddr;     // (Both)  Address           (Y)

	char  *Name;
	char  *Symbol = NULL;

	opCode = INST_GETOP(Inst);

	if (mode & OP_EXT) {
		dbg_Printf("CPU: Extend - Code %03o\n", opCode);
		return;
	}

	if ((opCode >= 0700) && pdp10_OpcodeIO[(Inst >> 23) & 01777]) {
		// I/O Instruction Format
		opDevice   = INST_GETDEV(Inst);
		opFunction = INST_GETFUNC(Inst);
		opIndirect = INST_GETI(Inst);
		opIndex    = INST_GETX(Inst);
		opAddr     = INST_GETY(Inst);
		opAC       = 0;

		Name = pdp10_OpcodeIO[(Inst >> 23) & 01777]->Name;
	} else if ((opCode >= 0700) && pdp10_OpcodeDEV[(Inst >> 26) & 0177]) {
		// I/O Instruction Format
		opDevice   = INST_GETDEV(Inst);
		opFunction = INST_GETFUNC(Inst);
		opIndirect = INST_GETI(Inst);
		opIndex    = INST_GETX(Inst);
		opAddr     = INST_GETY(Inst);
		opAC       = 0;

		Name   = pdp10_OpcodeFUNC[(Inst >> 23) & 07]->Name;
		Symbol = pdp10_OpcodeDEV[(Inst >> 26) & 0177]->Name;
	} else {
		// Basic Instruction Format
		opAC       = INST_GETAC(Inst);
		opIndirect = INST_GETI(Inst);
		opIndex    = INST_GETX(Inst);
		opAddr     = INST_GETY(Inst);

		if (pdp10_Opcode[opCode]->Flags & OP_AC) 
			Name = pdp10_Opcode[opCode]->AC[opAC]->Name;
		else
			Name = pdp10_Opcode[opCode]->Name;
	}

	dbg_Printf("CPU: %06llo %06llo,,%06llo %-6s ",
		RH(Addr), LHSR(Inst), RH(Inst), Name);

	if (opAC)
		dbg_Printf("%o,", opAC);
	if (Symbol)
		dbg_Printf("%s,", Symbol);
	if (opIndirect)
		dbg_Printf("@");
	if (opAddr)
		dbg_Printf("%o", opAddr);
	if (opIndex)
		dbg_Printf("(%o)", opIndex);
	dbg_Printf("\n");
}

#endif
