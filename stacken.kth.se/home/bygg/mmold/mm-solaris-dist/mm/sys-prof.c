/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/sys-prof.c,v 2.1 90/10/04 18:26:50 melissa Exp $";
#endif

/*
 * sys-profile:
 * like mm's profile command, but this writes the system-wide mminit file
 */

#include "mm.h"
#include "ccmd.h"
#include "set.h"			/* so we can set things */
#include "version.h"

#define comment cmcsb._cmntb		/* comment character */
#define tempfilename "mm.conf"
#define Prompt(x) {cmseter(); prompt(x); cmsetrp();}

int cmdbuf[BUFLEN];			/* ccmd work buffers */
static buffer wrkbuf;
buffer atmbuf, sprbuf;
extern char cmcont;			/* line continuation character */
pval pv;				/* parse value */
fdb *used;				/* which fdb was used to parse field */

/* break mask for an email address */
static brktab rembrk = {		/* break table for remote user name */
    {					/* letters, $ only in first position */
      0xff, 0xff, 0xff, 0xff, 0xf7, 0xef, 0xff, 0xff,
      0x80, 0x00, 0x00, 0x1e, 0x80, 0x00, 0x00, 0x1f
    },
    {					/* letters, digits and hyphens here */
      0xff, 0xff, 0xff, 0xff, 0xb2, 0xe9, 0x00, 0x3f,
      0x00, 0x00, 0x00, 0x1e, 0x80, 0x00, 0x00, 0x1f
    }					/* (also @+%_.!) */
};
static fdb remarks = { _CMFLD, CM_SDH, NULL, NULL, "local bug-report address",
			   NULL, &rembrk };
static fdb number = { _CMNUM, CM_SDH, NULL, (pdat) 10,
			  "number from above list" };
extern keytab formattab;
extern char *defmailfile[];
static fdb formatfdb = { _CMKEY, 0 , NULL, (pdat)&formattab, 
			     "default format, ", NULL, NULL };


char *bugmmcomment="bug-mm is a local consultant who handles most mm problems";

char *paths[] = {
    "bug-mm@columbia.edu", 
    "rutgers!columbia!bug-mm",
    "bug-mm%columbia@cuvma.bitnet"
 /* "cunixc::bug-mm", */
    };
numpaths = sizeof(paths) / sizeof(char *);

static keywrd yesnokeys[] = {
    { "always", 0, SET_YES },		/* 0 */
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
fdb yesno_fdb = { _CMKEY, 0, NULL, (pdat) &yesnotab, "keyword, "};

/*
 * here it is, here's main!!!
 */
main(argc,argv)
int argc;
char *argv[];
{
    FILE *tmpfile;
    int i, gnu, format;
    int noremarks = TRUE;		/* no local remarks address */
    char *progname;

    init();
    if ((tmpfile = fopen (tempfilename, "w")) == NULL) {
	fprintf (stderr, "Can't open temp file for composing %s\n", SYSINIT);
	cmdone();
	exit();
    }

    /* get name without path */
    if ((progname = rindex (argv[0], '/')) == NULL)
	progname = argv[0];
    else
	progname++;

    printf("\n\
This is the %s program, which will help you set up a\n\
site-wide initialization file for MM.  It asks you a series of\n\
questions and then makes MM remember them (via the\n\
%s file).\n\
\n\
You will not be bound to these choices -- the init file is plain text\n\
and can be easily modified with your favorite editor.  Every time a\n\
user on your system starts up MM, this environment will be read in.\n\
\n\
Everything set in this initialization file can be changed by\n\
individual users to suit their own needs.  MM provides various means\n\
to help them write their own init file.  The variables that they are\n\
unlikely to want to change will be invisible, but still valid.\n\
\n\
We will not go through all of the settable variables, but will attempt\n\
to set those options that seem most important in a site-wide\n\
initialization file.  To set the fancier options, you can use the SET\n\
command along with CREATE-INIT, then merge the resulting personal init\n\
file with %s, or just use an editor on the\n\
sysem-wide initialization file.  The HELP SET <option-name> command,\n\
or the SHOW command will document each particular option, e.g.\n\
\"help set default-mail-type\".\n\n",
	   progname, SYSINIT, SYSINIT);    
    hold();

    fprintf (tmpfile, "\
%s System-wide initialization file for %s\n\
%s note that all lines beginning with \"%s\" are comments\n",
	     comment, MM_VERSION, comment, comment);

    printf ("\
MM supports various different mail formats.  To find out about them,\n\
type \"help ?\" inside MM, and try the various keywords listed as\n\
formats.  Please pick one to be the default for your site.  Briefly,\n\
\"mtxt\" format is the fastest, but is not compatible with any other\n\
UNIX mail programs, while \"mbox\" is compatible with the Berkeley\n\
mail(1) program provided with UNIX.  Hit a question mark to see all\n\
the options.\n\n");

    fprintf (tmpfile, "\n%s default mail format\n", comment);
    Prompt ("Default mail format for your site: ");
    parse(&formatfdb, &pv, &used);
    strcpy  (sprbuf, atmbuf);
    format = pv._pvkey;
    confirm();
    fprintf (tmpfile, "set default-mail-type %s\n", sprbuf);

    printf ("The default name for that kind of mail file is %s.\n",
	    defmailfile[format]);
    fprintf (tmpfile, "%s default mail file\n", comment);
    Prompt ("Shall I set MM to look for that? ");
    parse (&yesno_fdb, &pv, &used);
    confirm();
    if (pv._pvint)
	fprintf (tmpfile, "set mail-file %s\n", defmailfile[format]);
    else {
	printf ("\
Okay, I'll need to know where you do want it -- I'll parse for a file\n\
in your directory, but remember the file as \"~/filename\" so it will\n\
work for all users.\n");
	Prompt ("File for storing users' mail, relative to home dir: ");
	parse_file ("mail file", "~/");
/* ************* make sure they didn't type /cu/sy/melissa/mailfile */
	if (atmbuf[0] == '~')
	    strcpy (sprbuf, &atmbuf[2]); /* past ~/ */
	else
	    strcpy (sprbuf, atmbuf);
	confirm();
	fprintf (tmpfile, "set mail-file ~/%s\n", sprbuf);
    }

    printf ("\
\n\
Because MM is always being improved upon, we invite users to report\n\
any bugs they notice, or send us any suggestions they might have,\n\
using MM's \"bug\" command.  However, we have found that many \"bugs\" are\n\
just misunderstandings, and many suggestions have already been\n\
implemented (perhaps under a different name than the user expected).\n\
In addition, we get many bug-reports that should have been sent to\n\
Postmaster (or Mailman, or the local equivalent).\n\
\n\
In order to cut down on the level of network traffic generated, we\n\
would like to have a local systems person filter the bug-mm mail and\n\
send us only those things which seem appropriate.  On many systems,\n\
there is already a \"consultant\" or \"remarks\" address for this sort of\n\
mail.\n\n");

    Prompt ("Will you be able to provide such a person? ");
    parse (&yesno_fdb, &pv, &used);
    confirm();
    if (pv._pvint == SET_YES) {
	noremarks = FALSE;		/* they DO have one! */
	fprintf(tmpfile, "\n%s %s\n", comment, bugmmcomment);
	printf ("\n\
What is the email address of that user?  Please note that you can\n\
later change this address by modifying the line saying\n\
\"define bug-mm ...\" in the init file.\n\n"); 
	Prompt("email address? ");
	parse(&remarks, &pv, &used);
	strcpy  (sprbuf, atmbuf);
	confirm();
	printf ("Thank you.\n");
	fprintf (tmpfile, "define bug-mm %s\n", sprbuf);
    }
    
    printf ("\
\n\
Next we need to know the path for sending bugs to mm's authors.  This\n\
will be defined as \"mm-authors\" in the init file.  This is where the\n\
user receiving bug-mm mail should forward non-local problems and\n\
suggestions.\n\
\n\
Here are several possibilities for the path.  If you do not know which\n\
will work, just pick the one that looks most reasonable.  You can\n\
always change it later, since all the possibilities will be in the\n\
init file, with all but one commented out.\n\n");
    fprintf (tmpfile, "\n%s possible paths for the mm authors\n", comment);

    for (i = 0; i < numpaths; i++)	/* show choices */
	printf ("%3d\) %s\n", i+1, paths[i]);

    do {
	printf ("\n\
Pick the number from the above list corresponding to the address you\n\
think will work.\n");
	Prompt("Which address: ");
	parse(&number, &pv, &used);
    } while ((pv._pvint <= 0) || (pv._pvint > numpaths));

    for (i = 0; i < numpaths; i++) {
	if (i != pv._pvint-1)		/* comment out unselected ones */
	    fprintf (tmpfile, "%s ", comment);
	fprintf (tmpfile, "define mm-authors %s\n", paths[i]);
    }
    if (noremarks) {
	fprintf (tmpfile, "\n%s %s\n", comment, bugmmcomment);
	fprintf (tmpfile, "%s (No local bug address)\n", comment);
	fprintf (tmpfile, "define bug-mm mm-authors\n");
    }

    printf("\n\
MM forks up an editor for composing outgoing mail and editing received\n\
mail.  It works well with editors such as emacs which support multiple\n\
buffers, and comes with a module to support GNU Emacs (available from\n\
the Free Software Foundation, 1000 Mass Ave, Cambridge, MA 02138, and\n\
distributed with many versions of Unix).  The \"editor\" variable can be\n\
set by any user, but you should set a default.\n\n");

    fprintf (tmpfile, "\n%s path of default editor\n", comment);
    Prompt ("Absolute path of default editor: ");
    parse_file("default editor", "/");
    strcpy  (sprbuf, atmbuf);
    confirm();
    fprintf (tmpfile, "set editor %s\n", sprbuf);

    putchar ('\n');
    Prompt ("Is that GNU Emacs? ");
    parse (&yesno_fdb, &pv, &used);
    confirm();
    gnu = pv._pvint;
    fprintf (tmpfile, "%s is editor GNU Emacs\n", comment);
    fprintf (tmpfile, "set gnuemacs-mmail %s\n",
	     gnu == SET_YES ? "yes" : "no");

    printf ("\
\n\
MM will run a spell-checker on messages, and needs the path of a\n\
program that will read from a file and write corrections back to that\n\
same file.  ispell will do this, if you have it.\n\
\n");

    fprintf (tmpfile, "\n%s path for spell program\n", comment);
    Prompt ("Absolute path of spell program: ");
    parse_file("spell checker", "/");
    strcpy  (sprbuf, atmbuf);
    confirm();
    fprintf (tmpfile, "set speller %s\n", sprbuf);


#ifdef undef
    cmsystem("/bin/mv -i /tmp/mminit SYSINIT");
#endif /* undef */
    printf ("\nDone\n");
    cmdone();
    exit(0);
}

/*
 * init:
 * initialize some stuff
 */
init()
{
    cmcont = '\\';			/* lines are continued with \ */
    cmbufs (cmdbuf, sizeof cmdbuf, atmbuf, sizeof atmbuf,
	    wrkbuf, sizeof wrkbuf);	/* set up buffers for ccmd */

    cmseti (stdin, stdout, stderr);	/* set up file descriptors */
    cmcsb._cmntb = "#";			/* comments start with # */
}


fdb textfdb = { _CMTXT, CM_SDH, NULL, NULL, "confirm to continue" };
fdb cfmfdb = { _CMCFM, CM_SDH };

/*
 * hold:
 * wait for carriage return
 */
hold() {
    Prompt ("Hit return to continue ");
    parse (fdbchn(&textfdb, &cfmfdb, NULL), &pv, &used);
    putchar('\n');
}

/*
 * parse_file:
 * parse for a filename, but don't insist on its existing
 */
parse_file(hlp, dir)
char *hlp, *dir;
{
    static fdb filefdb = {
	_CMFIL, FIL_NODIR|FIL_PO, NULL, NULL, NULL, NULL, NULL};
    filblk fb;
    static char *dirs[] = { nil , nil }; /* start them at root */

    dirs[0] = dir;
    fb.pathv = dirs;
    fb.exceptionspec = nil;
    fb.def_extension = nil;
    filefdb._cmdat = (pdat) &fb;
    filefdb._cmhlp = hlp;

    parse (&filefdb, &pv, &used);
}
