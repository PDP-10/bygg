/*
** sim_eth.c: os-dependent routines to access an ethernet.
*/

/*
** Written 2001-2002 by Johnny Eriksson <bygg@stacken.kth.se>
*/

/*
** Revision history:
**
** 2002-04-15    Adopted from previous code.
** 2002-01-23    Modify for version 2.9.
** 2001-10-11    Second beta release.
** 2001-09-30    First beta release.
*/

/*
** Entry points we should provide:
**
** eth_open(char* name, "callback handler")
**       -- open the network device.
**
** eth_close(void)
**       -- close the open channel.
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
#include "sim_eth.h"

/*
** Find out what code to generate:
*/

#if defined (ETH_DUMMY)		/* Force just a dummy module? */
#  include "eth_dummy.h"

#elif defined (ETH_BPF)		/* Force a BPF speaker? */
#  include "eth_bpf.h"

/*
** No forced version, try heuristics:
*/

#elif defined (__FreeBSD__) || \
      defined (__NetBSD__) || \
      defined (__OpenBSD__)	/* Known BSD? */
#  include "eth_bpf.h"

#elif defined (__linux__)	/* Thorvald-ux (tux)? */
#  include "eth_linux.h"

/*
** Can't do anything, have to use a dummy.
*/

#else
#  include "eth_dummy.h"
#endif
