// timer.c - timer routines are provided for the PDP emulator
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

#include <sys/time.h>
#include <signal.h>

#include "emu/defs.h"

#ifdef SVR4SIGNALS
struct sigaction sigTimer;
#endif

struct itimerval startTimer =
{ 
	{ 0, 1000 },
	{ 0, 1000 }
};

struct itimerval stopTimer =
{ 
	{ 0, 0 },
	{ 0, 0 }
};

struct itimerval oldTimer;

int timer_Start(void)
{
	setitimer(ITIMER_REAL, &startTimer, &oldTimer);
	return EMU_OK;
}

int timer_Stop(void)
{
	setitimer(ITIMER_REAL, &stopTimer, &oldTimer);
	return EMU_OK;
}

int timer_SetAlarm(void (*handler)(int))
{
#ifdef SVR4SIGNALS
	sigTimer.sa_handler = handler;
	sigTimer.sa_flags   = 0;
	sigaction(SIGALRM, &sigTimer, NULL);
#else
	signal(SIGALRM, handler);
#endif
	return EMU_OK;
}
