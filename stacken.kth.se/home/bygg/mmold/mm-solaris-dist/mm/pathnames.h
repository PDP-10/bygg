/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifdef RCSID
#ifndef lint
static char *pathnames_rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/pathnames.h,v 2.1 90/10/04 18:25:29 melissa Exp $";
#endif
#endif /* RCSID */

/*
 * Provide default values for any pathnames not defined in config.h.
 *
 * This file is meant primarily as documentation, not something that
 * should be changed to match site-specific needs (use config.h for that).
 */

#ifndef TMPDIR
#define TMPDIR		"/usr/tmp"
#endif

#ifndef SPOOL_DIRECTORY
#define SPOOL_DIRECTORY	"/usr/spool/mail"
#endif

#ifndef EDITOR
#define EDITOR		"emacs"
#endif
#ifndef PAGER
#define PAGER		"more"
#endif
#ifndef SPELLER
#define SPELLER		"ispell"
#endif

#ifndef BUGSTO
#define BUGSTO		"bug-mm"
#endif

#ifndef LIBDIR
#define LIBDIR		"/usr/local/lib/mm"
#endif
#ifndef SYSINIT
#define SYSINIT		"/usr/local/lib/mm/mm.conf"
#endif
#ifndef HELPFILE
#define HELPFILE	"/usr/local/lib/mm/mm.help"
#endif
#ifndef HELPDIR
#define HELPDIR		"/usr/local/lib/mm/help"
#endif
#ifndef MMAIL_PATH
#define MMAIL_PATH	"/usr/local/lib/mm/mmail.el"
#endif
#ifndef MOVEMAIL
#define MOVEMAIL	"/usr/local/lib/mm/movemail"
#endif
#ifdef USAGE
#ifndef USAGEFILE
#define USAGEFILE	"/usr/local/lib/mm/usage.log"
#endif
#endif
