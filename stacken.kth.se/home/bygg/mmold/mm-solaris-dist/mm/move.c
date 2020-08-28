/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/move.c,v 2.5 90/10/04 18:25:08 melissa Exp $";
#endif

/*
 * move.c - support for the move and copy commands
 */

#include "mm.h"				/* for msgvec */
#include "cmds.h"			/* for CMD_MOVE */
#include "parse.h"			/* for atmbuf */
#include "rd.h"				/* for PR_xxx's */

/* global guys just for move and copy */
static msgvec *dest;			/* file to copy to */
static int delete;			/* delete the message after copy? */
static int copy_err;			/* did we have a problem copying? */

msgvec *getdestfile();			/* get this type right... */
int do_copy();				/* this exists... */

/*
 * cmd_move:
 * copy a message from the current file to another file
 * if n = CMD_MOVE, delete it as well
 */
int
cmd_copy (n)
int n;
{
    string fname;			/* new file to copy to */
    int format;				/* format to use for new file */

    if (!check_cf (O_RDONLY))		/* make sure we have current file */
	return;
    delete = (n == CMD_MOVE);		/* delete if it's "move" not "copy" */

    noise ("into file");		/* hint hint */
    parse_copyfile (fname, &format);	/* parse filename and maybe switches */

    parse_sequence ("current",NULL,NULL); /* parse a message sequence */

    if (delete && (cf->flags & MF_RDONLY)) { /* trying to delete? */
	if (modify_read_only == SET_NEVER) {
	   fprintf(stderr, 
		   "Warning: cannot modify a file visited with \"examine\"\n");
	   delete = FALSE;		/* don't bother trying */
        }
	else if (modify_read_only == SET_ASK)
	   delete = yesno("File is read-only, delete messages anyway? ", "no");
    }
    dest = getdestfile (fname, format,
                       (cmcsb._cmflg & CM_ITTY) ? TRUE : FALSE);
        if (dest == NULL) {
	return false;
    }
    sequence_loop (do_copy);		/* copy each message */
    (*msg_ops[dest->type].close)(dest->filep); /* close it */
    free (dest);			/* maybe free other guys too */
    if ((mode == MM_TOP_LEVEL) && !copy_err)
	seq_print (true);
    return true;
}

/*
 * parse_copyfile:
 * parse the switches for the move/copy command and the filename
 * switches are optional, filename is not
 */
parse_copyfile (name,ptype)
char name[];
int *ptype;
{
    pval parseval;
    fdb *used;
    extern keytab formattab;
    static char *def = NULL;
    extern char cmswbeg;
    static fdb formatfdb = {
	_CMSWI, 0, NULL, (pdat)&formattab, NULL, NULL, NULL };
    static fdb filefdb = {
	_CMFIL, FIL_PO|FIL_VAL, NULL, NULL, "file to copy to", 
	NULL, NULL};
    filblk fb;
    char *dirs[3];
    char *cp;
    char wd[MAXPATHLEN];
    char *getwd();

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

    /* set up defaults for parse */
    if (def != nil)			/* in case of reparse */
	free(def);
    if (default_mail_type.current != NULL) {
	def = (char *) malloc (strlen (default_mail_type.current) +2);
	def[0] = cmswbeg;
	strcpy (&def[1], default_mail_type.current); /* get our default */
    }
    else
	def = NULL;
    formatfdb._cmdef = def;		/* insert the default */
    *ptype = -1;			/* mark that it hasn't been set */

    if (directory_folders)
	filefdb._cmffl &= ~FIL_NODIR;
    else
	filefdb._cmffl |= FIL_NODIR;
    parse(fdbchn (&formatfdb,&filefdb,NULL), &parseval, &used);
    if (used == &formatfdb) {
	*ptype = parseval._pvint;	/* what they want */
	parse (fdbchn (&filefdb,NULL), &parseval, &used); /* NEED a file */
    }

    free (def);				/* free us some space */
    def = NULL;

    /* handle the parsed file now */
#ifdef UNDEF
    if (rindex(parseval._pvfil[0], '/') == NULL) { /* XXX assumes name */
	if (access (parseval._pvfil[0], F_OK) != 0 && /* XXX is big enough */
	    mail_directory[0] != '\0')	           
	    sprintf (name, "%s/%s", mail_directory, parseval._pvfil[0]);
	else
	    sprintf (name, "./%s", parseval._pvfil[0]);
    }
    else				/* directory specified by user */
	strncpy(name, parseval._pvfil[0], STRLEN);
#endif /* UNDEF */

    /* ****  SEE ALSO, similar code in parse.c: parse_in_out_file **** */
    if (*parseval._pvfil[0] == '/') {		/* absolute path specified */
      strcpy (name, parseval._pvfil[0]);
    }
    else {
	/* 
	 * file does not exist in mail-directory (mail-directory is an
	 * absolute path), so see if it is in .
	 * in which case use it, otherwise, try to default to new file
	 * in mail-directory
	 */
	if ((index(parseval._pvfil[0], '/') != NULL) || /* a dir specified */
	    access (parseval._pvfil[0], F_OK) == 0 || /* file exists in . */
	    mail_directory[0] == '\0') { /* or no mail_directory, use wd */
	    if (getwd (wd) == NULL) {	/* got some kind of error */
		fprintf (stderr, "%s\n", wd); /* print the error message */
		strcpy (name, parseval._pvfil[0]);
	    }
	    else {
		cp = parseval._pvfil[0];
		if (cp[0] == '.' && cp[1] == '/')
		    cp += 2;
		sprintf (name, "%s/%s", wd, cp);
	    }
	}
	else				/* use mail_directory */
	    sprintf (name, "%s/%s", mail_directory, parseval._pvfil[0]);
    }

    if (access (name, F_OK) != 0)	{ /* does it exist? */
	int ok;
	char c = 0;
	char *p = rindex(name, '/');	/* no, check directory */
					/* (NOTE: there WILL be a slash) */
	c = *p; *p = 0;			/* consider only the directory */
	ok = access(name, F_OK|W_OK);	/* can we create a file in there? */
	*p = c;				/* fix the path again */
	if (ok < 0)
	    cmerr(errstr(-1));		/* nope, complain */
    }
}

/*
 * getdestfile:
 * open up this file for appending to, figure out its type
 */
msgvec *
getdestfile(name,format,inter)
char name[];
int format;				/* default format */
int inter;				/* interactive errors */
{
    msgvec *df;
    int type;
    int flags;				/* flags for msg_ops.open */
    extern int num_msg_ops;		/* how many mail formats we know */
    extern int mail_probe();		/* to probe the mail */
    extern int auto_create_files;	/* do we need to ask? */
    int fd;

    if (same_file (cf->filename, name)) {
	if (inter)
	    cmerr ("Cannot append to current mail file");
	else
	    return (NULL);
    }
    switch (mail_probe (name, &type))	/* can we read this? */
    {
    case PR_NAME:
	if (inter)
	    cmerr ("Badly formed filename: %s", name);
	else 
	    return (NULL);
    case PR_PERM:
	if (inter)
	    cmerr ("Cannot determine mail format: %s", name); /* can't read */
	else 
	    return (NULL);
    case PR_NOEX:			/* must make new file */
	if (inter) {
	    fprintf (stderr, "File does not exist: %s\n", name);
	    if ((auto_create_files == SET_NO) || 
		(auto_create_files == SET_ASK &&
		 !yesno("Do you want to create it? ", "yes")))
		return(NULL);
	}
	fd = creat(name, new_file_mode); /* XXX assuming local file */
	if (fd >= 0)			/* error msg on open later */
	    close(fd);
	/* ** fall through ** */
    case PR_EMPTY:
	if (format != -1)		/* did we parse something? */
	    type = format;		/* whatever we parsed in switch */
	else {
	    for (type = 0; type < num_msg_ops; type++) /* try default */
		if (strcmp (default_mail_type.current, msg_ops[type].typename)
		    == 0)
		    break;		/* use their default */
	    if (type == num_msg_ops)
		type = TYPE_MTXT;	/* no match, pick some type */
	}
	break;
    case PR_NOTOK:
	if (inter)
	    cmerr ("File is damaged or in unknown format: %s", name);
	else 
	    return (NULL);
    default:
	if ((format != type) && (format != -1) 
	    && inter)			/* they guessed wrong */
	    fprintf (stderr, "File is in %s format, ignoring switch\n",
		     msg_ops[type].typename);
    }
    df = (msgvec *) malloc (sizeof (msgvec)); /* room to put all this stuff */
    if (df == NULL) {
	if (inter)
	    cmerr ("No room to handle file: %s", name);
	else 
	    return (NULL);
    }

    bzero (df, sizeof (msgvec));

    df->type = type;

    strcpy (df->filename, name);

    flags = inter ? (OP_INT|OP_APP) : OP_APP;
    if ((*msg_ops[df->type].open)(df,flags) != 0) /* open */
	return (NULL);
    return (df);			/* don't need data filled in */
}

/*
 * do_copy:
 * copy the nth message from cf (current file) into dest
 * delete it if in CMD_MOVE.
 */
static int
do_copy (n)
int n;					/* which message */
{
    int oldflags;

    if (n == 0) {
	copy_err = FALSE;		/* no errors yet */
	return (true);
    }
    if (n < 0)
	return (true);			/* no special ending */

    if (!copy_err && !ignore (n)) {
	oldflags = cf->msgs[n].flags;
	cf->msgs[n].flags &= ~M_DELETED; /* copy we move not deleted */

	/* copy this message to the end of dest (0 for this unused field) */
	if (copy_err = (*msg_ops[dest->type].wr_msg)
	             (dest,&cf->msgs[n],0,WR_COPY)) {
	    cf->msgs[n].flags = oldflags;
	    fprintf (stderr, "Trouble writing message %d to %s, aborting %s\n",
		     n, dest->filename, delete ? "move" : "copy");
	    return (false);
	}
	cf->msgs[n].flags = oldflags;
	if (delete && !(cf->msgs[n].flags & M_DELETED)) { /* changing it? */
	    cf->msgs[n].flags |= (M_DELETED|M_MODIFIED); /* delete it */
	    cf->flags |= MF_DIRTY;	/* file (msg flags) must be saved */
	}
    }
    return (copy_err);
}


