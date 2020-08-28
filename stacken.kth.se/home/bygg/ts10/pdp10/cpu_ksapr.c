// cpu_ksapr.c - KS10 Processor: APR (Arithmetic Processor) routines
//
// Written by
//  Timothy Stark <sword7@speakeasy.org>
//
// This file is part of the TS10 Emulator.
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

int apr_Enables; // Interrupt Enables
int apr_Flags;   // System Flags
int apr_Level;   // PI Channel Level

// Initialize/Reset APR registers
void p10_ResetAPR(void)
{
	apr_Enables = 0; // Interrupt Enables
	apr_Flags   = 0; // System Flags
	apr_Level   = 0; // PI Channel Level
}

// Request an interrupt
inline void p10_aprInterrupt(int flag)
{
	apr_Flags |= flag;
	if (apr_Enables & flag) {
		p10_piRequestAPR(apr_Level);
		p10_piEvaluate();
	}
}

// 70000 APRID  Get APR Identification
void p10_ksOpcode_APRID(void)
{
	BR = KS10_MC_OPTS & APRID_M_MC_OPTS;
	BR |= (KS10_MC_VER << APRID_V_MC_VER) & APRID_M_MC_VER;
	BR |= KS10_HW_OPTS & APRID_M_HW_OPTS;
	BR |= KS10_SN & APRID_M_PROC_SN;
	p10_vWrite(eAddr, BR, NOPXCT);
}

// 70020 WRAPR  Write APR register
void p10_ksOpcode_WRAPR(void)
{
	int flags = eAddr & APR_FLAGS;

	apr_Level = eAddr & APR_LEVEL;
	if (eAddr & APR_ENABLE)  apr_Enables |= flags;
	if (eAddr & APR_DISABLE) apr_Enables &= ~flags;
	if (eAddr & APR_SET)     apr_Flags   |= flags;
	if (eAddr & APR_CLEAR)   apr_Flags   &= ~flags;

	// Process CTY device when an interrupt console had been set.
	if (apr_Flags & APRSR_F_INT_CON) {
		p10_ctyOutput();
		apr_Flags &= ~APRSR_F_INT_CON;
	}

	p10_piEvaluate();
}

// 70024 RDAPR  Read APR register
void p10_ksOpcode_RDAPR(void)
{
	int36 apr_sr;

	apr_sr = ((apr_Enables & apr_Flags) ? APR_IRQ : 0) |
	         (apr_Enables << 18) | apr_Flags | apr_Level;

	p10_vWrite(eAddr, apr_sr, NOPXCT);
}

// 70030 CONSO APR, Instruction
void p10_ksOpcode_COAPR(void)
{
	int18 apr_sr;

	apr_sr = ((apr_Enables & apr_Flags) ? APR_IRQ : 0) |
	         apr_Flags | apr_Level;

	if (apr_sr & eAddr)
		DO_SKIP;
}

// 70034 CONSZ APR, Instruction
void p10_ksOpcode_CZAPR(void)
{
	int18 apr_sr;

	apr_sr = ((apr_Enables & apr_Flags) ? APR_IRQ : 0) |
	         apr_Flags | apr_Level;

	if ((apr_sr & eAddr) == 0)
		DO_SKIP;
}
