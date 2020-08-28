/*
 *  This module implements functions to run the cpu in the background,
 *  as a separate thread, at a given number of instructions/second.
 */

#include "hexsim.h"

#include <pthread.h>
#include <time.h>
#include <sys/time.h>

/*
 *  Background state, used for checking if we are running or not.
 */

volatile int state;		/* Non-zero if running. */
volatile int startflag;		/* Want to start. */
volatile int stopflag;		/* Want to stop. */

unsigned int ips = 100;		/* Default speed. */

pthread_t       bg_cpu_id;
pthread_cond_t  bg_cpu_cond;
pthread_mutex_t bg_cpu_lock;
pthread_mutex_t bg_win_lock;

/*
 *  The actual thread.
 */

void* bg_cpu_thread(void* arg)
{
  struct timeval zerotime, now;
  struct timespec zzz;
  unsigned long long icount, tcount;

  stopflag = 0;			/* Clear stop flag. */
  state = 0;			/* Initial state is stopped. */

  pthread_mutex_lock(&bg_cpu_lock);

  for (;;) {
    while (!startflag)
      pthread_cond_wait(&bg_cpu_cond, &bg_cpu_lock);
    state = 1;
    startflag = 0;

    pthread_mutex_lock(&bg_win_lock);

    //    printf("cpu starting...\n");

    (void) gettimeofday(&zerotime, NULL);
    icount = 0;

    while (!stopflag) {
      (void) gettimeofday(&now, NULL);
      tcount = 1000000 * (now.tv_sec - zerotime.tv_sec);
      tcount += (now.tv_usec - zerotime.tv_usec);
      tcount *= ips;
      tcount /= 1000000;

      while (icount < tcount) {
        icount += 1;
	(void) cpu_execute();
      }

      w_background();

      if (ips > 0) {
        zzz.tv_sec = 0;
        zzz.tv_nsec = 1000000000 / ips;
        (void) nanosleep(&zzz, NULL);
      }
    }

    state = 0;
    stopflag = 0;

    //    printf("cpu stopping...\n");

    pthread_mutex_unlock(&bg_win_lock);
  }

  return NULL;
}

/*
 *  Get/set ips.
 */

unsigned int bg_ips_get(void)
{
  return ips;
}

void bg_ips_set(unsigned int newips)
{
  ips = newips;
}

/*
 *  Start running in the background.
 */

void bg_run(void)
{
  if (state != 0)
    return;

  startflag = 1;
  pthread_cond_signal(&bg_cpu_cond);
}

/*
 *  Return non-zero if we are running.
 */

int bg_state(void)
{
  return state;
}

/*
 *  Stop the background process.
 */

void bg_stop(void)
{
  if (state == 0)
    return;

  stopflag = 1;

  /* The following just makes sure the background process is stopped. */

  pthread_mutex_lock(&bg_cpu_lock);
  pthread_mutex_unlock(&bg_cpu_lock);
}

/*
 *  Try to get the window update lock.  Return non-zero if we got it.
 */

int  bg_w_trylock(void)
{
  if (pthread_mutex_trylock(&bg_win_lock) == 0)
    return 1;

  return 0;
}

/*
 *  Release the window update lock.
 */

void bg_w_unlock(void)
{
  pthread_mutex_unlock(&bg_win_lock);
}

/*
 *  Init this module, i.e. set up mutexes and other stuff.
 */

void bg_init(void)
{
  int ret;

  ret = pthread_cond_init(&bg_cpu_cond, NULL);
  if (ret != 0) {
    printf("bg_init (pthread_cond_init): %s\n", strerror(ret));
    return;
  }

  ret = pthread_mutex_init(&bg_cpu_lock, NULL);
  if (ret != 0) {
    printf("bg_init (pthread_mutex_init (cpu lock)): %s\n", strerror(ret));
    return;
  }

  ret = pthread_mutex_init(&bg_win_lock, NULL);
  if (ret != 0) {
    printf("bg_init (pthread_mutex_init (win lock)): %s\n", strerror(ret));
    return;
  }

  ret = pthread_create(&bg_cpu_id, NULL, bg_cpu_thread, NULL);
  if (ret != 0) {
    printf("bg_init (pthread_create): %s\n", strerror(ret));
    return;
  }
}
