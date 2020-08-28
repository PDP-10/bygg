// system.c - TS10 Emulator System Configurations
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

#include "emu/defs.h"

extern DEVICE p10_Device;
extern DEVICE vax_Device;

DEVICE *ts10_Devices[] =
{
	&p10_Device, // PDP-10 CPU System
	&vax_Device  // VAX-11 CPU System
};

// Root device for TS10 Emulator
DEVICE ts10_Device =
{
	"CPU",            // Device Name
	"TS10 Emulator",  // System Name
	"v0.7.2 (Alpha)", // Version
	NULL,             // Drive Type
	NULL,             // Number of Units
	ts10_Devices,     // Listing of devices
	NULL,             // List of Commands
	NULL,             // List of Set Commands
	NULL,             // List of Show Commands

	0,                // Number of Devices
	0,                // Number of Units

	ts10_Initialize,  // Initialize Routine
	NULL,             // Reset Routine
	ts10_Create,      // Create Routine
	ts10_Delete,      // Delete Routine
	NULL,             // Configure Routine
	NULL,             // Enable Routine
	NULL,             // Disable Routine
	NULL,             // Attach/Mount Routine
	NULL,             // Detach/Unmount Routine
	NULL,             // Format Routine
	NULL,             // Read I/O Routine
	NULL,             // Write I/O Routine
	NULL,             // Process Routine
	NULL,             // Boot Routine
	NULL,             // Execute Routine

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

// Root units
int    ts10_nUnits     = 0;    // Desired number of CPU units
UNIT   **ts10_Units    = NULL; // List of current CPU units
UNIT   *ts10_UseUnit   = NULL; // Current Unit
DEVICE *ts10_UseDevice = NULL; // Current Device

void ts10_Initialize(void)
{
}

// Create CPU unit
int ts10_Create(UNIT *pUnit, char *devName, int argc, char **argv)
{
	int unit, len;

	len  = strlen(ts10_Device.Name);

	// Create root CPU: device
	if (devName[len] == ':' || devName[len] == '\0') {
		int  nUnits;
		char *tptr;

		if (ts10_Units != NULL) {
			printf("Error: Device %s already created.\n", devName);
			return EMU_ARG;
		}

		if (argc != 1) {
			printf("Error: wrong number of arguments.\n");
			printf("Usage: create %s: <units>\n", ts10_Device.Name);
			return EMU_ARG;
		}

		nUnits = strtol(argv[0], &tptr, 10);
		if (argv[0] == tptr) {
			printf("Error: argument must be value.\n");
			printf("Usage: create %s <units>\n", ts10_Device.Name);
			return EMU_ARG;
		}

		ts10_Units = (UNIT **)calloc(1, nUnits);
		if (ts10_Units == NULL) {
			printf("Error: Memory Not Enough\n");
			return EMU_MEMERR;
		}
		ts10_nUnits = nUnits;

		return EMU_OK;
	}

	if (ts10_Units == NULL) {
		printf("Device %s: - Non-existant.\n", ts10_Device.Name);
		printf("Type 'create %s: <units>' first.\n", ts10_Device.Name);
		return EMU_FATAL;
	}

	unit = devName[len++] - '0';

	if (unit < ts10_nUnits) {
		DEVICE *dptr;
		UNIT   *uptr;
		char   *ndevName = &devName[len+1];
		int    st;

		if (devName[len] == '>') {

			// Make sure that unit is existing
			if (ts10_Units[unit] == NULL) {
				printf("Error: Unit %s is non-existant.\n", devName);
				return EMU_ARG;
			}

			// Get a specific device for desired processor.
			uptr = ts10_Units[unit];
			dptr = uptr->Device;

			if (dptr->Create)
				return dptr->Create(uptr, ndevName, argc, argv);
			else {
				printf("Device %s - Create routine.\n", dptr->Name);
				printf("Reason: Not implemented yet.\n");
				return EMU_OK;
			}

		} else if (devName[len] == ':' || devName[len] == '\0') {

			// Check if unit already is created or not.
			if (ts10_Units[unit] != NULL) {
				printf("Error: Unit %s already was created.\n", devName);
				return EMU_ARG;
			}

			// Check memory available first
			if ((uptr = (UNIT *)calloc(1, sizeof(UNIT))) == NULL) {
				printf("Error: Memory not enough.\n");
				return EMU_MEMERR;
			}	

			if ((dptr = unit_FindDevice(ts10_Devices, argv[0])) == NULL) {
				printf("Error: Unknown Processor: %s\n", argv[0]);
				return EMU_ARG;
			}

			// Set specific processor within its architecture.
			if (dptr->Configure) {
				st = dptr->Configure(uptr, devName, argc-1, &argv[1]);
				if (st == EMU_OK)
					ts10_Units[unit] = uptr;
			} else {
				printf("Device %s - Configure routine.\n", dptr->Name);
				printf("Reason: Not implemented yet.\n");
				return EMU_OK;
			}

			return st;
		}

		printf("Error: Device name is not complete.\n");
		return EMU_ARG;
	}

	printf("Error: Maximum numbers of units is %d.\n", ts10_nUnits);
	return EMU_ARG;
}

// Delete CPU unit
int ts10_Delete(UNIT *ts10_Unit)
{
/*
	if (rhUnit->Flags & UNIT_DISABLED)
		return EMU_ARG;

	free(rhUnit->sUnits);
	free(rhUnit->uData);
	rhUnit->Flags |= UNIT_DISABLED;

	return EMU_OK;
*/
}
