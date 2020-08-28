// debug.c - Debugging Facility
//
// Written by
//  Timothy Stark <sword7@speakeasy.org>
//
// This file is part of the TS10 Emulator.
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

#ifdef DEBUG

#include "emu/defs.h"

DBG_MODES dbg_Modes[] = {
	{ "Console",   DBG_CONSOLE },
	{ "Data",      DBG_DATA  },
	{ "Interrupt", DBG_INTERRUPT },
	{ "IOData",    DBG_IODATA },
	{ "IORegs",    DBG_IOREGS },
	{ "Operand",   DBG_OPERAND },
	{ "Sockets",   DBG_SOCKETS },
	{ "Table",     DBG_TABLE },
	{ "Trace",     DBG_TRACE },
	{ NULL,        0         }  // Null terminator
};

int dbg_Mode = 0;

boolean dbg_Check(int mode)
{
	return ((dbg_Mode & mode) == mode);
}

void dbg_SetMode(int newMode)
{
	dbg_Mode |= newMode;
}

void dbg_ClearMode(int newMode)
{
	dbg_Mode &= ~newMode;
}

int dbg_GetMode(void)
{
	return dbg_Mode;
}

void dbg_Printf(cchar *Format, ...)
{
	char tmpBuffer[1024];
	va_list Args;
	int len;

	va_start(Args, Format);
	len = vsnprintf(tmpBuffer, 1023, Format, Args);
	tmpBuffer[1023] = 0;
	va_end(Args);

/*
	if (emu_State == EMU_RUN)
		fwrite(tmpBuffer, len, 1, debug);
	else if (emu_State == EMU_CONSOLE)
		printf(tmpBuffer);
*/
	fwrite(tmpBuffer, len, 1, debug);
	fflush(debug);
}

int dbg_CmdDebug(int argc, char **argv)
{
	int  idx;

	if (argc == 1) {
		for (idx = 0; dbg_Modes[idx].Name; idx++)
			printf("Debug %s:\t%s\n", dbg_Modes[idx].Name,
				(dbg_Mode & dbg_Modes[idx].Mode) ? "on" : "off");
	} else {
		for (idx = 0; dbg_Modes[idx].Name; idx++)
			if (!strcasecmp(argv[1], dbg_Modes[idx].Name))
				break;

		if (dbg_Modes[idx].Name) {
			util_RemoveSpaces(argv[2]);
			if (argc == 2)
				printf("Debug %s: %s\n", dbg_Modes[idx].Name,
					(dbg_Mode & dbg_Modes[idx].Mode) ? "on" : "off");
			else {
				if (argc == 3) {
					if (!strcasecmp(argv[2], "on")) {
						dbg_Mode |= dbg_Modes[idx].Mode;
						printf("Debug %s had been turned on.\n",
							dbg_Modes[idx].Name);
					} else if (!strcasecmp(argv[2], "off")) {
						dbg_Mode &= ~dbg_Modes[idx].Mode;
						printf("Debug %s had been turned off.\n",
							dbg_Modes[idx].Name);
					}
				} else
					printf("Usage: debug [mode] [on|off]\n");
			}
		} else
			printf("Unknown debug mode: %s\n", argv[1]);
	}

	return EMU_OK;
}

int dbg_CmdTrace(int argc, char **argv)
{
	if (argc == 1)
		printf("Trace: %s\n", (dbg_Mode & DBG_TRACE) ? "on" : "off");
	else {
		util_RemoveSpaces(argv[1]);
		if (!strcasecmp(argv[1], "on")) {
			dbg_Mode |= DBG_TRACE;
			printf("Trace had been turned on.\n");
		} else if (!strcasecmp(argv[1], "off")) {
			dbg_Mode &= ~DBG_TRACE;
			printf("Trace had been turned off.\n");
		} else
			printf("Usage: trace [on|off]\n");
	}
	return EMU_OK;
}

#endif
