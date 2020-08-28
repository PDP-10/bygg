/*
** I/O package for simulating synchronous lines.
*/

/*
** Revision history:
**
** 2002-01-23    Modify for version 2.9.
** 2002-01-22    First attempt.
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>

#include "sim_defs.h"

#define MAXLINES 16		/* Max. number of lines we can handle. */

struct syncblock {
  t_bool running;		/* True if this line is up. */
  int sockfd;			/* Socket. */
  uint16 lport, rport;		/* Local/Remote ports. */
  uint32 raddr;			/* Remote address. */
  struct sockaddr_in loc_addr;	/* Local address of this connection. */
  struct sockaddr_in rem_addr;	/* Remote address of this connection. */
};

struct syncblock sbtab[MAXLINES] = { 0 };

t_stat sync_open(int* retval, char* cptr)
{
  int ivalue;

  int sbindex;
  struct syncblock* sb;

  uint16 lport, rport;
  uint32 raddr;

  lport = strtotv(cptr, &cptr, 10);
  rport = strtotv(cptr, &cptr, 10);

  while (isspace(*cptr)) cptr++;
  if (*cptr != (char) 0) {
    raddr = inet_addr(cptr);
    if (raddr == INADDR_NONE) {
      return SCPE_ARG;
    }
  } else {
    raddr = inet_addr("127.0.0.1");
  }

  if ((lport == 0) || (rport == 0)) {
    return SCPE_ARG;
  }

  for (sbindex = 0; sbindex < MAXLINES; sbindex += 1) {
    sb = &sbtab[sbindex];
    if (sb->running) continue;

    if ((sb->sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      return SCPE_OPENERR;
    }

    bzero((char*) &(sb->rem_addr), sizeof(struct sockaddr_in));
    sb->rem_addr.sin_family = AF_INET;
    sb->rem_addr.sin_addr.s_addr = raddr;
    sb->rem_addr.sin_port = htons(rport);

    bzero((char*) &(sb->loc_addr), sizeof(struct sockaddr_in));
    sb->loc_addr.sin_family = AF_INET;
    sb->loc_addr.sin_addr.s_addr = INADDR_ANY;
    sb->loc_addr.sin_port = htons(lport);

    if (bind(sb->sockfd, (struct sockaddr*) &(sb->loc_addr),
	     sizeof(struct sockaddr)) < 0) {
      return SCPE_OPENERR;
    }

    ivalue = 1;
    if (ioctl(sb->sockfd, FIONBIO, &ivalue) < 0) {
      return SCPE_OPENERR;
    }

    sb->running = TRUE;
    *retval = sbindex;
    return SCPE_OK;
  }
  return SCPE_OPENERR;		/* None free, fail. */
}

int sync_read(int line, uint8* packet, int length)
{
  struct syncblock* sb;
  struct sockaddr_in peer;
  int peerlen;

  sb = &sbtab[line & (MAXLINES - 1)];

  if (sb->running) {
    length = recvfrom(sb->sockfd, packet, length, 0,
		      (struct sockaddr*) &peer, &peerlen);

    /* should check for correct peer here. */

    if (length > 0) {
      return length;
    }
  }
  return 0;    
}

void sync_write(int line, uint8* packet, int length)
{
  struct syncblock* sb;

  sb = &sbtab[line & (MAXLINES - 1)];

  if (sb->running) {
    sendto(sb->sockfd, packet, length, 0,
	   (struct sockaddr*) &(sb->rem_addr),
	   sizeof(struct sockaddr));
  }
}

void sync_close(int line)
{
  struct syncblock* sb;

  sb = &sbtab[line & (MAXLINES - 1)];

  if (sb->running) {
    (void) close(sb->sockfd);
    sb->running = FALSE;
    sb->sockfd = -1;
  }
}
