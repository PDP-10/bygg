// proto.h - Prototypes for all .c files.
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

// commands.c

// cpu_branch.c
int vax_SetBit(int32 *, int32);
int vax_TestBit(int32 *);
void vax_Opcode_AOBLEQ(void);
void vax_Opcode_AOBLSS(void);
void vax_Opcode_BBC(void);
void vax_Opcode_BBS(void);
void vax_Opcode_BBCC(void);
void vax_Opcode_BBCS(void);
void vax_Opcode_BBSC(void);
void vax_Opcode_BBSS(void);
void vax_Opcode_BCC(void);
void vax_Opcode_BCS(void);
void vax_Opcode_BEQL(void);
void vax_Opcode_BGEQ(void);
void vax_Opcode_BGTR(void);
void vax_Opcode_BGTRU(void);
void vax_Opcode_BLBC(void);
void vax_Opcode_BLBS(void);
void vax_Opcode_BLEQ(void);
void vax_Opcode_BLEQU(void);
void vax_Opcode_BLSS(void);
void vax_Opcode_BNEQ(void);
void vax_Opcode_BRB(void);
void vax_Opcode_BRW(void);
void vax_Opcode_BSBB(void);
void vax_Opcode_BSBW(void);
void vax_Opcode_BVC(void);
void vax_Opcode_BVS(void);
void vax_Opcode_CASEB(void);
void vax_Opcode_CASEW(void);
void vax_Opcode_CASEL(void);
void vax_Opcode_JMP(void);
void vax_Opcode_JSB(void);
void vax_Opcode_RSB(void);
void vax_Opcode_SOBGEQ(void);
void vax_Opcode_SOBGTR(void);

// cpu_call.c
void vax_Call(int32 *, int);
void vax_Opcode_CALLG(void);
void vax_Opcode_CALLS(void);
void vax_Opcode_RET(void);

// cpu_compare.c
void vax_Opcode_CMPB(void);
void vax_Opcode_CMPW(void);
void vax_Opcode_CMPL(void);
void vax_Opcode_TSTB(void);
void vax_Opcode_TSTW(void);
void vax_Opcode_TSTL(void);

// cpu_field.c
int32 vax_GetField(int32 *, int32);
void vax_Opcode_CMPV(void);
void vax_Opcode_CMPZV(void);
void vax_Opcode_EXTV(void);
void vax_Opcode_EXTZV(void);

// cpu_integer.c
void vax_Opcode_ADAWI(void);
void vax_Opcode_ADDB(void);
void vax_Opcode_ADDW(void);
void vax_Opcode_ADDL(void);
void vax_Opcode_ADWC(void);
void vax_Opcode_ASHL(void);
void vax_Opcode_ASHQ(void);
void vax_Opcode_BICB(void);
void vax_Opcode_BICW(void);
void vax_Opcode_BICL(void);
void vax_Opcode_BISB(void);
void vax_Opcode_BISW(void);
void vax_Opcode_BISL(void);
void vax_Opcode_BITB(void);
void vax_Opcode_BITW(void);
void vax_Opcode_BITL(void);
void vax_Opcode_CLRB(void);
void vax_Opcode_CLRW(void);
void vax_Opcode_CLRL(void);
void vax_Opcode_CLRQ(void);
void vax_Opcode_DECB(void);
void vax_Opcode_DECW(void);
void vax_Opcode_DECL(void);
void vax_Opcode_INCB(void);
void vax_Opcode_INCW(void);
void vax_Opcode_INCL(void);
void vax_Opcode_PUSHL(void);
void vax_Opcode_ROTL(void);
void vax_Opcode_SBWC(void);
void vax_Opcode_SUBB(void);
void vax_Opcode_SUBW(void);
void vax_Opcode_SUBL(void);
void vax_Opcode_XORB(void);
void vax_Opcode_XORW(void);
void vax_Opcode_XORL(void);

// cpu_main.c
void vax_Abort(int32);
char *vax_DisplayCondition(void);
void vax_DecodeOperand(INSTRUCTION *, int32 *);
int  vax_Execute(void);

// cpu_misc.c - Miscellaneous Instructions
void vax_Opcode_BICPSW(void);
void vax_Opcode_BISPSW(void);
void vax_Opcode_BPT(void);
void vax_Opcode_BUGL(void);
void vax_Opcode_BUGW(void);
void vax_Opcode_HALT(void);
void vax_Opcode_INDEX(void);
void vax_Opcode_MOVPSL(void);
void vax_Opcode_NOP(void);
void vax_Opcode_POPR(void);
void vax_Opcode_PUSHR(void);
void vax_Opcode_XFC(void);

// cpu_move.c
void vax_Opcode_MCOMB(void);
void vax_Opcode_MCOMW(void);
void vax_Opcode_MCOML(void);
void vax_Opcode_MNEGB(void);
void vax_Opcode_MNEGW(void);
void vax_Opcode_MNEGL(void);
void vax_Opcode_MOVB(void);
void vax_Opcode_MOVW(void);
void vax_Opcode_MOVL(void);
void vax_Opcode_MOVQ(void);
void vax_Opcode_MOVZBW(void);
void vax_Opcode_MOVZBL(void);
void vax_Opcode_MOVZWL(void);

// cpu_system.c
void vax_Opcode_MFPR(void);
void vax_Opcode_MTPR(void);

// disasm.c
void vax11disasm_AddDest(int32);
int  vax11disasm_Operand(INSTRUCTION *, int, int32 *, char *, int);
int  vax11disasm_Opcode(int32 *);
int  vax11disasm_Initialize(void);

// memory.c
int vax11mem_Initialize(int);
int vax11mem_Cleanup(void);
int vax11mem_pbRead(int32, uint8 *);
int vax11mem_pRead(int32, uint8 *, int);
int vax11mem_pbWrite(int32, uint8);
int vax11mem_pWrite(int32, uint8 *, int);
int vax11mem_vbRead(int32, uint8 *);
int vax11mem_vRead(int32, uint8 *, int);
int vax11mem_vbWrite(int32, uint8);
int vax11mem_vWrite(int32, uint8 *, int);

// system.c
int  vax_Create(UNIT *, char *, int, char **);
int  vax_Configure(UNIT *, char *, int, char **);
int  vax_LoadBoot(char *, int32);
int  vax_LoadROM(char *);
int  vax_LoadNVRAM(char *);
int  vax_Dump(int32, int32);
void vax_Initialize(void);
void vax_CheckInstructions(void);

// ka780.c (KA780 System Configurations)
int32 ka780_prRead(int32);
void  ka780_prWrite(int32, int32);
void  ka780_Initialize(void);
