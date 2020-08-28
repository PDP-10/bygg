// fe.c - Front-end 8080 communication routines for KS10 processor
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
#include "emu/proto.h"
#include "emu/socket.h"

#ifdef SVR4SIGNALS
#include <sys/file.h>
#include <unistd.h>
#include <stropts.h>
#include <signal.h>
#endif

#define KS10FE_ESCAPE    0x1C 
#define KS10FE_COUNTDOWN 500  // Default countdown

// CTY Buffer
//static uchar keyBuffer[256];
static uchar inBuffer[4096];
static char outBuffer[4096];
static int  idxInQueue, idxOutQueue;
static char *pBuffer = outBuffer;
static char lastSeen;

// KLINIK Buffer
static char out_kluBuffer[4096];
static char *p_kluBuffer = out_kluBuffer;

static SOCKET *ctyServer = NULL; // CTY Listening Socket
static SOCKET *ctySocket = NULL; // CTY Socket
static SOCKET *kliServer = NULL; // KLINIK Listening Socket
static SOCKET *kliSocket = NULL; // KLINIK Socket

// Send initialization codes to make sure that telnet behave correctly.
static int  n_telnetInit = 15;
static char telnetInit[] =
{
	255, 251,  34, // IAC WILL LINEMODE
	255, 251,   3, // IAC WILL SGA
	255, 251,   1, // IAC WILL ECHO
	255, 251,   0, // IAC WILL BINARY
	255, 253,   0, // IAC DO BINARY
};

int p10_ctyCountdown = -1;

void p10_ctyAccept(SOCKET *);
void p10_ctyEof(SOCKET *, int, int);
void p10_ctyInput(SOCKET *, char *, int);

int p10_ctyInitialize(void)
{
	idxInQueue  = 0;
	idxOutQueue = 0;

	// Set up the listing socket for CTY device.
	ctyServer = sock_Open(5000, NET_SERVER);
	if (ctyServer != NULL) {
		ctyServer->maxConns = 1;
		ctyServer->nConns   = 0;
		ctyServer->Accept   = p10_ctyAccept;
		ctyServer->Eof      = NULL;
		ctyServer->Process  = NULL;

		// Now accept incoming connections;
		sock_Listen(ctyServer, 5);
	}

	return EMU_OK;
}

int p10_ctyCleanup(void)
{
	sock_Close(ctySocket);
	sock_Close(ctyServer);

	ctySocket = NULL;
	ctyServer = NULL;

	return EMU_OK;
}

void p10_ctySendDone(void)
{
	// Send a done interrupt to the KS10 Processor.
	p10_aprInterrupt(APRSR_F_CON_INT);
}

void p10_ctyAccept(SOCKET *srvSocket)
{
	SOCKET *newSocket;

	if (newSocket = sock_Accept(srvSocket)) {
		// First, check if CTY connection already was taken.
		// If so, tell operator that.
		if (ctySocket != NULL) {
			int idSocket = newSocket->idSocket;

			sock_Send(idSocket, "Console (CTY) connection already was taken.\r\n", 0);
			sock_Send(idSocket, "Check other terminal which has that connection.\r\n", 0);
			sock_Send(idSocket, "\r\nTerminated.\r\n", 0);
			sock_Close(newSocket);

			return;
		}
 
		// Set up the CTY socket connection.
		ctySocket = newSocket;
		ctySocket->Accept  = NULL;
		ctySocket->Eof     = p10_ctyEof;
		ctySocket->Process = p10_ctyInput;

		// Send initialization codes and welcome messages
		sock_Send(ctySocket->idSocket, telnetInit, n_telnetInit);
		sock_Send(ctySocket->idSocket, "Welcome to KS10 Emulator\r\n\r\n", 0);
	}
}

void p10_ctyEof(SOCKET *Socket, int rc, int nError)
{
	sock_Close(ctySocket);

	ctySocket = NULL;
}

void p10_ctyInput(SOCKET *Socket, char *keyBuffer, int len)
{
	int36 cty;
	int   idx;

	// Process telnet codes and filter them out of data stream.
	if (len > 1) {
		if ((len = sock_ProcessTelnet(keyBuffer, len)) == 0)
			return;
	}

	for (idx = 0; idx < len; idx++) {
		if (keyBuffer[idx] == KS10FE_ESCAPE) {
			emu_State = EMU_HALT;
			continue;
		}

		// Convert CR NL to CR line.
		if ((keyBuffer[idx] == 012) && (lastSeen == 015))
			continue;
		lastSeen = keyBuffer[idx];

		inBuffer[idxInQueue] = keyBuffer[idx];
		if (++idxInQueue == 4096)
			idxInQueue = 0;
		if (idxInQueue == idxOutQueue) {
#ifdef DEBUG
			dbg_Printf("CTY: Error - Overrun!!\n");
#endif
			break;
		}
	}
			
	cty = p10_pRead(FE_CTYIWD, 0);
	if (!(cty & 0400)) {
		p10_pWrite(FE_CTYIWD, inBuffer[idxOutQueue] | 0400, 0);
		if (++idxOutQueue == 4096)
			idxOutQueue = 0;
		p10_aprInterrupt(APRSR_F_CON_INT);
	}
}

void p10_ctyCheckQueue(void)
{
	static int count = 0;
	int36 cty;
	int   pi;

	if (count++ == 1000) {
		count = 0;
		if (idxOutQueue == idxInQueue)
			return;
		cty = p10_pRead(FE_CTYIWD, 0);
		if (!(cty & 0400)) {
			p10_pWrite(FE_CTYIWD, inBuffer[idxOutQueue] | 0400, 0);
			if (++idxOutQueue == 4096)
				idxOutQueue = 0;
			p10_aprInterrupt(APRSR_F_CON_INT);
		} else {
//			cty = p10_pRead(FE_CTYIWD, 0);
//			if (cty & 0400)
//				p10_aprInterrupt(APRSR_F_CON_INT);
		}
		if (ctySocket)
			p10_ctyOutput();
	}
}

void p10_ctyOutput(void)
{
	int36 cty;
	uchar flag, ch;
	int pi;
	int status;

	// Write a CTY character to the terminal
/*
	cty = p10_pRead(FW_KLUOWD, 0);
	if (cty) {
		flag = (cty >> 8) & 0377;
		switch (flag) {
			case 1:
				// Print a character on terminal
				ch = cty & 0177;
				sock_Send(ctySocket->idSocket, &ch, 1);
#ifdef DEBUG
				if (dbg_Check(DBG_CONSOLE)) {
					if (ch == '\r') {
						*p_kluBuffer = '\0';
						dbg_Printf("KLU: %s\n", out_kluBuffer);
						p_kluBuffer = out_kluBuffer;
					} else {
						if (ch != '\n')
							*p_kluBuffer++ = ch;
					}
				}
#endif DEBUG
				break;

			case 2:
				// Send a carrier loss to KS10 Processor
#ifdef DEBUG
				if (dbg_Check(DBG_CONSOLE))
					dbg_Printf("KLU: *** Carrier Loss ***\n");
#endif DEBUG
				p10_pWrite(FE_KLUIWD, (2 << 8), 0);
				break;
		}

		// Inform KS10 Processor that it was done.
		p10_pWrite(FE_KLUOWD, 0, 0);
		p10_aprInterrupt(APRSR_F_CON_INT);
	}
*/
	if (ctySocket == NULL)
		return;

	cty = p10_pRead(FE_CTYOWD, 0);
	if (cty) {
		flag = (cty >> 8) & 0377;
		if (flag == 1) {
			ch = cty & 0177;
			sock_Send(ctySocket->idSocket, &ch, 1);
			if (ch == '\n') {
				*pBuffer++ = ch;
				*pBuffer   = '\0';
#ifdef DEBUG
				if (dbg_Check(DBG_CONSOLE))
					dbg_Printf("CTY: %s", outBuffer);
#endif DEBUG
				if (emu_logFile >= 0)
					write(emu_logFile, outBuffer, strlen(outBuffer));
				pBuffer = outBuffer;
			} else {
				if (ch == '\b' || ch == 127) {
					if (pBuffer > outBuffer)
						pBuffer--;
				} else if (ch != '\r' && ch != '\0')
					*pBuffer++ = ch;
			}
			p10_ctyCountdown = KS10FE_COUNTDOWN;
			p10_pWrite(FE_CTYOWD, 0, 0);
//			p10_aprInterrupt(APRSR_F_CON_INT);
		}
	}
}
