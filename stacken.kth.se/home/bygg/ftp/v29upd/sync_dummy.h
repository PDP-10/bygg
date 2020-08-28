/*
** Dummy sync line module.  Included from sim_sync.c, see that file for
** copyrights etc.
*/

/*
** Revision history:
**
** 2002-04-15    Adopted from previous code.
** 2002-01-23    Modify for version 2.9.
** 2002-01-22    First attempt.
*/

t_stat sync_open(int* retval, char* cptr)
{
  return SCPE_OK;		/* Say OK, play nul device. */
}

int sync_read(int line, uint8* packet, int length)
{
  return 0;    
}

void sync_write(int line, uint8* packet, int length)
{
  /* do nothing */
}

void sync_close(int line)
{
  /* do nothing */
}
