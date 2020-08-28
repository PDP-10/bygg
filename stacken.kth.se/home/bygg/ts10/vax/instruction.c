// instruction.c - Complete VAX Instruction Table for all VAX series
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

INSTRUCTION vax_Instruction[] =
{
	{
		"HALT",
		"Halt",
		0x00, 0x00,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_HALT // Execute Routine
	},

	{
		"NOP",
		"No operation",
		0x00, 0x01,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_NOP // Execute Routine
	},

	{
		"REI",
		"Return from exception or interrupt",
		0x00, 0x02,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"BPT",
		"Break point trap",
		0x00, 0x03,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"RET",
		"Return from called procedure",
		0x00, 0x04,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_RET // Execute Routine
	},

	{
		"RSB",
		"Return from subroutine",
		0x00, 0x05,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_RSB // Execute Routine
	},

	{
		"LDPCTX",
		"Load program context",
		0x00, 0x06,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"SVPCTX",
		"Save program context",
		0x00, 0x07,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTPS",
		"Convert packed to leading separate",
		0x00, 0x08,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ RW, AB, RW, AB, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTSP",
		"Convert leading separate to packed",
		0x00, 0x09,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ RW, AB, RW, AB, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"INDEX",
		"Index calculation",
		0x00, 0x0A,   // Opcode (Extended + Normal)
		6,            // Number of Operands
		{ RL, RL, RL, RL, RL, WL }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_INDEX // Execute Routine
	},

	{
		"CRC",
		"Calcuate cyclic redundancy check",
		0x00, 0x0B,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ AB, RL, RW, AB, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"PROBER",
		"Probe read access",
		0x00, 0x0C,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RB, RW, AB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"PROBEW",
		"Probe write access",
		0x00, 0x0D,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RB, RW, AB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"INSQUE",
		"Insert into queue",
		0x00, 0x0E,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ AB, AB, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"REMQUE",
		"Remove from queue",
		0x00, 0x0F,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ AB, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"BSBB",
		"Branch to subroutine with byte displacment",
		0x00, 0x10,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ BB, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BSBB // Execute Routine
	},

	{
		"BRB",
		"Branch with byte displacement",
		0x00, 0x11,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ BB, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BRB // Execute Routine
	},

	{
		"BNEQ",
		"Branch on not equal",
		0x00, 0x12,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ BB, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BNEQ // Execute Routine
	},

	{
		"BNEQU",
		"Branch on not equal unsigned",
		0x00, 0x12,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ BB, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BNEQ // Execute Routine
	},

	{
		"BEQL",
		"Branch on equal",
		0x00, 0x13,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ BB, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BEQL // Execute Routine
	},

	{
		"BEQLU",
		"Branch on equal unsigned",
		0x00, 0x13,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ BB, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BEQL // Execute Routine
	},

	{
		"BGTR",
		"Branch on greater",
		0x00, 0x14,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ BB, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BGTR // Execute Routine
	},

	{
		"BLEQ",
		"Branch on less or equal",
		0x00, 0x15,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ BB, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BLEQ // Execute Routine
	},

	{
		"JSB",
		"Jump to subroutine",
		0x00, 0x16,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ AB, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_JSB // Execute Routine
	},

	{
		"JMP",
		"Jump",
		0x00, 0x17,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ AB, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_JMP // Execute Routine
	},

	{
		"BGEQ",
		"Branch on greater or equal",
		0x00, 0x18,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ BB, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BGEQ // Execute Routine
	},

	{
		"BLSS",
		"Branch on less",
		0x00, 0x19,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ BB, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BLSS // Execute Routine
	},

	{
		"BGTRU",
		"Branch on greater unsigned",
		0x00, 0x1A,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ BB, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BGTRU // Execute Routine
	},

	{
		"BLEQU",
		"Branch on less or equal unsigned",
		0x00, 0x1B,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ BB, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BLEQU // Execute Routine
	},

	{
		"BVC",
		"Branch on overflow clear",
		0x00, 0x1C,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ BB, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BVC // Execute Routine
	},

	{
		"BVS",
		"Branch on overflow set",
		0x00, 0x1D,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ BB, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BVS // Execute Routine
	},

	{
		"BCC",
		"Branch on carry clear",
		0x00, 0x1E,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ BB, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BCC // Execute Routine
	},

	{
		"BGEQU",
		"Branch on greater or equal unsigned",
		0x00, 0x1E,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ BB, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BCC // Execute Routine
	},

	{
		"BCS",
		"Branch on carry set",
		0x00, 0x1F,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ BB, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BCS // Execute Routine
	},

	{
		"BLSSU",
		"Branch on less unsigned",
		0x00, 0x1F,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ BB, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BCS // Execute Routine
	},

	{
		"ADDP4",
		"Add packed 4 operand",
		0x00, 0x20,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ RW, AB, RW, AB, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"ADDP6",
		"Add packed 6 operand",
		0x00, 0x21,   // Opcode (Extended + Normal)
		6,            // Number of Operands
		{ RW, AB, RW, AB, RW, AB }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"SUBP4",
		"Subtract packed 4 operand",
		0x00, 0x22,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ RW, AB, RW, AB, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"SUBP6",
		"Subtract packed 6 operand",
		0x00, 0x23,   // Opcode (Extended + Normal)
		6,            // Number of Operands
		{ RW, AB, RW, AB, RW, AB }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTPT",
		"Convert packed to trailing",
		0x00, 0x24,   // Opcode (Extended + Normal)
		5,            // Number of Operands
		{ RW, AB, AB, RW, AB, 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MULP6",
		"Multiply packed",
		0x00, 0x25,   // Opcode (Extended + Normal)
		6,            // Number of Operands
		{ RW, AB, RW, AB, RW, AB }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTTP",
		"Convert trailing to packed",
		0x00, 0x26,   // Opcode (Extended + Normal)
		5,            // Number of Operands
		{ RW, AB, AB, RW, AB, 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"DIVP6",
		"Divide packed",
		0x00, 0x27,   // Opcode (Extended + Normal)
		6,            // Number of Operands
		{ RW, AB, RW, AB, RW, AB }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MOVC3",
		"Move character 3 operand",
		0x00, 0x28,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RW, AB, AB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CMPC3",
		"Compare character 3 operand",
		0x00, 0x29,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RW, AB, AB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"SCANC",
		"Scan for character",
		0x00, 0x2A,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ RW, AB, AB, RB, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"SPANC",
		"Span characters",
		0x00, 0x2B,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ RW, AB, AB, RB, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MOVC5",
		"Move character 5 operand",
		0x00, 0x2C,   // Opcode (Extended + Normal)
		5,            // Number of Operands
		{ RW, AB, RB, RW, AB, 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CMPC5",
		"Compare character 5 operand",
		0x00, 0x2D,   // Opcode (Extended + Normal)
		5,            // Number of Operands
		{ RW, AB, RB, RW, AB, 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MOVTC",
		"Move translated characters",
		0x00, 0x2E,   // Opcode (Extended + Normal)
		6,            // Number of Operands
		{ RW, AB, RB, AB, RW, AB }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MOVTUC",
		"Move translated until character",
		0x00, 0x2F,   // Opcode (Extended + Normal)
		6,            // Number of Operands
		{ RW, AB, RB, AB, RW, AB }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"BSBW",
		"Branch to subroutine with word displacment",
		0x00, 0x30,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ BW, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BSBW // Execute Routine
	},

	{
		"BRW",
		"Branch with word displacement",
		0x00, 0x31,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ BW, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BRW // Execute Routine
	},

	{
		"CVTWL",
		"Convert word to longword",
		0x00, 0x32,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RW, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTWB",
		"Convert word to byte",
		0x00, 0x33,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RW, WB, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MOVP",
		"Move packed",
		0x00, 0x34,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RW, AB, AB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CMPP3",
		"Compare packed 3 operand",
		0x00, 0x35,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RW, AB, AB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTPL",
		"Convert packed to longword",
		0x00, 0x36,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RW, AB, WL, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CMPP4",
		"Compare packed 4 operand",
		0x00, 0x37,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ RW, AB, RW, AB, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"EDITPC",
		"Edit packed to character",
		0x00, 0x38,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ RW, AB, AB, AB, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MATCHC",
		"Match characters",
		0x00, 0x39,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ RW, AB, RW, AB, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"LOCC",
		"Locate character",
		0x00, 0x3A,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RB, RW, AB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"SKPC",
		"Skip character",
		0x00, 0x3B,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RB, RW, AB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MOVZWL",
		"Move zero-extended word to longword",
		0x00, 0x3C,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RW, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_MOVZWL // Execute Routine
	},

	{
		"ACBW",
		"Add compare and branch word",
		0x00, 0x3D,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ RW, RW, MW, BW, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MOVAW",
		"Move address of word",
		0x00, 0x3E,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ AW, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_MOVL // Execute Routine
	},

	{
		"PUSHAW",
		"Push address of word",
		0x00, 0x3F,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ AW, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_PUSHL // Execute Routine
	},

	{
		"ADDF2",
		"Add F_floating 2 operand",
		0x00, 0x40,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RF, MF, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"ADDF3",
		"Add F_floating 3 operand",
		0x00, 0x41,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RF, RF, WF, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"SUBF2",
		"Subtract F_floating 2 operand",
		0x00, 0x42,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RF, MF, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"SUBF3",
		"Subtract F_floating 3 operand",
		0x00, 0x43,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RF, RF, WF, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MULF2",
		"Multiply F_floating 2 operand",
		0x00, 0x44,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RF, MF, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MULF3",
		"Multiply F_floating 3 operand",
		0x00, 0x45,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RF, RF, WF, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"DIVF2",
		"Divide F_floating 2 operand",
		0x00, 0x46,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RF, MF, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"DIVF3",
		"Divide F_floating 3 operand",
		0x00, 0x47,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RF, RF, WF, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTFB",
		"Convert F_floating to byte",
		0x00, 0x48,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RF, WB, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTFW",
		"Convert F_floating to word",
		0x00, 0x49,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RF, WW, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTFL",
		"Convert F_floating to longword",
		0x00, 0x4A,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RF, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTRFL",
		"Convert rounded F_floating to longword",
		0x00, 0x4B,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RF, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTBF",
		"Convert byte to F_floating",
		0x00, 0x4C,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RB, WF, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTWF",
		"Convert word to F_floating",
		0x00, 0x4D,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RW, WF, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTLF",
		"Convert longword to F_floating",
		0x00, 0x4E,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RL, WF, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"ACBF",
		"Add compare and branch F_floating",
		0x00, 0x4F,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ RF, RF, MF, BW, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MOVF",
		"Move F_floating",
		0x00, 0x50,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RF, WF, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CMPF",
		"Compare F_floating",
		0x00, 0x51,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RF, RF, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MNEGF",
		"Move negated F_floating",
		0x00, 0x52,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RF, WF, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"TSTF",
		"Test F_floating",
		0x00, 0x53,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ RF, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"EMODF",
		"Extended modulus F_floating",
		0x00, 0x54,   // Opcode (Extended + Normal)
		5,            // Number of Operands
		{ RF, RB, RF, WL, WF, 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"POLYF",
		"Evaluate polynomial F_floating",
		0x00, 0x55,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RF, RW, AB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTFD",
		"Convert F_floating to D_floating",
		0x00, 0x56,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RF, WD, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"ADAWI",
		"Add aligned word interlocked",
		0x00, 0x58,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RW, WW, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_ADAWI // Execute Routine
	},

	{
		"INSQHI",
		"Insert into queue at head, interlocked",
		0x00, 0x5C,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ AB, AQ, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"INSQTI",
		"Insert into queue at tail, interlocked",
		0x00, 0x5D,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ AB, AQ, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"REMQHI",
		"Remove from queue at head, interlocked",
		0x00, 0x5E,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ AQ, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"REMQTI",
		"Remove from queue at tail, interlocked",
		0x00, 0x5F,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ AQ, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"ADDD2",
		"Add D_floating 2 operand",
		0x00, 0x60,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RD, MD, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"ADDD3",
		"Add D_floating 3 operand",
		0x00, 0x61,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RD, RD, MD, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"SUBD2",
		"Subtract D_floating 2 operand",
		0x00, 0x62,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RD, MD, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"SUBD3",
		"Subtract D_floating 3 operand",
		0x00, 0x63,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RD, RD, WD, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MULD2",
		"Multiply D_floating 2 operand",
		0x00, 0x64,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RD, MD, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MULD3",
		"Multiply D_floating 3 operand",
		0x00, 0x65,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RD, RD, WD, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"DIVD2",
		"Divide D_floating 2 operand",
		0x00, 0x66,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RD, MD, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"DIVD3",
		"Divide D_floating 3 operand",
		0x00, 0x67,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RD, RD, WD, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTDB",
		"Convert D_floating to byte",
		0x00, 0x68,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RD, WB, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTDW",
		"Convert D_floating to word",
		0x00, 0x69,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RD, WW, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTDL",
		"Convert D_floating to longword",
		0x00, 0x6A,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RD, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTRDL",
		"Convert rounded D_floating to longword",
		0x00, 0x6B,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RD, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTBD",
		"Convert byte to D_floating",
		0x00, 0x6C,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RB, WD, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTWD",
		"Convert word to D_floating",
		0x00, 0x6D,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RW, WD, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTLD",
		"Convert longword to D_floating",
		0x00, 0x6E,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RL, WD, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"ACBD",
		"Add compare and branch D_floating",
		0x00, 0x6F,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ RD, RD, MD, BW, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MOVD",
		"Move D_floating",
		0x00, 0x70,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RD, WD, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CMPD",
		"Compare D_floating",
		0x00, 0x71,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RD, RD, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MNEGD",
		"Move negated D_floating",
		0x00, 0x72,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RD, WD, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"TSTD",
		"Test D_floating",
		0x00, 0x73,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ RD, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"EMODD",
		"Extended modulus D_floating",
		0x00, 0x74,   // Opcode (Extended + Normal)
		5,            // Number of Operands
		{ RD, RB, RD, WL, WD, 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"POLYD",
		"Evaluate polynomial D_floating",
		0x00, 0x75,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RD, RW, AB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTDF",
		"Convert D_floating to F_floating",
		0x00, 0x76,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RD, WF, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"ASHL",
		"Arithmetic shift longword",
		0x00, 0x78,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RB, RL, WL, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_ASHL // Execute Routine
	},

	{
		"ASHQ",
		"Arithmetic shift quadword",
		0x00, 0x79,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RB, RQ, WQ, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_ASHQ // Execute Routine
	},

	{
		"EMUL",
		"Extended multiply",
		0x00, 0x7A,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ RL, RL, RL, WQ, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"EDIV",
		"Extended divide",
		0x00, 0x7B,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ RL, RQ, WL, WL, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CLRQ",
		"Clear quadword",
		0x00, 0x7C,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ WQ, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_CLRQ // Execute Routine
	},

	{
		"CLRD",
		"Clear D_floating",
		0x00, 0x7C,   // Opcode (Extended + Normal)
		1,            // Number of Operands: 1
		{ WD, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_CLRQ // Execute Routine
	},

	{
		"CLRG",
		"Clear G_floating",
		0x00, 0x7C,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ WG, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_CLRQ // Execute Routine
	},

	{
		"MOVQ",
		"Move quadword",
		0x00, 0x7D,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RQ, WQ, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_MOVQ // Execute Routine
	},

	{
		"MOVAQ",
		"Move address of quadword",
		0x00, 0x7E,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ AQ, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_MOVL // Execute Routine
	},

	{
		"MOVAD",
		"Move address of D_floating",
		0x00, 0x7E,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ AD, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_MOVL // Execute Routine
	},

	{
		"MOVAG",
		"Move address of G_floating",
		0x00, 0x7E,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ AG, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_MOVL // Execute Routine
	},

	{
		"PUSHAQ",
		"Push address of quadword",
		0x00, 0x7F,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ AQ, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_PUSHL // Execute Routine
	},

	{
		"PUSHAD",
		"Push address of D_floating",
		0x00, 0x7F,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ AD, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_PUSHL // Execute Routine
	},

	{
		"PUSHAG",
		"Push address of G_floating",
		0x00, 0x7F,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ AG, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_PUSHL // Execute Routine
	},

	{
		"ADDB2",
		"Add byte 2 operand",
		0x00, 0x80,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RB, MB, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_ADDB // Execute Routine
	},

	{
		"ADDB3",
		"Add byte 3 operand",
		0x00, 0x81,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RB, RB, WB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_ADDB // Execute Routine
	},

	{
		"SUBB2",
		"Subtract byte 2 operand",
		0x00, 0x82,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RB, MB, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_SUBB // Execute Routine
	},

	{
		"SUBB3",
		"Subtract byte 3 operand",
		0x00, 0x83,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RB, RB, WB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_SUBB // Execute Routine
	},

	{
		"MULB2",
		"Multiply byte 2 operand",
		0x00, 0x84,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RB, MB, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MULB3",
		"Multiply byte 3 operand",
		0x00, 0x85,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RB, RB, WB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"DIVB2",
		"Divide byte 2 operand",
		0x00, 0x86,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RB, MB, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"DIVB3",
		"Divide byte 3 operand",
		0x00, 0x87,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RB, RB, WB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"BISB2",
		"Bit set byte 2 operand",
		0x00, 0x88,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RB, MB, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BISB // Execute Routine
	},

	{
		"BISB3",
		"Bit set byte 3 operand",
		0x00, 0x89,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RB, RB, WB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BISB // Execute Routine
	},

	{
		"BICB2",
		"Bit clear byte 2 operand",
		0x00, 0x8A,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RB, MB, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BICB // Execute Routine
	},

	{
		"BICB3",
		"Bit clear byte 3 operand",
		0x00, 0x8B,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RB, RB, WB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BICB // Execute Routine
	},

	{
		"XORB2",
		"Exclusive-OR byte 2 operand",
		0x00, 0x8C,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RB, MB, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_XORB // Execute Routine
	},

	{
		"XORB3",
		"Exclusive-OR byte 3 operand",
		0x00, 0x8D,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RB, RB, WB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_XORB // Execute Routine
	},

	{
		"MNEGB",
		"Move negated byte",
		0x00, 0x8E,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RB, MB, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_MNEGB // Execute Routine
	},

	{
		"CASEB",
		"Case byte",
		0x00, 0x8F,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RB, RB, RB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_CASEB // Execute Routine
	},

	{
		"MOVB",
		"Move byte",
		0x00, 0x90,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RB, WB, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_MOVB // Execute Routine
	},

	{
		"CMPB",
		"Compare byte",
		0x00, 0x91,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RB, RB, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_CMPB // Execute Routine
	},

	{
		"MCOMB",
		"Move complemented byte",
		0x00, 0x92,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RB, WB, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_MCOMB // Execute Routine
	},

	{
		"BITB",
		"Bit test byte",
		0x00, 0x93,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RB, RB, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BITB // Execute Routine
	},

	{
		"CLRB",
		"Clear byte",
		0x00, 0x94,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ WB, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_CLRB // Execute Routine
	},

	{
		"TSTB",
		"Test byte",
		0x00, 0x95,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ RB, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_TSTB // Execute Routine
	},

	{
		"INCB",
		"Increment byte",
		0x00, 0x96,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ MB, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_INCB  // Execute Routine
	},

	{
		"DECB",
		"Decrement byte",
		0x00, 0x97,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ MB, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_DECB  // Execute Routine
	},

	{
		"CVTBL",
		"Convert byte to longword",
		0x00, 0x98,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RB, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTBW",
		"Convert byte to word",
		0x00, 0x99,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RB, WW, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MOVZBL",
		"Move zero-extended byte to longword",
		0x00, 0x9A,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RB, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_MOVZBL // Execute Routine
	},

	{
		"MOVZBW",
		"Move zero-extended byte to word",
		0x00, 0x9B,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RB, WW, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_MOVZBW // Execute Routine
	},

	{
		"ROTL",
		"Rotate longword",
		0x00, 0x9C,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RB, RL, WL, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_ROTL // Execute Routine
	},

	{
		"ACBB",
		"Add compare and branch byte",
		0x00, 0x9D,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ RB, RB, MB, BW, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MOVAB",
		"Move address of byte",
		0x00, 0x9E,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ AB, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_MOVL // Execute Routine
	},

	{
		"PUSHAB",
		"Move address of byte",
		0x00, 0x9F,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ AB, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_PUSHL // Execute Routine
	},

	{
		"ADDW2",
		"Add word 2 operand",
		0x00, 0xA0,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RW, MW, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_ADDW // Execute Routine
	},

	{
		"ADDW3",
		"Add Word 3 operand",
		0x00, 0xA1,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RW, RW, WW, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_ADDW // Execute Routine
	},

	{
		"SUBW2",
		"Subtract Word 2 operand",
		0x00, 0xA2,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RW, MW, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_SUBW // Execute Routine
	},

	{
		"SUBW3",
		"Subtract word 3 operand",
		0x00, 0xA3,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RW, RW, WW, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_SUBW // Execute Routine
	},

	{
		"MULW2",
		"Multiply word 2 operand",
		0x00, 0xA4,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RW, MW, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MULW3",
		"Multiply word 3 operand",
		0x00, 0xA5,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RW, RW, WW, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"DIVW2",
		"Divide word 2 operand",
		0x00, 0xA6,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RW, MW, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"DIVW3",
		"Divide word 3 operand",
		0x00, 0xA7,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RW, RW, WW, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"BISW2",
		"Bit set word 2 operand",
		0x00, 0xA8,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RW, MW, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BISW // Execute Routine
	},

	{
		"BISW3",
		"Bit set word 3 operand",
		0x00, 0xA9,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RW, RW, WW, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BISW // Execute Routine
	},

	{
		"BICW2",
		"Bit clear word 2 operand",
		0x00, 0xAA,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RW, MW, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BICW // Execute Routine
	},

	{
		"BICW3",
		"Bit clear word 3 operand",
		0x00, 0xAB,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RW, RW, WW, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BICW // Execute Routine
	},

	{
		"XORW2",
		"Exclusive-OR word 2 operand",
		0x00, 0xAC,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RW, MW, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_XORW // Execute Routine
	},

	{
		"XORW3",
		"Exclusive-OR word 3 operand",
		0x00, 0xAD,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RW, RW, WW, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_XORW // Execute Routine
	},

	{
		"MNEGW",
		"Move negated word",
		0x00, 0xAE,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RW, MW, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_MNEGW // Execute Routine
	},

	{
		"CASEW",
		"Case word",
		0x00, 0xAF,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RW, RW, RW, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_CASEW // Execute Routine
	},

	{
		"MOVW",
		"Move word",
		0x00, 0xB0,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RW, WW, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_MOVW // Execute Routine
	},

	{
		"CMPW",
		"Compare word",
		0x00, 0xB1,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RW, RW, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_CMPW // Execute Routine
	},

	{
		"MCOMW",
		"Move complemented word",
		0x00, 0xB2,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RW, WW, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_MCOMW // Execute Routine
	},

	{
		"BITW",
		"Bit test word",
		0x00, 0xB3,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RW, RW, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BITW // Execute Routine
	},

	{
		"CLRW",
		"Clear word",
		0x00, 0xB4,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ WW, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_CLRW // Execute Routine
	},

	{
		"TSTW",
		"Test word",
		0x00, 0xB5,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ RW, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_TSTW // Execute Routine
	},

	{
		"INCW",
		"Increment word",
		0x00, 0xB6,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ MW, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_INCW  // Execute Routine
	},

	{
		"DECW",
		"Decrement word",
		0x00, 0xB7,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ MW, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_DECW  // Execute Routine
	},

	{
		"BISPSW",
		"Bit set program status word",
		0x00, 0xB8,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ RW, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BISPSW // Execute Routine
	},

	{
		"BICPSW",
		"Bit clear program status word",
		0x00, 0xB9,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ RW, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BICPSW // Execute Routine
	},

	{
		"POPR",
		"Pop registers",
		0x00, 0xBA,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ RW, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_POPR // Execute Routine
	},

	{
		"PUSHR",
		"Push registers",
		0x00, 0xBB,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ RW, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_PUSHR // Execute Routine
	},

	{
		"CHMK",
		"Change mode to kernel",
		0x00, 0xBC,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ RW, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CHME",
		"Change mode to executive",
		0x00, 0xBD,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ RW, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CHMS",
		"Change mode to supervisor",
		0x00, 0xBE,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ RW, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CHMU",
		"Change mode to user",
		0x00, 0xBF,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ RW, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"ADDL2",
		"Add longword 2 operand",
		0x00, 0xC0,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RL, ML, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_ADDL // Execute Routine
	},

	{
		"ADDL3",
		"Add longword 3 operand",
		0x00, 0xC1,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RL, RL, WL, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_ADDL // Execute Routine
	},

	{
		"SUBL2",
		"Subtract longword 2 operand",
		0x00, 0xC2,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RL, ML, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_SUBL // Execute Routine
	},

	{
		"SUBL3",
		"Subtract longword 3 operand",
		0x00, 0xC3,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RL, RL, WL, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_SUBL // Execute Routine
	},

	{
		"MULL2",
		"Multiply longword 2 operand",
		0x00, 0xC4,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RL, ML, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MULL3",
		"Multiply longword 3 operand",
		0x00, 0xC5,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RL, RL, WL, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"DIVL2",
		"Divide longword 2 operand",
		0x00, 0xC6,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RL, ML, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"DIVL3",
		"Divide longword 3 operand",
		0x00, 0xC7,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RL, RL, WL, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"BISL2",
		"Bit set longword 2 operand",
		0x00, 0xC8,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RL, ML, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BISL // Execute Routine
	},

	{
		"BISL3",
		"Bit set longword 3 operand",
		0x00, 0xC9,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RL, RL, WL, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BISL // Execute Routine
	},

	{
		"BICL2",
		"Bit clear longword 2 operand",
		0x00, 0xCA,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RL, ML, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BICL // Execute Routine
	},

	{
		"BICL3",
		"Bit clear longword 3 operand",
		0x00, 0xCB,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RL, RL, WL, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BICL // Execute Routine
	},

	{
		"XORL2",
		"Exclusive-OR longword 2 operand",
		0x00, 0xCC,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RL, ML, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_XORL // Execute Routine
	},

	{
		"XORL3",
		"Exclusive-OR longword 3 operand",
		0x00, 0xCD,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RL, RL, WL, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_XORL // Execute Routine
	},

	{
		"MNEGL",
		"Move negated longword",
		0x00, 0xCE,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RL, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_MNEGL // Execute Routine
	},

	{
		"CASEL",
		"Case longword",
		0x00, 0xCF,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RL, RL, RL, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_CASEL // Execute Routine
	},

	{
		"MOVL",
		"Move longword",
		0x00, 0xD0,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RL, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_MOVL // Execute Routine
	},

	{
		"CMPL",
		"Compare longword",
		0x00, 0xD1,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RL, RL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_CMPL // Execute Routine
	},

	{
		"MCOML",
		"Move complemented longword",
		0x00, 0xD2,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RL, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_MCOML // Execute Routine
	},

	{
		"BITL",
		"Bit test longword",
		0x00, 0xD3,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RL, RL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BITL // Execute Routine
	},

	{
		"CLRL",
		"Clear longword",
		0x00, 0xD4,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ WL, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_CLRL // Execute Routine
	},

	{
		"CLRF",
		"Clear longword",
		0x00, 0xD4,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ WF, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_CLRL // Execute Routine
	},

	{
		"TSTL",
		"Test longword",
		0x00, 0xD5,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ RL, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_TSTL // Execute Routine
	},

	{
		"INCL",
		"Increment longword",
		0x00, 0xD6,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ ML, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_INCL  // Execute Routine
	},

	{
		"DECL",
		"Decrement longword",
		0x00, 0xD7,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ ML, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_DECL  // Execute Routine
	},

	{
		"ADWC",
		"Add with carry",
		0x00, 0xD8,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RL, ML, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_ADWC // Execute Routine
	},

	{
		"SBWC",
		"Subtract with carry",
		0x00, 0xD9,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RL, ML, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_SBWC // Execute Routine
	},

	{
		"MTPR",
		"Move to processor register",
		0x00, 0xDA,     // Opcode (Extended + Normal)
		2,              // Number of Operands
		{ RL, RL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,             // Profiling Data
		vax_Opcode_MTPR  // Execute Routine
	},

	{
		"MFPR",
		"Move from processor register",
		0x00, 0xDB,     // Opcode (Extended + Normal)
		2,              // Number of Operands
		{ RL, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,             // Profiling Data
		vax_Opcode_MFPR  // Execute Routine
	},

	{
		"MOVPSL",
		"Move program status longword",
		0x00, 0xDC,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ WL, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_MOVPSL // Execute Routine
	},

	{
		"PUSHL",
		"Push longword",
		0x00, 0xDD,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ RL, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_PUSHL // Execute Routine
	},

	{
		"MOVAL",
		"Move address of longword",
		0x00, 0xDE,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ AL, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_MOVL // Execute Routine
	},

	{
		"MOVAF",
		"Move address of F_floating",
		0x00, 0xDE,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ AF, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_MOVL // Execute Routine
	},

	{
		"PUSHAL",
		"Push address of longword",
		0x00, 0xDF,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ AL, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_PUSHL // Execute Routine
	},

	{
		"PUSHAF",
		"Push address of F_floating",
		0x00, 0xDF,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ AF, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_PUSHL // Execute Routine
	},

	{
		"BBS",
		"Branch on bit set",
		0x00, 0xE0,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RL, VB, BB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BBS // Execute Routine
	},

	{
		"BBC",
		"Branch on bit clear",
		0x00, 0xE1,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RL, VB, BB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BBC // Execute Routine
	},

	{
		"BBSS",
		"Branch on bit set and set",
		0x00, 0xE2,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RL, VB, BB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BBSS // Execute Routine
	},

	{
		"BBCS",
		"Branch on bit clear and set",
		0x00, 0xE3,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RL, VB, BB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BBCS // Execute Routine
	},

	{
		"BBSC",
		"Branch on bit set and clear",
		0x00, 0xE4,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RL, VB, BB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BBSC // Execute Routine
	},

	{
		"BBCC",
		"Branch on bit clear and clear",
		0x00, 0xE5,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RL, VB, BB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BBCC // Execute Routine
	},

	{
		"BBSSI",
		"Branch on bit set and set interlocked",
		0x00, 0xE6,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RL, VB, BB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BBSS // Execute Routine
	},

	{
		"BBCCI",
		"Branch on bit clear and clear interlocked",
		0x00, 0xE7,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RL, VB, BB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BBCC // Execute Routine
	},

	{
		"BLBS",
		"Branch on low bit set",
		0x00, 0xE8,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RL, BB, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BLBS // Execute Routine
	},

	{
		"BLBC",
		"Branch on low bit clear",
		0x00, 0xE9,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RL, BB, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_BLBC // Execute Routine
	},

	{
		"FFS",
		"Find first set bit",
		0x00, 0xEA,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ RL, RB, VB, WL, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"FFC",
		"Find first clear bit",
		0x00, 0xEB,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ RL, RB, VB, WL, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CMPV",
		"Compare field",
		0x00, 0xEC,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ RL, RB, VB, RL, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_CMPV // Execute Routine
	},

	{
		"CMPZV",
		"Compare zero-extended field",
		0x00, 0xED,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ RL, RB, VB, RL, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_CMPZV // Execute Routine
	},

	{
		"EXTV",
		"Extract field",
		0x00, 0xEE,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ RL, RB, VB, WL, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_EXTV // Execute Routine
	},

	{
		"EXTZV",
		"Extract zero-extended field",
		0x00, 0xEF,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ RL, RB, VB, WL, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_EXTZV // Execute Routine
	},

	{
		"INSV",
		"Insert field",
		0x00, 0xF0,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ RL, RL, RB, VB, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"ACBL",
		"Add compare and branch longword",
		0x00, 0xF1,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ RL, RL, ML, BW, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"AOBLSS",
		"Add one and branch on less",
		0x00, 0xF2,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RL, ML, BB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_AOBLSS // Execute Routine
	},

	{
		"AOBLEQ",
		"Add one and branch on less or equal",
		0x00, 0xF3,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RL, ML, BB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_AOBLEQ // Execute Routine
	},

	{
		"SOBGEQ",
		"Subtract one and branch on greater or equal",
		0x00, 0xF4,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ ML, BB, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_SOBGEQ // Execute Routine
	},

	{
		"SOBGTR",
		"Subtract one and branch on greater",
		0x00, 0xF5,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ ML, BB, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_SOBGTR // Execute Routine
	},

	{
		"CVTLB",
		"Convert longword to byte",
		0x00, 0xF6,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RL, WB, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTLW",
		"Convert longword to word",
		0x00, 0xF7,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RL, WW, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"ASHP",
		"Arithmetic shift and round packed",
		0x00, 0xF8,   // Opcode (Extended + Normal)
		6,            // Number of Operands
		{ RB, RW, AB, RB, RW, AB }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTLP",
		"Convert longword to packed",
		0x00, 0xF9,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RL, RW, AB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CALLG",
		"Call with general argument list",
		0x00, 0xFA,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ AB, AB, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_CALLG // Execute Routine
	},

	{
		"CALLS",
		"Call with stack",
		0x00, 0xFB,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RL, AB, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_CALLS // Execute Routine
	},

	{
		"XFC",
		"Extended function call",
		0x00, 0xFC,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MFVP",
		"Move from vector processor",
		0xFD, 0x31,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTDH",
		"Convert D_floating to H_floating",
		0xFD, 0x32,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RD, WH, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTGF",
		"Convert G_floating to F_floating",
		0xFD, 0x33,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RD, WF, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VLDL",
		"Load longword vector from memory to vector register",
		0xFD, 0x34,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VGATHL",
		"Gather longword vector from memory to vector register",
		0xFD, 0x35,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VLDQ",
		"Load quadword vector from memory to vector register",
		0xFD, 0x36,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VGATHQ",
		"Gather quadword vector from memory to vector register",
		0xFD, 0x37,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"ADDG2",
		"Add G_floating 2 operand",
		0xFD, 0x40,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RG, MG, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"ADDG3",
		"Add G_floating 3 operand",
		0xFD, 0x41,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RG, RG, WG, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"SUBG2",
		"Subtract G_floating 2 operand",
		0xFD, 0x42,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RG, MG, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"SUBG3",
		"Subtract G_floating 3 operand",
		0xFD, 0x43,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RG, RG, WG, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MULG2",
		"Multiply G_floating 2 operand",
		0xFD, 0x44,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RG, MG, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MULG3",
		"Multiply G_floating 3 operand",
		0xFD, 0x45,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RG, RG, WG, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"DIVG2",
		"Divide G_floating 2 operand",
		0xFD, 0x46,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RG, MG, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"DIVG3",
		"Divide G_floating 3 operand",
		0xFD, 0x47,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RG, RG, WG, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTGB",
		"Convert G_floating to byte",
		0xFD, 0x48,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RG, WB, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTGW",
		"Convert G_floating to word",
		0xFD, 0x49,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RG, WW, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTGL",
		"Convert G_floating to longword",
		0xFD, 0x4A,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RG, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTRGL",
		"Convert rounded G_floating to longword",
		0xFD, 0x4B,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RG, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTBG",
		"Convert byte to G_floating",
		0xFD, 0x4C,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RB, WG, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTWG",
		"Convert word to G_floating",
		0xFD, 0x4D,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RW, WG, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTLG",
		"Convert longword to G_floating",
		0xFD, 0x4E,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RL, WG, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"ACBG",
		"Add compare and branch G_floating",
		0xFD, 0x4F,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ RG, RG, WG, BW, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MOVG",
		"Move G_floating",
		0xFD, 0x50,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RG, WG, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CMPG",
		"Compare G_floating",
		0xFD, 0x51,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RG, RG, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MNEGG",
		"Move negated G_floating",
		0xFD, 0x52,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RG, WG, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"TSTG",
		"Test G_floating",
		0xFD, 0x53,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ RG, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"EMODG",
		"Extended modulus G_floating",
		0xFD, 0x54,   // Opcode (Extended + Normal)
		5,            // Number of Operands
		{ RG, RW, RG, WL, WG, 0  },
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"POLYG",
		"Evaluate polynomial G_floating",
		0xFD, 0x55,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RG, RW, AB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTGH",
		"Convert G_floating to H_floating",
		0xFD, 0x56,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RG, WH, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"ADDH2",
		"Add H_floating 2 operand",
		0xFD, 0x60,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RH, MH, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"ADDH3",
		"Add H_floating 3 operand",
		0xFD, 0x61,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RH, RH, WH, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"SUBH2",
		"Subtract H_floating 2 operand",
		0xFD, 0x62,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RH, MH, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"SUBH3",
		"Subtract H_floating 3 operand",
		0xFD, 0x63,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RH, RH, WH, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MULH2",
		"Multiply H_floating 2 operand",
		0xFD, 0x64,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RH, MH, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MULH3",
		"Multiply H_floating 3 operand",
		0xFD, 0x65,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RH, RH, WH, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"DIVH2",
		"Divide H_floating 2 operand",
		0xFD, 0x66,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RH, MH, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"DIVH3",
		"Divide H_floating 3 operand",
		0xFD, 0x67,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RH, RH, WH, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTHB",
		"Convert H_floating to byte",
		0xFD, 0x68,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RH, WB, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTHW",
		"Convert H_floating to word",
		0xFD, 0x69,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RH, WW, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTHL",
		"Convert H_floating to longword",
		0xFD, 0x6A,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RH, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTRHL",
		"Convert rounded H_floating to longword",
		0xFD, 0x6B,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RH, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTBH",
		"Convert byte to H_floating",
		0xFD, 0x6C,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RB, WH, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTWH",
		"Convert word to H_floating",
		0xFD, 0x6D,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RW, WH, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTLH",
		"Convert longword to H_floating",
		0xFD, 0x6E,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RL, WH, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"ACBH",
		"Add compare and branch H_floating",
		0xFD, 0x6F,   // Opcode (Extended + Normal)
		4,            // Number of Operands
		{ RH, RH, WH, BW, 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MOVH",
		"Move H_floating",
		0xFD, 0x70,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RH, WH, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CMPH",
		"Compare H_floating",
		0xFD, 0x71,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RH, RH, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MNEGH",
		"Move negated H_floating",
		0xFD, 0x72,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RH, WH, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"TSTH",
		"Test H_floating",
		0xFD, 0x73,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ RH, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"EMODH",
		"Extended modulus H_floating",
		0xFD, 0x74,   // Opcode (Extended + Normal)
		5,            // Number of Operands
		{ RH, RW, RH, WL, WH, 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"POLYH",
		"Evaluate polynomial H_floating",
		0xFD, 0x75,   // Opcode (Extended + Normal)
		3,            // Number of Operands
		{ RH, RW, AB, 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTHG",
		"Convert H_floating to G_floating",
		0xFD, 0x76,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RH, RG, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CLRO",
		"Clear octaword",
		0xFD, 0x7C,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ WO, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CLRH",
		"Clear H_floating",
		0xFD, 0x7C,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ WH, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MOVO",
		"Move octaword",
		0xFD, 0x7D,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RO, WO, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MOVAO",
		"Move address of octaword",
		0xFD, 0x7E,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ AO, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_MOVL // Execute Routine
	},

	{
		"MOVAH",
		"Move address of H_floating",
		0xFD, 0x7E,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ AH, WL, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		vax_Opcode_MOVL // Execute Routine
	},

	{
		"PUSHAO",
		"Push address of octaword",
		0xFD, 0x7F,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ AO, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"PUSHAH",
		"Push address of H_floating",
		0xFD, 0x7F,   // Opcode (Extended + Normal)
		1,            // Number of Operands
		{ AH, 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVADDL",
		"Vector vector add longword",
		0xFD, 0x80,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSADDL",
		"Vector scalar add longword",
		0xFD, 0x81,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVADDG",
		"Vector vector add G_floating",
		0xFD, 0x82,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSADDG",
		"Vector scalar add G_floating",
		0xFD, 0x83,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVADDF",
		"Vector vector add F_floating",
		0xFD, 0x84,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSADDF",
		"Vector scalar add F_floating",
		0xFD, 0x85,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVADDD",
		"Vector vector add D_floating",
		0xFD, 0x86,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSADDD",
		"Vector scalar add D_floating",
		0xFD, 0x87,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVSUBL",
		"Vector vector subtract longword",
		0xFD, 0x88,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSSUBL",
		"Vector scalar subtract longword",
		0xFD, 0x89,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVSUBG",
		"Vector vector subtract G_floating",
		0xFD, 0x8A,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSSUBG",
		"Vector scalar subtract G_floating",
		0xFD, 0x8B,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVSUBF",
		"Vector vector subtract F_floating",
		0xFD, 0x8C,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSSUBF",
		"Vector scalar subtract F_floating",
		0xFD, 0x8D,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVSUBD",
		"Vector vector subtract D_floating",
		0xFD, 0x8E,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSSUBD",
		"Vector scalar subtract D_floating",
		0xFD, 0x8F,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTFH",
		"Convert F_floating to H_floating",
		0xFD, 0x98,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RF, WH, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTFG",
		"Convert F_floating to G_floating",
		0xFD, 0x99,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RF, WG, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSTL",
		"Store longword vector from vector register to memory",
		0xFD, 0x9C,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSCATL",
		"Scatter longword vector from vector register to memory",
		0xFD, 0x9D,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSTQ",
		"Store quadword vector from vector register to memory",
		0xFD, 0x9E,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSCATQ",
		"Scatter quadword vector from vector register to memory",
		0xFD, 0x9F,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVMULL",
		"Vector vector multiply longword",
		0xFD, 0xA0,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSMULL",
		"Vector scalar multiply longword",
		0xFD, 0xA1,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVMULG",
		"Vector vector multiply G_floating",
		0xFD, 0xA2,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSMULG",
		"Vector scalar multiply G_floating",
		0xFD, 0xA3,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVMULF",
		"Vector vector multiply F_floating",
		0xFD, 0xA4,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSMULF",
		"Vector scalar multiply F_floating",
		0xFD, 0xA5,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVMULD",
		"Vector vector multiply D_floating",
		0xFD, 0xA6,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSMULD",
		"Vector scalar multiply D_floating",
		0xFD, 0xA7,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSYNC",
		"Synchronize vector memory access",
		0xFD, 0xA8,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"MTVP",
		"Move to vector processor",
		0xFD, 0xA9,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVDIVG",
		"Vector vector divide G_floating",
		0xFD, 0xAA,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSDIVG",
		"Vector scalar divide G_floating",
		0xFD, 0xAB,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVDIVF",
		"Vector vector divide F_floating",
		0xFD, 0xAC,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSDIVF",
		"Vector scalar divide F_floating",
		0xFD, 0xAD,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVDIVD",
		"Vector vector divide D_floating",
		0xFD, 0xAE,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSDIVD",
		"Vector scalar divide D_floating",
		0xFD, 0xAF,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVCMPL",
		"Vector vector compare longword",
		0xFD, 0xC0,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSCMPL",
		"Vector scalar compare longword",
		0xFD, 0xC1,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVCMPG",
		"Vector vector compare G_floating",
		0xFD, 0xC2,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSCMPG",
		"Vector scalar compare G_floating",
		0xFD, 0xC3,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVCMPF",
		"Vector vector compare F_floating",
		0xFD, 0xC4,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSCMPF",
		"Vector scalar compare F_floating",
		0xFD, 0xC5,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVCMPD",
		"Vector vector compare D_floating",
		0xFD, 0xC6,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSCMPD",
		"Vector scalar compare D_floating",
		0xFD, 0xC7,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVBISL",
		"Vector vector bit set longword",
		0xFD, 0xC8,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSBISL",
		"Vector scalar bit set longword",
		0xFD, 0xC9,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVBICL",
		"Vector vector bit clear longword",
		0xFD, 0xCC,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSBICL",
		"Vector scalar bit clear longword",
		0xFD, 0xCD,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVSRLL",
		"Vector vector shift right logical longword",
		0xFD, 0xE0,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSSRLL",
		"Vector scalar shift right logical longword",
		0xFD, 0xE1,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVSLLL",
		"Vector vector shift left logical longword",
		0xFD, 0xE4,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSSLLL",
		"Vector scalar shift left logical longword",
		0xFD, 0xE5,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVXORL",
		"Vector vector exclusive-OR longword",
		0xFD, 0xE8,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSXORL",
		"Vector scalar exclusive-OR longword",
		0xFD, 0xE9,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVCVT",
		"Vector convert",
		0xFD, 0xEC,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"IOTA",
		"Generate compressed iota vector",
		0xFD, 0xED,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VVMERGE",
		"Vector vector merge",
		0xFD, 0xEE,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"VSMERGE",
		"Vector scalar merge",
		0xFD, 0xEF,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ 0 , 0 , 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTHF",
		"Convert H_floating to F_floating",
		0xFD, 0xF6,   // Opcode (Extended + Normal)
		2,            // Number of Operands
		{ RH, WF, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	{
		"CVTHD",
		"Convert H_floating to D_floating",
		0xFD, 0xF7,   // Opcode (Extended + Normal)
		0,            // Number of Operands
		{ RH, WD, 0 , 0 , 0 , 0  }, // Operand Scale/Mode
		0L,           // Profiling Data
		NULL          // Execute Routine
	},

	// Null terminator here
	{
		NULL, NULL,
		0, 0, 0,
		{ 0, 0, 0, 0, 0, 0 },
		0, NULL
	}

};
