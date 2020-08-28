/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/parse.c,v 2.2 90/10/04 18:25:21 melissa Exp $";
#endif

/*
 * parse.c - miscellaneous parsing functions
 */

#include "mm.h"
#include "parse.h"
#include <varargs.h>

/*
 * miscellaneous function descriptor blocks
 */ 

static brktab shell_brk = {
    {
	0xff, 0xff, 0xff, 0xff, 0xbf, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    },
    {
	0xff, 0xff, 0xff, 0xff, 0xbf, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    }
};

fdb shell_fdb = {
    _CMTOK, CM_SDH|CM_NLH|TOK_WAK, nil, 
    (pdat) "!", "\"!\" for shell escape", NULL, &shell_brk
};

fdb cfm_fdb = { _CMCFM, CM_SDH, nil, nil, "confirm with carriage return" };


static brktab fldbrk = {
    {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x3f,
	0x80, 0x00, 0x00, 0x1f, 0x80, 0x00, 0x00, 0x1f,
    },
    {
	0xff, 0xff, 0xff, 0xff, 0xfb, 0xfb, 0x00, 0x1f,
	0x00, 0x00, 0x00, 0x1f, 0x80, 0x00, 0x00, 0x1f,
    },
};

/* lend this to all the other modules */
fdb aliasfdb = { _CMKEY, KEY_EMO, nil, nil, "mail alias, ", NULL, &fldbrk };


/* 
 * This routine is dangerous in that it's use may cause memory leaks; it
 * should not be used where it might orphan chunks of memory allocated
 * with malloc (the same is true of any call to a parsing routine).
 */
void
cmerr(va_alist)
va_dcl
{
    char *fmt;
    va_list arg_ptr;

    if ((cmcsb._cmflg & CM_TTY) == 0) {
	va_start(arg_ptr);
	fmt = va_arg(arg_ptr, char *);
	fprintf(cmcsb._cmej, "%s: ", progname);
	vfprintf(cmcsb._cmej,fmt,arg_ptr);
	va_end(arg_ptr);
	cmerjnp(0);		       /* reprompt N.B. argument is ignored */
    }
    else {
	va_start(arg_ptr);
	fmt = va_arg(arg_ptr, char *);
	cmflush(cmcsb._cmij);
	if (cmcpos() != 0)
	    cmnl(cmcsb._cmoj);
	cmcsb._cmcol = 0;
	fputc('?', cmcsb._cmej);
	vfprintf(cmcsb._cmej,fmt,arg_ptr);
	fputc('\n', cmcsb._cmej);
	va_end(arg_ptr);
#ifdef undef
	cmnl(cmcsb._cmoj);		/* tie it off with a newline */
#endif
	cmerjnp(0);			/* reprompt N.B. argument is ignored */
    }
}

/*
 * sorry is identical to cmerr, except that it doesn't unwind.
 */
int
sorry (va_alist)
va_dcl
{
    char *fmt;
    va_list arg_ptr;

    if ((cmcsb._cmflg & CM_TTY) == 0) {
	va_start(arg_ptr);
	fmt = va_arg(arg_ptr, char *);
	fprintf(cmcsb._cmej, "%s: ", progname);
	vfprintf(cmcsb._cmej,fmt,arg_ptr);
	va_end(arg_ptr);
    }
    else {
	va_start(arg_ptr);
	fmt = va_arg(arg_ptr, char *);
	cmflush(cmcsb._cmij);
	if (cmcpos() != 0)
	    cmnl(cmcsb._cmoj);
	cmcsb._cmcol = 0;
	fputc('?', cmcsb._cmej);
	vfprintf(cmcsb._cmej,fmt,arg_ptr);
	fputc('\n', cmcsb._cmej);
	va_end(arg_ptr);
    }
    return false;
}


time_t
datimetogmt (d)
datime *d;
{
    time_t gmt;
    struct tm t;

    t.tm_sec = d->_dtsec;
    t.tm_min = d->_dtmin;
    t.tm_hour = d->_dthr;
    t.tm_mday = d->_dtday + 1;		/* sad but true */
    t.tm_mon = d->_dtmon;
    t.tm_year = d->_dtyr - 1900;
    t.tm_wday = d->_dtdow;
    t.tm_yday = 0;			/* XXX ccmd doesn't support this */
    t.tm_isdst = 0;			/* fake this */
    gmt = itime (&t);			/* get almost-gmt */
    gmt -= (minutes_west - d->_dttz) * 60;
    /* dtdst contains DST adjustment in minutes (like "-60") */
    gmt += d->_dtdst * 60;
    return gmt;
}

#define T_SUNDAY -7
#define T_MONDAY -6
#define T_TUESDAY -5
#define T_WEDNESDAY -4
#define T_THURSDAY -3
#define T_FRIDAY -2
#define T_SATURDAY -1

keywrd timekeys[] = {
    { "friday", 0, (keyval) T_FRIDAY },
    { "monday", 0, (keyval) T_MONDAY },
#define T_NEVER 0
    { "never", 0, T_NEVER },
#define T_NOW 1
    { "now", 0, (keyval) T_NOW },
    { "sunday", 0, (keyval) T_SUNDAY },
    { "saturday", 0, (keyval) T_SATURDAY },
    { "thursday", 0, (keyval) T_THURSDAY },
#define T_TODAY 2
    { "today", 0, (keyval) T_TODAY },
#define T_TOMORROW 3
    { "tomorrow", 0, (keyval) T_TOMORROW },
    { "tuesday", 0, (keyval) T_TUESDAY },
    { "wednesday", 0, (keyval) T_WEDNESDAY },
#define T_YESTERDAY 4
    { "yesterday", 0, (keyval) T_YESTERDAY }
};

keytab timetab = { sizeof (timekeys) / sizeof (keywrd), timekeys };

time_t
p_date(help)
char *help;
{
    static fdb tadfdb = { _CMTAD };
    static fdb keytimefdb = { _CMKEY, 0, NULL, NULL, "relative date, " };
    tadfdb._cmhlp = (help ? help : "date");
    keytimefdb._cmdat = (pdat) &timetab;
    parse(fdbchn(&tadfdb, &keytimefdb, NULL), &pv, &used);
    if (used == &tadfdb)
	return (datimetogmt(&pv._pvtad));

    return key2time(pv._pvint);
}

time_t
key2time(n)
int n;
{

    time_t tx;
    struct tm *t;

    if (n == T_NEVER)
	return 0;

    time(&tx);

    if (n == T_NOW)
	return tx;

    if (n < 0) {
	t = localtime(&tx);
	/* reduce tx by some number of days */
	tx -= ((t->tm_wday + 7 - n) % 7) * (60*60*24);
	/* reduce tx by seconds since midnight */
	tx -= t->tm_sec + (t->tm_min * 60) + (t->tm_hour * (60*60));
	return tx;
    }

    switch (n) {
      case T_TOMORROW:
	tx += 60*60*24;
	goto cvt;
      case T_YESTERDAY:
	tx -= 60*60*24;
	goto cvt;
      case T_TODAY:
cvt:	t = localtime(&tx);
	t->tm_sec = 1;
	t->tm_min = 0;
	t->tm_hour = 0;
	return (itime(t));
      default:
	cmerr("Unknown time keyword (%d) parsed", n);
    }
}

int
p_num(help)
char *help;
{
    static fdb numfdb = { _CMNUM, CM_SDH, 0, (pdat) 10 };

    numfdb._cmhlp = (help ? help : "number");
    parse(&numfdb, &pv, &used);
    return pv._pvint;
}


/* confirmit - prompt for and parse crlf */

void
confirmit(promptstring)
char *promptstring;
{
    volatile int doprev = false;

    if (promptstring == (char *) 0)
	promptstring = "[Confirm]";

    cmseteof();
    if (cmseter ()) {			/* errors return here */
	if (cmcsb._cmerr == CMxEOF) {
	    return;
	}
	else
	    doprev = true;
    }
    
    prompt(promptstring);
    if (doprev) {
	doprev = false;
	prevcmd();
    }
    cmsetrp();				/* back here on reparse */
    confirm();				/* go for confirmation */
}

static keywrd yesnokeys[] = {
    { "always", 0, SET_ALWAYS },	/* 0 */
    { "false", 0, SET_NO },		/* 1 */
    { "never", 0, SET_NO },		/* 2 */
    { "no", 0, SET_NO },		/* 3 */
    { "ok", 0, SET_YES },		/* 4 */
    { "true", 0, SET_YES },		/* 5 */
    { "yes", 0, SET_YES },		/* 6 */
    { "0", KEY_INV, SET_NO },		/* 7 */
    { "1", KEY_INV, SET_YES },		/* 8 */
    { "n", KEY_ABR|KEY_INV, (keyval) 3 } /* this means "no" */
};

keytab yesnotab = { sizeof(yesnokeys) / sizeof(keywrd), yesnokeys };
fdb yesno_fdb = { _CMKEY, 0, NULL, (pdat) &yesnotab, "keyword, " };

static keywrd yesnoask_keys[] = {
    { "always", 0, SET_ALWAYS },
    { "ask", 0, SET_ASK },
    { "never", 0, SET_NEVER },
    { "no", 0, SET_NEVER },
    { "yes", 0, SET_ALWAYS },
    { "n", KEY_ABR|KEY_INV, (keyval) 3 },
};

keytab yesnoasktab = { sizeof(yesnoask_keys)/sizeof(keywrd), yesnoask_keys };
fdb yesnoask_fdb = { _CMKEY, 0, NULL, (pdat) &yesnoasktab, "keyword, " };

int
yesno(promptstring, def)
char *promptstring, *def;
{
    int n;
    volatile int doprev = false;

    save_parse_context();		/* save reparse/error handlers */
    if (promptstring == NULL)
	promptstring = "Yes or no? ";

    cmseteof();				/* errors come back here */
    if (cmseter ()) {			/* errors return here */
	if (cmcsb._cmerr == CMxEOF)
	    panic ("Unexpected EOF in yesno()");
	else
	    doprev = true;
    }
    prompt(promptstring);
    if (doprev) {
	doprev = false;
	prevcmd();
    }
    cmsetrp();				/* come back here on reparse */
    n = parse_yesno (def);
    restore_parse_context();		/* restore error/reparse handlers */
    return (int) n;			/* return the answer */
}

int
parse_yesno (def)
char *def;
{
    yesno_fdb._cmdef = def;
    parse (&yesno_fdb, &pv, &used);
    confirm();
    return pv._pvint;
}

int
parse_yesnoask (def)
char *def;
{
    yesnoask_fdb._cmdef = def;
    parse (&yesnoask_fdb, &pv, &used);
    confirm();
    return pv._pvint;
}

/*
 * parse_text:
 * parse some text, ending with CR
 * NOTE that nothing is malloced so nothing should be freed!
 */
char *
parse_text (help, def)
char *help, *def;
{
    static fdb text_fdb = { _CMTXT, CM_SDH };

    text_fdb._cmhlp = (help != nil) ? help : "string";
    text_fdb._cmdef = def;
    parse (&text_fdb, &pv, &used);
    return (char *) atmbuf;
}

char *
parse_prompt (help)
char *help;
{
    static fdb qstr_fdb = { _CMQST, CM_SDH };
    static fdb text_fdb = { _CMTXT, CM_SDH };

    qstr_fdb._cmhlp = (help != nil) ? help : "quoted prompt";
    parse (fdbchn(&qstr_fdb, &text_fdb, NULL), &pv, &used);
    if (used == &qstr_fdb)
	confirm();
    return (char *) atmbuf;
}

char *
parse_quoted (help, def)
char *help, *def;
{
    static fdb qstr_fdb = { _CMQST, CM_SDH };

    qstr_fdb._cmhlp = (help != nil) ? help : "string";
    qstr_fdb._cmdef = def;
    parse (&qstr_fdb, &pv, &used);
    return (char *) atmbuf;
}

char *
parse_username (help, def)
char *help, *def;
{
    static fdb uname_fdb = { _CMUSR, CM_SDH };

    uname_fdb._cmhlp = (help != nil) ? help : "Username";
    uname_fdb._cmdef = def;
    parse (&uname_fdb, &pv, &used);
    return (pv._pvusr[0]->pw_name);
}

void
brkch(p, s, s2)
brktab *p;
unsigned char *s, *s2;
{
    unsigned char c;

    /* process characters which are break characters anywhere in the field */
    while ((c = *s++) && (c < 128)) {
	p->_br1st[c/8] |= (0x80 >> (c % 8));
	p->_brrest[c/8] |= (0x80 >> (c % 8));
    }
    /* process characters which break only if they're the first character */
    if (s2) while ((c = *s2++) && (c < 128))
	p->_br1st[c/8] |= (0x80 >> (c % 8));
}

void
unbrk(p,s)
brktab *p;
unsigned char *s;
{
    unsigned char c;

    while ((c = *s++) && (c < 128)) {
	p->_br1st[c/8] &= ~(0x80 >> (c % 8));
	p->_brrest[c/8] &= ~(0x80 >> (c % 8));
    }
}

p_file(deflt, input)
char *deflt;
{
    static fdb filefdb = {
	_CMFIL, FIL_PO|FIL_VAL, NULL, NULL, NULL, NULL, NULL};
    fdb *f = &filefdb;

    f->_cmhlp = (input ? "input filename" : "output filename");
    f->_cmdef = deflt;
    parse(f, &pv, &used);
    strncpy(atmbuf, pv._pvfil[0], sizeof(atmbuf) -1);
    if (input) {
	if (access(atmbuf, F_OK|R_OK) < 0)
	    cmerr(errstr(-1));
    }
    else {
	int ok;
	char c = 0;
	char *p = rindex(atmbuf, '/');

	if (p == NULL) p = atmbuf;	/* XXX depends on ("" == ".") */
	c = *p; *p = 0;			/* consider only the directory */
	ok = access(atmbuf, F_OK|W_OK);	/* can we write on it? */

	*p = c;				/* fix the path again */
	if (ok < 0)
	    cmerr(errstr(-1));		/* nope, complain */
    }
}

#define IOSTACKLIMIT 16
struct io_fileset {
    FILE *i;
    FILE *o;
    FILE *e;
} io_stack[IOSTACKLIMIT];
int io_stackp = -1;

void
stack_input(fd)
FILE *fd;
{
    if ((io_stackp + 1) < IOSTACKLIMIT) {
	io_stackp++;
	io_stack[io_stackp].i = cmcsb._cmij;
	io_stack[io_stackp].o = cmcsb._cmoj;
	io_stack[io_stackp].e = cmcsb._cmej;
	cmseti(fd, cmcsb._cmoj, cmcsb._cmej);
    }
    else cmerr("Command input stack overflow");
}

int
pop_input()
{
    if (io_stackp < 0)
	panic ("input file stack is empty in pop_input");
    if (cmcsb._cmij != io_stack[io_stackp].i)
	(void) fclose(cmcsb._cmij);
    cmseti(io_stack[io_stackp].i,io_stack[io_stackp].o,io_stack[io_stackp].e);
    cmcsb._cmwrp = autowrap_column;	/* put that back */
    io_stackp--;
    return CMxEOF;
}

/*
 * this routine is a replacement for the ccmd-supplied error handler
 */

jmp_buf eofjmpb;
static int errchars = 0;

prevcmd()
{
  int *cp;				/* pointer into command buffer */
  int count,i;				/* number of chars to reinstate */

  count = cmcsb._cmhst - cmcsb._cmbfp - errchars; /* count buffered chars */
  cp = cmcsb._cmbfp;			/* point to beginning of buffer */
  for (i = 0; i < count; i++) {
      if ((cp[i] & CC_CHR) == '\n') {
	  count = i;
	  break;
      }
  }
  while(count > 0) {
      if (cp[count-1] == (CC_HID|CC_NEC|'\ '))
	  count--;
      else
	  break;
  }



  cmcsb._cminc = count;			/* this many chars now to parse */
  cmcsb._cmcnt -= count;		/* count their presence */
  cmcsb._cmcur = cmcsb._cmptr + cmcsb._cminc;
  cmcsb._cmflg |= CM_DRT;		/* now the buffer is dirty */
  while (count-- > 0) 			/* step through the buffer */
    if (((*cp) & CC_NEC) == 0)		/* originally echoed? */
      cmechx((char) (*cp++) & CC_CHR);	/* then echo it now */
    else
      cp++;				/* else just move on */
  return(CMxOK);
}


int
ccmd_error(err,str,flags)
int err;
char *str;
int flags;
{
  if (err == CMxEOF)			/* handle EOF specially */
    longjmp(eofjmpb,err);
  errchars = cmcsb._cminc;
  if (str == NULL)
      cmperr(err,flags);
  else
      cmpemsg(str,flags);
  longjmp(cmerjb,err);
}


int
ccmd_errmsg(str)
int str;
{
  errchars = cmcsb._cminc;
  cmpemsg(str,0);
  longjmp(cmerjb,CMxNOP);
}

int
ccmd_errnp(err)
int err;
{
  if (err == CMxEOF)			/* handle EOF specially */
    longjmp(eofjmpb,err);
  errchars = cmcsb._cminc;
  longjmp(cmerjb,err);
}


int
parse_number (radix, help, def)
int radix;
char *help, *def;
{
    static fdb numfdb = { _CMNUM, CM_SDH, 0, (pdat) 10 };
    numfdb._cmhlp = (help == nil) ? help : "number";
    numfdb._cmdat = (pdat) radix;
    numfdb._cmdef = def;
    parse (&numfdb, &pv, &used);
    return pv._pvint;
}

/*
 * Parse a list of keywords, returning a (char **) pointing to the list.
 *
 * This routine recursively parses keywords in a comma-separated list until
 * a confirm is found or a parse error occurs; after successfully parsing the
 * list and a confirm, the bottom-most invocation allocates enough space to
 * hold the list of pointers and the strings themselves, which are stored
 * into the allocated buffer by each level as recursion unwinds.  The
 * cumulative byte and string count are passed downwards so the bottom-most
 * level knows how much space to allocate; it in turn passes the size of the
 * list back in dest[0] so the higher levels know where the pointers end and
 * the string space begins.
 *
 * Admittedly one of the ugliest routines I ever wrote.
 */

char **
parse_keylist (count, nbytes, help)
int count, nbytes; char *help;
{
    int keylen;
    keyword key;
    char **dest;
    static fdb key_cfm_fdb = { _CMCFM, CM_SDH, nil, nil,
				   "confirm to delete list" };
    static fdb key_fdb = { _CMFLD, CM_SDH|FLD_EMPTY };
    static fdb comma_fdb = { _CMTOK, CM_SDH, nil, (pdat) ",",
			 "comma and another keyword" };

    key_fdb._cmhlp = ((help == nil) ? "keyword" : help);

    if (count == 0) {
	parse (fdbchn(&key_cfm_fdb, &key_fdb, nil), &pv, &used);
	if (used == &key_cfm_fdb)
	    return nil;
	if (strlen (atmbuf) == 0)
	    cmerr ("Invalid null %s", help);
    }
    else {
	parse (&key_fdb, &pv, &used);
	if  (atmbuf[0] == 0)
	    cmerr ("Invalid %s after comma", help);
    }
    if ((keylen = strlen (atmbuf)) >= sizeof (key))
	cmerr ("String too long for %s buffer: %s", key_fdb._cmhlp, atmbuf);

    count++;				/* account for this string */
    nbytes += keylen + 1;
    strcpy (key, atmbuf);		/* save it on the stack */

    parse (fdbchn (&comma_fdb, &cfm_fdb, nil), &pv, &used);
    if (used == &comma_fdb) {
	/* parsed a comma, so call ourself recursively for more */
	dest = parse_keylist (count, nbytes, help);
    }
    else {
	dest = (char **) malloc (((count+1) * sizeof (char *)) + nbytes);
	dest[0] = (char *) count;	/* pass string count back up */
	dest[count] = nil;		/* mark the end of the list */
    }

    dest[count-1] = ((char *) dest) +
	((((int) dest[0]) + 1) * sizeof (char *)) + nbytes - (keylen + 1);
    strcpy (dest[count-1], key);
    return dest;
}

char **
parse_filelist (count, nbytes, help, del)
int count, nbytes; char *help;
{
    int filelen;
    buffer file;
    char **dest;
    static fdb file_cfm_fdb = { _CMCFM, CM_SDH, nil, nil, nil };
    static fdb file_fdb = { _CMFIL, FIL_PO|FIL_VAL };
    static fdb comma_fdb = { _CMTOK, CM_SDH, nil, (pdat) ",",
			 "comma and another filename" };

    file_fdb._cmhlp = ((help == nil) ? "filename" : help);
    file_cfm_fdb._cmhlp = (count == 0 && del) ?
	"confirm to delete the list" : "confirm with carriage return";

    if (directory_folders)
	file_fdb._cmffl &= ~FIL_NODIR;
    else
	file_fdb._cmffl |= FIL_NODIR;
    if (count > 0)
	parse (fdbchn(&file_cfm_fdb, &comma_fdb, &file_fdb, nil), &pv, &used);
    else
	parse (fdbchn(&file_cfm_fdb, &file_fdb, nil), &pv, &used);
    if (used == &file_cfm_fdb)
	return nil;
    if (used == &comma_fdb)
	return(parse_filelist (count, nbytes, help, del));
    if (strlen (pv._pvfil[0]) == 0)
	cmerr ("Invalid null %s", help);
    if ((filelen = strlen (pv._pvfil[0])) >= sizeof (file))
	cmerr ("String too long for %s buffer: %s", file_fdb._cmhlp, atmbuf);

    count++;				/* account for this string */
    nbytes += filelen + 1;
    strcpy (file, pv._pvfil[0]);		/* save it on the stack */

    if (!(dest = parse_filelist (count, nbytes, help, del))) {
	dest = (char **) malloc (((count+1) * sizeof (char *)) + nbytes);
	dest[0] = (char *) count;	/* pass string count back up */
	dest[count] = nil;		/* mark the end of the list */
    }

    dest[count-1] = ((char *) dest) +
	((((int) dest[0]) + 1) * sizeof (char *)) + nbytes - (filelen + 1);
    strcpy (dest[count-1], file);
    return dest;
}

char *parse_in_out_file();

/*
 * parse the name of a file.  returns the absolute pathname.
 * Note: caller must free storage.
 */


char *
parse_input_file (help, def, allowdir)
char *help, *def;
{
    static fdb filefdb = { _CMFIL, FIL_OLD|FIL_RD };

    if (allowdir && directory_folders)
	filefdb._cmffl &= ~FIL_NODIR;
    else
	filefdb._cmffl |= FIL_NODIR;
    return (parse_in_out_file (help, def, filefdb));
}

/*
 * parse the name of a file.  returns the absolute pathname.
 * Note: caller must free storage.
 */

char *
parse_output_file (help, def, allowdir)
char *help, *def;
{
    static fdb filefdb = { _CMFIL, FIL_PO|FIL_VAL };

    if (allowdir && directory_folders)
	filefdb._cmffl &= ~FIL_NODIR;
    else
	filefdb._cmffl |= FIL_NODIR;
    return (parse_in_out_file (help, def, filefdb));
}


/*
 * parse_in_out_file:
 * do the work for parse_input_file or parse_output_file
 */
char *
parse_in_out_file (help, def, filefdb)
char *help, *def;
fdb filefdb;
{
    char wd[MAXPATHLEN];
    char *getwd();
    char *fname, *index();
    char *cp;
    filblk fb;
    char *dirs[3];

    /* don't list files twice, also if mail_directory is unset, it means "." */
    if (mail_directory[0] == '\0' || same_file(mail_directory, ".")) {
	dirs[0] = ".";
	dirs[1] = nil;
    }
    else {
	dirs[0] = mail_directory;
	dirs[1] = ".";
    }
    dirs[2] = nil;
    
    fb.pathv = dirs;
    fb.exceptionspec = nil;
    fb.def_extension = nil;
    filefdb._cmdat = (pdat) &fb;
    filefdb._cmhlp = help;
    filefdb._cmdef = def;

    parse (&filefdb, &pv, &used);
    if (*pv._pvfil[0] == '/') {		/* absolute path specified */
      fname = (char *) malloc (strlen(pv._pvfil[0])+1);
      strcpy (fname, pv._pvfil[0]);
      return (fname);
    }

    /* 
     * file does not exist in mail-directory (mail-directory is an
     * absolute path), so see if it is in .
     * in which case use it, otherwise, try to default to new file
     * in mail-directory
     * ****  See also, similar code in move.c: parse_copyfile ****
     */
    if ((index(pv._pvfil[0], '/') != NULL) || /* directory specified */
	access (pv._pvfil[0], F_OK) == 0 || /* file exists in . */
	mail_directory[0] == '\0') {	/* or no mail_directory, use wd */
      if (getwd (wd) == NULL) {		/* got some kind of error */
	fprintf (stderr, "%s\n", wd);	/* print the error message */
	fname = (char *) malloc (strlen(pv._pvfil[0])+1);
	strcpy (fname, pv._pvfil[0]);
	return (fname);
      }
      cp = pv._pvfil[0];
      if (cp[0] == '.' && cp[1] == '/')
	cp += 2;
      fname = (char *) malloc (strlen(cp)+strlen(wd)+2);
      sprintf (fname, "%s/%s", wd, cp);
      return (fname);
    }

    /* else, use mail_directory */

    fname = (char *) malloc (strlen(mail_directory)+strlen(pv._pvfil[0])+2);
    sprintf (fname, "%s/%s", mail_directory, pv._pvfil[0]);
    return (fname);
}

/*
 * parse a directory name.
 * the caller is responsible for freeing the string returned.
 */

char *
parse_directory (help, def)
char *help, *def;
{
    int len;
    char cwd[MAXPATHLEN], *cp, *pp;
    static fdb dirfdb = { _CMFIL, FIL_DIR, NULL, NULL, 
			      NULL, NULL, NULL };
    static fdb cfmfdb = { _CMCFM, CM_SDH, NULL, NULL, "confirm to unset", 
			      NULL, NULL };

    dirfdb._cmhlp = help ? help : "directory name";
    dirfdb._cmdef = def;
    parse (fdbchn(&dirfdb,&cfmfdb,NULL), &pv, &used);

    if (used == &cfmfdb) {		/* unset, set to empty string */
	return ("");			/* since caller does a strcpy */
    }

    confirm();

    cwd[0] = 0;
    pp = pv._pvfil[0];
    if (*pv._pvfil[0] != '/') {
	if (pp[0] == '.' && pp[1] == '/')
	    pp += 2;
	if (getwd (cwd) == NULL) {
	    fprintf (stderr, "%s\n", cwd);
	    cwd[0] = 0;
	}
    }

    len = cwd[0] ? strlen (cwd) + strlen (pp) + 1 : strlen (pp);
    cp = malloc (len+1);
    if (!cp)
	cmerr ("Out of memory");

    if (cwd[0])
	sprintf (cp, "%s/%s", cwd, pp);
    else
	strcpy (cp, pp);

    if (cp[len-1] == '/')		/* ends with / */
        cp[len-1] = '\0';
    if (cp[len-2] == '/' && cp[len-1] == '.') /* ends with /. */
	cp[len-2] = '\0';
	
    return cp;
}



/*
 * Try to use Andy's ccmd routines to parse a date string
 */

/*
 * This is currently used to parse dates out of the message separators;
 * this is probably horribly inefficient compared to a quick-and-dirty
 * parse that would accept only exactly what we expect in certain cases, but...
 */

int tadflags = 0;			/* no flags (for generality) */

time_t
stringtotime(s)
register char *s;
{
    static fdb dtfdb = { _CMTAD, 0 };
    int len;
    pval pv;
    fdb *used;
    int ret;

    if ((ret = match(s, strlen(s), &dtfdb, &pv, &used, &len)) == CMxOK)
	return(datime_to_time(&pv._pvtad));
    return(0);
}

char *
parse_keyword(kt,nothing)
keytab *kt;
int nothing;				/* is nothing okay? */
{
    pval pv;
    fdb *used;
    int i;
    char *safe_strcpy();
    static fdb keyfdb = {_CMKEY };
    static fdb cfm_fdb = {_CMCFM, CM_SDH };

    keyfdb._cmdat = (pdat) kt;
    if (nothing)
	parse(fdbchn(&keyfdb,&cfm_fdb, NULL), &pv, &used);
    else
	parse(fdbchn(&keyfdb,NULL), &pv, &used);
    if (used == &cfm_fdb)
	return(NULL);
    i = 0;
    while (kt->_ktwds[i]._kwval != pv._pvkey)
	i++;
    return(kt->_ktwds[i]._kwkwd);
}

char *
parse_field(help,def) 
char *help,*def;
{
    static fdb fldfdb = { _CMFLD, CM_SDH };
    pval pv;
    fdb *used;
    
    fldfdb._cmhlp = help;
    fldfdb._cmdef = def;
    parse(&fldfdb, &pv, &used);
    return(atmbuf);
}

keytab *
mk_alias_keys()
{
    static keywrd *aliaskeys = nil;
    static keytab aliastab;
    int i;

    if (aliaskeys != nil) {
	free(aliaskeys);
    }
    if (mail_aliases.count != 0)
	aliaskeys = (keywrd *)malloc(mail_aliases.count * sizeof(keywrd));
    else
	aliaskeys = nil;
    for(i = 0; i < mail_aliases.count; i++) {
	aliaskeys[i]._kwkwd = mail_aliases.aliases[i].name;
	aliaskeys[i]._kwflg = 0;
	aliaskeys[i]._kwval = i;
    }
    aliastab._ktcnt = mail_aliases.count;
    aliastab._ktwds = aliaskeys;
    return(&aliastab);
}

parse_alias(help,def)
char *help, *def;
{
    static fdb aliasfdb = { _CMKEY };
    pval pv;
    fdb *used;

    aliasfdb._cmhlp = help;
    aliasfdb._cmdef = def;
    aliasfdb._cmdat = (pdat) mk_alias_keys();
    parse(&aliasfdb, &pv, &used);
    return(pv._pvkey);
}
