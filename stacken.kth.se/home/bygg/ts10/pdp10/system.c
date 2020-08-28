// system.c - system routines for PDP10 emulation
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
#include "pdp10/fe.h"
#include "pdp10/proto.h"

extern DEVICE ks10uba_Device;

DTYPE p10_dTypes[] =
{
	{
		"KS10",       // CPU Name
		"Model 2020",
		UNIT_DISABLE, // CPU is disable
		0, 0, 0, 0,   // Not used
		0x00000000,   // System ID
		NULL,         // Slave Drive Types - Not Used

		0,            // KS10 is a central process unit
		NULL          // Not Used
	},

	{ NULL }
};

DEVICE *p10_Devices[] =
{
	&ks10uba_Device, // Unibus System (UBA) for KS10 only
	NULL             // NULL Terminator
};

DEVICE p10_Device =
{
	"PDP10",            // Device Name
	"DECsystem-10 Series",
	"v0.8.2 (Alpha)",   // Version
	p10_dTypes,         // Drive Type
	NULL,               // Unit table
	p10_Devices,        // Listing of devices
	p10_Commands,       // Listing of Commands
	NULL,               // Listing of Set Commands
	NULL,               // Listing of Show Commands
	1,                  // Number of Devices
	0,                  // Number of Units

	p10_Initialize,     // Initialize Routine
	p10_ResetCPU,       // Reset Routine
	p10_Create,         // Create Routine
	NULL,               // Delete Routine
	p10_Configure,      // Configure Routine
	NULL,               // Enable Routine
	NULL,               // Disable Routine
	NULL,               // Attach/Mount Routine
	NULL,               // Detach/Unmount Routine
	NULL,               // Format Routine
	NULL,               // Read I/O Routine
	NULL,               // Write I/O Routine
	NULL,               // Process Routine
	NULL,               // Boot Routine
	p10_Go,             // Go (Execute) Routine

	NULL,               /* SetUnit Routine - Not Used */
	NULL,               /* SetUnit Routine - Not Used */
	NULL,               /* SetUnit Routine - Not Used */
	NULL,               /* SetUnit Routine - Not Used */
	NULL,               /* SetUnit Routine - Not Used */
	NULL,               /* SetUnit Routine - Not Used */
	NULL,               /* SetUnit Routine - Not Used */
	NULL,               // Ready (Job Done) Routine - Not Used
	NULL,               // Check Word Count Routine - Not Used
};

int p10_Create(UNIT *uptr, char *devName, int argc, char **argv)
{
	DEVICE *dptr;

	if ((dptr = unit_FindDevice(p10_Devices, devName)) == NULL) {
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

int p10_Configure(UNIT *uptr, char *devName, int argc, char **argv)
{
	int    idx;

	if (argc != 1) {
		printf("Error: Wrong number of arguments.\n");
		printf("Usage: create %s: PDP10 <model>\n", devName);
		return EMU_ARG;
	}

	for (idx = 0; p10_dTypes[idx].Name; idx++)
		if (!strcasecmp(argv[0], p10_dTypes[idx].Name))
			break;

	if (!p10_dTypes[idx].Name)
		return EMU_ARG;

	p10_Initialize();
	p10_InitMemory(512 * 1024);

	// Mark this unit as CPU/Processor type.
	uptr->Device = &p10_Device;
	uptr->dType  = &p10_dTypes[idx];
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

char *pdp10_DisplayData(int36 data)
{
	static char text[80];
	char text6[7];
	char text7[6];
	int i, p;

	data &= WORD36_ONES;

	for (i = 30, p = 0; i >= 0; i -= 6, p++)
		text6[p] = ((data >> i) & 077) + 040;
	text6[p] = 0;

	for (i = 29, p = 0; i >= 1; i -= 7, p++) {
		text7[p] = (data >> i) & 0177;
		if ((text7[p] < 32) || (text7[p] > 126))
			text7[p] = ' ';
	}
	text7[p] = 0;

	sprintf(text, "%012llo ('%s' '%s')", data, text6, text7);

	return text;
}

/*
 * DEC Core-Dump file.
 * Format for storing 36-bits into 5 tape frames.
 *
 *    DEC Core-Dump Mode          ANSI Compatible Mode
 * |00 01 02 03 04 05 06|07     |__ 00 01 02 03 04 05 06|
 *  08 09 10 11 12 13|14 15     |__ 07 08 09 10 11 12 13|
 *  16 17 18 19 20|21 22 23     |__ 14 15 16 17 18 19 20|
 *  24 25 26 27|28 29 30 31     |__ 21 22 23 24 25 26 27|
 *  __ __ __ __ 32 33 34|35|    |35|28 29 30 31 32 33 34|
 *
 * Note: "|" separate the 7-bit bytes,
 * "__" are unused bits (which is zeros).
 *
 */

int36 pdp10_Convert8to36(uchar *data8)
{
	int36 data36;

	data36 = data8[0];
	data36 = (data36 << 8) | data8[1];
	data36 = (data36 << 8) | data8[2];
	data36 = (data36 << 8) | data8[3];
	data36 = (data36 << 4) | data8[4];

	return data36;
}

uchar *pdp10_Convert36to8(int36 data36)
{
	static uchar data8[6];

	data8[0] = (data36 >> 28) & 0xFF;
	data8[1] = (data36 >> 20) & 0xFF;
	data8[2] = (data36 >> 12) & 0xFF;
	data8[3] = (data36 >>  4) & 0xFF;
	data8[4] = data36 & 0xF;
	data8[5] = '\0';

	return data8;
}

int exe_GetWord(int inFile, int36 *data)
{
	char inBuffer[5];
	int  st;

	if ((st = read(inFile, &inBuffer, 5)) > 0) {
		*data = inBuffer[0] & 0377;
		*data = (inBuffer[1] & 0377) | (*data << 8);
		*data = (inBuffer[2] & 0377) | (*data << 8);
		*data = (inBuffer[3] & 0377) | (*data << 8);
		*data = (inBuffer[4] & 0017) | (*data << 4);
	}

	return st;
}

// Load RIM file into memory
int pdp10_LoadRimFile(char *rimFilename)
{
	int   rimFile;
	char  inBuffer[5];
	int18 len, addr;
	int36 data36;

	if ((rimFile = open(rimFilename, O_RDONLY)) < 0) {
		perror(rimFilename);
		return EMU_OK;
	}

	while (exe_GetWord(rimFile, &data36) > 0) {
		len = LHSR(data36);
		len = SXT18(len);
		addr = RH(data36);

		if (len < 0) {
			printf("RIM: Address: %06o at %06o words\n", addr+1, -len);
			while (len++) {
				if (exe_GetWord(rimFile, &data36) <= 0)
					break;
				p10_pWrite(++addr, data36, 0);
			}
		}
	}

	close(rimFile);

	// Set PC to execute that program.
	data36 = p10_pRead(0120, 0);
	PC = RH(data36);
	printf("RIM: Start Address: %06llo\n", PC);

	return EMU_OK;
}

// Load EXE file into memory
int pdp10_LoadExeFile(char *exefilename)
{
	int exefile;
	int idx, idx1, len, st;
	char cBuffer[SV_BLK_SIZE * 5];
	int36 data36;
	int36 ndir;      // number of directory entries
	int36 dir[128];  // directory entries for page information
	int36 id, flags, fpage, ppage, paddr, count;

	if ((exefile = open(exefilename, O_RDONLY)) < 0) {
		perror(exefilename);
		return EMU_OK;
	}

	// Now read EXE header information
	do {
		st = read(exefile, cBuffer, 5);
		data36 = pdp10_Convert8to36(cBuffer);
		id = (data36 >> 18) & 0777777;
		len = (data36 & 0777777) - 1;

		switch (id) {
			case SV_ID_DIRECTORY:
				ndir = len;
				if (ndir >= 128) {
					printf("EXE: %s has too long directory entry.\n", exefilename);
					return EMU_OK;
				}
				st = read(exefile, cBuffer, ndir * 5);
				idx = 0;
				while (idx < ndir) {
					dir[idx] = pdp10_Convert8to36(&cBuffer[idx*5]);
					idx++;
				}

				printf("EXE: Directory Entries:\n");
				idx = 0;
				while (idx < ndir) {
					flags = dir[idx];
					printf("EXE: Flags: %c%c%c%c%c%c Repeat: %06llo File Page: %06llo CPU Page: %06llo\n",
						((flags & SV_M_HIGH_SEG)  ? 'H' : '-'),
						((flags & SV_M_SHARABLE)  ? 'S' : '-'),
						((flags & SV_M_WRITABLE)  ? 'W' : '-'),
						((flags & SV_M_CONCEALED) ? 'C' : '-'),
						((flags & SV_M_SYM_TABLE) ? 'T' : '-'),
						((flags & SV_M_ALLOCATED) ? 'A' : '-'),
						((dir[idx+1] >> 18) & 0777777),
						(dir[idx] & 0777777),
						(dir[idx+1] & 0777777));

					idx += 2;
				}

				break;

			case SV_ID_END_BLOCK:
				printf("EXE: End of block, now loading data\n");
				break;

			default:
				printf("EXE: Unknown ID code %06llo\n", id);
				lseek(exefile, (len-1) * 5, SEEK_CUR); /* Skip data */
				break;
		}
	} while (id != SV_ID_END_BLOCK);

	// Seek to the starting file page 1
	st = lseek(exefile, SV_BLK_SIZE * 5, SEEK_SET);
	printf("EXE: Starting data block at %d\n", st);

	// Now start to load data block.
	idx = 0;
	while (idx < ndir) {
		flags = (dir[idx] >> 18) & 0777777;
		fpage = dir[idx] & 0777777;
		count = (dir[idx+1] >> 18) & 0777777;
		ppage = dir[idx+1] & 0777777;
		paddr = ppage << 9;

		printf("EXE: Starting address at %06llo (Page %06llo)\n", paddr, ppage);
		while (count >= 0) {
			if (fpage) {
				printf("EXE:   Fetch file page %06llo\n", fpage);
				st = read(exefile, cBuffer, SV_BLK_SIZE * 5);
			}
			printf("EXE:   Load data block into address %06llo\n", paddr);
			if (fpage) {
				idx1 = 0;
				while (idx1 < SV_BLK_SIZE) {
					data36 = pdp10_Convert8to36(&cBuffer[(idx1++)*5]);
					p10_pWrite(paddr, SXT36(data36), 0);
//					printf("EXE:     %06llo: %06llo,,%06llo\n",
//						paddr, LHSR(data36), RH(data36));
					paddr++;
				}
			} else {
				while (idx1++ < SV_BLK_SIZE)
					p10_pWrite(paddr++, 0, 0);
			}

			fpage++;
			ppage++;
			count -= SV_BLK_SIZE;
		}
		idx += 2;
	}

	close(exefile);

	// Set PC to execute that program.
	data36 = p10_pRead(0120, 0);

	PC = RH(data36);
	printf("EXE: Start Address: %06llo\n", PC);

	return EMU_OK;
}

int pdp10_BootDisk(UNIT *uptr, int argc, char **argv)
{
	DTYPE *dType = uptr->dType;
	int   reqBoot = 2; // Monitor Preboot Page
//	int   reqBoot = FE_P0_MONITOR_PREBOOT;
//	int   reqBoot = FE_P0_DIAGNOSTIC_PREBOOT;
//	int   reqBoot = FE_P0_BOOTCHECK2_PREBOOT;
	uint8 inBuffer[01000 * 5];
	int36 inBlock[01000];
	int36 idHom = util_PackedASCII6("HOM");
	int36 dAddr36;
	int36 data36;
	int   dCylinder, dTrack, dSector, dAddr;
	int   pAddr;
	int   idx;

	// Get parameters from 'boot' command.
	if (argc >= 1)
		sscanf(argv[0], "%d", &reqBoot);

	// Check first HOM block (Block 1).

	printf("[Reading first HOM block...]\n");

	dType->Seek(uptr, 1, SEEK_SET);
	dType->Read(uptr, inBuffer);
	for (idx = 0; idx < 0777; idx++)
		inBlock[idx] = util_Convert8to36(&inBuffer[idx * 5]);

	if (inBlock[0] != idHom) {
		// Check alternate HOM block (Block 10).

		printf("[Reading second HOM block...]\n");

		dType->Seek(uptr, 10, SEEK_SET);
		dType->Read(uptr, inBuffer);
		for (idx = 0; idx < 0777; idx++)
			inBlock[idx] = util_Convert8to36(&inBuffer[idx * 5]);

		if (inBlock[0] != idHom) {
			printf("[HOM block: Not found.]\n");
			printf("Boot aborted.\n");
			return EMU_IOERR;
		}
	}

	// Check if disk is bootable.
	if ((dAddr36 = inBlock[FE_BT_8080]) == 0)
		return EMU_NOTBOOTABLE;

	// Now get FE-FILE Page zero.

	// Extract cylinder, track, and sector fields
	// from disk address word.
	dCylinder = FE_DA_CYL(dAddr36);
	dTrack    = FE_DA_TRK(dAddr36);
	dSector   = FE_DA_SEC(dAddr36);

	printf("[Reading FE-FILE Page 0 at C %d T %d S %d]\n",
		dCylinder, dTrack, dSector);

	dAddr = dType->GetDiskAddr(uptr, dCylinder, dTrack, dSector);
	for (idx = 0; idx < 4; idx++) {
		dType->Seek(uptr, dAddr++, SEEK_SET);
		dType->Read(uptr, &inBuffer[(idx * 128) * 5]);
	}
	for (idx = 0; idx < 0777; idx++)
		inBlock[idx] = util_Convert8to36(&inBuffer[idx * 5]);

	// Now get the pre-boot loader page from disk.
	if ((dAddr36 = inBlock[reqBoot << 1]) == 0) {
		printf("Pre-boot loader not available - Boot aborted.\n");
		return EMU_NOTBOOTABLE;
	}
	dCylinder = FE_DA_CYL(dAddr36);
	dTrack    = FE_DA_TRK(dAddr36);
	dSector   = FE_DA_SEC(dAddr36);
		
	printf("[Reading Monitor Pre-boot at C %d T %d S %d]\n",
		dCylinder, dTrack, dSector);

	dAddr = dType->GetDiskAddr(uptr, dCylinder, dTrack, dSector);
	for (idx = 0; idx < 4; idx++) {
		dType->Seek(uptr, dAddr++, SEEK_SET);
		dType->Read(uptr, &inBuffer[(idx * 128) * 5]);
	}

	// Now load the pre-boot loader into location 1000;
	pAddr = 01000;
	for (idx = 0; idx < 0777; idx++) {
		data36 = util_Convert8to36(&inBuffer[idx * 5]);
		p10_pWrite(pAddr++, data36, 0);
	}
	p10_pWrite(FE_BRH11BA, 0000001776700LL, 0);
	p10_pWrite(FE_BDRVNUM, uptr->idUnit, 0);
	PC = 01000;

	return EMU_OK;
}

int pdp10_BootTape(UNIT *uptr, int argc, char **argv)
{
	DTYPE  *dType = uptr->dType;
	int    reqBoot = 1;
	uchar  inBuffer[32768];
	int36  data36;
	int18  pAddr;
	int    count, idx, st;

	// Get parameters from 'boot' command.
	if (argc >= 1)
		sscanf(argv[0], "%d", &reqBoot);

	// Now must do a rewind first
	dType->Rewind(uptr);
#ifdef DEBUG
	if (dbg_Check(DBG_IODATA))
		dbg_Printf("BOOT: Rewind\n");
#endif DEBUG

	// Now skip a first file (contains microcode)
	while(reqBoot--) {
		count = 0;
		while (st = dType->Skip(uptr, 1))
			count++;

#ifdef DEBUG
		if (dbg_Check(DBG_IODATA))
			dbg_Printf("BOOT: Skip %d records\n", count);
#endif DEBUG
	}


	// Start to load RDI file into PDP-10's memory at starting 
	// location 1000.
	pAddr = 01000;
	st = dType->Read(uptr, inBuffer, 32768);
	if (st > 0) {
#ifdef DEBUG
		if (dbg_Check(DBG_IODATA))
			dbg_Printf("BOOT: %d bytes had been read.\n", st);
#endif DEBUG
		for (idx = 0; idx < st; idx += 5) {
			data36 = util_Convert8to36(&inBuffer[idx]);
#ifdef DEBUG
			if (dbg_Check(DBG_IODATA))
				dbg_Printf("BOOT:   %06o <- %s\n", pAddr,
					pdp10_DisplayData(data36));
#endif DEBUG
			p10_pWrite(pAddr++, data36, 0);
		}
	}
#ifdef DEBUG
	if (dbg_Check(DBG_IODATA))
		dbg_Printf("BOOT: RDI file had been loaded successfully.\n");
#endif DEBUG

	p10_pWrite(FE_BRH11BA, 0000003772440LL, 0);
	p10_pWrite(FE_BDRVNUM, uptr->pUnit->idUnit, 0);
	p10_pWrite(FE_MTBFSN,  uptr->idUnit, 0);

	// Set PC to execute the RDI loader.
	PC = 01000;

	return EMU_OK;
}

int pdp10_Boot(UNIT *uptr, int mode, int argc, char **argv)
{
	switch (mode) {
		case BOOT_DISK:
			return pdp10_BootDisk(uptr, argc, argv);

		case BOOT_MAGTAPE:
			return pdp10_BootTape(uptr, argc, argv);
	}

	return EMU_NOTSUPPORTED;
}

int pdp10_Start(char *strAddr)
{
	sscanf(strAddr, "%o", &PC);
	printf("CPU: Start Address: %06o\n", PC);
	return EMU_OK;
}

