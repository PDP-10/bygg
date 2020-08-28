// memory.c - PDP10 memory routines
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

#include <malloc.h>

#include "pdp10/defs.h"
#include "pdp10/proto.h"

int36 p10_MemorySize;
int36 *p10_Memory;

// Fast memory for AC blocks
int36 p10_ACB[NACBLOCKS][020];
int36 *curAC, *prvAC; // Current/Previous AC block

void p10_InitMemory(int32 size)
{
	int i, j;

	// Initialize main memory
	p10_MemorySize = size;
	p10_Memory = (int36 *)calloc(size, sizeof(int36));

	// Initialize AC blocks
	curAC = &p10_ACB[CACB = 0][0];
	prvAC = &p10_ACB[PACB = 0][0];
	for (i = 0; i < NACBLOCKS; i++)
		for (j = 0; j < 020; j++)
			p10_ACB[i][j] = 0;
}

void p10_ResetMemory(void)
{
	int i, j;

	// Clear all memory.
	memset(p10_Memory, 0, p10_MemorySize * sizeof(int36));

	// Clear all AC blocks
	for (i = 0; i < NACBLOCKS; i++)
		for (j = 0; j < 020; j++)
			p10_ACB[i][j] = 0;
}

void p10_ReleaseMemory(void)
{
	free(p10_Memory);
}

/*****************************************************/

// Check Non-existing memory area by using physical address 
//   Return TRUE  if memory is not existing 
//   Return FALSE if memory is existing 

/*
int p10_DoNXM(uint30 pAddr, int mode)
{
#ifdef DEBUG
	dbg_Printf("MEM: Non-existing Memory at address %06llo,,%06llo\n",
		LHSR(pAddr), RH(pAddr));
#endif DEBUG

	// Request an interrupt for non-existing memory encounter.
	p10_aprInterrupt(APRSR_F_NO_MEMORY);

	// If pager system is on, go page fail trap.
	if (pager_On) {
		lhPFW = PFW_NXM|PFW_PAGED;
		rhPFW = pAddr;
		PAGE_FAIL_TRAP(mode);
	}

	return EMU_NXM;
}

inline int p10_CheckNXM(uint30 pAddr, int mode)
{
	if (pAddr >= p10_MemorySize)
		return p10_DoNXM(pAddr, mode);
	return EMU_OK;
}
*/

inline int p10_CheckNXM(uint30 pAddr, int mode)
{
	if (pAddr >= p10_MemorySize) {
#ifdef DEBUG
		dbg_Printf("MEM: Non-existing Memory at address %06llo,,%06llo\n",
			LHSR(pAddr), RH(pAddr));
#endif DEBUG

		// Request an interrupt for non-existing memory encounter.
		p10_aprInterrupt(APRSR_F_NO_MEMORY);

		// If pager system is on, go page fail trap.
		if (pager_On) {
			lhPFW = PFW_NXM|PFW_PAGED;
			rhPFW = pAddr;
			PAGE_FAIL_TRAP(mode);
		}

		return EMU_NXM;
	}
	return EMU_OK;
}

// Physical Direct Memory Access
//
// Note: do not provide access to AC blocks,
//       use virtual memory access instead.

inline int36 mem_plhRead(int30 paddr)
{
	int36 data;

	if (!p10_CheckNXM(paddr, 0))
		data = p10_Memory[paddr];
	else
		data = 0;

	return LHSR(data);
}

inline int36 mem_prhRead(int30 paddr)
{
	int36 data;

	if (!p10_CheckNXM(paddr, 0))
		data = p10_Memory[paddr];
	else
		data = 0;

	return RH(data);
}

inline void mem_plhWrite(int30 paddr, int36 ldata)
{
	if (!p10_CheckNXM(paddr, 0)) {
		p10_Memory[paddr] = RHSL(ldata) | RH(p10_Memory[paddr]);
		p10_Memory[paddr] = SXT36(p10_Memory[paddr]);
	}
}

inline void mem_prhWrite(int30 paddr, int36 rdata)
{
	if (!p10_CheckNXM(paddr, 0)) {
		p10_Memory[paddr] = LH(p10_Memory[paddr]) | RH(rdata);
		p10_Memory[paddr] = SXT36(p10_Memory[paddr]);
	}
}

// Virtual Memory Access

inline int mem_vRead36(uint30 vAddr, int36 *data, int mode)
{
	int30 pAddr;
	int   st;

	if (vAddr < 020)
		*data = ((mode & PTF_PREV) ? prvAC : curAC)[vAddr];
	else {
		if (pager_On) {
			if (st = p10_PageFill(vAddr, &pAddr, mode | PTF_READ))
				return st;
		} else
			pAddr = vAddr;
		if (st = p10_CheckNXM(pAddr, mode))
			return st;
		*data = p10_Memory[pAddr];
	}

	return EMU_OK;
}

inline int mem_vWrite36(uint30 vAddr, int36 data, int mode)
{
	int30 pAddr;
	int   st;

	if (vAddr < 020)
		((mode & PTF_PREV) ? prvAC : curAC)[vAddr] = SXT36(data);
	else {
		if (pager_On) {
			if (st = p10_PageFill(vAddr, &pAddr, mode | PTF_WRITE))
				return st;
		} else
			pAddr = vAddr;
		if (st = p10_CheckNXM(pAddr, mode))
			return st;
		p10_Memory[pAddr] = SXT36(data);
	}

	return EMU_OK;
}

//**************  New Memory Routines *************

// Read Executive Memory
inline int36 p10_eRead(uint30 vAddr)
{
	if (vAddr < 020)
		return curAC[vAddr];
	else {
		int30 pAddr;

		// Translate virtual address into physical address.
		if (pager_On)
			p10_PageFill(vAddr, &pAddr, PTF_EXEC);
		else
			pAddr = vAddr;
		p10_CheckNXM(pAddr, 0);

		return p10_Memory[pAddr];
	}
}

// Read Physical Memory
inline int36 p10_pRead(uint30 pAddr, int mode)
{
	p10_CheckNXM(pAddr, mode);
	return p10_Memory[pAddr];
}

// Read Virtual Memory
inline int36 p10_vRead(uint30 vAddr, int mode)
{
	int30 pAddr;

	if (vAddr < 020)
		return ((mode & PTF_PREV) ? prvAC : curAC)[vAddr];
	else {

		// Translate virtual address into physical address.
		if (pager_On) {
			p10_PageFill(vAddr, &pAddr, mode | PTF_READ);
		} else
			pAddr = vAddr;
		p10_CheckNXM(pAddr, mode);

		return p10_Memory[pAddr];
	}
}

// Write Executive Memory
inline void p10_eWrite(uint30 vAddr, int36 data)
{
	if (vAddr < 020)
		curAC[vAddr] = SXT36(data);
	else {
		int30 pAddr;

		// Translate virtual address into physical address.
		if (pager_On)
			p10_PageFill(vAddr, &pAddr, PTF_EXEC|PTF_WRITE);
		else
			pAddr = vAddr;
		p10_CheckNXM(pAddr, 0);

		p10_Memory[pAddr] = SXT36(data);
	}
}

// Write Physical Memory
inline void p10_pWrite(uint30 pAddr, int36 data, int mode)
{
	p10_CheckNXM(pAddr, mode);
	p10_Memory[pAddr] = SXT36(data);
}

//  Write Virtual Memory
inline void p10_vWrite(uint30 vAddr, int36 data, int mode)
{
	int30 pAddr;

	if (vAddr < 020)
		((mode & PTF_PREV) ? prvAC : curAC)[vAddr] = SXT36(data);
	else {

		// Translate virtual address into physical address.
		if (pager_On)
			p10_PageFill(vAddr, &pAddr, mode | PTF_WRITE);
		else
			pAddr = vAddr;
		p10_CheckNXM(pAddr, mode);

		p10_Memory[pAddr] = SXT36(data);
	}
}


// Provide direct access to virtual memory.
// Also implies read/write access test.
inline int36 *p10_Access(uint30 vAddr, int mode)
{
	int30 pAddr;

	if (vAddr < 020)
		return &((mode & PTF_PREV) ? prvAC : curAC)[vAddr];
	else {

		// Translate virtual address into physical address.
		if (pager_On)
			p10_PageFill(vAddr, &pAddr, mode);
		else
			pAddr = vAddr;
		p10_CheckNXM(pAddr, mode);

		return &p10_Memory[pAddr];
	}
}
