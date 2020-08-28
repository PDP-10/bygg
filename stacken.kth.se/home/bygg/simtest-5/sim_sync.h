/*
** sim_sync.h: os-dependent sync line access header file.
*/

/*
** Written 2001-2002 by Johnny Eriksson <bygg@stacken.kth.se>
*/

/*
** Revision history:
**
** 2002-04-15    Adopted from previous code.
** 2002-01-23    Modify for version 2.9.
** 2002-01-22    First attempt.
*/

t_stat sync_open(int* retval, char* cptr);
int sync_read(int line, uint8* packet, int length);
void sync_write(int line, uint8* packet, int length);
void sync_close(int line);
