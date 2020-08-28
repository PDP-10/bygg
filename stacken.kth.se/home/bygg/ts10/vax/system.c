// system.c - VAX System Configurations
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

VAXUNIT *vax = NULL;
INSTRUCTION *vax_basOpcode[256];
INSTRUCTION *vax_extOpcode[256];

char *vax_Prompt  = "VAX";
char *vax_Version = "v0.2a";

DTYPE vax_dTypes[] =
{
	{
		"KA630",      // CPU Name
		"MicroVAX II Series",
		UNIT_DISABLE, // CPU is disable
		0, 0, 0, 0,   // Not used
		0x08000000,   // System ID
		NULL,         // Slave Drive Types - Not Used

		0,            // KA630 is a central process unit
		NULL          // Not Used
	},

	{
		"KA780",     // CPU Name
		"VAX-11/780 Series",
		UNIT_DISABLE, // CPU is disable
		0, 0, 0, 0,   // Not used
		0x01000000,   // System ID
		NULL,         // Slave Drive Types - Not Used

		0,            // KA780 is a central process unit
		NULL          // Not Used
	},
		
	{ NULL }
};

DEVICE *vax_Devices[] =
{
	// ACTION: Q-bus/Unibus routines need to being implemented

	NULL
};

DEVICE vax_Device =
{
	"VAX",           // Device Name
	"Virtual Address Extension",
	"v0.2a (Pre-Alpha)",  // Version
	vax_dTypes,      // Drive Type
	NULL,            // Number of Units
	vax_Devices,     // Listing of devices
	vax_Commands,    // List of Commands
	NULL,            // List of Set Commands
	NULL,            // List of Show Commands

	0,               // Number of Devices
	0,               // Number of Units

	vax_Initialize,   // Initialize Routine
	NULL,             // Reset Routine
	vax_Create,       // Create Routine
	NULL,             // Delete Routine
	vax_Configure,    // Configure Routine
	NULL,             // Enable Routine
	NULL,             // Disable Routine
	NULL,             // Attach/Mount Routine
	NULL,             // Detach/Unmount Routine
	NULL,             // Format Routine
	NULL,             // Read I/O Routine
	NULL,             // Write I/O Routine
	NULL,             // Process Routine
	NULL,             // Boot Routine
	vax_Execute,      // Execute Routine

	NULL,             // SetUnit Routine
	NULL,             // SetATA Routine
	NULL,             // ClearATA Routine
	NULL,             // ReadData (18-bit) Routine
	NULL,             // ReadData (36-bit) Routine
	NULL,             // WriteData (18-bit) Routine
	NULL,             // WriteData (36-bit) Routine
	NULL,             // Ready Rountine
	NULL,             // CheckWordCount Routine
};

int vax_Create(UNIT *uptr, char *devName, int argc, char **argv)
{
	DEVICE *dptr;

	if ((dptr = unit_FindDevice(vax_Devices, devName)) == NULL) {
		printf("Device %s - Aborted (Reason: Not Recongized)\n",
			devName);
		return EMU_ARG;
	}

	if (dptr->Create)
		return dptr->Create(uptr, devName, argc, argv);
	else {
		printf("Device %s - Aborted (Reason: Not Implemented.)\n",
			devName);
		return EMU_OK;
	}
}

int vax_Configure(UNIT *uptr, char *devName, int argc, char **argv)
{
	int    idx;

	if (argc != 1) {
		printf("Error: Wrong number of arguments.\n");
		printf("Usage: create %s: VAX <model>\n", devName);
		return EMU_ARG;
	}

	for (idx = 0; vax_dTypes[idx].Name; idx++)
		if (!strcasecmp(argv[0], vax_dTypes[idx].Name))
			break;

	if (!vax_dTypes[idx].Name)
		return EMU_ARG;

	vax_Initialize();

	// Mark this unit as CPU/Processor type.
	uptr->Device = &vax_Device;
	uptr->dType  = &vax_dTypes[idx];
	uptr->tFlags = UT_PROCESSOR;
	uptr->Flags  = UNIT_PRESENT;

	// Make this CPU device current for following commands.
	ts10_UseDevice = uptr->Device;
	ts10_UseUnit   = uptr;

	// Display information now.
	printf("Device %s - %s (%s, %s)\n", devName, uptr->dType->Name,
		uptr->Device->Desc, uptr->dType->Desc);
	printf("Version: %s\n", uptr->Device->Version);

	return EMU_OK;
}

// Load VMB.EXE into VAX memory
int vax_LoadBoot(char *fn, int32 sAddr)
{
	int vmbFile;
	int eAddr;
	int idx;
	int st = 1;

	if ((vmbFile = open(fn, O_RDONLY)) < 0)
		return EMU_OPENERR;

	for (idx = sAddr; st > 0; idx += 512) {
		st = read(vmbFile, &vax->RAM[idx], 512);
	}
	eAddr = idx;

	close(vmbFile);

	printf("Bootstrap code %s loaded from %08X to %08X\n",
		fn, sAddr, eAddr);

	R0  = 0;
	R1  = 0;
	R2  = 0;
	R3  = 0;
	R4  = 0;
	R5  = 0;
	R6  = 0;
	R7  = 0;
	R8  = 0;
	R9  = 0;
	R10 = 0;
	R11 = 0;
	AP  = 0;
	FP  = 0;
	SP  = sAddr;
	PC  = sAddr;

	return VAX_OK;
}

// Load ROM image into VAX memory
int vax_LoadROM(char *fn)
{
	int romFile;
	int baseAddr, lenAddr;
	int idx;

	if ((romFile = open(fn, O_RDONLY)) < 0)
		return EMU_OPENERR;

	read(romFile, &baseAddr, sizeof(baseAddr));
	read(romFile, &lenAddr, sizeof(lenAddr));

	vax->ROM     = (uint8 *)malloc(lenAddr);
	vax->baseROM = baseAddr;
	vax->endROM  = (baseAddr + lenAddr) - 1;
	vax->sizeROM = lenAddr;

	for (idx = 0; idx < lenAddr; idx += 32768) {
		read(romFile, &vax->ROM[idx], 32768);
	}

	close(romFile);

	printf("ROM Image loaded from %08X to %08X\n", vax->baseROM, vax->endROM);
	return VAX_OK;
}

// Load NVRAM image into VAX memory
int vax_LoadNVRAM(char *fn)
{
}

// Dump contents into terminal from memory area
int vax_Dump(int32 sAddr, int32 lAddr)
{
	int32 eAddr = (sAddr + lAddr) - 1;
	int32 pAddr;
	int   idx;
	char  ascBuffer[16];
	char  *pasc;
	uint8 data;

	for(pAddr = sAddr; pAddr <= eAddr;) {
		printf("%08X: ", pAddr);
		pasc = ascBuffer;
		for (idx = 0; (idx < 16) && (pAddr <= eAddr); idx++) {
			vax11mem_pbRead(pAddr++, &data);
			printf("%02X%c", data, (idx == 7) ? '-' : ' ');
			*pasc++ = ((data >= 32) && (data < 127)) ? data : '.';
		}
		*pasc = '\0';
		printf(" |%-16s|\n", ascBuffer);
	}

	return VAX_OK;
}

void vax_CheckInstructions(void)
{
	char outBuffer[80];
	int  done, total;
	int  idx;

	outBuffer[0] = 0;
	total = 0;
	done  = 0;

	printf("Following instructions that need to be implemented:\n");
	for (idx = 0; vax_Instruction[idx].Name; idx++) {
		total++;
		if (vax_Instruction[idx].Execute == NULL) {
			if (strlen(outBuffer) > 70) {
				outBuffer[strlen(outBuffer)] = '\0';
				printf("%s\n", outBuffer);
				outBuffer[0] = '\0';
			}

			if (outBuffer[0])
				strcat(outBuffer, ", ");
			strcat(outBuffer, vax_Instruction[idx].Name);
		} else
			done++;
	}

	if (outBuffer[0]) {
		outBuffer[strlen(outBuffer)] = '\0';
		printf("%s\n", outBuffer);
	}

	printf("\n%d of %d instructions had been implemented.\n", done, total);
}

// ACTION: This routine will be moved to model routines later
// Build the generic instruction table for execution routines
int vax_gInitialize(void)
{
	uint8 extended, opcode;
	int   idx;

	// Clear all instruction tables first.
	for(idx = 0; idx < 256; idx++) {
		vax_basOpcode[idx] = NULL;
		vax_extOpcode[idx] = NULL;
	}

	// Build new instruction tables now.
	for(idx = 0; vax_Instruction[idx].Name; idx++) {
		extended = vax_Instruction[idx].Extended;
		opcode   = vax_Instruction[idx].Opcode;

		switch(extended) {
			case 0x00: // Normal function
				if (vax_basOpcode[opcode] == NULL)
					vax_basOpcode[opcode] = &vax_Instruction[idx];
				break;

			case 0xFD: // Extended function
				if (vax_extOpcode[opcode] == NULL)
					vax_extOpcode[opcode] = &vax_Instruction[idx];
				break;
		}
	}

	return VAX_OK;
}

void vax_Initialize(void)
{
	vax = malloc(sizeof(VAXUNIT));

	memset(vax, 0, sizeof(VAXUNIT));
	vax11mem_Initialize(8096 * 1024);

	vax11disasm_Initialize();
	vax_gInitialize();
	ka780_Initialize();
}
