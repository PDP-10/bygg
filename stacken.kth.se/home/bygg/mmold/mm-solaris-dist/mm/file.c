/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

/* XXXXXXXX  memory leaks at most all the cmerr's in here */

#ifndef lint
static char *rcsid = "$Header: /amd/banzai/b/src0/sun4.bin/cucca/mm/RCS/file.c,v 2.0.1.3 1996/11/22 16:16:30 howie Exp $";
#endif

/*
 * file.c - routines for handling mail files
 */

#include "mm.h"
#include "parse.h"
#include "cmds.h"
#include "rd.h"

msgvec *cf = nil;			/* current mail file */

msgvec *getfile ();

/*
 * check_cf:
 * see if we have a mail file we're looking at
 * if they want to modify it, see if they can and give warning if not
 * if we're processing command line arguments, they must have done one
 * that requires their mail_file, so get it if we have no file
 * The modify parameter can be one of:
 *   O_RDONLY	- need a mail file
 *   O_WRONLY   - need a writeable file
 *   O_RDWR     - want a writeable file, but will settle for read-only
 *                e.g. this is used by cmd_read when called due to 
 *		  command line args, or for pre-checking writeable
 *		  files (we'll panic later, during O_WRONLY check)
 */
check_cf (modify)
int modify;
{
    extern int gotargs;
    int writeable = false;

    writeable = (modify == O_WRONLY) || (modify == O_RDWR);

    if (cf == nil) {
	if (!gotargs)
	    return sorry ("No current mail file");
	if (strlen(mail_file) != 0) {	/* want a file, get one */
	    printf("Reading %s ...\n", mail_file);
	    cf = getfile(mail_file, writeable ? CMD_GET : CMD_EXAMINE);
	    if (cf == NULL && modify == O_RDWR)
		cf = getfile (mail_file, CMD_EXAMINE);
	    if (cf == NULL)
		return sorry ("No current mail file");
	    do_flagged();		/* show flagged ones */
	} /* continue with other checks */
    }
    if (!(writeable && (cf->flags & MF_RDONLY))) /* don't want to change, */
						 /*   or can */
	return true;
    if ((modify_read_only == SET_ALWAYS) || (modify == O_RDWR))
	return true;
    if (modify_read_only == SET_ASK)
	if (yesno("File is read-only; modify anyway? ", "no"))
	    return true;
    return sorry ("Can't modify a file visited with the EXAMINE command");
}


/*
 * cmd_get:
 * get (or "examine") a file, releasing the old one if we get the new
 * one successfully.   
 */
int
cmd_get (n)
int n;
{
    msgvec *nf, *f;
    char *fname, *parse_output_file(), *parse_input_file();
    extern int gotargs;			/* are we processing command line? */
    int i;
    buffer filename;
    extern top_level_parser();

    gotargs = FALSE;			/* don't exit after get in cmd line */
    noise ("mail file");
    if (n == CMD_GET)
	fname = parse_output_file ("mail file", mail_file, true);
    else
	fname = parse_input_file ("mail file", mail_file, true);
    confirm ();				/* XXX leak */

    if (cf)				/* save it before reading new file */
	if (!update (&cf,UPD_SAVEMOD) && (cf->flags & MF_WRITERR)) {
	    fprintf (stderr,
		     "?Cannot save old file -- %s command aborted.\n",
		     n == CMD_EXAMINE ? "EXAMINE" : "GET");
	    free (fname);
	    return (false);
	}

    nf = getfile(fname, n);

    (void) sprintf(filename, "%s.rc", fname); /* setup filename.rc */

    free (fname);
    if (nf == nil)
	return false;

    if (cf && (cf != nf)) {		/* get rid of old current file */
	f = cf;				/* remember this another minute... */
	cf = nil;			/* ...but don't update it anymore */
	(*msg_ops[f->type].close)(f->filep); /* close old file */
	msgvec_free (f);		/* free all the bits and pieces */
    }

    cf = nf;				/* make new file current */
 
    /* "take" filename.rc if it is "safe" */
    if (safe_rc_file(filename))
	(void) take_file(filename, top_level_parser, FALSE);

    if (cf) do_flagged();

    return true;
}

/*
 * do_flagged:
 * display flagged messages of new file
 */
do_flagged()
{
    int i,count;
    extern FILE *header_pipe;
    FILE *more_pipe_open();

    if (!display_flagged_messages) 
	return;
    /* first find out how many */
    for (count = 0, i = 1; i <= cf->count; i++) {
	if (cf->msgs[i].flags & M_FLAGGED)
	    ++count;
    }

    if (count >= cmcsb._cmrmx)
	header_pipe = more_pipe_open(cmcsb._cmoj);
    else
	header_pipe = cmcsb._cmoj;
    if (header_pipe == nil)
	header_pipe == stdout;
    header_print(0);
    for (i = 1; i <= cf->count; i++) {
	if (cf->msgs[i].flags & M_FLAGGED)
	    header_print(i);		/* uses header_pipe */
    }
    header_print(-1);
    if (header_pipe == cmcsb._cmoj){
	if (cmcsb._cmoj)
	    fflush(cmcsb._cmoj);	/* didn't open the pipe */
    }
    else if (header_pipe != stdout)
	more_pipe_close(header_pipe);
    header_pipe = NULL;
}

/*
 * getfile:
 * open the file, initialize the msgvec structure
 */
msgvec *
getfile (file, flag)
char *file;
int flag;
{
    msgvec *nf;
    int type;
    extern int num_msg_ops;		/* how many mail formats we know */
    extern int auto_create_files;	/* do we need to ask? */
    int probeval;
    int err;
    char **q;
    int fd;

    if (same_file (file, mail_file))
	for (q = &(incoming_mail[0]); q && *q; q++)
	    if (same_file (file, *q)) {
		fprintf(stderr,"Cannot read a primary mail file that is also an incoming mail file.");
		return(NULL);
	    }

    switch (probeval = mail_probe (file, &type)) /* can we read this? */
    {
    case PR_NAME:
	fprintf (stderr,"Badly formed filename: %s", file);
	return(NULL);
    case PR_NOEX:
	if (flag == CMD_EXAMINE) {
	    fprintf(stderr,"File not found: %s", file);
	    return(NULL);
	}
	fprintf (stderr, "File does not exist: %s\n", file);
	if ((auto_create_files == SET_NO) || 
	    (auto_create_files == SET_ASK && (cmcsb._cmflg & CM_ITTY) &&
	     !yesno("Do you want to create it? ", "yes")))
	    return(NULL);
	fd = creat(file, new_file_mode); /* XXX assuming local file */
	if (fd >= 0)			/* error msg on open later */
	    close(fd);
	/* ** fall through ** */
    case PR_EMPTY:
	for (type = 0; type < num_msg_ops; type++)
	    if (strcmp(default_mail_type.current,msg_ops[type].typename) == 0)
		break;			/* use their default, if okay */
	if (type == num_msg_ops)	/* bad typename */
	    type = TYPE_MTXT;		/* pick some type */
	break;
    case PR_PERM:
	fprintf(stderr,"Cannot read file: %s\n", file);
	return(NULL);
    case PR_NOTOK:
	fprintf(stderr,"File is damaged or in unknown format: %s", file);
	return(NULL);
    }
    nf = (msgvec *) malloc (sizeof (msgvec)); /* room to put all this stuff */
    if (!nf) {
	fprintf(stderr,"Out of memory");
	return(NULL);
    }

    bzero(nf, sizeof (msgvec));		/* et voila, ne plus de core dumps */

    nf->type = type;

    strcpy (nf->filename, file);
    nf->flags = ((flag == CMD_GET) ? 0 : MF_RDONLY); /* only flag so far */
    if (same_file(file, mail_file))	/* primary mailbox is same file */
	nf->flags |= MF_MAILBOX;

    nf->count = 0;
    if (probeval == PR_NOEX) {		/*open file, interactive errors*/
	nf->msgs = NULL;		/* nothing there */
	err = (*msg_ops[nf->type].open)(nf,OP_APP|OP_INT);
    }
    else
	err = (*msg_ops[nf->type].open)(nf,OP_INT);
    if (err != 0) {
	if (flag == CMD_GET && ((errno == EACCES)
#ifdef EWOULDBLOCK
	    || (errno == EWOULDBLOCK)
#endif /* EWOULDBLOCK */
	    )) {
	    fprintf (stderr, "\
 Use the EXAMINE command if you want to open it in read-only mode.\n");
	    return(NULL);
	}
	return (NULL);
    }

    if (nf->filep == NULL) {		/* reuse cf */
	cf->flags |= MF_RDONLY;		/* downgrade file */
	free (nf);
	nf = cf;			/* flag that we are the same */
    }
    printf ("%d message%s read\n",nf->count,((nf->count==1) ? "" : "s"));

    record_mtime (nf);			/* save modify time for this file */

    nf->current = nf->count;		/* point to last message */
    if (nf != cf) {			/* downgrade.  did not reread file */
	time (&nf->when_read);
	if (!sequence_init (nf))
	    cmerr ("Out of memory");
    }
    return (nf);
}


/* 
 * mail_probe:
 * probe the file to see what mail format it's in
 * pass back probe error code, set typep if PR_OK
 */
int
mail_probe (file, typep)
char *file;
int *typep;
{
    int stat, newstat;
    extern int num_msg_ops;		/* how many formats we know */

    stat = PR_NAME;			/* assume the lowest level problem */
    for (*typep = 0; *typep < num_msg_ops; (*typep)++) { /* test each type */
	if ((newstat = (*msg_ops[*typep].probe)(file)) == PR_OK)
	    return (PR_OK);		/* that's the one */
	if (newstat > stat)		/* did we get any further? */
	    stat = newstat;
    }
    return (stat);			/* tell them how we did */
}


/*
 * get the size of a local file
 */
local_get_size(mail)
msgvec *mail;
{
    struct stat sbuf;
    if (fstat(fileno(mail->filep), &sbuf) == 0)
	mail->size = sbuf.st_size;
    else {
	fprintf(stderr,"?Couldn't determine size of %s\n", mail->filename);
	mail->size = 0;
    }
}

/*
 * local_contig_open:
 * Contiguous local files are opened the same way, regardless of format...
 */
local_contig_open (mail,flags)
msgvec *mail;
int flags;
{
    FILE *fp;
    char *openflags;			/* how to open the file */
    int err;
    struct stat sbuf;
    int lockable;			/* can we lock it? */
    int upgrade = FALSE;
    buffer bfile;

    if (flags & OP_APP)			/* gonna save to this file */
	openflags = "a";		/* write only at end */
    else if (mail->flags & MF_RDONLY)
	openflags = "r";
    else
	openflags = "r+";

    /* if #filename# exists, we were in the middle of modifying it
     * (probably the system crashed) */
    if (!(flags & OP_PND)) {		/* poundfile unexpected */
	poundfile (bfile, mail->filename);
	if (access (bfile, F_OK) == 0) { /* #filename# exists */
	    if (gone (mail->filename)) { /* but no "filename" */
		fprintf (stderr, "%s does not exist, but backup does\n",
			 mail->filename);
		if (mail->flags & MF_RDONLY) { /* examine, don't change */
		    fprintf (stderr, "Examining %s backup file instead\n",
			     bfile);
		    strcpy (mail->filename, bfile);
		}
		else {
		    fprintf (stderr, "Recovering from %s backup file...", 
			     bfile);
		    fflush (stderr);
		    if (rename (bfile, mail->filename) != 0) {
			fprintf (stderr, "rename failed, call a consultant\n");
			return (PR_PERM);
		    }
		    fprintf (stderr, "done\n");	/* now file exists */
		}
	    }
	    else {			/* #filename# *and* filename */
		fprintf (stderr, "%s exists.\n\
 Apparently MM had trouble while writing out %s.\n\
 Please contact the consultant (send mail to bug-mm or consultant).\n\
 Until you resolve this, you will not be able to modify this file, but you\n\
 can EXAMINE it.\n", bfile, mail->filename);
		if (mail->flags & MF_MAILBOX)
		    fprintf (stderr, "\
 Since this file is your primary mailbox, you must resolve this\n\
 problem before MM can process new mail.\n");
		mail->flags |= MF_RDONLY; /* get it read-only */
	    }
	}
    }
	
    /* be careful about opening a file that's already open */
    if (cf && same_file (mail->filename, cf->filename)) {
	if (cf->flags & MF_RDONLY) {	/* current file is read only */
	    if (mail->flags & MF_RDONLY) { /* r -> r */
		if (flags & OP_INT)
		    fprintf (stderr, "(Rereading %s)\n", mail->filename);
	    }
	    else {			/* r -> w */
		upgrade = TRUE;		/* open and lock */
	    }
	}
	else {				/* current file is read/write */
	    if (mail->flags & MF_RDONLY) { /* w -> r */
		unlock_file (cf->filename, fileno(cf->filep));
		mail->filep = NULL;	/* reuse cf since same file */
		return (0);
	    }
	    else {			/* w -> w */
		if (flags & OP_INT)
		    fprintf (stderr, 
			     "?%s already current writable mail file\n", 
			     mail->filename);
		free (mail);
		errno = ETXTBSY;
		return (ETXTBSY);
	    }
	}
    }    

    if (!upgrade && (flags & OP_APP) || ((mail->flags & MF_RDONLY) == 0)) {
	if (stat (mail->filename, &sbuf) != 0) {
	    /*
	     * if filename doesn't exist, make sure it gets created
	     * with the proper mode.
	     */
	    int fd = creat (mail->filename, new_file_mode);
	    if (fd < 0) {
		fprintf (stderr, "Cannot create %s: %s\n", mail->filename,
			 errstr (errno));
		(void) free (mail);	/* XXX bogus! */
		return errno;
	    }
	    else
		(void) close (fd);
	}
    }
    
    if ((fp = fopen (mail->filename, openflags)) == NULL) {
        if (flags & OP_INT) {		/* interactive? */
	    fprintf (stderr, "?Cannot open %s: %s\n", 
		     mail->filename, errstr (errno));
	}
	free (mail);
	return (errno);
    }

    if (fstat(fileno(fp), &sbuf) == 0) {
	mail->size = sbuf.st_size;
	mail->mtime = sbuf.st_mtime;	/* record_mtime(mail) */
	lockable = ((sbuf.st_mode & S_IFMT) == S_IFREG);
    }
    else {
	fprintf(stderr,"fstat: Couldn't determine size and type of %s\n",
		mail->filename);
	mail->size = 0;
	lockable = FALSE;
    }

    if (upgrade) {			/* close first since we are  */
	local_close (cf->filep);	/* about to lock.  too late */
	cf->filep = NULL;		/* if lock succeeds */
    }
    /* if opened for write, lock the file away */
    if (lockable && ((flags & OP_APP) || !(mail->flags & MF_RDONLY))) {
	if (lock_file (mail->filename, fileno (fp)) < 0) {
	    if (errno == EINVAL) {
		if (debug)
		    fprintf (stderr, "%%File %s could not be locked\n",
			     mail->filename);
		/*
		 * pretend we locked it, and hope for the best
		 */
	    }
	    else {
		if (!upgrade)
		    local_close(fp);	/* close it again */
		else {
		    cf->filep = fp;	/* use this instead */
		}
		if (!(flags & OP_INT)) {
		    free (mail);
		    return (errno);
		}
		fprintf (stderr, "?Can't lock %s: %s.\n\
 Apparently some other process is accessing this file in read/write mode.\n",
			 mail->filename,
#ifdef EWOULDBLOCK
			 (errno == EWOULDBLOCK) ? "file is busy" :
#endif
			 errstr (errno));
		free (mail);
		return (errno);
	    }
	}
    }

    mail->filep = fp;
    if (! (flags & OP_APP))		/* don't count if for append */
        local_contig_count (mail);	/* count 'em */
    return 0;				/* no error */
}


/*
 * local_contig_count:
 * count the messages in this mail file.  For formats which call this
 * routine, (ones which are local, contiguous mail files), this
 * requires reading them all in, so do that and keep in as many as we
 * have room for.  This fills in mail->count and mail->msgs.
 */
local_contig_count (mail)
msgvec *mail;
{
    int alloccnt;			/* how many we've allocated */
    int err;

    alloccnt = mail->size / 1000;
    if (alloccnt < 10)
	alloccnt = 10;			/* start with ten messages */
    mail->count = 0;			/* none yet */
    /* get space for headers */
    mail->msgs = (message *) malloc (alloccnt * sizeof (message));
    mail->last_read = 0;		/* haven't read any yet */
    mail->keywords = nil;
    /* read the next one as long as we can */
    while ((err = (*msg_ops[mail->type].rd_msg)(mail, ++mail->count)) == 
	   RD_OK) { 
	mail->msgs[mail->count].hdrsum = NULL; /* not used yet */
        if (mail->count+1 == alloccnt)	/* filled 'em all up (except #0) */
	    mail->msgs = (message *)
	        realloc (mail->msgs, (alloccnt+=100) * sizeof (message));
    }
    /* read 1:n-1, failed on n -> n-1 msgs */
    mail->count--;

    if (err == RD_FAIL) {		/* format problem */
	if (!(mail->flags & MF_RDONLY)) {
	    mail->flags |= MF_RDONLY;	/* make file read-only */
	    unlock_file(mail->filename, fileno(mail->filep)); /* unlock it */
	}

	fprintf (stderr, "\n\
There is a problem with %s.\n\
MM was unable to read the entire mail file because it appears to have\n\
been corrupted.  You may be able to read some of the messages in this file,\n\
but MM will not add messages to it or allow you to modify it.  See a\n\
consultant or systems administrator for help.\n\n",
		 mail->filename);
	if (mail->flags & MF_MAILBOX)
	    fprintf (stderr, "\
Since this file is your primary mailbox, you must resolve this\n\
problem before MM can process new mail.  If no assistance is available\n\
now, rename the file so MM won't find it at startup, and a new mailbox\n\
file will be created for you the next time you invoke MM.\n\n");
    }
    /* snug it down to fit just right */
    mail->msgs = (message *) realloc (mail->msgs, 
				      (mail->count+1)*sizeof (message));
}

/*
 * local_contig_proben:
 * open a local contiguous file for probin'
 * probe-open = proben -- get it??
 */
int
local_contig_proben (file,fpp)
char *file;
FILE **fpp;
{
    buffer bfile;
    char *f;

    f = file;
    if (gone (file)) {			/* file doesn't exist, or is empty */
	poundfile (bfile, file);
	if (access (bfile, F_OK) == 0) { /* but #file# does exist! */
	    f = bfile;			/* so look there */
	}
    }
    if ((*fpp = fopen (f, "r")) == NULL)
        switch (errno) {		/* nice switch */
	case EACCES:
	    return (PR_PERM);		/* couldn't look at it */
	default:
	    return (PR_NOEX);		/* say it doesn't exist */
	}
    return (PR_OK);			/* okay, we opened it */
}

/*
 * fail_msg:
 * send a nice printf that we failed on this message
 */
fail_msg(num)
int num;
{
    if (num > 1)
	fprintf (stderr, "\
?File is corrupted near message %d; the rest of file will be ignored.\n",
		 num - 1);
    else
	fprintf (stderr, "?Mail file is corrupted.\n");
}


/*
 * local_close:
 * local files are also mostly closed the same way
 * gotta free up the lock, though that probably gets done automagically
 * when we let go of the file...
 */
local_close (fp)
FILE *fp;
{
    if (fp != NULL)
	fclose (fp);
}


/*
 * fgetline:
 * Keep doing fgets till we get the whole line.
 * on end of file, return what we have, or NULL if we have nothing
 */
char *
fgetline (filep)
FILE *filep;
{
    int buflen;
    char *buf;
    char *bufp;

    buflen = 1;				/* start with room for NULL :-) */
    bufp = buf = malloc (buflen+=100);	/* need some space to start */
    while (true) {			/* till we return */
        if (fgets (bufp, 101, filep) == NULL) { /* get the line */
	    if (bufp == buf) {		/* should be end of file */
		free (buf);
		return (NULL);
	    }
	    else {			/* read some before EOF */
		*bufp = '\0';		/* close it off */
		return (buf);
	    }
	}
	if ((bufp = index (bufp, '\n')) != NULL) { /* get it? */
	    *bufp = '\0';		/* we don't need the CR */
	    return (buf);		/* return the whole line */
	}
	buf = (char *) realloc (buf,buflen+=100); /* get more space */
	bufp = &buf[buflen-101];	/* point to null of what we have */
    }
}


/*
 * cmd_check:
 * check for new messages in the current mail file
 */
cmd_check (n)
int n;
{
    noise ("for new mail");
    confirm ();
    if (new_mail(false) == false)
	printf("No new mail.\n");
}


/*
 * update and update_1:
 * Write out the current version of the mail, saving the old version
 * in mailname~.  Close old file.  If writing to an alternate file,
 * (name already in pf->filename) don't write a backup.
 *
 * Note that the msgvec isn't updated to reflect the deleted messages,
 * since if we're about to exit, there's no point.
 *
 * If file is now empty and wasn't before (and isn't main mail file),
 * delete it and free the msgvec.
 *
 * We cannot allow interrupts during update, since most of them want
 * to call update, and also we really don't want to stop in the middle
 * of writing the file.  So, we defer them until we're done (and
 * MF_DIRTY ensures we won't redo the update).  
 *
 * Return true on success, false if we fail anywhere.
 *
 * Note: update() calls check_mtime(), which calls update_1()
 *       (we didn't want to call update() from inside itself)
 */

int
update (pf,updflags)
msgvec **pf;
int updflags;
{
    int err;

    if (!(updflags & UPD_ALTFILE)) {	/* don't check altfile */
	if (!check_mtime(*pf, &err))	/* didn't pass the check */
	    return (err);
    }
    return (update_1(pf, updflags));
}

int
update_1 (pf,updflags)
msgvec **pf;
int updflags;
{
    msgvec *f;				/* get a non-pointered */
    int i, err;
    FILE *old_fp;
    buffer ofile, bfile;
    int backup;
    int flags;				/* flags to send to wr_msg */
    int empty;				/* is the file empty? */
    int doit = FALSE;			/* do we need to do an update? */
    unsigned short filemode = new_file_mode;

    int exp = updflags & UPD_EXPUNGE;	/* should we do an expunge? */
    int savemods = updflags & UPD_SAVEMOD; /* save even if only MF_MODIFED */
    int always = updflags & UPD_ALWAYS;	/* write it out no matter what */
    int altfile = updflags & UPD_ALTFILE; /* write to an alternate file */
    int quiet = updflags & UPD_QUIET;	/* be quiet */

    int result = true;			/* assume we'll succeed */
    long mask = block_signals ();	/* block out interrupts */

    if (pf == 0)
	goto failed;

    f = *pf;
    if (f == 0)
	goto failed;

    if ((f->flags & MF_RDONLY) && !altfile && !(f->flags & MF_SAVEONLY))
	goto failed;

    if (exp) {				/* check for deletions to expunge */
	for (i = 1; i <= f->count && !doit; i++)
	    if (f->msgs[i].flags & M_DELETED)
		doit = TRUE;
	if (doit)			/* expunge! */
	    printf ("Expunging deleted messages.\n");
	else
	    printf ("No messages deleted.\n");
    }

    if (always ||			/* forced to do it */
	(f->flags & MF_DIRTY) ||	/* needs saving */
	((f->flags & MF_MODIFIED) && savemods)) /* want to save flags */
	doit = TRUE;

    if (!doit && !altfile)
	goto failed;

    /****** use msg_ops when we add the rename routine to it */
    {
	struct stat sb;
	if (stat (f->filename, &sb) == 0)
	    filemode = sb.st_mode & 07777;

	poundfile (bfile, f->filename);	/* make #filename# */
	sprintf (ofile, "%s~", f->filename); /* and filename~ */
	if (backup = (rename (f->filename, bfile) != 0)) {
	    switch(errno) {
	    case ENOENT:
		if (!quiet)
		    fprintf (stderr,"%s does not exist.  Creating new file.\n",
			     f->filename);
		break;
#ifndef HAVE_FLEXFILENAMES
	    case EINVAL:
		if (!quiet)
		    fprintf (stderr, "Filename too long for rename: %s\n\
Use the WRITE or COPY commands to save your messages.", f->filename);
		goto failed;
#endif
	    default:
		if (!quiet)
		    fprintf (stderr,"Can't rename %s to %s: %s\n\
 Use the WRITE or COPY commands to save your messages.",
			     f->filename, bfile, errstr (errno));
		goto failed;
	    }
	}
	unlink(ofile);			/* delete filename~ to make room */
    }
    old_fp = f->filep;			/* don't overwrite that */
    /* now we've saved (or closed) the old file, open the new one */
    err = (*msg_ops[f->type].open)(f, OP_APP|OP_PND); /* new file, append */

    if (!err)
	if (chmod (f->filename, filemode) != 0)
	    if (!quiet)
		perror ("update: error updating file modes");

    flags = exp ? (WR_SAVE|WR_EXP) : WR_SAVE;
    if (f->count == 0)
	empty = FALSE;		  /* if they started "empty", leave it alone */
    else
	empty = TRUE;
    if (!err) {
        for (i = 1, err = 0; (i <= f->count) && (err == 0); i++) {
	    if (!exp || !(f->msgs[i].flags & M_DELETED))
		empty = FALSE;
	    err=(*msg_ops[f->type].wr_msg)(f,&f->msgs[i],i,flags);
	    if (err) err = errno;
	}
	if (!err)			/* again, in case file had 4000 bit */
	    if (chmod (f->filename, filemode) != 0)
		if (!quiet)
		    perror ("update: error updating file modes");
	/* XXX This should be done through a call via msg_ops XXX */
	if (fsync(fileno(f->filep)) < 0)
	    perror (f->filename);
    }
    if (err) {				/* trouble writing file */
        (*msg_ops[f->type].close)(f->filep); /* close failed file */
	f->filep = old_fp;		/* replace good one */
	if (rename (bfile, f->filename) != 0) {
	    if (!quiet)
		fprintf(stderr, "\
Can't write output file %s and can't restore from %s - %s\n",
			f->filename, bfile, errstr (err));
	}
	else {
	    if (!quiet)
		fprintf (stderr,"\
File not saved: can't write output file - %s\n",
			 errstr (err));
	}
	if (!altfile) {			/* don't set this for another file */
	    cf->flags |= MF_WRITERR;
	    record_mtime (f);
	}
	errno = err;
	goto failed;
    }
    if (altfile) {
	(*msg_ops[f->type].close)(f->filep); /* success, close the file */
	f->filep = old_fp;		/* restore original file */
	goto succeeded;
    }
	
    if (empty) {			/* nothing left, delete it */
	if (f->flags & MF_MAILBOX) {
	    if (!quiet)
		fprintf(stderr,
			"All messages deleted, not deleting main mail file\n");
	}
	else {
	    if (!quiet)
		fprintf (stderr, "All messages deleted, deleting file...");
	    /****************/
	    if (unlink (f->filename) != 0) { /* delete it */
		if (!quiet)
		    fprintf (stderr,"Can't delete %s - %s\n", 
			     f->filename, errstr (errno));
		goto failed1;
	    }
	    /****************/
	    if (!quiet)
		fprintf (stderr, "OK\n");
	    (*msg_ops[f->type].close)(f->filep); /* close old file */
	    msgvec_free (f);		/* free all the bits and pieces */
	    *pf = nil;			/* no more file */
	    goto succeeded;
	}
    }
    f->flags &= ~(MF_DIRTY|MF_MODIFIED); /* not dirty any more */
    (*msg_ops[f->type].close)(old_fp);	/* success, close old file */

    record_mtime (f);			/* update saved modify time */
    cf->flags &= ~(MF_WRITERR|MF_SAVEONLY);
    goto succeeded;

  failed:
    result = false;
    release_signals (mask);
    return result;
  failed1:
    result = false;
    release_signals (mask);

  succeeded:
    /* the last rename worked, so this should too.  if it doesn't,
     * what could we do anyway?  maybe unlink (bfile)??  Naah.
     */
    rename(bfile, ofile);
    release_signals (mask);
    return result;
}


/*
 * apnd_update:
 * write out the new messages (the last cnt messages)
 */

int
apnd_update(pf, cnt)
msgvec **pf;
int cnt;
{
    int i, err = 0;
    off_t fsize;
    msgvec *f = *pf;			/* to be consistent with update() */

    local_get_size(f);
    fsize = (off_t) f->size;		/* remember orig size for error cond */

    for (i = f->count + 1 - cnt; (i <= f->count) && (err == 0); i++)
	err = (*msg_ops[f->type].wr_msg)(f, &f->msgs[i], i, WR_SAVE);

    if (err) {				/* put back file on disk to previous */
	f->flags |= MF_WRITERR;		/*   state */
	ftruncate(fileno(f->filep), fsize); /* XXX low level */
	fsync(fileno(f->filep));	/* XXX low level */
	fseek(f->filep, 0, SEEK_END);	/* go to the end */ /* XXX low level */
	f->size = fsize;
    }

    record_mtime(f);			/* remember when we last touched it */
    return(!err);
}


/*
 * msgvec_free:
 * Free all the various pieces of the msgvec we have here.
 * We don't null out the pointer though...
 */
msgvec_free (mv)
msgvec *mv;
{
    int i;
    char *text;
    keylist keys;

    sequence_free (mv);			/* free any sequencing info */
    for (i = 1; i <= mv->count; i++) {	/* free the message */
	if ((text = mv->msgs[i].text) != NULL) /* the text */
	    safe_free (text);
	if ((keys = mv->msgs[i].keywords) != NULL) /* keywords */
	    free_keylist(keys);
	if ((text = mv->msgs[i].from) != NULL) /* from field */
	    safe_free(text);
    }
    free_keylist(mv->keywords);
    safe_free (mv->msgs);		/* and the message structures */
    safe_free (mv);			/* the data structure itself */
}

/*
 * record the mtime value for a mail file, so we can detect later on if
 * some other process has changed the file.
 */
record_mtime (mf)
msgvec *mf;
{
    struct stat sb;
    if (stat (mf->filename, &sb) != 0) {
	perror (mf->filename);
	mf->mtime = 0;
    }
    else
	mf->mtime = sb.st_mtime;
}


/* 
 * check_mtime
 * compare file's on-disk modified time with the one we recorded.
 * if file on disk has changed:
 *   warn the user, rename filename to filename.<pid>
 *   unset mail-file, if set
 *   if in core version was not modified, change to RDONLY
 *   if modified, try to save
 * return TRUE if we the mtime is unchanged (okay), FALSE otherwise
 * 	fill in upd_result with the result from update, if non-NULL
 */

int
check_mtime(mf, upd_result)
msgvec *mf;
int *upd_result;
{
    struct stat sb;
    int err;
    char pidstr[10];
    string oldfile;
    extern int handle_changed_modtime;

    if (!mf || (mf->flags & MF_RDONLY)) 
	return (TRUE);			/* no need to check */

    err = stat (mf->filename, &sb) != 0;
    if (!err && (sb.st_mtime == mf->mtime))
	return (TRUE);			/* everything fine */

    /* uh oh! */
    fprintf (stderr, "\nWarning: %s has changed on disk!\n",
	     mf->filename);

    if ((handle_changed_modtime == SET_NO) || 
	(handle_changed_modtime == SET_ASK && (cmcsb._cmflg & CM_ITTY) &&
	 !yesno("Attempt to recover? ", "yes"))) {
	record_mtime(mf);		/* don't nag them again for this mod */
	return(TRUE);			/* they said forget it */
    }

    /* rename current file to filename.pid */
    mf->flags &= ~MF_MAILBOX;		/* no longer primary mail file */
    sprintf (pidstr, "%d", PID);	/* stringize */
    strcpy(oldfile, mf->filename);	/* save old name */
    strcat(mf->filename, ".");
    strcat(mf->filename, pidstr);
    fprintf(stderr, "Renaming current (internal) version to %s (read-only)\n", 
	     mf->filename);
    fprintf(stderr, 
	    "Please exit MM and resolve any conflicts between %s and %s\n",
	    oldfile, mf->filename);
    fprintf (stderr, "Contact a consultant if you need assistance\n");
    mf->flags |= MF_RDONLY|MF_SAVEONLY;	/* make it read-only, but updateable */
    modify_read_only = SET_ASK;		/* annoy them */

    err = update_1(&mf, UPD_ALWAYS);

    if (upd_result)			/* maybe they don't care */
	*upd_result = err;
    return (FALSE);
}


/*
 * poundfile:
 * make #filename# or /path/#filename#
 */
poundfile (buf, fname)
char *buf, *fname;
{
    char *cp;

    cp = rindex (fname, '/');
    if (cp == 0)			/* no pathname, easy */
	sprintf (buf, "#%s#", fname);
    else {				/* slip # in */
	strcpy (buf, fname);
	sprintf (buf + (cp - fname) + 1, "#%s#", cp + 1);
    }
}

/*
 * make_backup_file:
 * (apparently unused)
 * try to rename "file" to "file~" or "#file#"
 * return filename used or NULL for failure
 */
char *
make_backup_file (filename)
char *filename;
{
    int err;
    char *cp;
    char backup[MAXPATHLEN];

    sprintf (backup, "%s~", filename);

    err = rename (filename, backup);
    if (err < 0) {
	poundfile (backup, filename);
	err = rename (filename, backup);
    }
    
    if (err < 0)
	return 0;

    if (cp = malloc (strlen (backup) + 4)) {
	strcpy (cp, backup);
	return cp;
    }
    return 0;
}


/*
 * given a new filename to open and a file pointer for a file we already
 * have open, determine if they are the same file.
 * returns TRUE if they are the same, FALSE if different or an error
 * (error in errno).
 * XXX Need to make this format specific.  For now only works with
 * XXX local files.
 */

int
same_file (name1, name2)
char *name1, *name2;
{
    struct stat buf1, buf2;

    if (stat (name1, &buf1) != 0) {
	return (FALSE);
    }
    if (stat (name2, &buf2) != 0) {
	return (FALSE);
    }
    if ((buf1.st_dev == buf2.st_dev) && (buf1.st_ino == buf2.st_ino))
	return (TRUE);
    return (FALSE);
}

/*
 * gone:
 * file gone: empty or nonexistent
 */
gone(file)
char *file;
{
    struct stat statb;

    if (access (file, F_OK) != 0)	/* it's gone */
	return (TRUE);
    if ((stat(file, &statb) == 0) && (statb.st_size == 0))
	return (TRUE);			/* it's empty */
    return (FALSE);
}


/*
 * safe_rc_file:
 * filename.rc is "safe" if it is owned either by the user running MM or
 * if the file is owned by root AND the file is not group or world writable.
 */

int
safe_rc_file(fname)
char *fname;
{
    struct stat sbuf;

    if (stat(fname, &sbuf) != 0) {
	return(FALSE);
    }
    if (!(sbuf.st_uid == UID || sbuf.st_uid == 0) || 
	sbuf.st_mode & 022) {
	fprintf (stderr, "%s not safe, ignoring\n", fname);
	return(FALSE);
    }
    return(TRUE);
}
