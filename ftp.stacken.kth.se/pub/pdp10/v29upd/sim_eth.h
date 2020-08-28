/*
** sim_eth.h: os-dependent ethernet access header file.
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

typedef void (eth_odhandler)(void);

t_stat eth_open(char* name, eth_odhandler callback);
t_stat eth_close(void);

t_stat eth_reset(void);

t_stat eth_getbia(uint8* mac);
t_stat eth_getmac(uint8* mac);
t_stat eth_setmac(uint8* mac);

t_stat eth_start(void);
t_stat eth_stop(void);

void eth_write(int length, uint8* packet);
int eth_read(uint8** packet);
