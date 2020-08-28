/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /src/sos5.bin/cucca/mm/mm/RCS/exit.c,v 2.9 1997/10/21 22:20:46 howie Exp $";
#endif

/*
 * exit.c - various ways to exit mm
 */

#include "mm.h"
#include "parse.h"
#include "cmds.h"

extern int OLD_UMASK;

int
cmd_quit (n)
int n;
{
    int i,updfl;
    extern int expunge_on_bye;

    confirm ();
    if (mode & MM_SEND) {		/* if sending, quit that mode */
	mode &= ~(MM_SEND|MM_ANSWER);
	return true;
    }
    if (mode & MM_READ) {		/* if reading, quit that mode */
	mode &= ~MM_READ;
	return true;
    }
    if (suspend_on_quit && (n == CMD_QUIT))  {
	suspend(UPD_SAVEMOD);
    }
    else {				/* qquit/bye */
	if (cf != NULL && 
	    (!(cf->flags & MF_RDONLY) || (cf->flags & MF_SAVEONLY))) {
	    /* if deleted messages, ask about expunging */
	    switch (expunge_on_bye) {
	    case SET_NO:
		updfl = UPD_SAVEMOD;
		break;
	    case SET_YES:
		updfl = UPD_SAVEMOD|UPD_EXPUNGE;
		break;
	    default:			/* SET_ASK */
		updfl = UPD_SAVEMOD;
		for (i = 1; i <= cf->count; i++)
		    if (cf->msgs[i].flags & M_DELETED) {
			if (yesno ("Expunge deleted messages? ", "yes"))
			    updfl |= UPD_EXPUNGE;
			break;
		    }
		break;
	    }
	    cmtend ();			/* let go of the terminal */
	    if (!update (&cf,updfl) && (cf->flags & MF_WRITERR)) {
		fprintf (stderr, "Cannot save file, not quitting.\n");
		fprintf (stderr,
"Use the SHELL or SUSPEND command to temporarily exit MM, free up some\n\
space, and then try again.\n");
		cmtset ();		/* set CCMD up again */
		cmcsb._cmwrp = autowrap_column;	/* put that back */
		return (false);
	    }
	}
	else
	    cmtend ();			/* let go of the terminal */
#ifdef MDEBUG
	m_done();
#endif /* MDEBUG */
#ifdef USAGEFILE
	usage_stop(USAGEFILE);
#endif
	exit (0);			/* quit entirely */
    }
}

int
cmd_exit (n)
int n;
{
    confirm ();
    if (!suspend_on_exit) {
	cmtend ();
	if (cf != NULL)			/* expunge while saving */
	    if (!update (&cf,UPD_EXPUNGE|UPD_SAVEMOD) &&
		(cf->flags & MF_WRITERR)) {
		fprintf (stderr, "Cannot save file, not exiting.\n\
Try suspending MM, and making space by deleting some files.\n");
		cmtset ();		/* set CCMD up again */
		cmcsb._cmwrp = autowrap_column;	/* put that back */
		return (false);
	    }

	done (0);
    }
    else {
	if (mode == MM_TOP_LEVEL) {
	    /* squish after we come back from suspend */
	    /* note that the return code from suspend comes from update */
	    /* and if the update failed we don't want to squish */
	    if (suspend(UPD_EXPUNGE|UPD_SAVEMOD) &&
		(cf != NULL))
		squish (cf);		/* free deleted messages */
	}
	else
	    suspend(0);			/* same as suspend */
    }
}


/*
 * cmd_expunge:
 * Write out the mail file without the deleted messages.  Adjust the
 * msgvec structure to remove these messages internally as well.
 */
int
cmd_expunge (n)
int n;
{
    confirm ();
    if (mode & MM_READ)	{		/* read mode? */
	fprintf (stderr,
		 "You cannot run expunge while in read mode.\n");
	return;
    }
    /* don't use check_cf because it consults modify_read_only */
    /* and we want to know if we can *really* write to the file */
    if (!cf)
	return;
    if (cf->flags & MF_RDONLY) {
	fprintf (stderr, "File is read-only, cannot expunge.\n");
	return;
    }
    if (!update (&cf,UPD_EXPUNGE|UPD_SAVEMOD)) /* do the expunge */
	return;
    if (cf != nil)			/* is it empty? */
	squish (cf);			/* no, free deleted messages */
}

/*
 * cmd_write:
 * Write out the mail file with the deleted messages.
 */
int
cmd_write (n)
int n;
{
    char *savefile;			/* where to write it to */
    char *parse_output_file();
    time_t save_mtime;

    noise ("out mail file to");
    if (!check_cf(O_RDONLY))		/* error return if no file */
	return;
    savefile = parse_output_file("confirm with carriage return\n\
  or alternate filename", cf->filename, true);
    confirm ();				/* XXX memory leak */
    if (strcmp (savefile, cf->filename) == 0) { /* same file, save work */
	if ((cf->flags & MF_RDONLY) && !(cf->flags & MF_SAVEONLY))
	    cmerr ("cannot save read-only (examined) file under same name\n");
	update (&cf,UPD_ALWAYS);	/* do the write */
	return;
    }
    else {
	char realfile[MAXPATHLEN];
	/* XXX This is bogus */
	strcpy (realfile,cf->filename);	/* remember real filename */
	strcpy (cf->filename, savefile); /* pretend this is the file */
	save_mtime = cf->mtime;
	update (&cf, UPD_ALWAYS|UPD_ALTFILE);
	cf->mtime = save_mtime;		/* shouldn't change for altfile */
	strcpy (cf->filename, realfile); /* put that back */
    }
    (void) free (savefile);
    return;
}


/*
 * squish:
 * Remove deleted messages from the msgvec structure passed, and
 * squish remaining messages together.
 */
squish (mail)
msgvec *mail;
{
    int oldpos,newpos;			/* to move remaining messages */
    char *text;				/* text to free */
    message *msgs;			/* gonna use it a lot */

#ifdef DEBUG
	debug_validate_msgvec("before squish");
#endif
    if ((msgs = mail->msgs) == NULL)	/* easy access! */
	return;				/* empty, nothing to squish */
    for (oldpos = newpos = 1; oldpos <= mail->count; oldpos++) {
	if (msgs[oldpos].flags & M_DELETED) {
	    if ((text = msgs[oldpos].text) != NULL)
		free (text);
	    if (msgs[oldpos].from != NULL)
	        free (msgs[oldpos].from);
	    if (msgs[oldpos].hdrsum != NULL)
	        free (msgs[oldpos].hdrsum);
	    if (msgs[oldpos].keywords != NULL)
	        free_keylist(msgs[oldpos].keywords);
	}
	else {
	    if (newpos != oldpos)	/* past deleted messages? */
		msgs[newpos] = msgs[oldpos]; /* move it down */
	    newpos++;
	}
    }
    mail->current = mail->count = newpos-1; /* didn't write to final pos'n */
    mail->msgs = (message *) realloc (mail->msgs, 
				      (mail->count+1)*sizeof (message));
#ifdef DEBUG
	debug_validate_msgvec("after squish");
#endif
}

int
cmd_take (n)
int n;
{
    extern top_level_parser();

    cmtake(top_level_parser);
    cmcsb._cmwrp = autowrap_column;	/* put that back! */
}

panic (s)
char *s;
{
    printf ("%s: panic: %s\n", progname, s);
    cmtend ();
    abort();
}

char *
errstr(err)
{
    static char temp[16];

    switch (err) {
      case 0:
	return (char *) "no error";
      case -1:
	err = errno;
      default:
	if ((err < 0) || (err > sys_nerr)) {
	    sprintf(temp,"Error %d", err);
	    return (char *) temp;
	}
	return (char *) sys_errlist[err];
    }
}

int
cmd_push (n)
int n;
{
    confirm ();
    shell (nil);
    return true;
}

shell (cmd)
char *cmd;
{
    int pid, status;
    char *getenv (), *cp;

    /*
     * put tty back the way we found it
     */
    cmtend ();

    /*
     * update the user's file, just in case
     */
    if (cf)
	update(&cf,0);

    /*
     * make sure user knows how to get back
     */
    if (!cmd)
	printf("Pushing to subshell, use \"exit\" to return to MM\n");

    if (!(cp = getenv ("SHELL")))
	cp = "/bin/sh";

    fix_signals_for_fork (true);

    pid = vfork ();
    if (pid == 0) {
	umask(OLD_UMASK);
	if (cmd)
	    /* one-shot command */
	    execl (cp, cp, "-c", cmd, 0);
	else
	    /* interactive shell */
	    execl (cp, cp, 0);
	perror (cp);
	_exit (1);
    }
    if (pid > 0)
	status = wait_for_process (pid);
    else
	perror ("mm: fork");

    fix_signals_for_fork (false);	/* restore SIGCHLD, tty pgrp */

    cmtend ();				/* clean up after child */
    cmtset ();				/* set up terminal again */
    cmcsb._cmwrp = autowrap_column;	/* put that back */
    return status;
}

/*
 * mm_execute:
 * fork and execvp using given arguments and wait for child to terminate.
 * based on shell() command above.
 */

mm_execute (file, argv)
char *file;
char **argv;
{
    int pid, status;

    cmtend ();
    if (cf)				/* update, since who knows what may */
	update(&cf,0);			/* happen, and we turn of sig hndlrs */

    fix_signals_for_fork (true);

    pid = vfork ();
    if (pid == 0) {
	umask(OLD_UMASK);
	execvp (file, argv);
	perror (file);
	_exit (1);
    }
    if (pid > 0)
	status = wait_for_process (pid);
    else
	perror ("mm: fork");

    fix_signals_for_fork (false);

    cmtend ();				/* clean up after child */
    cmtset ();				/* set up terminal again */
    cmcsb._cmwrp = autowrap_column;	/* put that back */

    return status;
}


/* fancy exit */
done(n)
int n;
{
    cmtend ();
#ifdef MDEBUG
    m_done();
#endif
#ifdef USAGEFILE
    usage_stop(USAGEFILE);
#endif
    exit (n);
}


/*
 * CMD_SUSPEND:
 * suspend MM.
 */

cmd_suspend (n) 
int n; 
{
  confirm();
  suspend(0);
}

/*
 * suspend ourself.  this routine may be called either interactively,
 * or from the SIGTSTP handler.
 */

suspend (updflags)
int updflags;				/* flags for update */
{
#ifdef SIGTSTP
    int updret = TRUE;
    long mask = block_signals ();

    cmtend ();				/* turn off ccmd's control of tty */

    if (cf)
	updret = update (&cf, updflags); /* make sure file is saved */

#ifdef USAGEFILE
    usage_stop (USAGEFILE);
#endif

    (void) kill (0, SIGSTOP);		/* stop ourself */

#ifdef USAGEFILE
    usage_start();
#endif

    cmtend ();				/* XXX fix up terminal again */
    cmtset ();				/* fix up the terminal */
    cmcsb._cmwrp = autowrap_column;	/* cmtset resets this */

    release_signals (mask);

    return (updret);
#else	/* !SIGTSTP */
    printf ("The SUSPEND command is not supported on this system.\n");
#endif	/* SIGTSTP */  
}

signalhandler
sighup_handler()
{
    if (cf != NULL)
	update (&cf, UPD_SAVEMOD);	/* make sure file is saved */
    done (1);
}

#ifdef SIGXCPU
signalhandler
sigxcpu_handler() {
    fprintf (stderr, "\
WARNING: This process has received an XCPU signal!\n\
This MM process has exceeded the per process CPU limit and will soon be\n\
killed.  Please save your work and exit MM.\n");
  if (cf != NULL) {
      printf ("Saving %s ...\n", cf->filename);
      update (&cf, UPD_SAVEMOD);
  }
}
#endif /* SIGXCPU */

/*
 * askem:
 * ask them the question and encourage them to give us a good answer
 */

#define MAXEOF 10

askem (tty, pr, prlen)
int tty;
char *pr;
int prlen;
{
    char ans[BUFSIZ];
    int i;
    static char hint[] = "Please answer 'y' or 'n'\n";
    int eofcount = 0;

    while (TRUE) {
	i = 0;
	write (tty, pr, prlen);
	do {				/* get a line */
	    if (read (tty, &ans[i], 1) <= 0) {
		eofcount++;
	    }
	    if (eofcount > MAXEOF)
		return (TRUE);		/* assume yes */
	} while ((ans[i++] != '\n') && (i < BUFSIZ));
	if (i < BUFSIZ)
	    ans[i] = '\0';
	if (i == 2) {
	    if ((ans[0] == 'n') || (ans[0] == 'N'))
		return (FALSE);
	    if ((ans[0] == 'y') || (ans[0] == 'Y'))
		return (TRUE);
	}
	if (ustrcmp (ans, "no\n") == 0)
	    return (FALSE);
	if (ustrcmp (ans, "yes\n") == 0)
	    return (TRUE);

	/* failed, try again */
	write (tty, hint, sizeof (hint)-1);
    }
}


/*
 * sigint_handler:
 * handle sigint (^C) by saving the file and then exiting
 * ask them if they really want to exit
 */
signalhandler
sigint_handler()
{
    int tty;
    static char ask[] = "\nDo you really want to exit MM? [y/n] ";
    static char ask2[] = "\nSave mail file before exiting? [y/n] ";
    static char warn[] = "Cannot save file, not exiting.\n\
Try suspending MM, and making space by deleting some files.\n";
    static char cont[] = "Continuing...\n";
    char ans[3];
    
    cmtend ();				/* restore sane tty settings */
    tty = open ("/dev/tty", O_RDWR, 0);	/* read and write at them */
    if (tty >= 0 && !askem(tty, ask, sizeof (ask)-1)) {
	write (tty, cont, sizeof(cont)-1);
	close (tty);
	cmtset ();			/* give tty back to CCMD */
	return;				/* that's it, no exit */
    }

    if ((cf != NULL) &&
	(cf->flags & (MF_DIRTY|MF_MODIFIED))) { /* save the file */
	if (tty < 0 || askem (tty, ask2, sizeof (ask2)-1)) {
	    if (!update (&cf, UPD_SAVEMOD|UPD_QUIET) 
		&& (cf->flags & MF_WRITERR)) {
		if (tty >= 0) {
		    write (tty, warn, sizeof(warn)-1);
		    close (tty);
		    cmtset ();		/* give tty back to CCMD */
		    return;		/* keep going */
		}
	    }
	}
    }
#ifdef USAGEFILE
    usage_stop(USAGEFILE);
#endif
    signal (SIGINT, SIG_DFL);
    kill (PID, SIGINT);
}

int
wait_for_process (pid)
int pid;
{
    return do_wait (pid, true);
}

int
collect_process (pid)
int pid;
{
    return do_wait (pid, false);
}

/* From K & R... */
int
getbits(x, p, n)
unsigned x, p, n;
{
    return((x >> (p+1-n)) & ~(~0 << n));
}

/*
 * wait for a process, optionally blocking
 *
 * a pid of zero means we are being invoked by the
 * SIGCHLD handler to collect the process that just
 * exited.
 *
 * signal handlers should not set "blocking" to true.
 */

int
do_wait (pid, blocking)
int pid, blocking;
{
    int n;
    unsigned int status;

    if (blocking)
	fix_signals_for_wait (true);

#ifdef DEBUG
    fprintf(stderr, "pid is  %d\n", pid);
#endif
    do {
#ifdef HAVE_WAIT3
#ifdef SOLARIS
        int w;
#else
	union wait w;
#endif /* SOLARIS */
	int flags = blocking ? WUNTRACED : (WUNTRACED|WNOHANG);
#endif
	/*
	 * don't wait for a process that's already gone.
	 * this could happen if we're called while the
	 * SIGCHLD handler is active.
	 */
	if (blocking && pid > 0 && kill (pid, 0) < 0 && errno == ESRCH) {
		status = -1;
		break;
	}

#ifdef HAVE_WAIT3
	n = wait3 (&w, flags, 0);
#ifdef SIGCONT /* AIX 2.2.1 has wait3() but not SIGCONT */
#ifdef SOLARIS

#ifdef DEBUG
	fprintf(stderr, "wait3 returned %d\n", n);
	fprintf(stderr, "wait3 errno %d\n", errno);
	fprintf(stderr, "wait3 status is %d\n", getbits(w, 7, 8));
#endif /* DEBUG */

	if (n > 0 && getbits(w, 7, 8) == 0177) {

#ifdef DEBUG
	  fprintf(stderr, "stopped -- continuing\n");
#endif /* DEBUG */
#else
	if (n > 0 && w.w_stopval == WSTOPPED) {
#endif /* SOLARIS */
	    (void) kill (n, SIGCONT);
	    continue;
	}
#endif
#ifdef SOLARIS
	if (n == -1 && errno == EINTR) {
#ifdef DEBUG
	  fprintf(stderr, "wait3 got EINTR\n");
#endif /* DEBUG */
	    continue;			/* wait again? */
	}

	status = getbits(w, 15, 8);

#ifdef DEBUG
	fprintf(stderr, "child status is %d\n", getbits(w, 15, 8));
#endif /* DEBUG */
#else
	status = w.w_status;
#endif /* SOLARIS */
#else /* ! HAVE_WAIT3 */
	n = wait (&status);
#endif /* ! HAVE_WAIT3 */
	if (n > 0)
	    forget_pid (n);
	else
	    if (n == 0 && !blocking) {
		status = 0;
		break;
	    }
	    else
		if (n < 0 && errno != EINTR) {
		    status = -1;
		    break;
		}

	if (n == pid)
	    break;

    } while (blocking);

    if (blocking)
	fix_signals_for_wait (false);

    return status;
}

#define MAXBG 10
static nrun = 0;
static unsigned bg_procs[MAXBG];

maybe_wait_for_process (pid)
int pid;
{
    int i;

    if (nrun < MAXBG) {
	for (i = 0; i < MAXBG; i++)
	    if (bg_procs[i] == 0) {
		bg_procs[i] = pid;
		nrun++;
		return;
	    }
    }
    /* see if any of the procs went away when we weren't looking */
    for (i = 0; i < MAXBG; i++)
	if ((kill (bg_procs[i], 0) < 0) && (errno == ESRCH)) {
	    bg_procs[i] = pid;		/* oh good, this slot's empty */
	    return;
	}
    
    /*
     * we've exceeded the number of allowed background processes,
     * so we'll just have to wait for this one.
     */
    wait_for_process (pid);
}

forget_pid (n)
unsigned n;
{
    int i;
    for (i = 0; i < MAXBG; i++)
	if (bg_procs[i] == n) {
	    bg_procs[i] = 0;
	    --nrun;
	}
}
