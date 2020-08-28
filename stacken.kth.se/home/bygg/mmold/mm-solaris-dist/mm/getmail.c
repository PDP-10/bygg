/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#include "config.h"
#include "osfiles.h"
#include "compat.h"

char *progname;
int locked = 0;
char lockname[MAXPATHLEN];

main (argc, argv)
int argc;
char *argv[];
{
    char *from, *to, *cp;
    char destdir[MAXPATHLEN];
    struct stat st;
    int c, ifd, ofd;
    time_t mtime;
    char *rindex ();

    if (argc != 3) {
	fprintf (stderr, "usage: %s source-file destination-file\n", argv[0]);
	exit (1);
    }

    progname = (cp = rindex (argv[0], '/')) ? cp + 1 : argv[0];

    from = argv[1];
    to = argv[2];

    if (cp = rindex (to, '/')) {
	strcpy (destdir, to);
	destdir[cp - to] = 0;
    }
    else
	strcpy (destdir, ".");

    if (access (from, R_OK) != 0)
	fatal ("cannot access", from);

    if (access (to, F_OK) == 0) {
	fprintf (stderr, "%s: will not overwrite %s\n", progname, to);
	exit (1);
    }

    if (access (destdir, W_OK|X_OK) != 0)
	fatal ("cannot write", destdir);

#ifdef MAIL_USE_FLOCK
    if ((ifd = open (from, O_RDONLY)) < 0)
	fatal ("cannot open", to);

    if (flock (ifd, LOCK_EX|LOCK_NB) != 0)
	fatal ("cannot lock", from);
#else
    sprintf (lockname, "%s.lock", from);
    if (creat (lockname, 0) < 0)
	fatal ("cannot lock", from);
    locked = 1;
    if ((ifd = open (from, O_RDONLY)) < 0)
	fatal ("cannot open", to);
#endif

    if (stat (from, &st) < 0)
	fatal ("cannot stat", from);
    mtime = st.st_mtime;

    if ((ofd = open (to, O_WRONLY|O_CREAT|O_EXCL, 0600)) < 0)
	fatal ("cannot open", to);

    for (;;) {
	char buf[BUFSIZ];
	c = read (ifd, buf, sizeof buf);
	if (c == 0)
	    break;
	if (c < 0)
	    fatal ("error reading", from);
	if (write (ofd, buf, c) < c)
	    fatal ("error writing", to);
    }

    if (close (ofd) < 0)
	fatal ("error closing", to);
    
    if (stat (from, &st) < 0)
	fatal ("cannot stat", from);

    if (st.st_mtime != mtime)
	fprintf (stderr, "%s: %s changed!\n", progname, from);

    /*
     * Try to unlink the source file.  If that
     * fails, just truncate it.
     */
    if (unlink (from) < 0) {
	if (stat (from, F_OK) == 0)
	    if ((c = creat (from, 0660)) < 0)
		fatal ("cannot truncate", from);
	    else
		(void) close (c);
    }
    done (0);

}

fatal (s1, s2)
char *s1, *s2;
{
    if (s2) {
	int saved_errno = errno;
	fprintf (stderr, "%s: %s ", progname, s1);
	errno = saved_errno;
	perror (s2);
    }
    else
	fprintf (stderr, "%s: %s\n", progname, s1);
    done (1);
}

done (n)
int n;
{
#ifndef MAIL_USE_FLOCK
    if (locked)
	unlink (lockname);
#endif
    exit (n);
}
