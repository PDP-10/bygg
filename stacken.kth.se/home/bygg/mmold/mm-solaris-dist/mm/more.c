/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /amd/sherluck/sh/src0/sun4.bin/cucca/mm/RCS/more.c,v 2.6 95/03/15 11:39:56 melissa Exp $";
#endif

/* 
 * more.c - routines for displaying messages (the 'type' command, etc),
 * by piping them through "more" if appropriate
 *
 */

#include "mm.h"
#include "parse.h"

char *malloc ();
FILE *mm_popen ();

#define H_END	0
#define H_SKIP	1
#define H_OK	2

/*
 * bad_header - succeeds if header should be skipped according to
 * how user has specified only-type-headers and dont-type-headers.
 */

int
bad_header (line, only, dont)
char *line;
char **only;
char **dont;
{
    char *p;

    if (only != (char **) NULL) {
	while (p = *only++)
	    if (hdrcmp (p, line))
		return false;
	return true;
    }
    if (dont != (char **) NULL) {
	while (p = *dont++)
	    if (hdrcmp (p, line))
		return true;
    }
    return false;
}

/*
 * span - find the length of the leading substring of s1 containing
 * only those characters from set s2
 */

int
span (s1, s2)
char *s1, *s2;
{
    char c;
    char *p;
    int n = 0;

    while (c = *s1++) {
	p = s2;
	while (*p && c != *p)
	    p++;
	if (*p == 0)
	    break;
	n++;
    }
    return n;
}

int
logical_lines (s, max)
char *s;
int max;
{
    int n, pos;

    for (n = 0, pos = 0; *s; s++)	/* count newlines, long lines */
	if (*s == '\n' || ++pos > cmcsb._cmcmx) {
	    n++, pos = 0;
	    if (n == max) return n;	/* stop if no point in continuing */
	}
    if (pos) n++;			/* count partial last line */
    return n;
}

char *
fmt_message (m, only, dont)
message *m;
char **only, **dont;
{
    char *buf, *cur;
    char *p = m->text;
    int left = m->size;
    int context = H_OK;
    int linelength;
    char c;

    buf = malloc (m->size + 4);
    if (buf == NULL) {
	fprintf (stderr, "fmt_message: out of memory\n");
	return(NULL);
    }

    cur = buf;
    while (left > 0 && context != H_END) {
	if (*p == '\n' || *p == '\0')
	    context = H_END;
	else
	    context = (bad_header (p, only, dont) ? H_SKIP : H_OK);
	linelength = skipheader (p) - p;
	switch (context) {
	  case H_OK:			/* normal header */
	    strncpy (cur, p, linelength);
	    cur += linelength;
	    /* fall through */
	  case H_SKIP:			/* header user doesn't want */
	    p += linelength;
	    left -= linelength;
	    break;
	  case H_END:			/* end of headers or error */
	    break;
	}
    }

    /* copy out the rest of the message */
    while (left > 0 && (c = *p++)) {
	*cur++ = c;
	--left;
    }

    if (cur > buf && (cur[-1] != '\n'))
	*cur++ = '\n';			/* drop in terminating newline */

    *cur = 0;				/* null-terminate the message */

    if (left != 0)
	fprintf (stderr, "fmt_message: expected %d chars, had %d left over\n",
		 m->size, left);
    return buf;
}

display_message (out, m, maybeclear, only, dont)
FILE *out;
message *m;
int maybeclear;
char **only, **dont;
{
    char *msg;
    FILE *fp;
    FILE *more_pipe_open();
    extern int use_crt_filter_always;
    int c;
    msg = fmt_message (m, only, dont);
    if (msg == NULL) return 0;
    
    if (maybeclear && clear_screen && (out == stdout))
	blank ();

#ifdef METAMAIL
    if (!nontext(msg) || (fp = mm_popen ("metamail -m MM -p -R", "w")) == NULL)
#endif

    if (use_crt_filter_always ||
	(logical_lines (msg, cmcsb._cmrmx) +1 >= cmcsb._cmrmx)) {
	fp = more_pipe_open(out);
    }
    else
	fp = out;			/* not long, just write it */

    c = (int) (m - cf->msgs);
    fprintf (fp, " Message %d (%d chars)\n", c, m->size);
#ifdef sun_stdio_bug
    fwrite (msg, sizeof(char), strlen(msg), fp);
#else
    fputs (msg, fp);
#endif /* sun_stdio_bug */
    free (msg);
    if (fp == out) {	 		/* not a pipe */
	fflush (fp);
	return(false);
    }
    more_pipe_close(fp);		/* really a pipe */
    return(true);
}


/* for more_pipe_open and _close */
static signalhandler (*old_sigpipe)();
static int pipe_broke;
static signalhandler (*sigint)(), (*sigtstp)(), (*sigquit)();

signalhandler
on_sigpipe ()
{
    pipe_broke = 1;
    signal (SIGPIPE, on_sigpipe);
}

/*
 * more_pipe_open:
 * try to open the pipe to more our message, return either the pipe FILE *
 * or the FILE * we were given
 */
FILE *
more_pipe_open (out)
FILE *out;
{
    FILE *fp;
    int n;

    if ((out == stdout) &&
	(isatty (fileno (out))) &&
	(cmcsb._cmrmx > 0) &&
	(crt_filter[0] != 0)) {
	fflush (stdout);
	fflush (stderr);
	cmtend ();			/* shut off ccmd's control of tty */
	if (fp = mm_popen (crt_filter, "w")) {
	    pipe_broke = 0;
	    old_sigpipe = signal (SIGPIPE, on_sigpipe);
	    fflush (stdout);
	    fflush (stderr);
#ifdef SIGTSTP
	    sigtstp = signal (SIGTSTP, SIG_DFL), 
#endif
	    sigint = signal (SIGINT, SIG_IGN),
	    sigquit = signal (SIGQUIT, SIG_IGN);
	    return (fp);
	}
	else {
	    cmtset ();
	    cmcsb._cmwrp = autowrap_column; /* gets reset */
	    return (out);		/* can't open pipe, send to stdout */
	}
    }
    return(out);			/* use what they gave us */
}

/*
 * more_pipe_close:
 * clean up and close this pipe
 */
more_pipe_close(fp)
FILE *fp;
{
    signal (SIGPIPE, SIG_IGN);
    mm_pclose (fp);
    signal (SIGPIPE, old_sigpipe);
    signal (SIGINT, sigint),
    signal (SIGQUIT, sigquit);
#ifdef SIGTSTP
    signal (SIGTSTP, sigtstp);
#endif
    fflush(stdout);
    cmtset ();				/* setup tty for ccmd again */
    cmcsb._cmwrp = autowrap_column;	/* gets reset */
    return;
}
    
#ifdef METAMAIL
nontext(msg) 
char *msg;
{
    char *s, *ct=NULL, *ctend;
    int LineStart=1;

    for (s=msg; *s; ++s) {
        if (LineStart) {
            if (*s == '\n') break; /* end of headers */
            LineStart = 0;
            if (!lc2strncmp(s, "content-type:", 13)) {
                ct=s + 13;
                break; /* found it */
            }
        } else if (*s == '\n') {
            LineStart = 1;
        }
    }
    if (!ct) return 0;
    ctend = ct;
    while (1) {
        ctend = index(ct, '\n');
        if (!ctend) break;/* use whole thing */
        if (*(ctend+1) == ' ' || *(ctend+1) == '\t') {
            ++ctend; /* keep looking */
        } else {
            break;
        }
    }
    *ctend = NULL;
    if (notplain(ct)) {
        *ctend = '\n';
        return(1);
    }
    *ctend = '\n';
    return(0);
}

notplain(s)
char *s;
{
    char *t;
    if (!s) return(1);
    while (*s && isspace(*s)) ++s;
    for(t=s; *t; ++t) if (isupper(*t)) *t = tolower(*t);
    while (t > s && isspace(*--t)) {;}
    if (((t-s) == 3) && !strncmp(s, "text", 4)) return(0);
    if (strncmp(s, "text/plain", 10)) return(1);
    t = (char *) index(s, ';');
    while (t) {
        ++t;
        while (*t && isspace(*t)) ++t;
        if (!strncmp(t, "charset", 7)) {
            s = (char *) index(t, '=');
            if (s) {
                ++s;
                while (*s && isspace(*s)) ++s;
                if (!strncmp(s, "us-ascii", 8)) return(0);
            }
            return(1);
        }
        t = (char *) index(t, ';');
    }
    return(0); /* no charset, was text/plain */
}

lc2strncmp(s1, s2, len)
char *s1, *s2;
int len;
{
    if (!s1 || !s2) return (-1);
    while (*s1 && *s2 && len > 0) {
	if (*s1 != *s2 && (tolower(*s1) != *s2)) return(-1);
	++s1; ++s2; --len;
    }
    if (len <= 0) return(0);
    return((*s1 == *s2) ? 0 : -1);
}
#endif
