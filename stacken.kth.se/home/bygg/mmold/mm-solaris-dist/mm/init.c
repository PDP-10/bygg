/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/init.c,v 2.5 90/10/04 18:24:32 melissa Exp $";
#endif

/*
 * init.c - various initialization procedures for mm
 */

#include "mm.h"
#include "parse.h"
#include "set.h"
#include "cmds.h"

/*
 * <pwd.h> has already been included, but it may not have declared getpwnam()
 */
struct passwd *getpwnam();
/*
 * Ditto for <grp.h> and getgrgid()
 */
struct group *getgrgid();

char *HOME, *real_personal_name;
int PID;
int UID;
int OLD_UMASK;				/* umask at time mm started */
int thisyear;
extern int minutes_west;		/* see dates.c */

extern int top_level_parser();
extern variable set_variables[];
extern int set_variable_count;


/*
 * clear all modified flags on variables and aliases
 */
clear_modified()
{
    int i;
    for (i = 0; i < set_variable_count; i++) {
	set_variables[i].changed = FALSE;
    }
    set_variables[SET_MAIL_ALIASES].changed = TRUE; /* ??? */
    for (i = 0; i < mail_aliases.count; i++)
	mail_aliases.aliases[i].type = MA_SYSTEM;
}


int
system_init ()
{
    int err;

    err = take_file (SYSINIT, top_level_parser, FALSE);
    return err;
}

group_init ()
{
    struct group *g;
    struct passwd *p;
    char *u;
    char fname[BUFSIZ];
	
    u = whoami();
    if (u) {
	p = getpwnam(u);
	if (p) {
	    g = getgrgid(getgid());
	    if (g) {
#ifdef GROUP_INIT_FORMAT
		sprintf (fname, GROUP_INIT_FORMAT, g->gr_name);
#else
		sprintf(fname,"%s/%s.ini", LIBDIR, g->gr_name);
#endif
		take_file(fname, top_level_parser, FALSE);
	    }
	}
    }
}

int
user_init ()
{
  buffer filename;

  struct stat filestat;
  extern int read_fast_init_file();
  extern int write_fast_init_file();
  int set_def_parser();

  if (HOME == NULL)		/* Can't do much without a home */
    return false;

  if (read_fast_init_file()) {	/* try to read it */
    cmcsb._cmwrp = autowrap_column;	/* make sure this takes effect */
    OLD_UMASK = umask (~(new_file_mode&04777));
    if (set_variables[SET_USER_NAME].changed) { /* validate username */
      struct passwd *p1;
      p1 = getpwnam(user_name);
      if (p1 == NULL || p1->pw_uid != getuid()) {
	char *u, *whoami();
	fprintf (stderr, 
		 "Value for user_name must have the same uid as you\n");
	u = whoami();
	if (u)
	  strcpy (user_name, u);
	else
	  user_name[0] = '\0';
      }
      sethome(user_name);
    }
    return true;			/* worked */
  }
  else {		/* fast init file failed, so parse and then write it */
    /* umask might not get set, and besides, we want the old mask */
    OLD_UMASK = umask (~(new_file_mode&04777));
    sprintf(filename, "%s/.mminit", HOME);
    if (take_file (filename, set_def_parser, FALSE)) {
      if ((fast_init_file == SET_YES) ||
	  ((fast_init_file == SET_ASK) &&
	   (yesno ("Make new fast init file? ", "yes"))))
	return write_fast_init_file();
	else
	  return true;		/* Don't write fast if init says not to */
    }
    else
      return false;		/* Couldn't parse it */
  }
}

int
user_rc ()
{
    buffer filename;

    if (HOME == NULL)
	return false;
    sprintf (filename, "%s/.mmrc", HOME);
    return(take_file (filename, top_level_parser, FALSE));
}

/*
 * initialize:
 * set up various variables we want to know about
 * default some mm-variables to things we have to figure out
 */

int
initialize (argv)
char **argv;
{
    char *getenv();
    char *username,*whoami();
    long t;
#if BSD
    struct timeval tv;
    struct timezone tz;
#endif
#if SYSV
    extern long timezone;
    struct tm *tm;
#endif
#ifdef UUCP				/* add uucp extension */
    char *hn;
    static char *uext = ".uucp";
#endif /* UUCP */
    char *getlocalhostname();
    char **split_args();
    char *get_default_temp_dir();

    PID = getpid();
    UID = getuid();

    getlocalhostname();

    username = whoami();		/* get the user name */
    if (username)
	strcpy (user_name, username);	/* keep it */
    
    sethome (user_name);		/* set global HOME */

    strcpy (temp_directory, get_default_temp_dir());

    progname = rindex (argv[0], '/');
    progname = (progname == nil) ? argv[0] : ++progname;

    time (&t);				/* get current time */
    thisyear = (localtime (&t))->tm_year; /* XXX note that this gets stale */
#if BSD
    gettimeofday(&tv, &tz);
    minutes_west = tz.tz_minuteswest;
#endif
#if SYSV
    tm = localtime(&t);
    asctime(tm);
    minutes_west = timezone / 60;
#endif

#ifdef SPELLER
    speller = split_args (SPELLER);
#endif /* SPELLER */
    autowrap_column = cmcsb._cmwrp;
}


/*
 * GET_PERSONAL_NAME:
 * Given a gecos string from a struct passwd, get the personal name,
 * into malloced space.
 * A typical gecos field looks something like:
 * Personal Name,Office,Extension,Phone
 */

char *
get_personal_name (s,user)
char *s;
char *user;
{
  char *cp, *p, *index();
  int quote;
  int size;

  size = strlen(s) * 2 + 2 + 1;		/* worst case */
  cp = (char *) malloc (size);
  if ((p = index (s, ',')) != NULL)	/* find comma (should be after name) */
    *p = '\0';				/* replace comma with NULL */

  for (p = s; *p; p++)
      if (!(isatom (*p) || isspace (*p)))
	  break;

  quote = *p;				/* must quote if didn't reach end */
  if (quote || (index(s,'&') != NULL)) { /* found a bad character */
      p = cp;				/* point at space */
      if (quote)
	  *p++ = '"';
      while (*s) {
	  if (*s == '\\' || *s == '"')
	      *p++ = '\\';
	  if (*s == '&') {		/* means put in username */
	      *p = '\0';		/* so we can find spot after realloc */
	      cp = (char *) realloc(cp, size+=strlen(user));
	      p = cp+strlen(cp);	/* get to end again */
	      *p++ = (islower(user[0])) ? toupper(user[0]) : user[0];
	      strcpy (p,&user[1]);	/* add the rest */
	      p += strlen(p);
	      s++;			/* skip the & */
	  }
	  else				/* put in everything but & */
	      *p++ = *s++;
      }
      if (quote)
	  *p++ = '"';
      *p = 0;
  }
  else
      strcpy (cp, s);			/* copy personal name */
  cp = (char *) realloc (cp, strlen(cp)+1); /* snug it down */
  return (cp);				/* return pointer to it */
}

/*
 * sethome:
 * set various things for the (new) user-name
 */
sethome (name)
char *name;
{
    struct passwd *pswd;

    pswd = getpwnam (name);		/* try to get it */

    if (real_personal_name != NULL)
	free (real_personal_name);	/* getting a new one */
    if (pswd != NULL)
	real_personal_name = get_personal_name (pswd->pw_gecos, name);
    else {
	real_personal_name = NULL;
	fprintf (stderr, "Warning: can't find personal name for %s\n", name);
    }

    if (((HOME = getenv ("HOME")) == NULL) || /* in environment? */
	(HOME[0] == '\0')) {
	if (pswd != NULL) {		/* in passwd file? */
	    HOME = (char *) malloc (strlen (pswd->pw_dir)+1);
	    strcpy (HOME, pswd->pw_dir);
	}
	else {
	    fprintf (stderr, "Warning: can't find home directory for %s\n",
		     name);
	}
    }
}

/*
 * This routine is used to read commands from the system and user init files
 *
 * The argument specifies the name of the file to open.
 */

int
take_file (s, take_parser, echo)
char *s;
int (*take_parser)();
int echo;
{
    int err, noecho, saved_flag;
    FILE *fd;

    fd = fopen (s, "r");
    if (fd == NULL)
	return false;

    saved_flag = cmcsb._cmflg;
    cmcsb._cmflg &= ~CM_ITTY;            /* non-interactive */
    stack_input (fd);
    if (!echo)
	cmcsb._cmoj = (FILE *) NULL;	/* pop_input will restore this */
    noecho = cmcsb._cmflg & CM_NEC;
    cmecho(echo);
    err = take_parser ();
    if (cmcsb._cmerr == CMxEOF)
	cmcsb._cmerr = CMxOK;
    else
	fprintf (stderr, "%s: unexpected error in %s\n",
		 progname, (s == nil) ? "input file" : s);
    pop_input ();
    cmcsb._cmcol = 0;			/* XXX bald assumption */
    cmecho (noecho ? false : true);
    cmcsb._cmflg = (cmcsb._cmflg & ~CM_ITTY) | (saved_flag & CM_ITTY);
    return true;			/* propagate error */
}

/*
 * set_def_parser:
 * parse "set" and "define" commands (from .mminit file)
 */
int
set_def_parser ()
{
    static keywrd set_def_keys[] = {
	{ "set", 0, (keyval) CMD_SET },	/* "set" more common, put first */
	{ "define", 0, (keyval) CMD_DEFINE },
    };
    static keytab set_def_keytab = 
        { sizeof (set_def_keys) / sizeof (keywrd), set_def_keys };
    static fdb set_def_fdb = 
        { _CMKEY, 0, nil, (pdat) &set_def_keytab, "command, " };

    cmcsb._cmerr = CMxOK;		/* assume no error */
    while (true) {
	if (cmseteof ())
	    return CMxEOF;
	cmseter ();			/* errors return here */

	prompt (top_level_prompt);	/* in case someone's watching */
	cmsetrp ();			/* reparse comes back here */
	parse (&set_def_fdb, &pv, &used); /* parse set or define */
	(void) (*mm_cmds[pv._pvkey]) (pv._pvkey); /* same routines */
    }
}


/*
 * get default for temp_directory variable.  Used in initialization, and
 * when the user has unset the variable.
 */

char *
get_default_temp_dir()
{
    if (HOME != NULL)
	return(HOME);
    else
	return ("/tmp");
}
