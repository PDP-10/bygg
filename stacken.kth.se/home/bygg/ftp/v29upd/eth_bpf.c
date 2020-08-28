/*
** Entry points we should provide:
**
** eth_open(char* name, "callback handler")
**       -- open the network device.
**
** eth_reset(void)
**       -- reset the interface, as in power-on reset.
**
** eth_getbia(uint8* mac)
**       -- return default (burned in) mac address.
**
** eth_getmac(uint8* mac)
**       -- return current mac address.
**
** eth_setmac(uint8* mac)
**       -- set current mac address.
**
** eth_start(void)
**       -- start xmit/recv.
**
** eth_stop(void)
**       -- stop xmit/recv.
**
** eth_write(int count, byte *packet)
**       -- write a packet to the network, and give a callback when done.
**
** eth_read(int length, uint8** packet)
**       -- try to read a packet from the network, non-blocking.
*/

#include "sim_defs.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <net/bpf.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <net/if_dl.h>
#include <net/if.h>
#include <signal.h>

/*
** BPF filter to accept ARP and IP packets not to/from the main IP
** address of the interface.
**
** "tcpdump -s 1536 -dd arp or \(ip and not host 192.168.111.59\)"
*/

struct bpf_insn inst_active[] = {
  { 0x28, 0, 0, 0x0000000c },	/* (0) */
  { 0x15, 5, 0, 0x00000806 },	/* (1) */
  { 0x15, 0, 5, 0x00000800 },	/* (2) */
  { 0x20, 0, 0, 0x0000001a },	/* (3) */
  { 0x15, 3, 0, 0x00000000 },	/* (4) contains IP address. */
  { 0x20, 0, 0, 0x0000001e },	/* (5) */
  { 0x15, 1, 0, 0x00000000 },	/* (6) contains IP address. */
  { 0x6, 0, 0, 0x00000600 },	/* (7) */
  { 0x6, 0, 0, 0x00000000 },	/* (8) */
};

struct bpf_program filt_active = {
  sizeof(inst_active)/sizeof(struct bpf_insn), inst_active
};

/*
** BPF filter to reject everything.  Used when the device is idle.
*/

struct bpf_insn inst_passive[] = {
  { 0x6, 0, 0, 0x00000000 },    /* Ignore everything. */
};

struct bpf_program filt_passive = {
  sizeof(inst_passive)/sizeof(struct bpf_insn), inst_passive
};

/*
** Local variables:
*/

void (*odcallback)(void);	/* output done callback. */

int bpf;			/* open bpf device. */

uint8* buffer;			/* input buffer. */
unsigned int bufsiz;		/* size of buffer. */
unsigned int buflen;		/* amount of data in buffer. */
unsigned int bufpos;		/* next pos in buffer. */

uint8 biaaddr[6];		/* burned-in mac address. */
uint8 macaddr[6];		/* current mac address. */
uint32 ipaddr;			/* current (main) IP addres. */

int interrupt;			/* interrupt flag. */

/*
** local subroutines:
*/

void inputdone(int sig)
{
  interrupt = 1;
}

int openbpf(char* ethname)
{
  struct ifaddrs* ifap;
  struct ifaddrs* ifa;
  struct sockaddr_dl* sdl;
  struct sockaddr_in* sin;
  unsigned char* lladdr;

  char devname[20];
  int i;

  if (getifaddrs(&ifap) != 0) {
    return 0;
  }

  for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
    if (strcmp(ethname, ifa->ifa_name) == 0) {
      if (ifa->ifa_addr == NULL) {
	continue;
      }
      switch (ifa->ifa_addr->sa_family) {
      case AF_LINK:
	sdl = (struct sockaddr_dl*) ifa->ifa_addr;
	if (sdl == NULL) {
	  continue;
	}
	lladdr = LLADDR(sdl);
	memcpy(biaaddr, lladdr, 6);
	memcpy(macaddr, lladdr, 6);
	break;
      case AF_INET:
	sin = (struct sockaddr_in*) ifa->ifa_addr;
	if (sin == NULL) {
	  continue;
	}
	ipaddr = ntohl(sin->sin_addr.s_addr);
	break;
      }
    }
  }

  freeifaddrs(ifap);

  /* fixup active filter */

  /*
  inst_active[4].k = ipaddr;
  inst_active[6].k = ipaddr;
  */

  for (i = 0; i < 100; i++) {
    sprintf(devname, "/dev/bpf%d", i);
    if ((bpf = open(devname, O_RDWR, 0)) < 0) {
      if (errno == EBUSY) {
	continue;
      }
      return 0;
    }
    break;
  }
  
  return 1;
}

/*
** open ethernet service.
*/

t_stat eth_open(char* name, void (callback)(void))
{
  int ivalue;
  unsigned int uvalue;
  struct ifreq ifreq;
  
  odcallback = callback;

  if (!openbpf(name)) {
    return SCPE_OPENERR;
  }

  if (ioctl(bpf, BIOCGBLEN, &bufsiz) < 0) {
    return SCPE_OPENERR;
  }

  buffer = malloc(bufsiz);
  if (buffer == NULL) {
    return SCPE_OPENERR;
  }

  if (ioctl(bpf, BIOCSETF, &filt_passive) < 0) {
    return SCPE_OPENERR;
  }

  strncpy(ifreq.ifr_name, name, IFNAMSIZ);
  if (ioctl(bpf, BIOCSETIF, &ifreq) < 0) {
    return SCPE_OPENERR;
  }

  uvalue = 1;
  if (ioctl(bpf, BIOCIMMEDIATE, &uvalue) < 0) {
    return SCPE_OPENERR;
  }

  ivalue = 1;
  if (ioctl(bpf, FIONBIO, &ivalue) < 0) {
    return SCPE_OPENERR;
  }

  (void) signal(SIGUSR1, inputdone);

  uvalue = SIGUSR1;
  if (ioctl(bpf, BIOCSRSIG, &uvalue) < 0) {
    return SCPE_OPENERR;
  }

  ivalue = getpid();
  if (ioctl(bpf, FIOSETOWN, &ivalue) < 0) {
    return SCPE_OPENERR;
  }

  ivalue = 1;
  if (ioctl(bpf, FIOASYNC, &ivalue) < 0) {
    return SCPE_OPENERR;
  }

#ifdef BIOCSWRSELF
  uvalue = 1;
  printf("BPF: setting write-to-self... ");
  if (ioctl(bpf, BIOCSWRSELF, &uvalue) < 0) {
    printf("fail.\n");
  } else {
    printf("OK.\n");
  }
#endif

  bufpos = 0;
  buflen = 0;
  interrupt = 1;

  return SCPE_OK;
}

/*
** reset the interface.
*/

t_stat eth_reset(void)
{
  (void) ioctl(bpf, BIOCSETF, &filt_passive);
  (void) ioctl(bpf, BIOCFLUSH, NULL);
  (void) memcpy(macaddr, biaaddr, 6);

  return SCPE_OK;
}

/*
** return burned-in addres.
*/

t_stat eth_getbia(uint8* addr)
{
  (void) memcpy(addr, biaaddr, 6);
  return SCPE_OK;
}

/*
** return current address.
*/

t_stat eth_getmac(uint8* addr)
{
  (void) memcpy(addr, macaddr, 6);
  return SCPE_OK;
}

/*
** write current address.
*/

t_stat eth_setmac(uint8* addr)
{
  (void) memcpy(macaddr, addr, 6);
  return SCPE_OK;
  /* can return SCPE_NOFNC if the driver can't do this. */
}

/*
** start xmit/recv.
*/

t_stat eth_start(void)
{
  if (ioctl(bpf, BIOCSETF, &filt_active) < 0) {
    return SCPE_IOERR;
  }
  return SCPE_OK;
}

/*
** stop xmit/recv.
*/

t_stat eth_stop(void)
{
  if (ioctl(bpf, BIOCSETF, &filt_passive) < 0) {
    return SCPE_IOERR;
  }
  return SCPE_OK;
}

/*
** write data to ethernet.
*/

void eth_write(int length, uint8* packet)
{
  (void) memcpy(&packet[6], macaddr, 6);
  (void) write(bpf, packet, length);
  (*odcallback)();
}

/*
** read data from ethernet.
*/

static int checkpacket(uint8* p, int length)
{
  static uint8 broadcast[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  uint16 proto;
  uint32 ip_src, ip_dst;

  if (length < 14) return 0;
  proto = (p[12] << 8) + p[13];

  switch (proto) {
  case 0x0800:
    if (length < 34) return 0;
    ip_src = (p[26] << 24) + (p[27] << 16) + (p[28] << 8) + p[29];
    ip_dst = (p[30] << 24) + (p[31] << 16) + (p[32] << 8) + p[33];
    if (ip_dst == ipaddr) return 0;
    break;
  case 0x0806:
    if (length < 42) return 0;
    ip_src = (p[28] << 24) + (p[29] << 16) + (p[30] << 8) + p[31];
    break;
  }

  if (memcmp(&p[6], macaddr, 6) == 0) {
    if (memcmp(&p[0], macaddr, 6) == 0) {
      if (proto == 0x0800) {
	if (ip_src == ipaddr) return 1;
      }
      if (proto == 0x0806) {
	if (ip_src == ipaddr) return 1;
      }
    }
    if (memcmp(&p[0], broadcast, 6) == 0) {
      if (proto == 0x0806) {
	if (ip_src == ipaddr) return 1;
      }
    }
    return 0;
  }

  return 1;
}

int eth_read(uint8** packet)
{
  int length;
  struct bpf_hdr* hdr;

  if (interrupt) {
    interrupt = 0;

    for (;;) {
      if (bufpos >= buflen) {
	buflen = read(bpf, buffer, bufsiz);
	bufpos = 0;
      }
      if (bufpos >= buflen) {
	return 0;
      } else {
	hdr = (struct bpf_hdr*) &buffer[bufpos];
	*packet = &buffer[bufpos + hdr->bh_hdrlen];
	length = hdr->bh_caplen;
	bufpos += BPF_WORDALIGN(hdr->bh_hdrlen + hdr->bh_caplen);
	if (!checkpacket(*packet, length)) {
	  continue;
	}
	interrupt = 1;
	return length;
      }
    }
  }
  return 0;
}
