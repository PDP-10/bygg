// proto.h - Prototypes for PDP10 emulation routines.
//
// Written by
//  Timothy Stark <sword7@speakeasy.org>
//
// This file is part of the TS-10 Emulator.
// See ReadMe for copyright notice.
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

// pdp10/fe.c
int  p10_ctyInitialize(void);
int  p10_ctyCleanup(void);
void p10_ctySendDone(void);
void p10_ctyCheckQueue(void);
void p10_ctyOutput(void);

// pdp10/extend.c
int  p10_extOpcode_EUUO(void);
int  p10_extOpcode_CMPS(void);
int  p10_extOpcode_EDIT(void);
int  p10_extOpcode_CVTDB(void);
int  p10_extOpcode_CVTBD(void);
int  p10_extOpcode_MOVS(void);
void p10_Opcode_EXTEND(void);

// pdp10/cpu_main.c
inline int36 p10_ksCalcJumpAddr(int36, int);
inline int36 p10_ksCalcBPAddr(int36, int);
inline int36 p10_ksGetEffAddr(int36);
inline int36 p10_ksGetIOAddr(int36);
inline int   p10_CheckPXCT(int);
inline void  p10_Execute(int30);
void  p10_Initialize(void);
void  p10_ResetCPU(void);
int   p10_Go(void);

void  p10_ksOpcode_LUUO(void);
void  p10_ksOpcode_UUO(void);

// pdp10/cpu_ksapr.c
void p10_ksOpcode_APRID(void);
void p10_ksOpcode_COAPR(void);
void p10_ksOpcode_CZAPR(void);
void p10_ksOpcode_RDAPR(void);
void p10_ksOpcode_WRAPR(void);

void p10_aprReset(void);
void p10_aprInterrupt(int);

// pdp10/cpu_ksio.c
void p10_ksOpcode_IO700(void);
void p10_ksOpcode_IO701(void);
void p10_ksOpcode_IO702(void);

void p10_ksOpcode_TIOE(void);
void p10_ksOpcode_TION(void);
void p10_ksOpcode_RDIO(void);
void p10_ksOpcode_WRIO(void);
void p10_ksOpcode_BSIO(void);
void p10_ksOpcode_BCIO(void);
void p10_ksOpcode_BLTBU(void);
void p10_ksOpcode_BLTUB(void);

void p10_ksOpcode_TIOEB(void);
void p10_ksOpcode_TIONB(void);
void p10_ksOpcode_RDIOB(void);
void p10_ksOpcode_WRIOB(void);
void p10_ksOpcode_BSIOB(void);
void p10_ksOpcode_BCIOB(void);

// pdp10/cpu_kspag.c
void p10_ksOpcode_RDSPB(void);
void p10_ksOpcode_RDCSB(void);
void p10_ksOpcode_RDPUR(void);
void p10_ksOpcode_RDCSTM(void);
void p10_ksOpcode_RDHSB(void);

void p10_ksOpcode_WRSPB(void);
void p10_ksOpcode_WRCSB(void);
void p10_ksOpcode_WRPUR(void);
void p10_ksOpcode_WRCSTM(void);
void p10_ksOpcode_WRHSB(void);

void p10_ksOpcode_RDUBR(void);
void p10_ksOpcode_CLRPT(void);
void p10_ksOpcode_WRUBR(void);
void p10_ksOpcode_WREBR(void);
void p10_ksOpcode_RDEBR(void);

int   p10_PageFailTrap1(int);
void  p10_PageFailTrap2(void);
inline int   p10_PageFill(uint30, uint30 *, int);
int36 p10_GetMap(int36, int);
int   p10_CmdShowMap(char **);
void  p10_SetHaltStatus(void);
void  p10_ClearCache(void);
void  p10_ResetPager(void);

// pdp10/cpu_kspi.c
void p10_ksOpcode_COPI(void);
void p10_ksOpcode_CZPI(void);
void p10_ksOpcode_RDPI(void);
void p10_ksOpcode_WRPI(void);

void p10_piReset(void);
inline void p10_piRequestIO(int);
inline void p10_piRequestAPR(int);
inline void p10_piDismiss(void);
inline void p10_piEvaluate(void);
void p10_piProcess(void);

// pdp10/cpu_kstim.c
void p10_ksOpcode_RDTIM(void);
void p10_ksOpcode_RDINT(void);
void p10_ksOpcode_WRTIM(void);
void p10_ksOpcode_WRINT(void);

void p10_HandleTimer(int);

// pdp10/uba.c
void  ks10uba_Initialize(UNIT *, int32);
void  ks10uba_Reset(UNIT *);
int   ks10uba_Create(UNIT *, char *, int, char **);
int   ks10uba_Delete(int32);
int   ks10uba_Configure(UNIT *, UNIT *, int32, int32);
void  ks10uba_DoInterrupt(UNIT *, int32, int32);
void  ks10uba_DisableInterrupt(UNIT *, int32);
int32 ks10uba_CheckInterrupt(int32 *, int32);
int18 ks10uba_ReadData18(UNIT *, int18);
int36 ks10uba_ReadData36(UNIT *, int18);
void  ks10uba_WriteData18(UNIT *, int18, int18);
void  ks10uba_WriteData36(UNIT *, int18, int36);
void  ks10uba_PageFailTrap(int30, int);
int36 ks10uba_ReadIO(int30, int);
void  ks10uba_WriteIO(int30, int36, int);

// pdp10/asm.c
int p10_Assemble(int36 *, char *);

// pdp10/disasm.c
void p10_Disassemble(int30, int36, int);

// pdp10/alu.c
inline void p10_spAdd(int36 *, int36);
inline void p10_dpAdd(int36 *, int36 *, int36, int36);

inline void p10_spSubtract(int36 *, int36);
inline void p10_dpSubtract(int36 *, int36 *, int36, int36);

inline void p10_spMultiply(int36 *, int36);
inline void p10_dpMultiply(int36 *, int36 *, int36);
inline void p10_qpMultiply(int36 *, int36 *, int36 *, int36 *, int36, int36);

inline int  p10_spDivide(int36 *, int36 *, int36);
inline int  p10_dpDivide(int36 *, int36 *, int36);
inline int  p10_qpDivide(int36 *, int36 *, int36 *, int36 *, int36, int36);

inline void p10_spMagnitude(int36 *);
inline void p10_spNegate(int36 *);
inline void p10_dpNegate(int36 *, int36 *);

inline void p10_spInc(int36 *);
inline void p10_spDec(int36 *);

inline void p10_spAShift(int36 *, int36);
inline void p10_dpAShift(int36 *, int36 *, int36);

inline void p10_spLShift(int36 *, int36);
inline void p10_dpLShift(int36 *, int36 *, int36);

inline void p10_spRotate(int36 *, int36);
inline void p10_dpRotate(int36 *, int36 *, int36);

// pdp10/fpu.c

void p10_Opcode_DFAD(void);
void p10_Opcode_DFSB(void);
void p10_Opcode_DFMP(void);
void p10_Opcode_DFDV(void);

void p10_Opcode_FIX(void);
void p10_Opcode_FIXR(void);
void p10_Opcode_FLTR(void);
void p10_Opcode_FSC(void);

void p10_Opcode_FAD(void);
void p10_Opcode_FADM(void);
void p10_Opcode_FADB(void);

void p10_Opcode_FADR(void);
void p10_Opcode_FADRI(void);
void p10_Opcode_FADRM(void);
void p10_Opcode_FADRB(void);

void p10_Opcode_FSB(void);
void p10_Opcode_FSBM(void);
void p10_Opcode_FSBB(void);

void p10_Opcode_FSBR(void);
void p10_Opcode_FSBRI(void);
void p10_Opcode_FSBRM(void);
void p10_Opcode_FSBRB(void);

void p10_Opcode_FMP(void);
void p10_Opcode_FMPM(void);
void p10_Opcode_FMPB(void);

void p10_Opcode_FMPR(void);
void p10_Opcode_FMPRI(void);
void p10_Opcode_FMPRM(void);
void p10_Opcode_FMPRB(void);

void p10_Opcode_FDV(void);
void p10_Opcode_FDVM(void);
void p10_Opcode_FDVB(void);

void p10_Opcode_FDVR(void);
void p10_Opcode_FDVRI(void);
void p10_Opcode_FDVRM(void);
void p10_Opcode_FDVRB(void);

// pdp10/memory.c
void  p10_InitMemory(int32);
void  p10_ResetMemory(void);
void  p10_ReleaseMemory(void);
// int   p10_DoNXM(uint30, int);
inline int   p10_CheckNXM(uint30, int);

inline int36 p10_eRead(uint30);
inline int36 p10_pRead(uint30, int);
inline int36 p10_vRead(uint30, int);
inline void  p10_eWrite(uint30, int36);
inline void  p10_pWrite(uint30, int36, int);
inline void  p10_vWrite(uint30, int36, int);
inline int36 *p10_Access(uint30, int);

inline int36 mem_plhRead(int30);
inline int36 mem_prhRead(int30);
inline void  mem_plhWrite(int30, int36);
inline void  mem_prhWrite(int30, int36);

inline int mem_vRead36(uint30, int36 *, int);
inline int mem_vWrite36(uint30, int36, int);

// pdp10/system.c
int    p10_Create(UNIT *, char *, int, char **);
int    p10_Configure(UNIT *, char *, int, char **);
char  *pdp10_DisplayData(int36);
int36  pdp10_Convert8to36(uchar *);
uchar *pdp10_Convert36to8(int36);
int    exe_GetWord(int, int36 *);
int    pdp10_LoadRimFile(char *);
int    pdp10_LoadExeFile(char *);
