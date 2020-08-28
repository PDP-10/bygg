/*
 *
 * console.c - the console routines for commands and prompt
 *
 * Written by
 *  Timothy Stark <sword7@speakeasy.org>
 *
 * This file is part of PDP/E, The PDP Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

//#include "emu/defs.h"
#include "pdp10/defs.h"
#include "pdp10/proto.h"

extern char *emu_Name;

char *emu_ErrorMessages[] =
{
	"Non-existant Memory",
	"Memory Error",
	"Open Error",
	"I/O Error",
	"Unit is already present",
	"Unit is not present",
	"Unit is disabled",
	"Unit is not attachable",
	"Unit is already attached",
	"Invalid Argument",
	"Unknown Command",
	"Not Found",
	"Conflict",
	"Not Supported",
	"Not Bootable"
};

/* ACTION: Move it to PDP10 section later. */
int console_CmdDump(char **args)
{
	char *buf = args[1];
	int addr, saddr, eaddr;
	int64 data;

	if (!buf[0])
		printf("Usage: dump <start> <end>\n");
	else {
		sscanf(buf, "%o %o", &saddr, &eaddr);

		for (addr = saddr; addr < eaddr; addr++) {
			mem_pRead36(addr, &data, PTF_CONSOLE);
			printf("%06o %s\n", addr, pdp10_DisplayData(data));
		}
	}
	return EMU_OK;
}

int console_CmdLoad(char **args)
{
	char *par = args[1];
	int  st = EMU_OK;

	if (!par[0])
		printf("Usage: load <filename>\n");
	else {
		util_RemoveSpaces(par);
		if ((st = pdp10_LoadExeFile(par)) == EMU_OK)
			printf("File '%s' had been loaded.\n", par);
	}
	return st;
}

int console_CmdLog(char **args)
{
	char *par = args[1];

	if (!par[0])
		printf("Usage: log <filename|off>\n");
	else {
		util_RemoveSpaces(par);
		if (!strcasecmp(par, "off")) {
			close(emu_logFile);
			emu_logFile = -1;
			return EMU_OK;
		}

		if (emu_logFile >= 0) {
			printf("Log file already was opened.\n");
			return EMU_OK;
		}

		if ((emu_logFile = open(par, O_CREAT|O_WRONLY|O_APPEND, 0700)) < 0) {
			printf("log: %s: %s\n", par, strerror(errno));
			return EMU_OK;
		}
	}
	return EMU_OK;
}

int console_CmdRim(char **args)
{
	char *par = args[1];
	int  st = EMU_OK;

	if (!par[0])
		printf("Usage: rim <filename>\n");
	else {
		util_RemoveSpaces(par);
		if ((st = pdp10_LoadRimFile(par)) == EMU_OK)
			printf("File '%s' had been loaded.\n", par);
	}
	return st;
}

int console_CmdRun(char **args)
{
	char *buf = args[1];

	printf("Running now...\n");
	emu_State = EMU_RUN;
	return EMU_OK;
}

int console_CmdStart(char **args)
{
	char *buf = args[1];

	pdp10_Start(buf);
	emu_State = EMU_RUN;
	return EMU_OK;
}

int console_CmdQuit(char **args)
{
	char *buf = args[1];

	printf("Exiting PDP10 Emulator\n");
	emu_State = EMU_QUIT;
//	emu_Cleanup();
//	exit(1);
	return EMU_OK;
}

COMMAND console_Cmds[] =
{
#ifdef DEBUG
	{ "asm",      dbg_CmdAsm },
#endif
	{ "attach",   unit_CmdAttach },
	{ "boot",     unit_CmdBoot   },
	{ "continue", console_CmdRun },
	{ "create",   unit_CmdCreate },
#ifdef DEBUG
	{ "debug",    dbg_CmdDebug },
#endif
	{ "delete",   unit_CmdDelete },
	{ "detach",   unit_CmdDetach },
#ifdef DEBUG
	{ "disasm",   dbg_CmdDisasm },
#endif
	{ "dump",     console_CmdDump },
	{ "format",   unit_CmdFormat },
	{ "init",     unit_CmdInit },
	{ "load",     console_CmdLoad },
	{ "log",      console_CmdLog },
	{ "map",      pager_CmdShowMap },
	{ "exit",     console_CmdQuit },
	{ "rim",      console_CmdRim },
	{ "run",      console_CmdRun },
	{ "start",    console_CmdStart },
#ifdef DEBUG
	{ "trace",    dbg_CmdTrace },
#endif
	{ "quit",     console_CmdQuit },
	{ NULL }
};

int console_Execute(char *line)
{
	char *args[2];
	char *ptr = line;
	char *cmd = (char *)util_SplitWord(&ptr);
	int  idx;

	for (idx = 0; console_Cmds[idx].nCommand; idx++) {
		if (!strcasecmp(cmd, console_Cmds[idx].nCommand)) {
			args[0] = cmd;
			args[1] = ptr;
			return console_Cmds[idx].Execute(args);
		}
	}
	return EMU_UNKNOWN;
}

void console_Prompt(void)
{
	char buf[80];
	int  st;

	printf("%s> ", emu_Name);
	fgets(buf, 80, stdin);
	buf[strlen(buf)-1] = '\0';

	if (st = console_Execute(buf)) {
		if (st >= EMU_BASE) {
			if (st == EMU_OPENERR)
				printf("%s: %s\n", emu_ErrorMessages[st - EMU_BASE],
					strerror(errno));
			else
				printf("%s\n", emu_ErrorMessages[st - EMU_BASE]);
		}
		return;
	}
}

void console_ExecuteFile(char *filename)
{
	FILE *config;
	char line[256];
	int  lineno = 0;
	int  len;
	int  idx;
	int  st;

	if ((config = fopen(filename, "r")) == NULL) {
		perror(filename);
		return;
	}

	while (!feof(config)) {
		if (fgets(line, 256, config) != NULL) {
			lineno++;

			// Remove '\n' from end of each line.
			len = strlen(line)-1;
			line[len] = '\0';

			// Ignore all comments after ';' character
			for (idx = 0; idx < len; idx++)
				if (line[idx] == ';')
					break;
			line[idx] = '\0';

			util_RemoveSpaces(line);

			// Now execute commands
			if (line[0]) {
				if (st = console_Execute(line)) {
					if (st >= EMU_BASE) {
						if (st == EMU_OPENERR)
							printf("%s: %s\n", emu_ErrorMessages[st - EMU_BASE],
								strerror(errno));
						else
							printf("%s\n", emu_ErrorMessages[st - EMU_BASE]);
					}
					printf("Error occurs at line %d\n", lineno);
				}
			}
		}
	}

	fclose(config);
}
