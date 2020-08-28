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

void (*odhandler)(void);

uint8 bia[6] = { 0x00, 0xa0, 0x24, 0x90, 0x0f, 0xa2 };
uint8 now[6] = { 0x00, 0xa0, 0x24, 0x90, 0x0f, 0xa2 };

t_stat eth_open(char* name, void (callback)(void))
{
  odhandler = callback;
  return SCPE_OK;
/* 
  printf("cannot attach ethernet to %s\n", name);
  return SCPE_OPENERR;
*/
}

t_stat eth_reset(void)
{
  return SCPE_OK;
}

t_stat eth_getbia(uint8* mac)
{
  (void) memcpy(mac, bia, 6);
  return SCPE_OK;
}

t_stat eth_getmac(uint8* mac)
{
  (void) memcpy(mac, now, 6);
  return SCPE_OK;
}

t_stat eth_setmac(uint8* mac)
{
  (void) memcpy(now, mac, 6);
  return SCPE_OK;
  /* can return SCPE_NOFNC if the driver can't do this. */
}

t_stat eth_start(void)
{
  return SCPE_OK;
}

t_stat eth_stop(void)
{
  return SCPE_OK;
}

void eth_write(int length, uint8* packet)
{
  (*odhandler)();
}

int eth_read(int length, uint8** packet)
{
  return 0;
}
