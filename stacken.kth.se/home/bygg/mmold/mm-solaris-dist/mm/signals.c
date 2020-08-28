/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /src/sos5.bin/cucca/mm/mm/RCS/signals.c,v 2.3 1997/11/14 18:40:23 howie Exp $";
#endif

/*
 * Signal handling support for MM.
 */

#include "mm.h"

#ifndef sigmask
#define sigmask(m) (1 << ((m) - 1))
#endif

#ifndef NSIG
#define NSIG 32
#endif

long signal_mask = 0, deferred_signals = 0;
signalhandler signal_handler();

/*
 * Set up signal handlers
 */

int
init_signals ()
{
    signal_mask = ~0;
    deferred_signals = 0;

    (void) signal (SIGHUP, signal_handler);
    (void) signal (SIGALRM, signal_handler);
    (void) signal (SIGINT, signal_handler);
#ifdef SIGPIPE
    (void) signal (SIGPIPE, signal_handler);
#endif
#ifdef SIGTSTP
    (void) signal (SIGTSTP, signal_handler);
#endif
#ifdef SIGXCPU
    (void) signal (SIGXCPU, signal_handler);
#endif
#ifdef SIGCHLD
    (void) signal (SIGCHLD, signal_handler);
#endif
    release_signals (0L);
}

/*
 * Block signals we want to lock out.  On BSD systems this
 * just calls sigblock(); on others, we maintain our own signal mask:
 * the signal_handler routine will catch signals and remember they
 * happened, so their handlers can be invoked by release_signals.
 */

long
block_signals ()
{
    long m = signal_mask;
    signal_mask |= sigmask (SIGHUP) | sigmask (SIGINT) |
#ifdef SIGPIPE
        sigmask (SIGPIPE) |
#endif
#ifdef SIGTSTP
	sigmask (SIGTSTP) |
#endif
#ifdef SIGCONT
	sigmask (SIGCONT) |
#endif
#ifdef SIGCHLD
	sigmask (SIGCHLD) |
#endif
#ifdef SIGXCPU
	sigmask (SIGXCPU) |
#endif
	sigmask (SIGALRM);
#ifdef HAVE_BSD_SIGNALS
    return sigblock (signal_mask);
#else
    return m;
#endif
}

/*
 * Restore the signal mask, and invoke handlers for any pending signals
 */
release_signals (m)
long m;
{
#ifdef HAVE_BSD_SIGNALS
    (void) sigsetmask (m);
#endif
    signal_mask = m;
    if (deferred_signals)
	run_deferred_signals ();
}

queue_signal (sig)
int sig;
{
    deferred_signals |= sigmask (sig);
}

run_deferred_signals ()
{
    int i;
    long m = deferred_signals & ~signal_mask;

    for (i = 0; (i < NSIG) && m; i++)
	if (m & sigmask (i)) {
	    m &= ~sigmask(i);
	    handle_signal (i);
	}
}

/*
 * This routine is the only one we install as a signal handler using
 * signal(2).  On BSD systems it just calls handle_signal(), while on
 * other systems it may defer invocation of the handler if the signal
 * has been disabled with block_signals.
 */

signalhandler
signal_handler (sig)
int sig;
{
#ifdef HAVE_BSD_SIGNALS
    handle_signal (sig);
#else
    unsigned long b = sigmask (sig);

    if (signal_mask & b) {
	queue_signal (sig);
	if (sig != SIGCHLD)
	    signal (sig, signal_handler);
	return;
    }

    handle_signal (sig);
    signal (sig, signal_handler);
#endif
    return;
}

/*
 * Dispatch to handle signal SIG.
 */

handle_signal (sig)
int sig;
{
    deferred_signals &= ~sigmask (sig);

    switch (sig) {
      case SIGALRM:
	new_mail (true);
	signal(SIGALRM, handle_signal);
	break;
      case SIGINT:
	sigint_handler ();
	break;
      case SIGHUP:
	sighup_handler ();
	break;
#ifdef SIGPIPE
      case SIGPIPE:			/* ignore */
	break;
#endif
#ifdef SIGCHLD
      case SIGCHLD:
	collect_process (0);
	break;
#endif
#ifdef SIGXCPU
      case SIGXCPU:
	sigxcpu_handler ();
	break;
#endif
#ifdef SIGTSTP
      case SIGTSTP:
	write (1, "\r\n", 2);		/* move to new line */
	suspend (0);			/* suspend ourself */
	break;
#endif
    }
}

/*
 * This routine should be called before and after any synchronous fork/wait
 * sequences; arg should be "true" on the first call and "false" on the
 * second.
 *
 * This routine serves to disable the SIGCHLD handler for callers who want
 * to wait for children to exit, and saves/restores the tty process group
 * for ill-behaved children who may change the tty process group and not
 * change it back before suspending themselves or exiting (e.g. ksh).
 *
 */

fix_signals_for_fork (before_the_fork)
int before_the_fork;
{
#ifdef SIGCHLD
    static signalhandler (*old_chld_handler)() = 0;
#endif
    signalhandler (*tmp)() = 0;
    static int ttypgrp = -1, nesting = 0;
    if (before_the_fork) {
	if (nesting++ != 0)
	    return;
#ifdef TIOCGPGRP
	if (isatty(2)) {
	    tmp = signal (SIGTTIN, SIG_IGN);
	    (void) ioctl (0, TIOCGPGRP, &ttypgrp);
	    if (tmp != SIG_IGN)
		signal (SIGTTIN, tmp);
	}
#endif
#ifdef SIGCHLD
	old_chld_handler = signal (SIGCHLD, SIG_DFL);
#endif
    }
    else {
#ifdef TIOCGPGRP
	if (--nesting != 0)
	    return;
	if (isatty(2) && ttypgrp > 0) {
	    tmp = signal (SIGTTOU, SIG_IGN);
	    (void) ioctl (2, TIOCSPGRP, &ttypgrp);
	    if (tmp != SIG_IGN)
		signal (SIGTTOU, tmp);
	}
	ttypgrp = -1;
#endif
#ifdef SIGCHLD
	(void) signal (SIGCHLD, old_chld_handler);
#endif
    }
}

/*
 * SIGINT and SIGQUIT must be ignored around blocking wait calls.
 * SIGTSTP should use the default signal handler.
 * 
 * under solaris (maybe elsewhere?) ignore SIGCHLD while we are waiting,
 * or the wait gets done by the signal handler, and we lose the return
 * status.
 */

fix_signals_for_wait (before_the_wait)
int before_the_wait;
{
    static nesting = 0;
    static signalhandler (*old_handlers[4]) ();
    
    if (before_the_wait) {
	if (++nesting == 1) {
	    old_handlers[0] = signal (SIGINT, SIG_IGN);
	    old_handlers[1] = signal (SIGQUIT, SIG_IGN);
#ifdef SIGTSTP
	    old_handlers[2] = signal (SIGTSTP, SIG_DFL);
#endif
#ifdef SOLARIS
	    old_handlers[3] = signal (SIGCHLD, SIG_DFL);
#endif
	}
    }
    else {
	if (--nesting == 0) {
	    (void) signal (SIGINT, old_handlers[0]);
	    (void) signal (SIGQUIT, old_handlers[1]);
#ifdef SIGTSTP
	    (void) signal (SIGTSTP, old_handlers[2]);
#endif
#ifdef SOLARIS
	    (void) signal (SIGCHLD, old_handlers[3]);
#endif
	}
    }
}

/*
 * mm_popen and mm_pclose:

 * Since MM traps SIGCHLD (to catch whatever sendmails it sent into
 * the background), it will wait() on the pipe process.  In fact, when
 * any process finishes, the SIGCHLD handler catches it.  So, when
 * pclose() does a wait, it never gets any processes.  The wait (in
 * pclose()) cannot return until all processes have sent their
 * SIGCHLDs and been waited on by the SIGCHLD handler.  This means
 * that pclose() must wait() until all those really SLOW sendmail
 * processes are done, which is pretty slow.
 * 
 * Therefore, in order to guarantee that the pipe process is still
 * around for pclose() to wait() on, we block or ignore all SIGCHLD
 * signals before we do the popen(), and turn our SIGCHLD handler back
 * on after the pclose() is done.  (Note that this may allow pclose()
 * to catch some of our sendmail processes, if they end before the
 * pipe process, but we check for that in maybe_wait_for_process().
 */

static signalhandler (*old_handler)();

FILE *
mm_popen (command, type)
char *command, *type;
{
    FILE *stream;

#ifdef SIGCHLD
    old_handler = signal (SIGCHLD, SIG_IGN);
#endif
    if ((stream = popen(command, type)) == NULL)
#ifdef SIGCHLD
	signal (SIGCHLD, old_handler);
#endif
    return (stream);
}

mm_pclose (stream)
FILE *stream;
{
    int ret;

    ret = pclose (stream);
#ifdef SIGCHLD
    signal (SIGCHLD, old_handler);
#endif
    return (ret);
}
