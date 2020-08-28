// commands.c - VAX commands
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

#ifdef DEBUG

int vax_cmdAsm(UNIT *uptr, int argc, char **argv)
{
	printf("Not implemented yet.\n");
	return EMU_OK;
}

int vax_cmdDisAsm(UNIT *uptr, int argc, char **argv)
{
	static int32 sAddr = 0; // Start address (Initially 0)
	int32 count;

	if (argc > 1)
		sscanf(argv[1], "%x", &sAddr);
	if (argc > 2)
		sscanf(argv[2], "%d", &count);
	else
		count = 20;

	// Display disassembly listing.
	while (count--)
		vax11disasm_Opcode(&sAddr);

	return EMU_OK;
}

int vax_cmdDump(UNIT *uptr, int argc, char **argv)
{
	static int32 sAddr = 0; // Start address (Initially 0)
	static int32 lAddr = 0; // Length address (Initially 0)

	if (argc > 1)
		sscanf(argv[1], "%x", &sAddr);
	if (argc > 2)
		sscanf(argv[2], "%x", &lAddr);
	else
		lAddr = 0x140;

	vax_Dump(sAddr, lAddr);

	return EMU_OK;
}

#endif DEBUG

int vax_cmdLoad(UNIT *uptr, int argc, char **argv)
{
	int st = EMU_OK;

	if (argc != 2)
		return EMU_ARG;
	else {
		util_RemoveSpaces(argv[1]);
		if ((st = vax_LoadBoot(argv[1], 0x1000)) == EMU_OK)
			printf("File '%s' had been loaded.\n", argv[1]);
	}
	return st;
}

IOCOMMAND vax_Commands[] = {
#ifdef DEBUG
	{ "asm",     "{Not Implemented Yet}",  vax_cmdAsm    },
	{ "disasm",  "[address] [count]",      vax_cmdDisAsm },
	{ "dump",    "[address] [length]",     vax_cmdDump   },
#endif DEBUG
	{ "load",    "<filename>",             vax_cmdLoad   },
	{ NULL,      NULL,                     NULL          },
};
