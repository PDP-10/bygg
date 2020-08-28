/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifdef RCSID
#ifndef lint
static char *osfiles_rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/osfiles.h,v 2.1 90/10/04 18:25:19 melissa Exp $";
#endif
#endif /* RCSID */

/*
 * This file pulls in most of the UNIX system header files needed by MM.
 * Include it after config.h, and before compat.h.
 */

#ifndef stdin
#include <stdio.h>
#endif
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef NEED_IOCTL
#include <sys/ioctl.h>
#endif

#ifdef NEED_FCNTL
#include <fcntl.h>
#endif

#ifdef NEED_UNISTD
#include <unistd.h>
#endif

#ifdef NEED_SYSFILE
#include <sys/file.h>
#endif

#ifdef NEED_SYSTIME
#include <sys/time.h>
#endif

#ifdef NEED_TIME
#include <time.h>
#endif

#ifdef NEED_WAIT
#include <sys/wait.h>
#endif

#ifdef NEED_VFORK
#include <vfork.h>
#endif

#include <signal.h>
