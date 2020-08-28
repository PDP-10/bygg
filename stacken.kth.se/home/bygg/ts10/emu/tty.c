// tty.c - the tty routines
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

#include <termios.h>
#include <unistd.h>

#include "emu/defs.h"

struct termios cmdtty; /* Command mode */
struct termios runtty; /* Run mode */

void tty_Initialize(void)
{
	tcgetattr(0, &cmdtty);
	runtty = cmdtty;

	runtty.c_lflag &= ~(ICANON | ECHO);  /* No echo and edit */
	runtty.c_iflag &= ~ICRNL;            /* No CR Conversion */
	runtty.c_oflag &= ~OPOST;            /* No output edit */
	runtty.c_cc[VINTR]  = 5;  // Interrupt - ^E
	runtty.c_cc[VQUIT]  = 20; // Trace     - ^T
	runtty.c_cc[VERASE] = 0;
	runtty.c_cc[VKILL]  = 0;
	runtty.c_cc[VEOF]   = 0;
	runtty.c_cc[VEOL]   = 0;
	runtty.c_cc[VSTART] = 0;
	runtty.c_cc[VSUSP]  = 0;
	runtty.c_cc[VSTOP]  = 0;
	runtty.c_cc[VMIN]   = 0;
	runtty.c_cc[VTIME]  = 0;
}

void tty_RunState(void)
{
	tcsetattr(0, TCSAFLUSH, &runtty);
}

void tty_CmdState(void)
{
	tcsetattr(0, TCSAFLUSH, &cmdtty);
}

int tty_GetChar(void)
{
	int status;
	char ch;

	status = read(0, &ch, 1);
	if (status > 0) 
		return ch;
	return 0;
}

int tty_GetString(char *pString, int len)
{
	return read(STDIN_FILENO, pString, len);
}

int tty_PutChar(int32 out)
{
	char ch = out;

	write(1, &ch, 1);
}
