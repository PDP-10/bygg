/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/compat.c,v 2.4 90/10/04 18:23:46 melissa Exp $";
#endif

#include "config.h"
#include "osfiles.h"
#include "compat.h"

/*
 * If getwd is a macro, we need to supply our own getwd() routine.
 */

#ifdef getwd
char *
getwd (buf)
char *buf;
{
#ifdef HAVE_GETCWD
    extern int sys_nerr, errno;
    extern char *sys_errlist[];

    if (getcwd (buf, MAXPATHLEN) == 0) {
	if (errno > 0 && errno <= sys_nerr)
	    strcpy (buf, sys_errlist[errno]);
	else
	    strcpy (buf, "getcwd failed");
	return (char *) 0;
    }
#else
    char *evalpipe ();

    if (evalpipe("/bin/pwd", buf, MAXPATHLEN) != buf) {
	strcpy (buf, "/bin/pwd failed");
	return 0;
    }
#endif
    return buf;
}
#endif

/*
 * If "rename" is a macro, we need to supply our own rename() routine.
 * This version is careful not to delete the source file; if your kernel
 * has a rename(2) that can unlink the source file accidentally, or may
 * succeed without effectively changing the name of the file, undefine
 * HAVE_RENAME in config.h and use this one instead.
 */
#ifdef rename
rename (from, to)
char *from, *to;
{
    struct stat s_from, s_to;

    /*
     * make sure file exists, and get the inode info
     */
    if (stat (from, &s_from) != 0)
	return -1;
    
    /*
     * short circuit attempts to link/unlink directory files
     * in case we're running as root.
     */
    if ((s_from.st_mode & S_IFMT) != S_IFREG) {
	errno = EINVAL;
	return -1;
    }

    /*
     * make sure that the source != destination, and
     * that the destination is not a directory.
     */
    if (stat (to, &s_to) == 0) {
	if (s_to.st_ino == s_from.st_ino ||
	    (s_to.st_mode & S_IFMT) != S_IFREG) {
	    errno = EINVAL;
	    return -1;
	}
	if (unlink (to) != 0)
	    return -1;
    }

    if (link (from, to) != 0)		/* make the link */
	return -1;

    if (unlink (from) != 0)		/* delete the source file */
	return -1;

    return 0;
}
#endif

/*
 * lock/unlock a file.
 */

lock_file (name, fd)
char *name;
int fd;
{
    int status;
#ifdef HAVE_F_SETLK
    struct flock flk;
    flk.l_type = F_WRLCK;
    flk.l_whence = 0;
    flk.l_start = 0;
    flk.l_len = 0;
    status = fcntl (fd, F_SETLK, &flk);
#else
#ifdef HAVE_FLOCK
    status = flock (fd, LOCK_NB|LOCK_EX);
#else
#ifdef HAVE_LOCKF
    status = lockf (fd, F_TLOCK, 0);
#else
    return (-1);			/* pretend we locked it */
#ifdef undef
    int lockfd;
    char lockfname[MAXPATHLEN];

    sprintf(lockfname, "%s.lock", name); /* XXX filename may exceed 14 chars */

    if ((lockfd = open(lockfname, O_CREAT|O_EXCL|O_WRONLY, 0)) >= 0) {
	status = 0;
	(void) close(lockfd);
    } else {
	status = -1;
	if (errno != EEXIST)
	    perror(lockfname);
    }
#endif /* undef */
#endif /* HAVE_LOCKF */
#endif /* HAVE_FLOCK */
#endif /* HAVE_F_SETLK */
    return status;
}

unlock_file (name, fd)
char *name;
int fd;
{
    int status;
#ifdef HAVE_F_SETLK
    struct flock flk;
    flk.l_type = F_UNLCK;
    flk.l_whence = 0;
    flk.l_start = 0;
    flk.l_len = 0;
    status = fcntl (fd, F_SETLK, &flk);
#else
#ifdef HAVE_FLOCK
    status = flock (fd, LOCK_UN);
#else
#ifdef HAVE_LOCKF
    status = lockf (fd, F_ULOCK, 0);
#else
    return (-1);			/* pretend success */
#ifdef undef
    char lockfname[MAXPATHLEN];

    sprintf(lockfname, "%s.lock", name);
    status = unlink(lockfname);
#endif /* undef */
#endif /* HAVE_LOCKF */
#endif /* HAVE_FLOCK */
#endif /* HAVE_F_SETLK */
    return status;
}

/* 
 * Return the "official" name of the local host.
 */

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

#ifdef HAVE_GETHOSTBYNAME
#include <netdb.h>
#endif

#ifdef HAVE_UNAME
#include <sys/utsname.h>
#endif

char shorthostname[MAXHOSTNAMELEN];
char fullhostname[MAXHOSTNAMELEN];
char *mailhostname = 0;
char *localdomain = 0;

char *
getlocalhostname ()
{
    char *cp;
    char *index ();

#ifdef HAVE_GETHOSTNAME
    gethostname (shorthostname, sizeof shorthostname - 1);
#else
#ifdef HAVE_UNAME
    struct utsname uts;
    uname (&uts);
    strcpy (shorthostname, uts.nodename);
#else
#ifdef PHOSTNAME
    char *evalpipe ();

    if (evalpipe (PHOSTNAME, shorthostname, sizeof shorthostname) == NULL)
	shorthostname[0] = 0;
#else
#ifdef HOSTNAME
    strncpy (shorthostname, HOSTNAME, MAXHOSTNAMELEN-1);
#endif
#endif
#endif
#endif
    /* 
     * See if the hostname already has a domain tacked on
     */
    if (cp = index (shorthostname, '.')) {
	strcpy (fullhostname, shorthostname);
	*cp++ = 0;
	localdomain = fullhostname + (cp - shorthostname);
    }
    else
	localdomain = 0;

    if (!localdomain) {
#ifdef HAVE_GETHOSTBYNAME
	struct hostent *hp = gethostbyname (shorthostname);
	if (hp) {
	    cp = index (hp->h_name, '.');
	    if (cp) {
		strcpy (fullhostname, hp->h_name);
		localdomain = fullhostname + (cp - hp->h_name + 1);
	    }
	}
#endif
    }

    if (!localdomain) {
#ifdef LOCALDOMAIN
	sprintf (fullhostname, "%s%s", shorthostname, LOCALDOMAIN);
	localdomain = fullhostname + strlen (shorthostname) + 1;
#else
	strcpy (fullhostname, shorthostname);
#endif
    }
#ifdef HIDDENNET
    mailhostname = HIDDENNET;
#else
    mailhostname = fullhostname;
#endif
    return fullhostname;
}

char *
evalpipe (cmd, result, maxlen)
char *cmd, *result;
int maxlen;
{
    char *cp;
    FILE *fp, *mm_popen ();
    
    if (fp = mm_popen (cmd, "r")) {
	if (fgets (result, maxlen, fp) == result) {
	    if (cp = index (result, '\n'))
		*cp = 0;
	    else
		result = 0;
	    (void) mm_pclose (fp);
	    return result;
	}
	else
	    result = NULL;
	(void) mm_pclose (fp);
    }
    return result;
}

#ifdef read
#undef read
sys_read (fd, buf, count)
int fd, count;
char *buf;
{
    int n;
    
    while ((n = read (fd, buf, count)) < 0 && errno == EINTR)
	;
    return n;
}
#endif

#ifdef write
#undef write
sys_write (fd, buf, count)
int fd, count;
char *buf;
{
    
    int n, left = count;

    for (left = count; left > 0; left -= n) {
	n = write (fd, buf, count);
	if (n < 0)
	    if (errno != EINTR)
		return n;
	    else
		n = 0;
    }

    return (count - left);
}
#endif
