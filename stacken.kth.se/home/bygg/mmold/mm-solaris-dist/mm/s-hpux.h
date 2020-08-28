/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

/*
 * s-hpux.h - configuration file for HP-UX
 *
 * Some attempt has been made to conditionalize this for both the
 * 6.01 version running on the Series 300, and whatever is running
 * on the Series 800 machines.  See "hp9000s800" conditionals below.
 */

#ifdef RCSID
#ifndef lint
static char *s_hpux_rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/s-hpux.h,v 2.1 90/10/04 18:25:54 melissa Exp $";
#endif /* lint */
#endif /* RCSID */

/*
 * The HOSTNAME macro is used only as a last resort, if no other way
 * of determining the hostname will work.  If you have a domain name,
 * i.e. there are dots in the official name of your host, include only
 * the first component of the name.
 */
/* #define HOSTNAME "ourhost"		/* local hostname */

/*
 * If you have an official domain name for your host, and the domain name
 * is not discernible with gethostbyname(3), define LOCALDOMAIN
 * to be the name of your domain, preceded by a "."; otherwise, define
 * LOCALDOMAIN to be a null string.  For example, if the name of your host
 * is podunk.edu, you should specify:
 *	#define LOCALDOMAIN ".edu"
 * If your host is vax.cs.podunk.edu, you'd use
 *	#define LOCALDOMAIN ".cs.podunk.edu"
 */
/* #define LOCALDOMAIN ".ourdomain.edu"	/* local domain */

/*
 * If your site likes to make outgoing mail appear as if it all comes
 * from one central host, define HIDDENNET as the name of that host.
 * Otherwise, leave HIDDENNET undefined.
 */
/* #define HIDDENNET "mailhost.ourdomain.edu"	/* fake mail host name */

/*
 * If your operating system supports filenames longer than 14 characters,
 * I.e. you are running a Berkeley UNIX derivative, define HAVE_FLEXFILENAMES.
 * If you are running some sort of operating system which supports
 * long filenames only on remote filesystems (e.g. HP-UX 6.0), don't
 * define it.
 */
#if hp9000s800
#define HAVE_FLEXFILENAMES		/* we have long filenames */
#endif

/*
 * If you have the Berkeley sendmail program, define SENDMAIL to be the
 * pathname by which it should be invoked.
 */
#define SENDMAIL "/usr/lib/sendmail"	/* we have sendmail */

/*
 * Define SPOOL_DIRECTORY to be the name of the directory to which
 * new mail is delivered.  On System V-based systems, this is usually
 * /usr/mail; on Berkeley systems, it's usually /usr/spool/mail.
 */
#define SPOOL_DIRECTORY "/usr/mail"

/*
 * If your mailer supports .forward files, and you want MM to display
 * the contents of .forward file (you may not wish to do this if it's
 * likely that a user's .forward file will be ignored because of conflicting
 * entries in /usr/lib/aliases), define FORWARD_FILE as the name of the
 * file in the user's home directory.
 */
#define FORWARD_FILE ".forward"		/* we use .forward files */

/*
 * If you have GNU Emacs, define GNUEMACS to be the name with which it
 * is usually referred by your users.  If GNUEMACS is not defined, special
 * code used to interface MM with GNU Emacs will not be compiled into MM.
 */
#define GNUEMACS "emacs"		/* we have GNU Emacs */

/*
 * If you have, and use, the Sun Yellow Pages facility, define
 * HAVE_YP.
 */
#define HAVE_YP				/* we use yp */

/*
 * If you have NFS and users will be referencing mail files on remote
 * filesystems, define HAVE_NFS.
 */
#define HAVE_NFS			/* we use NFS */

/*
 * Define one or more of NEED_FCNTL, NEED_UNISTD, and NEED_SYSFILE
 * as appropriate, depending on whether or not MM has to include
 * <fcntl.h>, <unistd.h>, and/or <sys/file.h> in order to obtain
 * the symbol definitions used in the open(2), fcntl(2), flock(2),
 * lockf(2), and lseek(2) system calls.
 */
#define NEED_FCNTL			/* include <fcntl.h> */
#define NEED_UNISTD			/* include <unistd.h> */
#undef NEED_SYSFILE

/*
 * You will probably need this, but you may want to undefine it if
 * another system header file includes it automatically.
 */
#define NEED_IOCTL			/* include <sys/ioctl.h> */

/*
 * MM needs to refer to definitions which may appear in either
 * <time.h> or <sys/time.h>, depending on what kind of system you have.
 * <time.h> usually contains the definition of the "tm" struct, while
 * on many BSD-based systems, <sys/time.h> will include the same information
 * in addition to the "timeval" and "timezone" structs.  Define one or
 * the other, or both, as appropriate.
 */
#undef NEED_SYSTIME			/* include <sys/time.h> */
#define NEED_TIME			/* don't include <time.h> */

/*
 * Define NEED_WAIT if your system defines "union wait" in <sys/wait.h>
 * See also HAVE_WAIT3 below.
 */
#if hp9000s800
#define NEED_WAIT			/* include <sys/wait.h> */
#endif

/*
 * Define HAVE_WAIT3 if your system has the wait3() system call.
 */
#if hp9000s800
#define HAVE_WAIT3			/* we have wait3() */
#endif

/*
 * Define HAVE_JOBS if your system has job control.  Currently 4.2 job control
 * is the only type supported.
 */
#if hp9000s800
#define HAVE_JOBS			/* we have job control */
#endif

/*
 * Define HAVE_BSD_SIGNALS if you have the 4.2BSD-style signal handling
 * facilities.
 *
 * Some HP-UX systems don't support this, others support it only if MM
 * is lunk with -lBSD.  I don't know what is true for the 6.2 or 2.0
 * releases.
 */
#define HAVE_BSD_SIGNALS		/* we have 4.2 signals */

/*
 * If you have support for disk quotas, and you use them, define HAVE_QUOTAS.
 * Otherwise leave it undefined.
 */
#undef HAVE_QUOTAS			/* we don't have quotas */

/*
 * If you defined HAVE_QUOTAS above, define HAVE_QUOTACTL if your system
 * has quotactl(2) system call.  If this is defined, MM also expects to
 * find quota.h in <ufs/quota.h> rather than <sys/quota.h>.
 */
#undef HAVE_QUOTACTL			/* we don't have quotactl() */

/*
 * Define HAVE_INDEX if your system has index(3) and rindex(3) rather than
 * than strchr(3) and strrchr(3).
 */
#undef HAVE_INDEX			/* we don't have index & rindex */

/*
 * Define HAVE_BSTRING if you have bcopy(3), bzero(3), and bcmp(3).  These
 * are usually present on BSD-based systems, and missing on older systems
 * where MM will use memcpy(3), memset(3), and memcmp(3).
 */
#undef HAVE_BSTRING			/* we don't have bcopy and friends */

/*
 * Define HAVE_VFORK if your system has the vfork(2) system call.
 */
#define HAVE_VFORK			/* we have vfork() */

/*
 * Define HAVE_GETWD if you have getwd(2).  Most BSD-based systems should.
 */
#undef HAVE_GETWD			/* we don't have the bsd getwd() */

/*
 * Define HAVE_GETCWD if you have getcwd(3) rather than getwd(2).  This will
 * be true on most SYSV-based systems.
 */
#define HAVE_GETCWD			/* we don't have getcwd() */

/*
 * Define HAVE_GETHOSTNAME if you have the gethostname(2) system call, and
 * HAVE_GETHOSTBYNAME if you have get gethostbyxxxx(3) library routines.
 * This is generally true on BSD-based systems, and some SYSV systems.
 * They should be undefined if you either don't have them, or don't use
 * them (because they're broken?).
 */
#define	HAVE_GETHOSTNAME		/* we have gethostname() */
#define	HAVE_GETHOSTBYNAME		/* we have gethostbyname() */

/*
 * Define HAVE_UNAME if you don't have the gethostname(2) system call,
 * but you do have uname(2).  This is usually true on non-networked
 * SYSV systems.
 */
#undef HAVE_UNAME			/* we have uname() but don't need it */

/*
 * Define PHOSTNAME if you have neither gethostname(2) or uname(2).
 * It's value should be a string containing a UNIX command which will
 * print your system's hostname.
 */
/* #define PHOSTNAME "uuname -l"	/* program that prints our hostname */

/*
 * Define HAVE_RENAME if you have the rename(2) system call, or an equivalent
 * C library routine.  This is true on later BSD releases and SVR3.
 */
#define HAVE_RENAME			/* we have rename() */

/*
 * Define one or more of the following symbols, depending on what file-
 * locking facilities are available on your system.
 * 
 * MM prefers the fcntl interface if you've got it, but you should probably
 * avoid telling MM you have the fcntl interface or lockf() if you've got
 * flock and you use NFS, unless you really know what you're doing.
 */
#define HAVE_F_SETLK			/* we have fcntl(fd, F_SETLK, ...) */
#define HAVE_LOCKF			/* lockf works too */
#undef HAVE_FLOCK			/* we don't have flock */

/*
 * If your /bin/mail program uses the flock(2) system call to prevent
 * simultaneous access to files in /usr/mail or /usr/spool/mail, define
 * MAIL_USE_FLOCK.  If this is not defined, the movemail program will
 * attempt to lock files in the spool directory using lock files.
 *
 * If you're not sure whether or not to define this symbol, you might
 * be able to find the answer by consulting the sources for /bin/mail
 * on your system, or looking in the appropriate s-*.h file from the
 * GNU Emacs distribution.  If you have the strings(1) program, you
 * can try "strings /usr/spool/mail" to see if any ".lock" files exist
 * there.
 */
#undef MAIL_USE_FLOCK			/* hp-ux binmail uses .lock files */

/*
 * Define HAVE_BSD_SETPGRP if your system's setpgrp() system call takes two
 * arguments.  This is generally true on later BSD releases with job control.
 * On some systems, there is both a SYSV setpgrp() call and a setpgrp2() call,
 * the latter of which takes two arguments like the BSD setpgrp() call.  If
 * that's true on your system, you should probably also add
 *	#define setpgrp setpgrp2
 * unless your system somehow figures this out automatically.
 */
#ifdef hp9000s800
#define SETPGRP2			/* setpgrp() takes two args */
#define setpgrp setpgrp2
#endif

/*
 * Define HAVE_VOIDSIG if your <signal.h> defines signal(2) as
 * void (*signal())(). This seems to be true in System V Release 3
 * and SunOS 4.0.
 */
#undef HAVE_VOIDSIG			/* void (*signal())() */

/*
 * Define NEED_VFORK if we need to include <vfork.h>.
 */

#if sparc
#define NEED_VFORK
#endif
/*
 * define volatile as static if your C compiler does not support the 
 * "volatile" directive (ANSI C).
 */

#define volatile static
