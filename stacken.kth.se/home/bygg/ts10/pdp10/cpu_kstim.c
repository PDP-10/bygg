// cpu_kstim.c - KS10 Processor: Timer System Routines
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

#include "pdp10/defs.h"
#include "pdp10/proto.h"

#ifndef SVR4SIGNALS
#include <signal.h>
#endif

int64 timebase;  // time base (59 (71-12) bits)
int36 period;    // Initial period for interval timer
int36 ttg;       // "Time to go" for interval timer
int   jiffy = 0; // 100 jiffies per second

// System Timing

void p10_HandleTimer(int x)
{
	int pi;

#ifndef SVR4SIGNALS
	// [AAK] Note to Tim - Is this handler reentrant?
	signal(SIGALRM, SIG_IGN);
#endif

	// Increase time base by 10 because 10 millisecond
	// (100 jiffies per second) limitation in
	// Red Hat Linux operating system
	timebase = (timebase + TB_JIFFY) & TB_MASK;

	if (period) {
		ttg -= TB_JIFFY;

		// Set "Interval Done" flag on APR and request an interrupt.
		// Reset TTG (Time To Go) by using initial Period
		if (ttg <= 0) {
			ttg = period;
			p10_aprInterrupt(APRSR_F_INT_DONE);
		}
	}

	if (jiffy++ == 100) {
		jiffy = 0;
//		dbg_Printf("CPU: %d instructions per second.\n", cips);
		fprintf(debug, "CPU: %d instructions per second.\n", cips);
		cips = 0;
	}

#ifndef SVR4SIGNALS
	signal(SIGALRM, p10_HandleTimer);
#endif
}


// 70260 WRTIM - Write Time Base Register
void p10_ksOpcode_WRTIM(void)
{
	// (E,E+1[0:23]) -> (Time Base)
	// 0 -> (Time Base[24:35])

	mem_vRead36(eAddr++, &AR, NOPXCT);
	mem_vRead36(eAddr,   &ARX, NOPXCT);
	timebase = (AR << (35 - TB_HW_BITS));
	timebase |= (ARX & WORD36_MAXP) >> TB_HW_BITS;

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA))
		dbg_Printf("TIME: Time Base <- %012llo %012llo\n", AR, ARX);
#endif DEBUG
}

// 70220 RDTIM - Read Time Base Register
void p10_ksOpcode_RDTIM(void)
{
	// (Time Base) -> (E,E+1)

	AR  = timebase >> (35 - TB_HW_BITS);
	ARX = (timebase << TB_HW_BITS) & WORD36_MAXP;
	mem_vWrite36(eAddr, AR, NOPXCT);
	mem_vWrite36((eAddr + 1) & VMA_MASK, ARX, NOPXCT);

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA))
		dbg_Printf("TIME: Time Base -> %012llo %012llo\n", AR, ARX);
#endif DEBUG
}

// 70264 WRINT - Write Interval Register
void p10_ksOpcode_WRINT(void)
{
	// (E) -> (Interval)

	mem_vRead36(eAddr, &AR, NOPXCT);
	period = AR >> TB_HW_BITS;
	ttg    = period;

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA))
		dbg_Printf("TIME: Interval <- %012llo\n", AR);
#endif DEBUG
}

// 70224 RDINT - Read Interval Register
void p10_ksOpcode_RDINT(void)
{
	// (Interval) -> (E)

	AR = period << TB_HW_BITS;
	mem_vWrite36(eAddr, AR, NOPXCT);

#ifdef DEBUG
	if (dbg_Check(DBG_TRACE|DBG_DATA))
		dbg_Printf("TIME: Interval -> %012llo\n", AR);
#endif DEBUG
}
