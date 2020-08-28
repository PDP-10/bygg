/*
** Dummy ethernet module.  Included from sim_eth.c, see that file for
** copyrights etc.
*/

eth_odhandler odhandler;

uint8 bia[6] = { 0x00, 0xa0, 0x24, 0x90, 0x0f, 0xa2 };
uint8 now[6] = { 0x00, 0xa0, 0x24, 0x90, 0x0f, 0xa2 };

t_stat eth_open(char* name, eth_odhandler callback)
{
  odhandler = callback;
  return SCPE_OK;
}

t_stat eth_close(void)
{
  return SCPE_OK;
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
