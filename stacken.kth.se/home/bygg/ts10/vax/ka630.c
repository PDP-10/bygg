// ka630.c - KA630 Bus System Configuration (MicroVAX II series)
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

// KA630 - Memory Map
//
// 00000000 +-------------------------------------+
//          |           Main Memory               |
//          |- - - - - - - - - - - - - - - - - - -|
//          |        Up to 16 MB Memory           |
// 01000000 +-------------------------------------+
//          |             Reserved                |
// 20000000 +-------------------------------------+
//          |          Q-Bus I/O space            |
// 20002000 +-------------------------------------+
//          |                                     |
// 20040000 +-------------------------------------+
//          |               ROM                   |
// 20080000 +-------------------------------------+
//          |      Local I/O Register Area        |
// 200C0000 +-------------------------------------+
//          |                                     |
// 3FFFFFFF +-------------------------------------+

#include "vax/defs.h"

typedef struct {
	void (*Read)(int32, uint *);
	void (*Write)(int32, uint);
} KA630MAP;

KA630MAP ka630_Map[0x4000];

void ka630_Fault(int32 pAddr, uint8 *data)
{
}

void ka630_Ignore(int32 pAddr, uint8 *data)
{
	// Do nothing - access ignored
}

void ka630_ReadRAM(int32 pAddr, uint8 *data)
{
}

void ka630_WriteRAM(int32 pAddr, uint8 *data)
{
}

void ka630_ReadROM(int32 pAddr, uint8 *data)
{
	*data = vax->ROM[pAddr & vax->maskROM];
}

int ka630_Initialize(void)
{
	int idx;

	for (idx = 0; idx < 0x4000; idx++) {
		ka630_Map[idx].Read  = ka630_Fault;
		ka630_Map[idx].Write = ka630_Fault;
	}

	// RAM Space Area - 0000000 to 00FFFFFF
	for (idx = 0x0000; idx < 0x00FF; idx++) {
		ka630_Map[idx].Read  = ka630_ReadRAM;
		ka630_Map[idx].Write = ka630_WriteRAM;
	}

	// ROM Space Area - 2004000 to 2007FFFF
	for (idx = 0x2004; idx < 0x2008; idx++) {
		ka630_Map[idx].Read  = ka630_ReadROM;
		ka630_Map[idx].Write = ka630_Ignore;
	}

	return VAX_OK;
}
