// main.c - main routines
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

#include <stdio.h>
#include <signal.h>

#ifdef SVR4SIGNALS
#include <unistd.h>
#include <stropts.h>
#endif

#include "emu/defs.h"

char *emu_Name    = "TS10";
char *emu_Version = "v0.7.1 (Alpha)";
void (*emu_IOTrap)(void) = NULL;
int  emu_State;
int  emu_logFile = -1;

FILE *debug;

#ifdef SVR4SIGNALS
struct sigaction sigInt;   // ^E Exit
struct sigaction sigSegV;  // Bad memory
struct sigaction sigQuit;  // ^T Trace
struct sigaction sigIO;    // Keyboard activity
#endif

void emu_Interrupt(int s)
{
	emu_State = EMU_HALT;

#ifndef SVR4SIGNALS
	signal(SIGINT, emu_Interrupt);
#endif
}

void emu_Trace(int s)
{
#ifdef DEBUG
	if (dbg_Check(DBG_TRACE)) {
		dbg_ClearMode(DBG_TRACE|DBG_DATA);
		printf("[Trace off]\r\n");
	} else {
		dbg_SetMode(DBG_TRACE|DBG_DATA);
		printf("[Trace on]\r\n");
	}
#endif DEBUG

#ifndef SVR4SIGNALS
	signal(SIGQUIT, emu_Trace);
#endif
}

void emu_BadMemory(int s)
{
	printf("*** Segmentation Failure ***\r\n");
	timer_Stop();
	tty_CmdState();
	fclose(debug);

#if defined(SVR4SIGNALS) && !defined(linux)
	if (ioctl(STDIN_FILENO, I_SETSIG, 0) < 0) {
		perror("IOCTL I_SETSIG");
	}
#endif

	exit(1);
}

void emu_IO(int s)
{
	if (emu_IOTrap)
		emu_IOTrap();

#ifndef SVR4SIGNALS
	signal(SIGIO,  emu_IO);
	signal(SIGURG, emu_IO);
#endif
}

int main(int argc, char **argv)
{
	int idx;

	printf("Welcome to %s Emulator %s\n", emu_Name, emu_Version);

#ifdef SVR4SIGNALS
	sigInt.sa_handler = emu_Interrupt;
	sigInt.sa_flags   = 0;
	sigaction(SIGINT, &sigInt, NULL);

	sigQuit.sa_handler = emu_Trace;
	sigQuit.sa_flags   = 0;
	sigaction(SIGINT, &sigQuit, NULL);

	sigSegV.sa_handler = emu_BadMemory;
	sigSegV.sa_flags   = 0;
//	sigaction(SIGSEGV, &sigSegV, NULL);

	sigIO.sa_handler = emu_IO;
	sigIO.sa_flags   = 0;
	sigaction(SIGIO, &sigIO, NULL);
	sigaction(SIGURG, &sigIO, NULL);
#else
	signal(SIGINT,  emu_Interrupt);
	signal(SIGQUIT, emu_Trace);
//	signal(SIGSEGV, emu_BadMemory);
	signal(SIGIO,   emu_IO);
	signal(SIGURG,  emu_IO);
#endif

	debug = fopen("debug.log", "w");

	tty_Initialize();
	sock_Initialize();

	emu_State = EMU_CONSOLE;

	for (idx = 1; idx < argc; idx++) {
		if (argv[idx][0] == '-') {
			switch (argv[idx][1]) {
				case 'f': // Configuration file
					console_ExecuteFile(argv[++idx]);
					break;

				default:
					printf("Unknown option: %s\n", argv[idx]);
			}
		}
	}
	
	while (emu_State) {
		if (emu_State == EMU_CONSOLE)
			console_Prompt();
		else {
			if (ts10_UseDevice) {
				if (ts10_UseDevice->Execute)
					ts10_UseDevice->Execute();
				else {
					printf("Device %s - Execute Not Supported.\n",
						ts10_UseDevice->Name);
				}
			} else
				printf("Please type 'use <device>' first.\n");
			emu_State = EMU_CONSOLE;
		}
	}

	fclose(debug);
}
