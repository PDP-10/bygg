/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/mm.c,v 2.5 90/10/04 18:24:57 melissa Exp $";
#endif

/*
 * mm.c - portable "mail manager" program
 */


#include "mm.h"
#include "parse.h"
#include "cmds.h"
#include "set.h"


char *progname;
int mode = MM_TOP_LEVEL;
int memdebug = false;
int debug = false;
#define dmsg(msg) if (debug) { printf (msg); fflush(stdout); } else debug = 0

#define CMDBUFLEN (10*BUFLEN)
int cmdbuf[CMDBUFLEN];			/* ccmd work buffers */
static buffer wrkbuf;
buffer atmbuf;				/* parsed field */
pval pv;				/* parse value */
fdb *used;				/* which fdb was used to parse field */
extern char cmcont;			/* line continuation character */

int mail_check = false;
typedef int (*(*acttab))();
acttab mmactini();
jmp_buf abortbuf;

char **Argv;
int Argc,doargs=false;
int gotargs=false;

main(argc, argv)
int argc;
char *argv[];
{
    int err, new_mail();
    volatile int doget;
    static fdb userfdb = { _CMUSR };
    char *root = "root";
    char **split_args();
    extern variable set_variables[];
    extern setkey user_level;

#ifdef USAGEFILE
    usage_start();
#endif
    Argv = argv;
    Argc = argc;
    doargs = false;
#ifdef MDEBUG
    m_init();
#endif /* MDEBUG */

    dmsg ("initialize...");
    initialize (argv);			/* set up some variables */

    mode = MM_TOP_LEVEL;

    cmcont = '\\';			/* lines are continued with \ */
    cmbufs (cmdbuf, sizeof (cmdbuf)/sizeof(*cmdbuf), /* set up buffers  */
	    atmbuf, sizeof (atmbuf)/sizeof(*atmbuf), /*   for ccmd */
	    wrkbuf, sizeof (wrkbuf)/sizeof(*wrkbuf));

    cmact(mmactini());
    cmcsb._cmntb = "#";			/* comments start with # */

    init_signals ();			/* see signals.c */

    printf ("%s\n", mm_version);

    dmsg ("crunch...");

    cmseti(NULL, NULL, NULL);	      /* no files needed while reading those */
    cmhst(0);				/* turn off cmd history */

    dmsg ("system init...");
    system_init ();			/* take system init file */

    dmsg ("group init...");		/* take group init file */
    group_init();

    clear_modified();			/* clear modified flags on variables */

    dmsg ("mailrc...");
    mailrc();
    if (isatty(0))
	cmseti(stdin, stdout, stderr);	/* DON'T TRY TO PARSE BEFORE HERE */
    else
	cmseti(stdin, NULL, stderr);	/* pipe or file redirect. */
					/* don't echo */
    dmsg ("user init...");
    user_init ();

    if (clear_screen) {			/* var is now appropriately set */
	cmcls();
	printf ("%s\n", mm_version);
    }
    printf ("\
Please report all problems using MM's BUG command, or send mail to BUG-MM.\n\
Suggestions are also welcome.\n");

    dmsg ("user rc...");
    user_rc ();
    if (editor == NULL && !set_variables[SET_EDITOR].changed) {
	char *ed;

	if ((ed = (char *) getenv ("EDITOR")) != NULL)
	    editor = split_args (ed);
	else
	    editor = split_args(EDITOR);
    }
    cmhst(100);				/* turn on command history */
    show_route(FALSE);			/* show mail routing if routed */

    dmsg ("\n");

    doget = true;
    cmseter();
    if (doget && cf == NULL && strlen(mail_file) != 0 && auto_startup_get &&
	Argc == 1) {
	doget = false;
	printf("Reading %s ...\n", mail_file);
	cf = getfile(mail_file,CMD_GET);
	if (cf != NULL)
	    do_flagged();		/* show flagged ones */
    }
    mail_check = true;
    doargs = true;
    while (true) {
	err = top_level_parser ();
	switch (err) {
	  case CMxEOF:
	    done (0);
	  default:
	    fprintf (stderr, "%s: main: fatal parser error\n", progname, err);
	    done (1);
	}
    }
}
    
int
top_level_parser ()
{
    fdb *fdbs;
    extern string default_send_command, default_read_command;
    static int level = 0;		/* see if we are here in a "take" */
    extern int user_aborted;
    volatile int doprev;

    level++;				/* count this entry */
    cmcsb._cmerr = CMxOK;		/* assume no error */
    while (true) {
	setjmp(abortbuf);
	allow_aborts = false;
#ifdef undef
	debug_validate_msgvec("top level loop");
#endif
	if (mail_check)
	    new_mail(true);		/* check for new mail. */
	if (cmseteof ()) {
	    level--;			/* count the return */
	    return CMxEOF;
	}
	doprev = false;
	if (cmseter ()) {		/* errors return here */
	    if (cmcsb._cmerr == CMxEOF) {
		level--;
		return CMxEOF;
	    }
	    else
		doprev = true;
	}

	if (user_aborted) {
	    mode &= ~(MM_SEND|MM_REPLY|MM_ANSWER);
	    user_aborted = false;
	}

	if (doargs && level <= 1) {
	    fdbs = fdbchn (&mm_top_fdb_abbr, &mm_top_fdb_1, &mm_top_fdb_2, 
			   &mm_top_fdb_3, &mm_top_fdb_4, &mm_top_fdb_5, 
			   &mm_top_fdb_6, &mm_top_fdb_7, &mm_top_fdb_inv, 
			   &shell_fdb, nil);
	    if (cmargs(Argc,Argv)) 
		gotargs = true;
	}

	if (gotargs && doargs && level <= 1) {
	    cmsetrp();
	}
	else if (mode == MM_REPLY) {
	    cm_set_ind(TRUE);		/* allow indirections */
	    alarm(0);			/* we don't get to the parse */
	    do_reply_many();		/*   further down where alarm is */
	    continue;			/*   turned off so do it here */
	} 
	else if (mode & MM_SEND) {
	    if (!doprev)
		novice_banner(MM_SEND);
	    prompt (send_prompt);
	    cmsetrp ();			/* reparse comes back here */
	    fdbs = fdbchn (&mm_send_fdb_abbr, &mm_send_fdb_1, 
			   &mm_send_fdb_2, &mm_send_fdb_3, &mm_send_fdb_4, 
			   &mm_send_fdb_5, &mm_send_fdb_inv, 
			   &shell_fdb, nil);
	    if (default_send_command[0] == '\0')
		mm_send_fdb_abbr._cmdef = NULL;
	    else
		mm_send_fdb_abbr._cmdef = default_send_command;
	}
	else if (mode & MM_BROWSE) {
	    alarm(0);
	    browse_message();
	    continue;
	}
	else if (mode & MM_READ) {
	    if (!doprev)
		novice_banner(MM_READ);
	    prompt (read_prompt);
	    cmsetrp ();			/* reparse comes back here */
	    fdbs = fdbchn (&mm_read_fdb_abbr, &mm_read_fdb_1, 
			   &mm_read_fdb_2, &mm_read_fdb_3, &mm_read_fdb_4, 
			   &mm_read_fdb_5, &mm_read_fdb_6, &mm_read_fdb_7, 
			   &mm_read_fdb_inv, 
			   &shell_fdb, nil);
	    if (default_read_command[0] == '\0')
		mm_read_fdb_abbr._cmdef = NULL;
	    else
		mm_read_fdb_abbr._cmdef = default_read_command;
	}
	else {
	    if (gotargs && level <= 1) {
		if (cf && !update(&cf,UPD_SAVEMOD) && (cf->flags & MF_WRITERR))
		    gotargs = FALSE;	/* don't exit if write fails */
		else
		    done(0);
	    }
	    if (!doprev)
		novice_banner(MM_TOP_LEVEL);
	    prompt (top_level_prompt);
	    cmsetrp ();			/* reparse comes back here */
	    fdbs = fdbchn (&mm_top_fdb_abbr, &mm_top_fdb_1, &mm_top_fdb_2, 
			   &mm_top_fdb_3, &mm_top_fdb_4, &mm_top_fdb_5, 
			   &mm_top_fdb_6, &mm_top_fdb_7, &mm_top_fdb_inv, 
			   &shell_fdb, nil);
	}
	if (doprev) {
	    doprev = false;
	    prevcmd();
	}
	cmcsb._cmerh = ccmd_error;	/* reset in case of reparse */
	cm_set_ind(TRUE);		/* allow indirections */
	if (fdbs->_cmdef != nil)
	    cmcsb._cmflg |= CM_PRS;	/* XXX kludge! */
	doargs = false;
	parse (fdbs, &pv, &used);
	alarm(0);
	if (used == &shell_fdb)
	    (void) shell_escape ();
	else {
	    keywrd *k;

	    k = (keywrd *) pv._pvkey;
	    (void) (*mm_cmds[k->_kwval]) (k->_kwval);
	}

    }
}

/* 
 * parse an (optional) shell command and invoke the shell
 */ 

shell_escape ()
{
    static fdb cmdfdb = { _CMTXT };

    parse (&cmdfdb, &pv, &used);	/* parse a line of text */
    if (strlen (atmbuf) == 0)
	shell (nil);
    else
	shell (atmbuf);
}

cmd_debug (n)
int n;
{
    debug = parse_yesnoask("yes");
}

cmd_debug_memory (n)
int n;
{
    memdebug = parse_yesnoask("yes");
}
