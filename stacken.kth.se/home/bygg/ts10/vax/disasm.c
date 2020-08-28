// disasm.c - VAX Symbolic Disassembler
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

static INSTRUCTION *disasm_basOpcode[256];
static INSTRUCTION *disasm_extOpcode[256];

static char *regNames[] = {
	"R0",  "R1",  "R2",  "R3",  "R4",  "R5",  "R6",  "R7",
	"R8",  "R9",  "R10", "R11", "AP",  "FP",  "SP",  "PC"
};

static char Comment[80];

void vax11disasm_AddDest(int32 data)
{
	char aschex[10];

	if (!Comment[0])
		strcat(Comment, " ; ");
	else
		strcat(Comment, ", ");

	sprintf(aschex, "%08X", data);
	strcat(Comment, aschex);
}

int vax11disasm_Operand(INSTRUCTION *opCode, int opCount, int32 *pc,
	char *disasm, int indexed)
{
	int   access = opCode->opMode[opCount];
	int   scale  = access & 0x00FF;
	uint8 opType, mode, reg;
	int32 data = 0;
	char  fmt[64];
	char  strReg[64];

	if ((opCount > 0) && !indexed)
		strcat(disasm, ",");

/*
	if (access & OP_IMMED) {
		vax11mem_vRead(*pc, (uint8 *)&data, scale);
		*pc += scale;

		if (scale == 1) {
			data = (data & 0x0080) ? (data | 0xFFFFFF00) : (data & 0x007F);
		} else if (scale == 2) {
			data = (data & 0x8000) ? (data | 0xFFFF0000) : (data & 0x7FFF);
		}

		sprintf(fmt, "#%%0%dX", scale * 2);
		sprintf(strReg, fmt, data);
		strcat(disasm, strReg);

		return VAX_OK;
	}
*/

	if (access & OP_BRANCH) {
		vax11mem_vRead(*pc, (uint8 *)&data, scale);
		*pc += scale;

		if (scale == 1) {
			data = (data & 0x0080) ? (data | 0xFFFFFF00) : (data & 0x007F);
		} else if (scale == 2) {
			data = (data & 0x8000) ? (data | 0xFFFF0000) : (data & 0x7FFF);
		}

		sprintf(strReg, "%08X", *pc + data);
		strcat(disasm, strReg);

		return VAX_OK;
	}

	vax11mem_vbRead((*pc)++, &opType);
	mode = (opType >> 4) & 0x0F;
	reg  = opType & 0x0F;

	if ((mode >= 8) && (reg == 0x0F)) {
		switch (mode) {
			case 0x08: // Immediate
				vax11mem_vRead(*pc, (uint8 *)&data, scale);
				switch (scale) {
					case 1:
						sprintf(strReg, "I^#%02X", data);
						break;

					case 2:
						sprintf(strReg, "I^#%04X", data);
						break;

					case 4:
					case 8:
						if (access & OP_FLOAT)
							sprintf(strReg, "I^#%f", 0.0);
						else
							sprintf(strReg, "I^#%08X", data);
						break;
				}
				*pc += scale;
				strcat(disasm, strReg);
				break;

			case 0x09: // Absolute
				vax11mem_vRead(*pc, (uint8 *)&data, 4);
				*pc += 4;
				sprintf(strReg, "@#%08X", data);
				strcat(disasm, strReg);
				break;

			case 0x0A: // Byte Relative
				vax11mem_vbRead((*pc)++, (uint8 *)&data);
				vax11disasm_AddDest(*pc + (int8)data);
				sprintf(strReg, "B^%02X", data);
				strcat(disasm, strReg);
				break;

			case 0x0B: // Deferred Byte Relative
				vax11mem_vbRead((*pc)++, (uint8 *)&data);
				vax11disasm_AddDest(*pc + (int8)data);
				sprintf(strReg, "@B^%02X", data);
				strcat(disasm, strReg);
				break;

			case 0x0C: // Word Relative
				vax11mem_vRead(*pc, (uint8 *)&data, 2);
				*pc += 2;
				vax11disasm_AddDest(*pc + (int16)data);
				sprintf(strReg, "W^%04X", data);
				strcat(disasm, strReg);
				break;

			case 0x0D: // Deferred Word Relative
				vax11mem_vRead(*pc, (uint8 *)&data, 2);
				*pc += 2;
				vax11disasm_AddDest(*pc + (int16)data);
				sprintf(strReg, "@W^%04X", data);
				strcat(disasm, strReg);
				break;

			case 0x0E: // Longword Relative
				vax11mem_vRead(*pc, (uint8 *)&data, 4);
				*pc += 4;
				vax11disasm_AddDest(*pc + data);
				sprintf(strReg, "L^%08X", data);
				strcat(disasm, strReg);
				break;

			case 0x0F: // Deferred Longword Relative
				vax11mem_vRead(*pc, (uint8 *)&data, 4);
				*pc += 4;
				vax11disasm_AddDest(*pc + data);
				sprintf(strReg, "@L^%08X", data);
				strcat(disasm, strReg);
				break;
		}

		return VAX_OK;
	}

	switch (mode) {
		case 0x00: // Literal
		case 0x01:
		case 0x02:
		case 0x03:
			if (access & OP_FLOAT)
				sprintf(strReg, "S^#%f", 0.0);
			else
				sprintf(strReg, "S^#%02X", opType);
			strcat(disasm, strReg);
			break;

		case 0x04: // Indexed
			vax11disasm_Operand(opCode, opCount, pc, disasm, 1);
			sprintf(strReg, "[%s]", regNames[reg]);
			strcat(disasm, strReg);
			break;

		case 0x05: // Register
			strcat(disasm, regNames[reg]);
			break;

		case 0x06: // Register Deferred
			sprintf(strReg, "(%s)", regNames[reg]);
			strcat(disasm, strReg);
			break;

		case 0x07: // Autodecrement
			sprintf(strReg, "-(%s)", regNames[reg]);
			strcat(disasm, strReg);
			break;

		case 0x08: // Autoincrement
			sprintf(strReg, "(%s)+", regNames[reg]);
			strcat(disasm, strReg);
			break;

		case 0x09: // Autoincrement Deferred 
			sprintf(strReg, "@(%s)+", regNames[reg]);
			strcat(disasm, strReg);
			break;

		case 0x0A: // Byte Displacement
			vax11mem_vbRead((*pc)++, (uint8 *)&data);
			sprintf(strReg, "B^%02X(%s)", data, regNames[reg]);
			strcat(disasm, strReg);
			break;

		case 0x0B: // Deferred Byte Displacement
			vax11mem_vbRead((*pc)++, (uint8 *)&data);
			sprintf(strReg, "@B^%02X(%s)", data, regNames[reg]);
			strcat(disasm, strReg);
			break;

		case 0x0C: // Word Displacement
			vax11mem_vRead(*pc, (uint8 *)&data, 2);
			*pc += 2;
			sprintf(strReg, "W^%04X(%s)", data, regNames[reg]);
			strcat(disasm, strReg);
			break;

		case 0x0D: // Deferred Word Displacement
			vax11mem_vRead(*pc, (uint8 *)&data, 2);
			*pc += 2;
			sprintf(strReg, "@W^%04X(%s)", data, regNames[reg]);
			strcat(disasm, strReg);
			break;

		case 0x0E: // Longword Displacement
			vax11mem_vRead(*pc, (uint8 *)&data, 4);
			*pc += 4;
			sprintf(strReg, "L^%08X(%s)", data, regNames[reg]);
			strcat(disasm, strReg);
			break;

		case 0x0F: // Deferred Longword Displacement
			vax11mem_vRead(*pc, (uint8 *)&data, 4);
			*pc += 4;
			sprintf(strReg, "@L^%08X(%s)", data, regNames[reg]);
			strcat(disasm, strReg);
			break;
	}

	return VAX_OK;
}

int vax11disasm_Opcode(int32 *pc)
{
	INSTRUCTION *opCode;
	char  disasm[256];
	char  strName[10];
	uint8 extended, opcode;
	int   idx;

	sprintf(disasm, "%08X ", *pc);

	extended = 0;
	vax11mem_vbRead((*pc)++, &opcode);
	if (opcode == 0xFD) {
		extended = opcode;
		vax11mem_vbRead((*pc)++, &opcode);
	}

	if ((opCode = disasm_basOpcode[opcode]) == NULL) {
		if (extended)
			printf(".BYTE %02X,%02X\n", extended, opcode);
		else
			printf(".BYTE %02X\n", opcode);
		return VAX_OK;
	}

	Comment[0] = 0;
	sprintf(strName, "%-8s ", opCode->Name);
	strcat(disasm, strName);

	if (opCode->nOperands) {
		for (idx = 0; idx < opCode->nOperands; idx++)
			vax11disasm_Operand(opCode, idx, pc, disasm, 0);
	}

	if (Comment[0])
		strcat(disasm, Comment);
	strcat(disasm, "\n");
	printf(disasm);

	return VAX_OK;
}

// Build the instruction table for disassembler routines
int vax11disasm_Initialize(void)
{
	uint8 extended, opcode;
	int   idx;

	// Clear all instruction tables first.
	for(idx = 0; idx < 256; idx++) {
		disasm_basOpcode[idx] = NULL;
		disasm_extOpcode[idx] = NULL;
	}

	// Build new instruction tables now.
	for(idx = 0; vax_Instruction[idx].Name; idx++) {
		extended = vax_Instruction[idx].Extended;
		opcode   = vax_Instruction[idx].Opcode;

//		printf("Name: %-8s  Extended: %02X Opcode %02X\n",
//			vax_Instruction[idx].Name, extended, opcode);

		switch(extended) {
			case 0x00: // Normal function
				if (disasm_basOpcode[opcode] == NULL)
					disasm_basOpcode[opcode] = &vax_Instruction[idx];
				break;

			case 0xFD: // Extended function
				if (disasm_extOpcode[opcode] == NULL)
					disasm_extOpcode[opcode] = &vax_Instruction[idx];
				break;
		}
	}

	return VAX_OK;
}
