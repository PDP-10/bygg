#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include <netinet/in.h>

/*
** stu [-k key] -v [-a remote] tcp-port udp-port udp-client
*/

int mastersock;			/* Master TCP socket. */
int worksock;			/* Working TCP socket. */
struct sockaddr_in loc_addr;	/* Local address of this connection. */
struct sockaddr_in rem_addr;	/* Remote address of this connection. */
int rem_len;			/* Should be socklen_t? */

int udpsock;			/* UDP socket. */
struct sockaddr_in udp_addr;	/* Local UDP params. */
struct sockaddr_in cli_addr;	/* Client UDP params. */

int verbose = 0;

int udpport;
int udpclient;
int tcpport;
char* remote = NULL;
char* key = NULL;

#define MAXBUF 65536

unsigned char tcibuf[4+MAXBUF];
unsigned char tcobuf[4+MAXBUF];
int tcipos, tcicount;
int tcopos, tcocount;

void work(void)
{
  int ivalue;
  fd_set rfd, wfd;
  struct timeval tv;
  int maxfd;
  int count;
  int len;
  int ret;

  if (verbose) printf("work: starting...\n");
  
  if ((udpsock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    /* should handle better. */
    perror("socket");
    exit(1);
  }

  /* bind local & remote addresses for UDP socket */

  bzero(&udp_addr, sizeof(struct sockaddr_in));
  udp_addr.sin_family = AF_INET;
  udp_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  udp_addr.sin_port = htons(udpport);

  if (bind(udpsock, (struct sockaddr*) &udp_addr,
	   sizeof(struct sockaddr)) < 0) {
    perror("bind");
    exit(1);
  }

  bzero(&cli_addr, sizeof(struct sockaddr_in));
  cli_addr.sin_family = AF_INET;
  cli_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  cli_addr.sin_port = htons(udpclient);

  if (connect(udpsock, (struct sockaddr*) &cli_addr,
	      sizeof(cli_addr)) < 0) {
    perror("connect");
    exit(1);
  }

  /* set sockets non-blocking */
 
  ivalue = 1;
  if (ioctl(worksock, FIONBIO, &ivalue) < 0) {
    perror("ioctl (FIONBIO)");
    exit(1);
  }

  if (ioctl(udpsock, FIONBIO, &ivalue) < 0) {
    perror("ioctl (FIONBIO)");
    exit(1);
  }

  /* move data, handle errors. */

  tcopos = tcocount = 0;
  tcipos = tcicount = 0;

  for (;;) {
    FD_ZERO(&rfd);
    FD_SET(worksock, &rfd);
    FD_SET(udpsock, &rfd);

    FD_ZERO(&wfd);
    if (tcocount > 0) {
      FD_SET(worksock, &wfd);
    }

    maxfd = (udpsock>worksock) ? udpsock : worksock;

    tv.tv_sec = 60;
    tv.tv_usec = 0;

    if (verbose) printf("calling select:\n");
    count = select(maxfd + 1, &rfd, &wfd, NULL, &tv);
    if (verbose) printf("select: return %d\n", count);
    
    /* handle:
    **   count = -1 -- error. (signal?) should not happen.
    **   count = 0  -- timeout. send dummy frame on tcp channel.
    **   count > 0  -- try move some data.
    */

    if ((count == 0) && (tcocount == 0)) {
      /* timeout, empty output buffer. build dummy frame. */
      tcobuf[0] = tcobuf[1] = tcobuf[2] = tcobuf[3] = 0;
      tcocount = 4;
      count = 1;
    }

    if (count > 0) {
      /* if any error indicates b0rken tcp connection, terminate loop. */

      if (tcocount > 0) {
	if ((ret = send(worksock, &tcobuf[tcopos], tcocount, 0)) < 0) {
	  if (errno != EAGAIN) break;
	} else {
	  tcopos += ret;
	  tcocount -= ret;
	}
      }
      if (FD_ISSET(worksock, &rfd)) {
	if (verbose) printf("read from worksock\n");
	ret = recv(worksock, &tcibuf[tcipos], 4+MAXBUF-tcipos, 0);
	if (verbose) printf("recv: read %d bytes\n", ret);
	if (ret < 0) {
	  if (verbose) {
	    printf("recv: errno = %d (%s)\n", errno, strerror(errno));
	  }
	  if (errno != EAGAIN) break;
	} else if (ret == 0) {
	  break;		/* connection was closed. */
	} else {
	  tcipos += ret;
	}
	while (tcipos >= 4) {
	  len = (tcibuf[2] << 8) + tcibuf[3];
	  if (len > (tcipos - 4)) break;
	  if (len > 0) {
	    (void) send(udpsock, &tcibuf[4], len, 0);
	  }
	  tcipos -= (len + 4);
	  bcopy(&tcibuf[len+4], &tcibuf[0], tcipos);
	}
      }
      if (FD_ISSET(udpsock, &rfd)) {
	if (verbose) printf("read from udpsock\n");
	if (tcocount > 0) {
	  (void) recv(udpsock, NULL, 0, 0);
	} else {
	  len = recv(udpsock, &tcobuf[4], MAXBUF, 0);
	  if (len > 0) {
	    tcobuf[0] = 0;
	    tcobuf[1] = 0;
	    tcobuf[2] = (len & 0xff00) >> 8;
	    tcobuf[3] = (len & 0x00ff) >> 0;
	    tcopos = 0;
	    tcocount = len + 4;
	    if ((ret = send(worksock, &tcobuf[tcopos], tcocount, 0)) < 0) {
	      if (errno != EAGAIN) break;
	    } else {
	      tcopos += ret;
	      tcocount -= ret;
	    }
	  }
	}
      }
    }
  }

  if (verbose) printf("exiting work loop\n");

  close(worksock);		/* Close us. */
  close(udpsock);
}

/*
** start up, active end:
*/

void do_active(void)
{
  for (;;) {
    if ((worksock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      perror("socket");
      exit(1);
    }

    bzero(&rem_addr, sizeof(rem_addr));
    rem_addr.sin_family = AF_INET;
    rem_addr.sin_addr.s_addr = inet_addr(remote);
    rem_addr.sin_port = htons(tcpport);

    if (connect(worksock, (struct sockaddr*) &rem_addr,
		sizeof(rem_addr)) < 0) {
      switch (errno) {
      case ETIMEDOUT:
      case ECONNREFUSED:
      case ENETUNREACH:
	if (verbose) printf("errno = %d (%s)\n", errno, strerror(errno));
	sleep(15);
	break;
      default:
	if (verbose) printf("errno = %d (%s)\n", errno, strerror(errno));
	sleep(15);
	break;
      }
      close(worksock);
    } else {
      work();
    }
  }
}

/*
** start up, passive end:
*/

void do_passive(void)
{
  int flag;

  if ((mastersock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }

  flag = 1;
  (void) setsockopt(mastersock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

  bzero(&loc_addr, sizeof(struct sockaddr_in));
  loc_addr.sin_family = AF_INET;
  loc_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  loc_addr.sin_port = htons(tcpport);

  if (bind(mastersock, (struct sockaddr*) &loc_addr,
	   sizeof(struct sockaddr)) < 0) {
    perror("bind");
    exit(1);
  }

  if (listen(mastersock, 1) < 0) {
    perror("listen");
    exit(1);
  }

  if (verbose) {
    printf("do_passive: starting accept loop\n");
  }

  for (;;) {
    rem_len = sizeof(struct sockaddr_in);
    worksock = accept(mastersock, (struct sockaddr*) &rem_addr, &rem_len);
    if (worksock < 0) {
      /* accept failed somehow. figure out how to do it again. */
      perror("accept");
      exit(1);
    }

    work();
  }
}

/*
** main program, command decoding etc.
*/

void usage(void)
{
  printf("usage: stu [-k key] [-v] [-a r-addr] r-port l-port client\n");
}

int main(int argc, char* argv[])
{
  int ch;

  while ((ch = getopt(argc, argv, "46a:k:v")) != -1)
    switch(ch) {
    case '4':
      /* set IPv4 only. */
      break;
    case '6':
      /* set IPv6 only. */
      break;
    case 'a':
      remote = optarg;
      break;
    case 'k':
      key = optarg;
    case 'v':
      verbose++;
      break;
    default:
      usage();
      exit(1);
    }
  argc -= optind;
  argv += optind;

  if (argc != 3) {
    usage();
    exit(1);
  }

  tcpport = atoi(argv[0]);
  udpport = atoi(argv[1]);
  udpclient = atoi(argv[2]);
  
  /* should verify non-zero, 16-bit data for above. */

  if (verbose) {
    printf("verbose level = %d\n", verbose);
    if (key) printf("key = %s\n", key);
    if (remote) {
      printf("active open to %s port %d\n", remote, tcpport);
    } else {
      printf("passive open on port %d\n", tcpport);
    }
    printf("my udp port = %d, client = %d\n", udpport, udpclient);
  }

  if (remote) {
    do_active();
  } else {
    do_passive();
  }
}
