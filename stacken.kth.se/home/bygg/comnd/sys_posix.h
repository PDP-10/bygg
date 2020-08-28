/*
**  Copyright (c) 2001 - 2004, Johnny Eriksson
**  All rights reserved.
**
**  Redistribution and use in source and binary forms, with or without
**  modification, are permitted provided that the following conditions
**  are met:
**
**  1. Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**
**  2. Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in the
**     documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
