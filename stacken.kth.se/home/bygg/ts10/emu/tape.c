// tape.c - the tape emulation routines
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <endian.h>

#include "emu/defs.h"
#include "emu/tape.h"

#if BYTE_ORDER == LITTLE_ENDIAN && !defined(lint)
#define        LE_TO_H_32(x)   (x)
#define        H_TO_LE_32(x)   (x)
#define        LE_TO_H_16(x)   (x)
#define        H_TO_LE_16(x)   (x)
#else
#define        LE_TO_H_32(x)   byte_swap_4((x))
#define        H_TO_LE_32(x)   byte_swap_4((x))
#define        LE_TO_H_16(x)   byte_swap_2((x))
#define        H_TO_LE_16(x)   byte_swap_2((x))

static uint32 byte_swap_4(uint32 x)
{
       uint8 *s = (uint8 *)&x;
       return (s[3] << 24 | s[2] << 16 | s[1] << 8 | s[0]);
}

static uint16 byte_swap_2(uint16 x)
{
       uint8 *s = (uint8 *)&x;
       return (s[1] << 8 | s[0]);
}
#endif

int tape_Open(UNIT *uptr, char *filename, int mode)
{
	MTAPE *mtape;

	if ((mtape = (MTAPE *)calloc(1, sizeof(MTAPE))) != NULL) {
		mtape->mode = MT_WRITABLE; // Assume it's write-enabled.
		if ((mtape->fd = open(filename, O_RDWR, 0)) < 0) {
			if (errno == EACCES) { // Can't write. Can we read?
				if ((mtape->fd = open(filename, O_RDONLY, 0)) < 0) {
					free(mtape);
					return EMU_OPENERR;
				} else {
					mtape->mode &= ~MT_WRITABLE; // Can read, can't write.
				}
			} else if ((mtape->fd = open(filename, O_RDWR|O_CREAT, 0600)) < 0) {
				free(mtape);
				return EMU_OPENERR;
			}
		}
		mtape->Type = MT_WILSON; // Default as Wilson's tape format
		mtape->pos  = 0;         // Start at bottom of tape.
		uptr->FileRef = (void *)mtape;
		return EMU_OK;
	}
	uptr->FileRef = NULL;
	return EMU_MEMERR;
}

int tape_Close(UNIT *uptr)
{
	MTAPE *mtape = (MTAPE *)uptr->FileRef;
	int st = 0;

	switch(mtape->Type) {
		case MT_WILSON:
			st = close(mtape->fd);
			free(mtape);
			break;

		case MT_TPC:
			st = close(mtape->fd);
			free(mtape);
			break;
	}

	return st ? EMU_IOERR : EMU_OK;
}

int tape_Read(UNIT *uptr, uint8 *data, int len)
{
	MTAPE *mtape = (MTAPE *)uptr->FileRef;
	int32 Blklen32, tBlklen32;
	int16 Blklen16;
	int wc, st;

	mtape->error = 0;

	switch (mtape->Type)
	{
		case MT_WILSON:
			// Move position to the beginning of a next record.
			if ((st = lseek(mtape->fd, mtape->pos, SEEK_SET)) < 0)
				break;

			// Get length from that record.
			if ((st = read(mtape->fd, &Blklen32, 4)) <= 0)
				break;
			mtape->pos += 4;
			
			Blklen32 = LE_TO_H_32(Blklen32);
			if (Blklen32) {
				wc = (Blklen32 + 1) & ~1;

				if ((st = read(mtape->fd, data, wc)) <= 0)
					break;

				if ((st = read(mtape->fd, &tBlklen32, 4)) <= 0)
					break;

				tBlklen32 = LE_TO_H_32(tBlklen32);
				if (tBlklen32 != Blklen32) {
					mtape->error = -1;
					return MT_ERROR;
				}

				mtape->pos += wc + 4;
			}

			return Blklen32;

		case MT_TPC:
			if ((st = read(mtape->fd, &Blklen16, 2)) <= 0)
				break;

			Blklen16 = LE_TO_H_16(Blklen16);
			if (Blklen16) {
				if ((st = read(mtape->fd, data, Blklen16)) <= 0)
					break;
			}

			return Blklen16;
	}

	if (st < 0) {
		// If st = -1, get error code.
		mtape->error = errno;
		return MT_ERROR;
	}

	// Bottom/End of tape had been reached.
	return (len < 0) ? MT_BOT : MT_EOT;
}

int tape_Write(UNIT *uptr, uint8 *data, int len)
{
	MTAPE *mtape = (MTAPE *)uptr->FileRef;
	int32 leBlklen32;
	int16 leBlklen16;
	int   wc, st;
	int   idx;

	if (!(mtape->mode & MT_WRITABLE)) {
		mtape->error = EACCES;
		return MT_ERROR;
	}
	mtape->error = 0;

	switch (mtape->Type) {
		case MT_WILSON:
			leBlklen32 = H_TO_LE_32(len);

			// Move position to the beginning of a next record.
			if ((st = lseek(mtape->fd, mtape->pos, SEEK_SET)) < 0)
				break;

			// Put length on the new record.
			if ((st = write(mtape->fd, &leBlklen32, 4)) < 0)
				break;
			mtape->pos += 4;
			
			if (len) {
				wc = (len + 1) & ~1;

				// Adding zero padding to end of record
				for (idx = len; idx < wc; idx++)
					data[idx] = '\0';

				if ((st = write(mtape->fd, data, wc)) < 0)
					break;

				if ((st = write(mtape->fd, &leBlklen32, 4)) < 0)
					break;

				mtape->pos += wc + 4;
			}

			return len;

		case MT_TPC:
			leBlklen16 = H_TO_LE_16(len);

			if ((st = write(mtape->fd, &leBlklen16, 2)) < 0)
				break;
			mtape->pos += 2;

			if (len) {
				if ((st = write(mtape->fd, data, len)) < 0)
					break;
				mtape->pos += len;
			}

			return len;
	}

	if (st < 0) {
		mtape->error = errno;
		return MT_ERROR;
	}

	return st;
}

int tape_Mark(UNIT *uptr)
{
	MTAPE *mtape = (MTAPE *)uptr->FileRef;
	int32 tapemark32 = 0;
	int16 tapemark16 = 0;
	int st;

	if (!(mtape->mode & MT_WRITABLE)) {
		mtape->error = EACCES;
		return MT_ERROR;
	}
	mtape->error = 0;

	switch (mtape->Type) {
		case MT_WILSON:
			if ((st = write(mtape->fd, &tapemark32, 4)) < 0)
				break;
			mtape->pos += 4;
			return MT_OK;

		case MT_TPC:
			if ((st = write(mtape->fd, &tapemark16, 2)) < 0)
				break;
			mtape->pos += 2;
			return MT_OK;
	}

	mtape->error = errno;
	return MT_ERROR;
}

int tape_Rewind(UNIT *uptr)
{
	MTAPE *mtape = (MTAPE *)uptr->FileRef;

	// Return to the beginning of tape
	switch (mtape->Type) {
		case MT_WILSON:
			mtape->pos = 0;
		case MT_TPC:
			return lseek(mtape->fd, 0L, SEEK_SET);
	}
	return -1;
}

int tape_SkipRec(UNIT *uptr, int recs)
{
	MTAPE *mtape = (MTAPE *)uptr->FileRef;
	int32 Blklen32;
	int16 Blklen16;
	int   count = 0;
	int   st;

	mtape->error = 0;

	switch (mtape->Type) {
		case MT_WILSON:
			if (recs < 0) {
				// Skip a record reverse.

				// Check if tape position already is at bottom of tape.
				if (mtape->pos == 0)
					return MT_BOT;

				// Move position to the ending of a pervious record.
				mtape->pos -= 4;
				if((st = lseek(mtape->fd, mtape->pos, SEEK_SET)) < 0)
					break;

				// Get length from tape.
				if ((st = read(mtape->fd, &Blklen32, 4)) <= 0)
					break;

				if (Blklen32) {
					// Move next record reverse and count it.
					Blklen32 = LE_TO_H_32(Blklen32);
					mtape->pos -= ((Blklen32 + 1) & ~1) - 4;
					count++;
				} else
					return MT_TMARK; // Tape Mark encountered.

			} else {
				// Skip a record forward

				// Move position to the beginning of a next record.
				if((st = lseek(mtape->fd, mtape->pos, SEEK_SET)) < 0)
					break;

				// Get length from a tape.
				if ((st = read(mtape->fd, &Blklen32, 4)) <= 0)
					break;

				if (Blklen32) {
					// Move next record forward and count it.
					Blklen32 = LE_TO_H_32(Blklen32);
					mtape->pos += ((Blklen32 + 1) & ~1) + 8;
					count++;
				} else {
					mtape->pos += 4;
					return MT_TMARK; // Tape Mark encountered.
				}
			}
			return count;

		case MT_TPC:
			// TPC format does not support reverse.
			if (recs < 0)
				return MT_NOTSUP;

			if ((st = read(mtape->fd, &Blklen16, 2)) <= 0)
				break;

			if (Blklen16) {
				Blklen16 = LE_TO_H_16(Blklen16);
				if ((st = lseek(mtape->fd, Blklen16, SEEK_CUR)) < 0)
					break;
				count++;
			} else
				return MT_TMARK; // tape mark (EOF)

			return count;
	}

	// Check any errors during tape operation.
	if (st < 0) {
		mtape->error = errno;
		return MT_ERROR;
	}

	// Bottom/End of Tape had been reached.
	return (recs < 0) ? MT_BOT : MT_EOT;
}
