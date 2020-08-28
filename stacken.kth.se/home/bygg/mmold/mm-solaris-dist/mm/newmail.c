/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/newmail.c,v 2.8 93/05/28 17:33:15 howie Exp $";
#endif

#include "mm.h"
#include "parse.h"
#include "cmds.h"
#include "rd.h"

#define tempfile ".mm-newmail"

extern int check_interval;		/* polling interval */

/* return from newmail and set the alarm for next time */
#if !defined(HAVE_BSD_SIGNALS) || defined(SOLARIS)
#define nmreturn(val)   { alarm(check_interval); return(val); }
#else
#define nmreturn(val)   { alarm(0); return(val); }
#endif

int gotmail=false, sawmail=false;
static int in_here = false;

new_mail (quiet)
int quiet;				/* explicit "check" or automatic? */
{
    keylist im;
    static msgvec *nf;
    msgvec *nmf;
    int type;
    static struct stat mailbuf;
    struct stat oldbuf, sbuf;
    extern int continuous_check;
    int result = true;
    string new_mail_filename;
    int getting_mail;

    /* if they're busy and don't want to be bothered, don't interrupt */
    if (quiet && !continuous_check && mode != MM_TOP_LEVEL)
	nmreturn (result);

    oldbuf = mailbuf;
    gotmail = false;

    check_mtime(cf, NULL);		/* don't bother reading in new mail */
					/*   if there is an inconsistency */
    if (in_here) {			/* in the middle? */
	if (cf->flags & MF_WRITERR) {	/* a write error, leave things alone */
	    if (!quiet)
		fprintf(stderr,
		       "Cannot check for new mail until this file is saved\n");
	    nmreturn (result);
	}
	else {				/* write error was cleared */
	    unlink(nf->filename);
	    (*msg_ops[nf->type].close)(nf->filep);
	    free (nf->msgs);
	    free (nf);
	    nf = nil;
	    in_here = false;
	}
    }

    /* use mail_directory if defined and not "." ("." hard to find later) */
    sprintf(new_mail_filename,"%s/%s", 
	    (strcmp(".",mail_directory) && mail_directory[0] != NULL) ?
	    mail_directory : HOME, tempfile);
    getting_mail = ((cf != NULL) && !(cf->flags & MF_RDONLY) && 
		    (cf->flags & MF_MAILBOX)); /* getting or just looking? */
    
    /* check for leftover (orphaned) .mm-newmail */
    if (getting_mail) {
	nmf = (msgvec *) malloc(sizeof(msgvec));
	if (!nmf)
	    return;			/* XXX */
	
	bzero (nmf, sizeof (msgvec));

	strcpy(nmf->filename, new_mail_filename);
	if (stat(nmf->filename,&sbuf) == 0) {
	    switch (mail_probe (nmf->filename,&type)) { /* can we read this? */
	    case PR_NAME:
		break;
	    case PR_NOEX:
		break;
	    case PR_PERM:
		if (!quiet)
		    cmxprintf("?Cannot read file: %s\n", nmf->filename);
		break;
	    case PR_EMPTY:
		unlink(nmf->filename);
		break;	    
	    case PR_NOTOK:
		if (!quiet)
		    cmxprintf("?File is damaged or in unknown format: %s\n",
			      nmf->filename);
		break;
	    default:
		nmf->type = type;
		if (same_file (nmf->filename, cf->filename)) {
		    if (!quiet)
			cmxprintf ("?Primary mail file cannot be %s.\n",
				   nmf->filename);
		    nmreturn (result);
		}
		if (!fetchmail(nmf,nil)) {
		    nf = nmf;
		    nmreturn (result);
		}
	    }
	}
    }


    /* check for (new) incoming mail */
    if (incoming_mail == NULL) {
	incoming_mail = (keylist) malloc(2 * sizeof(char *));
	incoming_mail[0] = malloc(strlen(user_name) + 
				  sizeof (SPOOL_DIRECTORY) + 2);
	sprintf(incoming_mail[0], "%s/%s", SPOOL_DIRECTORY, user_name);
	incoming_mail[1] = NULL;
    }
    for(im = incoming_mail; im && *im; im++) { /* loop over inboxes */
	if (stat(*im,&sbuf) < 0)
	    continue;
	switch (mail_probe (*im, &type))	{ /* can we read this? */
	case PR_NAME:
	    if (!quiet)
		cmxprintf("?Badly formed filename: %s\n", *im);
	    continue;
	case PR_NOEX:
	    continue;
	case PR_PERM:
	    if (!quiet)
		cmxprintf("?Cannot read file: %s\n", *im);
	    continue;
	case PR_EMPTY:
	    continue;
	case PR_NOTOK:
	    if (!quiet)
		cmxprintf("?File is damaged or in unknown format: %s\n", *im);
	    continue;
	}

	if (!getting_mail) {
	    if (sbuf.st_mtime > oldbuf.st_mtime || !quiet) {
		printf("You have new mail in %s.\n", *im);
		if (sbuf.st_mtime > mailbuf.st_mtime)
		    mailbuf = sbuf;
		sawmail = true;
	    }
	    continue;
	}

	nf = (msgvec *) malloc(sizeof(msgvec));
	bzero(nf,sizeof(msgvec));
	nf->type = type;
	strcpy (nf->filename, new_mail_filename);
	if (same_file (cf->filename, new_mail_filename)) {
	    cmxprintf ("?Primary mail file cannot be %s.\n",
		       new_mail_filename);
	    continue;
	}
	if (same_file (new_mail_filename, *im)) {
	    cmxprintf ("?Incoming mail file cannot be %s.\n",
		       new_mail_filename);
	    continue;
	}
	if (same_file (cf->filename, *im)) {
	    cmxprintf ("?Incoming mail file cannot be primary mail file %s.\n",
		       cf->filename);
	    continue;
	}
	if (move_mail (*im, new_mail_filename, quiet) != 0)
	    nmreturn (result);

	switch (mail_probe (new_mail_filename, &type)){ /* can we read this? */
	case PR_NAME:
	    if (!quiet)
		cmxprintf("?Badly formed filename: %s\n", new_mail_filename);
	    continue;
	case PR_NOEX:
	    continue;
	case PR_PERM:
	    if (!quiet)
		cmxprintf("?Cannot read file: %s\n", new_mail_filename);
	    continue;
	case PR_EMPTY:
	    continue;
	case PR_NOTOK:
	    if (!quiet)
		cmxprintf("?File is damaged or in unknown format: %s\n", 
			  new_mail_filename);
	    continue;
	}

	nf->type = type;
	if (!fetchmail(nf,*im))
	    return(true);
    }
    result = sawmail || gotmail;
    nmreturn (result);
}

fetchmail(nf,name)
msgvec *nf;
char *name;
{
    int j, err;
    keylist k, free_keylist(), add_keyword();
    extern FILE *header_pipe;
    FILE *more_pipe_open();
    extern int append_new_mail;

    if ((*msg_ops[nf->type].open)(nf,0) != 0) { /* open file */
	cmxprintf("?Could not open %s.\n", nf->filename);
	if (name != nil) {
	    cmxprintf("%s was moved to %s!\n", name, nf->filename);
	}
	return(false);
    }
    in_here = true;			/* read the mail in. */
    cf->msgs = (message *)safe_realloc(cf->msgs, (cf->count + nf->count + 1) *
				  sizeof(message));
    if (!cf->msgs) {
	fprintf (stderr, "Out of memory!  New mail is in %s\n",
		 nf->filename);
	fflush (stderr);		/* just in case */
	return(false);
    }		     
    bcopy(&nf->msgs[1], &cf->msgs[cf->count+1],
	  nf->count * sizeof(message));
    if (!gotmail)
	printf("\007");			/* XXX add newline? flush bell? */
    if (nf->count >= cmcsb._cmrmx)
	header_pipe = more_pipe_open(cmcsb._cmoj);
    else
	header_pipe = cmcsb._cmoj;
    header_print(0);
    for (j = cf->count+1; j <= cf->count + nf->count; j++) {
	cf->msgs[j].flags |= (M_RECENT|M_MODIFIED);
	header_print(j);
    }
    header_print(-1);
    if (header_pipe == cmcsb._cmoj) {	/* didn't open the pipe */
	if (cmcsb._cmoj)
	    fflush(cmcsb._cmoj);
    }
    else
	more_pipe_close(header_pipe);
    header_pipe = NULL;

    cf->count += nf->count;
    
    for(k = nf->keywords; k && *k; k++)
	cf->keywords = add_keyword(*k, cf->keywords);
    nf->keywords = free_keylist(nf->keywords);
    /*
     * Grow the sequence-encoding bit vectors if necessary.
     * Zeroing the new bits isn't necessary since the sequence
     * is explicitly bounded by <sequence_t>->last.
     */
    if (!((sequence_bits(cf->sequence) = (unsigned char *)
	   realloc (sequence_bits(cf->sequence), cf->count/NBBY+1)) &&
	  (sequence_bits(cf->prev_sequence) = (unsigned char *)
	   realloc (sequence_bits(cf->prev_sequence), cf->count/NBBY+1)) &&
	  (sequence_bits(cf->read_sequence) = (unsigned char *)
	   realloc (sequence_bits(cf->read_sequence), cf->count/NBBY+1))))
	panic("out of memory in newmail");
    
    if (append_new_mail == SET_NO) {
	cf->flags |= MF_DIRTY;		/* make sure all mail gets saved */
	err = !update(&cf,UPD_ALWAYS);
    }
    else {
	err = !apnd_update(&cf, nf->count); /* save by appending */
	if (err)
	    cf->flags |= MF_DIRTY;	/* will update next time around */
    }

    if (err) {				/* can we save it? */
#ifdef EDQUOT
	if (errno == EDQUOT) {
	    fprintf(stderr, "\n\
Try suspending MM, and making space by deleting some files.  Then, use the\n\
WRITE command to save your new mail!  If you can't make any space, your mail\n\
is still in %s, and the new messages are in %s\n",
		    mail_file, nf->filename);
	}
	else 
#endif /* BSD */
	{
	    if (name != NULL)
		fprintf (stderr,"New mail from %s has been moved to %s\n",
			 name, nf->filename);
	}
	alarm(0);
	return(false);
    }
    else {
	in_here = false;
	unlink(nf->filename);	/* it's saved, delete the new mail */
	(*msg_ops[nf->type].close)(nf->filep);
	free (nf->msgs);
	free (nf);
	nf = nil;
    }
    gotmail = true;
    return(true);
}

move_mail(from, tofile, quiet)
char *from, *tofile;
int quiet;
{
    int ret;
    extern string movemail_path;
    char *movemail_argv[4];
    
    movemail_argv[0] = movemail_path;
    movemail_argv[1] = from;
    movemail_argv[2] = tofile;
    movemail_argv[3] = nil;

    fix_signals_for_fork (true);
    ret = mm_execute(movemail_path, movemail_argv);
    fix_signals_for_fork (false);
    if (ret != 0 && !quiet)
      fprintf (stderr, "Could not get mail from %s\n", from);
    return (ret);
}
