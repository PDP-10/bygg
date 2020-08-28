/*
** idler module for BSD UNIX.  Included from sim_idle.c, see that file for
** copyrights etc.
*/

/*
** Revision history:
**
** 2002-04-16    Made into a module.
*/

#include <sys/time.h>

extern int32 tmxr_poll;		/* calibrated delay */

int32 sys_idle(int32 cycles)
{
  struct timeval starttime, stoptime;
  struct timespec zzz;

  uint32 nspi;			/* Nano-seconds per instruction. */
  uint32 ips;			/* Instructions per second. */
  uint32 sleeptime;		/* Time slept, in usec. */

  if (cycles <= 0) return 1;	/* Silly request? */

  ips = tmxr_poll * 50;		/* UGH! */
  nspi = 1000000000 / ips;

  if (nspi <= 0) return 1;	/* Fast hardware? */

  zzz.tv_sec = 0;
  zzz.tv_nsec = nspi * cycles;
  /*  zzz.tv_nsec = 1; */

  (void) gettimeofday(&starttime, NULL);
  (void) nanosleep(&zzz, NULL);
  (void) gettimeofday(&stoptime, NULL);

  sleeptime = (stoptime.tv_sec - starttime.tv_sec) * 1000000;
  sleeptime += stoptime.tv_usec;
  sleeptime -= starttime.tv_usec;
  sleeptime /= 1000;		/* Convert to milliseconds. */
  if (sleeptime == 0) return 1;	/* Short sleep. */

  return ((ips * sleeptime) / 1000);
}
