// memory.c - Memory Configurations
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

union {
	uint8  *Byte;
	uint16 *Word;
	uint32 *Long;
	uint64 *Quad;
} vax_Memory;

#define IN_ROM(addr) \
	(((addr) >= vax->baseROM) && ((addr) <= vax->endROM))
#define IN_NVRAM(addr) \
	(((addr) >= vax->baseNVRAM) && ((addr) <= vax->endNVRAM))

int vax11mem_Initialize(int reqSize)
{
	// Create physical memory
	vax->RAM     = (char *)malloc(reqSize);
	vax->baseRAM = 0L;
	vax->endRAM  = reqSize - 1;
	vax->sizeRAM = reqSize;

	// Clear all physical memory
	memset(vax->RAM, 0, reqSize);

	return VAX_OK;
}

int vax11mem_Cleanup(void)
{
	// Free physical memory back to host system.
	if (vax->RAM)
		free(vax->RAM);

	// Reset all memory values;
	vax->RAM     = NULL;
	vax->baseRAM = -1L;
	vax->endRAM  = -1L;
	vax->sizeRAM = 0;

	return VAX_OK;
}

// Access memory by physical address.

inline int vax11mem_pbRead(int32 pAddr, uint8 *data)
{
	pAddr &= PAMASK;
	*data = 0;
	if (pAddr < vax->sizeRAM) {
		*data = vax->RAM[pAddr];
		return VAX_OK;
	}
	if (IN_ROM(pAddr)) {
		*data = vax->ROM[pAddr - vax->baseROM];
		return VAX_OK;
	}
	return VAX_NXM;
}

inline int vax11mem_pRead(int32 pAddr, uint8 *data, int length)
{
	int count;

	for (count = 0; count < length; count++) {
		pAddr &= PAMASK;
		if (pAddr < vax->sizeRAM) {
			*data++ = vax->RAM[pAddr++];
		} else if (IN_ROM(pAddr)) {
			*data++ = vax->ROM[pAddr++ - vax->baseROM];
		}
	}
	return VAX_OK;
}

inline int vax11mem_pbWrite(int32 pAddr, uint8 data)
{
	pAddr &= PAMASK;
	if (pAddr < vax->sizeRAM) {
		vax->RAM[pAddr] = data;
		return VAX_OK;
	}
	return VAX_NXM;
}

inline int vax11mem_pWrite(int32 pAddr, uint8 *data, int length)
{
	int count;

	for (count = 0; count < length; count++) {
		pAddr &= PAMASK;
		if (pAddr < vax->sizeRAM) {
			vax->RAM[pAddr++] = *data++;
		} else if (IN_ROM(pAddr)) {
			vax->ROM[pAddr++ - vax->baseROM] = *data++;
		}
	}
	return VAX_OK;
}

// Access memory by virtual address.

inline int vax11mem_vbRead(int32 pAddr, uint8 *data)
{
	return vax11mem_pbRead(pAddr, data);
}

inline int vax11mem_vRead(int32 pAddr, uint8 *data, int length)
{
	return vax11mem_pRead(pAddr, data, length);
}

inline int vax11mem_vbWrite(int32 pAddr, uint8 data)
{
	return vax11mem_pbWrite(pAddr, data);
}

inline int vax11mem_vWrite(int32 pAddr, uint8 *data, int length)
{
	return vax11mem_pWrite(pAddr, data, length);
}
