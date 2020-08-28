// socket.c - Socket routines
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
#include "emu/socket.h"

static SOCKET Sockets[NET_MAXSOCKETS];

static fd_set fdsRead;
static fd_set fdsWrite;

static int sock_Error = NET_OK;

extern void (*emu_IOTrap)();

void sock_Initialize(void)
{
	// Initialize select table.
	FD_ZERO(&fdsRead);
	FD_ZERO(&fdsWrite);

	// Initialize socket table.
	memset(&Sockets, 0, sizeof(SOCKET) * NET_MAXSOCKETS);

	emu_IOTrap = sock_Handler;
}

void sock_Cleanup(void)
{
	emu_IOTrap = NULL;
}

#ifdef DEBUG
void sock_Dump(int idSocket, uchar *data, int len, char *method)
{
	char cDump[17], *pcDump;
	int  idx1, idx2;
	uchar ch;

	dbg_Printf("SOCKET: Socket ID: %d  Method: %s\n",
		idSocket, method);
	dbg_Printf("SOCKET:\n");

	for (idx1 = 0; idx1 < len;) {
		dbg_Printf("SOCKET: %04X  ", idx1);
		pcDump = cDump;
		for (idx2 = 0; (idx2 < 16) && (idx1 < len); idx2++) {
			ch = data[idx1++];
			dbg_Printf("%02X%c", ch, (idx2 == 7) ? '-' : ' ');
			*pcDump++ = ((ch >= 32) && (ch < 127)) ? ch : '.';
		}
		for (; idx2 < 16; idx2++)
			dbg_Printf("   ");
		*pcDump = '\0';
		dbg_Printf(" |%-16s|\n", cDump);
	}
}
#endif

SOCKET *sock_Open(int newPort, int mode)
{
	SOCKET *Socket;
	SOCKADDRIN locAddr;
	SOCKADDRIN remAddr;
	int newSocket;
	int flags;
	int idx;

	sock_Error = NET_OK; // Assume successfull.

	// Find a free socket slot.
	Socket = NULL;
	for (idx = 0; idx < NET_MAXSOCKETS; idx++) {
		if ((Sockets[idx].Flags & SOCK_OPENED) == 0) {
			Socket = &Sockets[idx];
			break;
		}
	}

	// No, all slots are occupied.
	if (Socket == NULL) {
		sock_Error = NET_FULLSOCKETS;
		return NULL;
	}

	// Open a socket for Internet (TCP/IP) connection.
	if ((newSocket = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket Error (Open)");
		sock_Error = NET_OPENERR;
		return NULL;
	}

	switch (mode) {
		case NET_SERVER:
			// Give socket a local name;
			locAddr.sin_family      = AF_INET;
			locAddr.sin_port        = htons(newPort);
			locAddr.sin_addr.s_addr = htonl(INADDR_ANY);
			if (bind(newSocket, (SOCKADDR *)&locAddr, sizeof(locAddr)) < 0) {
				perror("Socket Error (Bind)");
				close(newSocket);
				sock_Error = NET_BINDERR;
				return NULL;
			}

			// Now set I/O async for socket stream.
			flags = fcntl(newSocket, F_GETFL, 0);
			fcntl(newSocket, F_SETFL, flags | FASYNC | O_NONBLOCK);
			fcntl(newSocket, F_SETOWN, getpid());

			// Set flags for socket table.
			flags = SOCK_SERVER;

			break;

		default:
			printf("Open: Socket error: Unknown mode\n");
			sock_Error = NET_UNKNOWNMODE;
			return NULL;
	}

	// Initialize a socket slot for new socket.
	Socket->idSocket = newSocket;
	Socket->locAddr  = locAddr;
	Socket->remAddr  = remAddr;
	Socket->Flags    |= (flags | SOCK_OPENED); // Now occupied.

	FD_SET(newSocket, &fdsRead);

	return Socket;
}

int sock_Close(SOCKET *Socket)
{
	SOCKET *srvSocket = Socket->Server;
	int oldSocket     = Socket->idSocket;

	FD_CLR(oldSocket, &fdsRead);
	FD_CLR(oldSocket, &fdsWrite);
	close(oldSocket);

	// Decrement a number of opened connections by one.
	if (srvSocket && (srvSocket->nConns > 0))
		srvSocket->nConns--;

	// Clear all slot.
	memset(Socket, 0, sizeof(SOCKET));
}

int sock_Listen(SOCKET *Socket, int backlog)
{
	if ((Socket->Flags & SOCK_OPENED) == 0)
		return NET_NOTVALID;

	if ((backlog == 0) || (backlog > SOMAXCONN))
		backlog = SOMAXCONN;

	// Tell socket to listen incoming connections.
	if (listen(Socket->idSocket, backlog) < 0) {
		perror("Socket Error (Listen)");
		sock_Error = errno;
		return NET_SOCKERR;
	}
	Socket->Flags |= SOCK_LISTEN;

	return NET_OK;
}

SOCKET *sock_Accept(SOCKET *srvSocket)
{
	SOCKET *Socket;
	SOCKADDRIN remAddr;
	int lenAddr = sizeof(remAddr);
	int newSocket;
	int flags, idx;

	if (srvSocket == NULL)
		return NULL;

	// Get a new socket.
	newSocket = accept(srvSocket->idSocket, &remAddr, &lenAddr);
	if (newSocket < 0) {
		perror("Socket Error (Accept)");
		sock_Error = errno;
		return NULL;
	}

	// Now set I/O async for socket stream.
	flags = fcntl(newSocket, F_GETFL, 0);
	fcntl(newSocket, F_SETFL, flags | FASYNC | O_NONBLOCK);
	fcntl(newSocket, F_SETOWN, getpid());
	FD_SET(newSocket, &fdsRead);

	// Find a empty slot for the incoming connection.
	Socket = NULL; // Assume that slots are full.
	if ((srvSocket->maxConns == 0) ||
	    (srvSocket->nConns < srvSocket->maxConns)) {
		for (idx = 0; idx < NET_MAXSOCKETS; idx++) {
			if (Sockets[idx].Flags >= 0) {
				srvSocket->nConns++;
				Socket = &Sockets[idx];
				break;
			}
		}
	}

	// If sockets are full, inform user that all circuits are busy.
	if (Socket == NULL) {
		printf("Socket Error (Accept): Sockets are full.\n");
		sock_Send(newSocket, "All connections busy. Try again later.\r\n", 0);
		sock_Send(newSocket, "\r\nTerminated.\r\n", 0);

		FD_CLR(newSocket, &fdsRead);
		close(newSocket);

		return NULL;
	}

	// Set up a new socket slot.
	Socket->Server   = srvSocket;
	Socket->idSocket = newSocket;
	Socket->Flags    = SOCK_OPENED|SOCK_CONNECT;
	Socket->locAddr  = srvSocket->locAddr;
	Socket->remAddr  = remAddr;

	return Socket;
}

int sock_Send(int idSocket, char *str, int len)
{
	if (str && *str) {
		if (len == 0)
			len = strlen(str);
#ifdef DEBUG
		if (dbg_Check(DBG_SOCKETS))
			sock_Dump(idSocket, str, len, "Output");
#endif
		return write(idSocket, str, len);
	}
	return 0;
}

// Process telnet codes and filter them out of data stream.
int sock_ProcessTelnet(uchar *str, int len)
{
	int state = 0;
	int perform = 0;
	int idx1, idx2 = 0;

	for (idx1 = 0; idx1 < len; idx1++) {
		switch (state) {
			case 0:
				if (str[idx1] == TEL_IAC) {
					state++;
					break;
				}
				str[idx2++] = str[idx1];
				break;

			case 1:
				switch (str[idx1]) {
					case TEL_IAC:
						str[idx2++] = str[idx1];
						state = 0;
						break;

					case TEL_WILL:
					case TEL_WONT:
					case TEL_DO:
					case TEL_DONT:
						perform = str[idx1];
						state++;
						break;

					default:
						state = 0;
						break;
				}
				break;

			case 2:
				perform = 0;
				state = 0;
				break;
		}
	}

	return idx2;
}

// SIGIO Handler routine to process sockets.

// I have a problem with that sock_Handler routine on
// Linux machine.  This routine accepts only first socket connection
// and ignore other incoming sockets forever until you exit this
// T10 emulator.  Until the problem is resolved, leave console
// connection alone for a while.  :-(

void sock_Handler(int sig)
{
	SOCKET *pSocket;
	char   inBuffer[NET_MAXBUF+1];
	int    nBytes;
	fd_set fdtRead = fdsRead;
	struct timeval tv = { 0, 0 };
	int    idx, reqs, maxfd = 0;

	// Get a highest file description number.
	for (idx = 0; idx < NET_MAXSOCKETS; idx++)
		if (Sockets[idx].idSocket > maxfd)
			maxfd = Sockets[idx].idSocket;

	// Find which open sockets that have requests for you.
	if ((reqs = select(maxfd+1, &fdtRead, NULL, NULL, &tv)) <= 0) {
		if (reqs < 0)
			perror("Socket Error (Select)");
		return;
	}

	// Process open sockets to perform activities.
	for (idx = 0; idx < NET_MAXSOCKETS; idx++) {
		pSocket = &Sockets[idx];
		if (pSocket->Flags & SOCK_OPENED) {
			if (FD_ISSET(pSocket->idSocket, &fdtRead)) {
				if (pSocket->Flags & SOCK_LISTEN) {
					pSocket->Accept(pSocket);
				} else {
					// Socket successfully is connected.
					if (pSocket->Flags & SOCK_CONNECT)
						pSocket->Flags &= ~SOCK_CONNECT;

					// Attempt to read a packet from Internet.
					nBytes = read(pSocket->idSocket, inBuffer, NET_MAXBUF);
					if (nBytes < 0) {
						// Socket Error
						if (errno == EAGAIN)
							break;
						pSocket->Eof(pSocket, nBytes, errno);
						perror("Socket Error (Receive)");
					} else if (nBytes == 0) {
						// End-of-File - Close a socket normally.
						printf("Socket %d - EOF Read\n", pSocket->idSocket);
						pSocket->Eof(pSocket, nBytes, errno);
					} else {
						// Incoming Data
#ifdef DEBUG
						if (dbg_Check(DBG_SOCKETS))
							sock_Dump(pSocket->idSocket, inBuffer, nBytes, "Input");
#endif
						pSocket->Process(pSocket, inBuffer, nBytes);
					}
				}
			}
		}
	}
}

void sock_ShowList(void)
{
	int idx;

	for (idx = 0; idx < NET_MAXSOCKETS; idx++) {
		if (Sockets[idx].Flags < 0) {
			printf("%3d %5d %08X/%-5d %08X/%-5d\n",
				idx, Sockets[idx].idSocket,
				ntohl(Sockets[idx].locAddr.sin_addr.s_addr),
				ntohs(Sockets[idx].locAddr.sin_port),
				ntohl(Sockets[idx].remAddr.sin_addr.s_addr),
				ntohs(Sockets[idx].remAddr.sin_port));
		}
	}
}
