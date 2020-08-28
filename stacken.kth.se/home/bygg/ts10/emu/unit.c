// unit.c - the unit routines for attach/detach and commands
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

#include "emu/defs.h"

extern DEVICE ts10_Device;

// Mapping Device listing with using binary tree algorithm
// for mapping device names (i.e. tma0:, rpa0:, dua0:, etc.)

MAPDEVICE emu_mapDevices[MAX_MAPDEVICES];

int unit_mapCreateDevice(char *devName, UNIT *uptr)
{
	int idx, len;

	// Check device names for conflicts first
	if (unit_mapFindDevice(devName))
		return EMU_CONFLICT;

	for (idx = 0; idx < MAX_MAPDEVICES; idx++) {
		if (!emu_mapDevices[idx].Name) {
			MAPDEVICE *mptr = &emu_mapDevices[idx];

			if (mptr->Name = (char *)malloc(strlen(devName)+1)) {
				util_ToUpper(devName);
				strcpy(mptr->Name, devName);
			} else
				return EMU_MEMERR;

			mptr->Unit = uptr;

			return EMU_OK;
		}
	}

	return EMU_MEMERR;
}

int unit_mapDeleteDevice(char *devName)
{
	int idx, len;

	for (idx = 0; idx < MAX_MAPDEVICES; idx++) {
		if (emu_mapDevices[idx].Name) {
			len = strlen(emu_mapDevices[idx].Name);
			if (!strncasecmp(devName, emu_mapDevices[idx].Name, len)) {
				MAPDEVICE *mptr = &emu_mapDevices[idx];

				free(mptr->Name);
				mptr->Name = NULL;
				mptr->Unit = NULL;
				
				return EMU_OK;
			}
		}
	}

	return EMU_NOTFOUND;
}

UNIT *unit_mapFindDevice(char *devName)
{
	int idx, len;

	for (idx = 0; idx < MAX_MAPDEVICES; idx++) {
		if (emu_mapDevices[idx].Name) {
			if (!strcasecmp(devName, emu_mapDevices[idx].Name))
				return emu_mapDevices[idx].Unit;
		}
	}

	return NULL;
}

DEVICE *unit_FindDevice(DEVICE **Devices, char *devName)
{
	DEVICE *dptr;
	int    idx;
	int    len;

	for (idx = 0; dptr = Devices[idx]; idx++) {
		len = strlen(dptr->Name);
		if (!strncasecmp(devName, dptr->Name, len))
			return dptr;
	}
	return NULL;
}

int unit_Attach(UNIT *uptr, char *filename)
{
	int reason;

	if (uptr->Flags & UNIT_DISABLED)
		return EMU_DISABLED;
	if (!(uptr->Flags & UNIT_ATTABLE))
		return EMU_NOATT;

	if (uptr->Flags & UNIT_ATTACHED) {
		reason = unit_Detach(uptr);
		if (reason != EMU_OK)
			return reason;
	}

	if (uptr->FileName = (char *)malloc(strlen(filename)+1))
		strcpy(uptr->FileName, filename);
	else
		return EMU_MEMERR;

	if(reason = uptr->dType->Open(uptr, filename))
		return reason;

	uptr->Flags |= UNIT_ATTACHED;
//	uptr->Position = 0;
	return EMU_OK;
}

int unit_Detach(UNIT *uptr)
{
	if (!(uptr->Flags & UNIT_ATTACHED))
		return EMU_OK;

	free(uptr->FileName);
	uptr->FileName = NULL;

	uptr->dType->Close(uptr);

	uptr->Flags &= ~UNIT_ATTACHED;
	return EMU_OK;
}

int unit_CmdCreate(int argc, char **argv)
{
	if (argc == 1)
		printf("Usage: create <unit> ...\n");
	else {
		DEVICE *dptr;
		int    st;
		int    idx;

		util_RemoveSpaces(argv[1]);

		dptr = &ts10_Device;
		if (st = dptr->Create(NULL, argv[1], argc-2, &argv[2]))
			return st;
	}
	return EMU_OK;
}

int unit_CmdDelete(int argc, char **argv)
{
	if (argc == 1)
		printf("Usage: delete <unit>\n");
	else {
/*
		char   *devname = util_SplitWord(&par);
		DEVICE *dptr;
		UNIT   *uptr;
		int32  unit;
		int    st;

		util_RemoveSpaces(devname);
		if (*devname == 0)
			return EMU_ARG;
		uptr = &dptr->Units[unit];

		if (uptr->Flags & UNIT_DISABLE) {
			if (uptr->Flags & UNIT_ATT) {
				if (st = unit_Detach(uptr))
					return st;
			}

			dptr = &ts10_Device;
			if (st = dptr->Delete(uptr))
				return st;

			printf("Unit %s had been removed.\n", devname);
		} else
			printf("Unit %s is not disable.\n", devname);
*/
	}
	return EMU_OK;
}

int unit_CmdAttach(int argc, char **argv)
{
	int  st;

	if (argc != 3)
		printf("Usage: attach <device> <filename>\n");
	else {
		char   *devName = argv[1];
		DEVICE *dptr;
		UNIT   *uptr;

		util_RemoveSpaces(devName);
		if (*devName == 0)
			return EMU_ARG;
		*StrChar(devName, ':') = '\0';
		if ((uptr = unit_mapFindDevice(devName)) == NULL)
			return EMU_ARG;
		dptr = uptr->Device;

		if (uptr->Flags & UNIT_ATTACHED) {
			if (dptr->Detach)
				st = dptr->Detach(uptr);
			else
				st = unit_Detach(uptr);
			if (st)
				return st;
		}

		printf("Unit %d dType %s\n", uptr->idUnit, uptr->dType->Name);

		if (dptr->Attach)
			st = dptr->Attach(uptr, argv[2]);
		else
			st = unit_Attach(uptr, argv[2]);

		if (st == EMU_OK)
			printf("Unit %s had been attached with '%s' file.\n",
				devName, argv[2]);

		return st;
	}
	return EMU_OK;
}

int unit_CmdDetach(int argc, char **argv)
{
	if (argc != 2)
		printf("Usage: detach <device>\n");
	else {
		char   *devName = argv[1];
		DEVICE *dptr;
		UNIT   *uptr;
		int    st;

		util_RemoveSpaces(devName);
		if (*devName == 0)
			return EMU_ARG;
		*StrChar(devName, ':') = '\0';
		if ((uptr = unit_mapFindDevice(devName)) == NULL)
			return EMU_ARG;
		dptr = uptr->Device;

		if (uptr->Flags & UNIT_ATTACHED) {
			if (dptr->Detach)
				st = dptr->Detach(uptr);
			else
				st = unit_Detach(uptr);

			if (st == EMU_OK)
				printf("Unit %s had been detached.\n", devName);

			return st;
		} else
			printf("Unit %s already is detached.\n", devName);
	}
	return EMU_OK;
}

// Initialize (Format) medium storage
int unit_CmdFormat(int argc, char **argv)
{
	if (argc != 2)
		printf("Usage: format <device>\n");
	else {
		char   *devName = argv[1];
		DEVICE *dptr;
		UNIT   *uptr;
		int    st;

		util_RemoveSpaces(devName);
		if (*devName == 0)
			return EMU_ARG;
		*StrChar(devName, ':') = '\0';
		if ((uptr = unit_mapFindDevice(devName)) == NULL)
			return EMU_ARG;
		dptr = uptr->Device;

		if (dptr->Format) {
			dptr->Format(uptr);
			printf("Unit %s had been formatted.\n", devName);
		} else {
			printf("Format is not supported for unit %s\n", devName);
		}
	}
	return EMU_OK;
}

// Power Up Initialization
int unit_CmdInit(int argc, char **argv)
{
	if (argc != 2)
		printf("Usage: init <device>\n");
	else {
		char   *devName = argv[1];
		DEVICE *dptr;
		UNIT   *uptr;
		int    st;

		util_RemoveSpaces(devName);
		if (*devName == 0)
			return EMU_ARG;
		*StrChar(devName, ':') = '\0';
		if ((uptr = unit_mapFindDevice(devName)) == NULL)
			return EMU_ARG;
		dptr = uptr->Device;

		dptr->Initialize(uptr); // Power Up Initialization

		printf("Unit %s had been initialized.\n", devName);
	}
	return EMU_OK;
}

// Boot a device
int unit_CmdBoot(int argc, char **argv)
{
	if (argc < 2)
		printf("Usage: boot <device> [parameters...]\n");
	else {
		char   *devName = argv[1];
		DEVICE *dptr;
		UNIT   *uptr;
		int    st;

		util_RemoveSpaces(devName);
		if (*devName == 0)
			return EMU_ARG;
		*StrChar(devName, ':') = '\0';
		if ((uptr = unit_mapFindDevice(devName)) == NULL)
			return EMU_ARG;
		dptr = uptr->Device;
		util_ToUpper(devName);

		if (dptr->Boot == NULL) {
			printf("Device %s: Not bootable.\n", devName);
			return EMU_OK;
		}

		if ((uptr->Flags & UNIT_ATTACHED) == 0) {
			printf("Device %s: Not attached.\n", devName);
			return EMU_OK;
		}

		printf("Booting %s...\n", devName);
		if (!(st = dptr->Boot(uptr, argc-2, &argv[2]))) {
			emu_State = EMU_RUN;
			printf("Now running...\n");
		} else {
			if (st == EMU_NOTBOOTABLE)
				printf("Device %s: Not bootable\n", devName);
			else
				printf("Boot failure.\n");
		}
	}
	return EMU_OK;
}

// Use desired CPU device.
int ts10_CmdUse(int argc, char **argv)
{
	if (argc != 2)
		printf("Usage: use <device>\n");
	else {
		char   *devName = argv[1];
		DEVICE *dptr;
		UNIT   *uptr;
		int    st;

		util_RemoveSpaces(devName);
		if (*devName == 0)
			return EMU_ARG;
		*StrChar(devName, ':') = '\0';
		if ((uptr = unit_mapFindDevice(devName)) == NULL)
			return EMU_ARG;

		// Set current unit/device to being used.
		ts10_UseUnit   = uptr;
		ts10_UseDevice = uptr->Device;
	}
	return EMU_OK;
}
