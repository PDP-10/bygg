// commands.c - the console routines for commands and prompt
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

//#include "emu/defs.h"
#include "pdp10/defs.h"
#include "pdp10/proto.h"
#include "emu/socket.h"

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

int ts10_CmdList(int argc, char **argv)
{
	sock_ShowList();
	return EMU_OK;
}

int console_CmdLog(int argc, char **argv)
{
	if (argc != 2)
		printf("Usage: log <filename|off>\n");
	else {
		util_RemoveSpaces(argv[1]);
		if (!strcasecmp(argv[1], "off")) {
			close(emu_logFile);
			emu_logFile = -1;
			return EMU_OK;
		}

		if (emu_logFile >= 0) {
			printf("Log file already was opened.\n");
			return EMU_OK;
		}

		if ((emu_logFile = open(argv[1], O_CREAT|O_WRONLY|O_APPEND, 0700)) < 0) {
			printf("log: %s: %s\n", argv[1], strerror(errno));
			return EMU_OK;
		}
	}
	return EMU_OK;
}

int console_CmdRun(int argc, char **argv)
{
	if (argc != 1)
		printf("Usage: run\n");
	else {
		printf("Running now...\n");
		emu_State = EMU_RUN;
	}
	return EMU_OK;
}

int console_CmdStart(int argc, char **argv)
{
	if (argc != 2)
		printf("Usage: start <address>\n");
	else {
		pdp10_Start(argv[1]);
		emu_State = EMU_RUN;
	}
	return EMU_OK;
}

int console_CmdQuit(int argc, char **argv)
{
	printf("Exiting PDP10 Emulator\n");
	emu_State = EMU_QUIT;
	return EMU_OK;
}

COMMAND console_Cmds[] =
{
	{ "attach",   unit_CmdAttach },
	{ "boot",     unit_CmdBoot   },
	{ "continue", console_CmdRun },
	{ "create",   unit_CmdCreate },
#ifdef DEBUG
	{ "debug",    dbg_CmdDebug },
#endif
	{ "delete",   unit_CmdDelete },
	{ "detach",   unit_CmdDetach },
	{ "format",   unit_CmdFormat },
	{ "init",     unit_CmdInit },
	{ "list",     ts10_CmdList },
	{ "log",      console_CmdLog },
	{ "exit",     console_CmdQuit },
	{ "run",      console_CmdRun },
	{ "start",    console_CmdStart },
#ifdef DEBUG
	{ "trace",    dbg_CmdTrace },
#endif
	{ "use",      ts10_CmdUse },
	{ "quit",     console_CmdQuit },
	{ NULL }
};

int console_Execute(char *line)
{
	int  argc;
	char *argv[TS10_MAXARGS];
	int  idx;

	// Break a line into a list of arguments.
	for (argc = 0; (*line && argc < TS10_MAXARGS); argc++)
		argv[argc] = util_SplitWord(&line);

	if (argc > 0) {
		// Search table for desired I/O command.
		if (ts10_UseUnit) {
			UNIT      *uptr   = ts10_UseUnit;
			DEVICE    *dptr   = uptr->Device;		
			IOCOMMAND *ioCmds = dptr->Commands;
			int       st;

			for (idx = 0; ioCmds[idx].Name; idx++) {
				if (!strcasecmp(argv[0], ioCmds[idx].Name)) {
					if ((st = ioCmds[idx].Execute(uptr, argc, argv)) == EMU_ARG) {
						printf("Usage: %s %s\n", ioCmds[idx].Name, ioCmds[idx].Usage);
					}
					return st;
				}
			}
		}

		// Search table for desired command.
		for (idx = 0; console_Cmds[idx].nCommand; idx++) {
			if (!strcasecmp(argv[0], console_Cmds[idx].nCommand)) {
				return console_Cmds[idx].Execute(argc, argv);
			}
		}

		return EMU_UNKNOWN;
	}

	return EMU_OK;
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
