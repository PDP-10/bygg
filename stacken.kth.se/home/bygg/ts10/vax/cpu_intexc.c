// cpu_intexc.c - VAX Interrupt/Exception routines
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

void vax_Exception(int32 code)
{

}

void vax_Emulate(void)
{
	if (vax->PSL.Bit.FPD) {

	} else {
		vax11mem_vWrite(vax->SP + 44, vax->opc, 4);
		vax11mem_vWrite(vax->SP + 40, fault_PC, 4);
		vax11mem_vWrite(vax->SP + 36, OP0, 4);
		vax11mem_vWrite(vax->SP + 32, OP1, 4);
		vax11mem_vWrite(vax->SP + 28, OP2, 4);
		vax11mem_vWrite(vax->SP + 24, OP3, 4);
		vax11mem_vWrite(vax->SP + 20, OP4, 4);
		vax11mem_vWrite(vax->SP + 16, OP5, 4);
		vax11mem_vWrite(vax->SP + 12, OP6, 4);
		vax11mem_vWrite(vax->SP + 8,  OP7, 4);
		vax11mem_vWrite(vax->SP + 4,  vax->PC, 4);
		vax11mem_vWrite(vax->SP, saved_PSL, 4);
		vax->SP -= 44;
	}
}
