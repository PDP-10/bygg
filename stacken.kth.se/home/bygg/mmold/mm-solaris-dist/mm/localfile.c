/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

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

    if (flags & OP_APP)			/* gonna save to this file */
	openflags = "a";		/* write only at end */
    else if (mail->flags & MF_RDONLY)
	openflags = "r";
    else
	openflags = "r+";
	
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

    alloccnt = 10;			/* start with ten messages */
    mail->count = 0;			/* none yet */
    /* get space for headers */
    mail->msgs = (message *) malloc (alloccnt * sizeof (message));
    mail->last_read = 0;		/* haven't read any yet */
    mail->keywords = nil;
    /* read the next one as long as we can */
    while (1) {
	mail->count++;
	mail->msgs[mail->count].msgno = mail->count;
	mail->msgs[mail->count].hdrsum = NULL;
	mail->msgs[mail->count].private = NULL;
	
	err = (*msg_ops[mail->type].rd_msg)(mail, mail->count);
	if (err != RD_OK)
	    break;
        if (mail->count+1 == alloccnt)	/* filled 'em all up (except #0) */
	    mail->msgs = (message *)
	        realloc (mail->msgs, (alloccnt+=10) * sizeof (message));
    }
    /* read 1:n-1, failed on n -> n-1 msgs */
    mail->count--;

    if (err == RD_FAIL) {		/* format problem */
	mail->flags |= MF_RDONLY;	/* make file read-only */

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
    if ((*fpp = fopen (file, "r")) == NULL)
        switch (errno) {		/* nice switch */
	case EACCES:
	    return (PR_PERM);		/* couldn't look at it */
	default:
	    return (PR_NOEX);		/* say it doesn't exist */
	}
    return (PR_OK);			/* okay, we opened it */
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
 * given a new filename to open and a file pointer for a file we already
 * have open, determine if they are the same file.
 * returns TRUE if they are the same, FALSE if different or an error
 * (error in errno).
 * XXX Need to make this format specific.  For now only works with
 * XXX local files.
 */

int
local_same_file (name1, name2)
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

char *
local_backup_file (filename)
char *filename;
{
    int err;
    char *cp;
    char backup[MAXPATHLEN];

    sprintf (backup, "%s~", filename);

    err = rename (filename, backup);
    if (err < 0) {
	cp = rindex (filename, '/');
	if (cp == 0)
	    sprintf (backup, "#%s#", filename);
	else {
	    strcpy (backup, filename);
	    sprintf (backup + (cp - filename) + 1, "#%s#", cp + 1);
	}
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
 * record the mtime value for a mail file, so we can detect later on if
 * some other process has changed the file.
 */
local_record_mtime (mf)
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
