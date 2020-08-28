/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/help.c,v 2.4 90/10/04 18:24:27 melissa Exp $";
#endif

/*
 * help.c:
 * get help strings from fancy database file, for MM
 */

#include "mm.h"
#include "set.h"
#include "cmds.h"
#include "parse.h"
#include "help.h"

hlp_offset *offsets = NULL;		/* indexes into file, strlens */
FILE *helpfp = NULL;
static char buf[BUFSIZ];
static time_t help_time = 0;

extern string help_file, help_dir;

char *help_subdirs[] = {		/* must appear in order defined */
    "TOP.DIR",				/* by HELP_TOP...HELP_TOPIC */
    "READ.DIR",				/* in help.h */
    "SEND.DIR",
    "TOPICS.DIR",
    "VARS.DIR",
};

#define TOPIC_DIR help_subdirs[HELP_TOPIC]



/*
 * cmd_help:
 * give help on a command, or some other topic
 * Note that most of the help strings were stolen from the DEC20 version,
 * with thanks to Mark Crispin, Mike McMahon, and friends...
 */
cmd_help(n)
int n;
{
    int help_mode;

    if (mode & MM_SEND)
	help_mode = HELP_SEND;
    else if (mode & MM_READ)
	help_mode = HELP_READ;
    else
	help_mode = HELP_TOP;

    return(cmd_help_1(n,help_mode),0);
}

cmd_help_1 (n, mode,recurse)
int n;
{
    extern keytab formattab;
    extern string default_read_command, default_send_command;

    static fdb cfmfdb = { _CMCFM,		/* confirm for general help */
			      CM_SDH|CM_NLH, NULL, NULL, 
			      "confirm for a brief help message", NULL, NULL };

    static fdb formatfdb = { _CMKEY, CM_NLH, nil, 
			       (pdat) &formattab, "format, " };

    static keywrd concept_keys[] = {
	{ "CCMD", 0, (keyval) HLP_CCMD },
	{ "command-completion", 0, (keyval) HLP_CCMD },
    	{ "message-sequence", 0 , (keyval) HLP_MESSAGE_SEQUENCE },
	{ "mm-initialization-file", 0, (keyval) HLP_MMINIT },
	{ "mminit", 0, (keyval) HLP_MMINIT },
    };
    static keytab concepttab = { sizeof (concept_keys) / sizeof (keywrd),
			      concept_keys };
    static fdb conceptfdb = { _CMKEY, CM_NLH, nil, 
				(pdat) &concepttab, "concept, "};

					/* use to lookup the help files */
    static fdb topic_filfdb = { _CMFIL, FIL_WLD, nil, nil, nil, nil, nil,
				    nil };
    static fdb topicfdb = { _CMKEY, CM_NLH|KEY_PTR, nil, nil,
				"other topic, ", nil, nil, nil };

    static keywrd mode_keys[] = { 
	{ "top-level-mode", 0, (keyval) HELP_TOP },
	{ "read-mode", 0, (keyval) HELP_READ },
	{ "send-mode", 0, (keyval) HELP_SEND },
    };
    static keytab modetab = { sizeof(mode_keys)/sizeof(keywrd), mode_keys, 
				16 };	/* MAXCOL in cmds.c */
    static fdb modefdb = { _CMKEY, CM_NLH|KEY_WID, nil, (pdat) &modetab,
			       "other mode, " };

    pval parseval;
    fdb *used, *use;
    int level, topval;			/* what we're giving help about */
    char *topic;
    char **topicfiles;
    int mret, plen;
    char mbuf[BUFSIZ];
    static time_t topic_mtimes[3] = { 0, 0, 0 }; /* TOP, READ, SEND */
    struct stat sbuf;
    keytab * helpfiles_to_keytab();
    static keywrd nulkwd[] = {
	{ "", 0, 0 }
    };
    static keytab nuldat = {0, nulkwd };
    static keytab *topic_pdats[3] = { &nuldat, &nuldat, &nuldat };

    if (recurse == 0)
	noise("me with");
    else 
	noise("subject");

    sprintf(mbuf, "%s/%s/%s", help_dir, TOPIC_DIR, help_subdirs[mode]);
    if (stat(mbuf, &sbuf) < 0) {
	topicfdb._cmdat = (pdat) topic_pdats[mode]; /* use old one */
	fprintf(stderr,"?Could not find help directory\n");
    }
    else if (sbuf.st_mtime != topic_mtimes[mode]) {
	sprintf(mbuf, "%s/%s/%s/*.HLP", 
		help_dir, TOPIC_DIR, help_subdirs[mode]);
	topic_mtimes[mode] = sbuf.st_mtime;
	mret = match(mbuf, strlen(mbuf), &topic_filfdb, &parseval,
		     &used, &plen);
	if (mret != CMxOK) {
	    topicfdb._cmdat = (pdat) topic_pdats[mode];
	    fprintf(stderr,"?Could not read help directory\n");
	}
	else {
	    if ((topicfdb._cmdat = (pdat) helpfiles_to_keytab(parseval._pvfil))
		== NULL)
		topicfdb._cmdat = (pdat) topic_pdats[mode]; /* use old one */
	    else {
		if (topic_pdats[mode] != (keytab *)&nuldat)
		    free_ktab((keytab *) topic_pdats[mode]);
		topic_pdats[mode] = (keytab *) topicfdb._cmdat; /* save new one */
	    }
	}
    }
    else {				/* directory unchanged */
	topicfdb._cmdat = (pdat) topic_pdats[mode];
    }

    if (mode & MM_READ) {
	use = fdbchn (&mm_read_fdb_abbr, &mm_read_fdb_1, 
		      &mm_read_fdb_2, &mm_read_fdb_3, &mm_read_fdb_4, 
		      &mm_read_fdb_5, &mm_read_fdb_6, &mm_read_fdb_7, 
		      &mm_read_fdb_inv, 
		      &shell_fdb, &topicfdb, &modefdb, &cfmfdb,NULL);
	if (default_read_command[0] == '\0')
	    mm_read_fdb_abbr._cmdef = default_read_command;
	level = HELP_READ;
    }
    else if (mode & MM_SEND) {
	use = fdbchn (&mm_send_fdb_abbr, &mm_send_fdb_1, 
		      &mm_send_fdb_2, &mm_send_fdb_3, &mm_send_fdb_4, 
		      &mm_send_fdb_5, &mm_send_fdb_inv, 
		      &shell_fdb, &topicfdb, &modefdb, &cfmfdb,NULL);
	if (default_send_command[0] == '\0')
	    mm_send_fdb_abbr._cmdef = default_send_command;
	level = HELP_SEND;
    }
    else {				/* (mode & MM_TOP_LEVEL) */
	use = fdbchn (&mm_top_fdb_abbr, &mm_top_fdb_1, &mm_top_fdb_2, 
		      &mm_top_fdb_3, &mm_top_fdb_4, &mm_top_fdb_5,
		      &mm_top_fdb_6, &mm_top_fdb_7, &mm_top_fdb_inv, 
		      &shell_fdb, &topicfdb, &modefdb, &cfmfdb,NULL);
	level = HELP_TOP;
    }

    parse(use,&parseval,&used);
    if (used == &cfmfdb) {
	printhelp ("mm", level, level);	/* general help */
	return;
    }
    if (used == &shell_fdb) {
	confirm();
	printhelp ("shell", level, level);
	return;
    }
    if (used == &modefdb) {
	cmd_help_1(n, parseval._pvkey,recurse+1);
	return;
    }
    topic = ((keywrd *) parseval._pvkey)->_kwkwd; /* which topic? */
    topval = ((keywrd *)parseval._pvkey)->_kwval; /* what value returned? */
    if (used == &topicfdb) {
	confirm();
	printhelp(topic, HELP_TOPIC, level);
	return;
    }
    /* just commands left */
    if ((topval != CMD_SET) && (topval != CMD_DEFINE)) {
	confirm();
	printhelp (topic, level, level);
    }
    if (topval == CMD_SET)		/* SET and DEFINE take sub-commands */
	help_set(level);
    else if (topval == CMD_DEFINE)
	help_define(level);
}




/*
 * printhelp:
 * print specified help message, piping through pager when 
 * appropriate
 */

printhelp (command, level, topic_mode)
int level;
char *command;
{
    char fname[BUFSIZ];
    struct stat sbuf;
    int len;
    char *msg;

    if (level > sizeof(help_subdirs)/sizeof(help_subdirs[0])) {
	fprintf(stderr,"?Oops, invalid help request\n");
	return;
    }
    if (level == HELP_TOPIC)
	sprintf(fname,"%s/%s/%s/%s.HLP", 
		help_dir, help_subdirs[level], help_subdirs[topic_mode], 
		command);
    else
	sprintf(fname,"%s/%s/%s.HLP", help_dir, help_subdirs[level], command);
    if (stat(fname, &sbuf) < 0) {
	fprintf(stderr,"?Trouble finding help file: %s\n", fname);
	return;
    }
    if ((helpfp = fopen(fname,"r")) == NULL) {
	fprintf(stderr,"?Trouble reading help file: %s\n", fname);
	return;
    }
    len = sbuf.st_size;

    if ((msg = (char *) malloc (len+1)) == NULL) {
	fprintf (stderr, "?Out of memory\n"); /* XXX do something else? */
	return;
    }
    if (fread (msg, sizeof (char), len, helpfp) != len) {
	fprintf (stderr, "?Trouble reading help file - please report\n");
	free (msg);
	fclose(helpfp);
	return;
    }
    msg[len] = '\0';			/* tie off with a null */
    fclose(helpfp);
    display_help(msg);
    free (msg);
}


/*
 * display_help:
 * display a help message, piping through crt-filter when apporpriate
 */

display_help (msg)
char *msg;
{
    FILE *fp, *out, *more_pipe_open();
    extern int use_crt_filter_always;

    out = cmcsb._cmoj ? cmcsb._cmoj : stdout;

    if (use_crt_filter_always ||
	(logical_lines (msg, cmcsb._cmrmx) +1 >= cmcsb._cmrmx)) {
	fp = more_pipe_open(out);
    }
    else
	fp = out;			/* not long, just write it */
#ifdef sun_stdio_bug
    fwrite (msg, sizeof(char), strlen(msg), fp);
#else
    fputs (msg, fp);
#endif /* sun_stdio_bug */
    if (fp == out) {	 		/* not a pipe */
	fflush (fp);
	return;
    }
    more_pipe_close(fp);		/* really a pipe */
}

/*
 * help_set:
 * give help on the various set variables, OR
 * give general help on the set command
 */
help_set(level)
int level;
{
    static fdb cfmfdb = { _CMCFM };
    extern variable set_variables[];

    noise("variable");
    parse (fdbchn(&cfmfdb,&set_cmd_fdb,nil), &pv, &used);
    if (used == &cfmfdb) {
	printhelp ("set", level, level);
	return;
    }
    else {
	int which = pv._pvkey;
	confirm();
	printhelp(set_variables[which].name, HELP_VARS, level);
#ifdef undef
	printf("\n\nThe current value of %s is: ", set_variables[which].name);
	show_variable(stdout, which, false);
#endif
    }
    return;
}


/*
 * help_define:
 * give help on specific aliases, or general help on define command
 */
help_define()
{
    keytab *mk_alias_keys(), *ak;
    extern fdb aliasfdb;
    static fdb cfmfdb = { _CMCFM };
    pval pv;
    fdb *used;

    ak = mk_alias_keys();
    aliasfdb._cmdat = (pdat) ak;

    if (mail_aliases.count > 0)
	parse(fdbchn(&cfmfdb, &aliasfdb, NULL), &pv, &used);
    else
	parse(fdbchn(&cfmfdb, nil), &pv, &used);
    if (used == &cfmfdb) {
	printhelp ("define", HELP_TOP, HELP_TOP);
	return;
    }
    else {
	int n = pv._pvkey;
	confirm();
	disp_alias(stdout, n, true,true);
	return;
    }
}




keytab *
helpfiles_to_keytab(fl)
char ** fl;
{
    char **k;
    keytab *tab;
    char *basename();
    int i,len;


    tab = (keytab *) malloc (sizeof(keytab));
    for(len = 0, k = fl; k && *k; k++, len++);
    if (len > 0)
	tab->_ktwds = (keywrd *)malloc(len * sizeof(keywrd));
    else tab->_ktwds = nil;
    for(i = 0; i < len; i++) {
	tab->_ktwds[i]._kwkwd = basename(fl[i]);
	tab->_ktwds[i]._kwflg = 0;
	tab->_ktwds[i]._kwval = i;
    }
    tab->_ktcnt = len;
    return(tab);
}

static char *
basename(fn)
char *fn;
{
    char *cp1, *cp2;
    char *bp;
    int len;

    if ((cp1 = rindex(fn,'/')) == NULL) 
	cp1 = fn;
    else
	cp1++;
    if ((cp2 = index(cp1,'.')) == NULL) 
        cp2 = cp1 + strlen(cp1);
    len = cp2 - cp1;
    bp = (char *) malloc(len + 1);
    strncpy(bp, cp1, len);
    bp[len] = '\0';
    return(bp);
}


free_ktab(tab)
keytab *tab;
{
    int i;

    if (tab != nil) {
	if (tab->_ktwds) {
	    for(i = 0; i < tab->_ktcnt; i++) {
		free(tab->_ktwds[i]._kwkwd);
	    }
	    free(tab->_ktwds);
	}
	free(tab);
    }
}
