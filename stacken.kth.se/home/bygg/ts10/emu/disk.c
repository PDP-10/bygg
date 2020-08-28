// disk.c - the disk emulation routines
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
#include "emu/disk.h"

int disk_Open(UNIT *uptr, char *filename, int mode)
{
	DISK  *disk;
	int32 Blocks;
	char  ZeroBlock[uptr->szBlock];
	int   st;

	if (disk = (DISK *)calloc(1, sizeof(DISK))) {
		if ((disk->fd = open(filename, O_RDWR, 0)) < 0) {
			if ((disk->fd = open(filename, O_RDWR|O_CREAT, 0700)) < 0) {
				free(disk);
				return EMU_OPENERR;
			}
			Blocks = uptr->Blocks;
			memset(ZeroBlock, 0, uptr->szBlock);
			while (Blocks--)
				if((st = write(disk->fd, ZeroBlock, uptr->szBlock)) < 0) {
					close(disk->fd);
					unlink(filename);
					return EMU_OPENERR;
				}
		}
		disk->Type = DISK_IMAGE;
		uptr->FileRef = (void *)disk;
		return EMU_OK;
	}
	uptr->FileRef = NULL;
	return EMU_MEMERR;
}

int disk_Close(UNIT *uptr)
{
	DISK *disk = (DISK *)uptr->FileRef;
	int st = 0;

	if (disk->Type == DISK_IMAGE) {
		st = close(disk->fd);
		free(disk);
	}
	uptr->FileRef = NULL;
	return st ? EMU_IOERR : EMU_OK;
}

int disk_Seek(UNIT *uptr, int32 daddr, int pos)
{
	DISK *disk = (DISK *)uptr->FileRef;
	int st = 0;

	disk->Error = 0;
	if (disk->Type == DISK_IMAGE) {
		if ((st = lseek(disk->fd, daddr * uptr->szBlock, pos)) < 0)
			disk->Error = errno;
	}
	return st;
}

int disk_Read(UNIT *uptr, uint8 *inBuffer)
{
	DISK *disk = (DISK *)uptr->FileRef;
	int st = 0;

	disk->Error = 0;
	if (disk->Type == DISK_IMAGE) {
		if ((st = read(disk->fd, inBuffer, uptr->szBlock)) < 0)
			disk->Error = errno;
	}
	return st;
}

int disk_Write(UNIT *uptr, uint8 *outBuffer)
{
	DISK *disk = (DISK *)uptr->FileRef;
	int st = 0;

	disk->Error = 0;
	if (disk->Type == DISK_IMAGE) {
		if ((st = write(disk->fd, outBuffer, uptr->szBlock)) < 0)
			disk->Error = errno;
	}
	return st;
}

int disk_GetError(UNIT *uptr)
{
	DISK *disk = (DISK *)uptr->FileRef;
	
	return disk->Error;
}

int disk_GetDiskAddr(UNIT *uptr, int Cylinder, int Track, int Sector)
{
	DTYPE  *dType = uptr->dType;

	return (((Cylinder * dType->Tracks) + Track) * dType->Sectors) + Sector;
}
