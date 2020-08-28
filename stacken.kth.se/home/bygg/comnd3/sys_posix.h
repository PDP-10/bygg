/*
** System dependent code for comnd, generic POSIX version.
*/

#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

/*
** our variables:
*/

static struct termios savedstate;

/*
** save terminal settings and set up things the way we want them.
*/

#ifndef _POSIX_VDISABLE
#  define _POSIX_VDISABLE 0
#endif

static void setuptty(void)
{
  struct termios state;
  long vdisable;

  (void) tcgetattr(STDIN_FILENO, &state); /* XXX should check for errs. */
  savedstate = state;		/* Save for later. */

  if ((vdisable = fpathconf(STDIN_FILENO, _PC_VDISABLE)) < 0) {
    vdisable = _POSIX_VDISABLE;
  }

#ifdef VDSUSP
  state.c_cc[VDSUSP] = vdisable;
#endif
  state.c_cc[VQUIT] = vdisable;
  
  state.c_cc[VMIN] = 1;
  state.c_cc[VTIME] = 0;

  state.c_iflag |= (IGNBRK);
  state.c_lflag &= ~(ECHO | ICANON);

  (void) tcsetattr(STDIN_FILENO, TCSANOW, &state);
}

/*
** restore terminal settings since we are either suspening ourselves
** or exiting.
*/

static void restoretty(void)
{
  (void) tcsetattr(STDIN_FILENO, TCSANOW, &savedstate);
}

/*
** Signal trickery to handle suspend & restore terminal state.
*/

static void sighandler(int sig)
{
  sigset_t mask;

  restoretty();

  sigemptyset(&mask);
  sigaddset(&mask, sig);
  sigprocmask(SIG_UNBLOCK, &mask, NULL);
  signal(sig, SIG_DFL);
  raise(sig);
  signal(sig, sighandler);

  setuptty();
}

/*
** This routine is called to init the terminal:
*/

void sys_init(void)
{
  setuptty();
  (void) atexit(restoretty);
  (void) signal(SIGTSTP, sighandler);
  (void) signal(SIGINT, sighandler);
}
