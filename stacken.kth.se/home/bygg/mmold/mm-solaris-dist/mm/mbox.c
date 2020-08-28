/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/mbox.c,v 2.3 93/06/17 17:49:54 howie Exp $";
#endif

/* mbox.c: routines to handle unix mbox format mail files */

#include "mm.h"				/* good stuff! */
#include "rd.h"

#define key "From "			/* key to mbox format */
#define keysize (sizeof(key)/sizeof(char))-1 /* don't count the null */
#define flagkey "Flags: "		/* key for flags field */
#define flagsize (sizeof(flagkey)/sizeof(char))-1

extern int local_close();
char *fgetline();
static char *cache;

/*
 * mbox_open:
 * see rd.h comments about open
 */
mbox_open (mail,flags)
msgvec *mail;
int flags;
{
    return (local_contig_open (mail,flags));
}

/*
 * see rd.h comments about close
 */
mbox_close (fp)
FILE *fp;
{
    local_close (fp);
    if (cache) {
	free(cache);
	cache = NULL;
    }
}
				   

/*
 * mbox_rdmsg:
 * see rd.h comments about rd_msg
 * Each mbox format message starts with a line of the form "From sender date"
 * which may be followed by a flags line.
 */
mbox_rdmsg(mail,num) 
msgvec *mail;
int num;
{
    char *header;			/* initial line of a message */
    char *cp, *from;
    int line;				/* offset in text of line start */
    u_long size, flags = 0, keybits = 0;
    char *text;				/* body of message */
    time_t date;
    int buflen, space;			/* for snarfing in the body */
    int eof, inheaders;			/* end of file yet?  in headers? */
    message *m;

    if (num > mail->count)		/* can't read more than we have */
       return (RD_FAIL);		/* not there */

    if (mail->last_read > num) { 	/* gotta go back to it */
	if (cache) {
	    free(cache);
	    cache = NULL;
	}
        rewind (mail->filep);		/* have to start from beginning */
	mail->last_read = 0;		/* haven't read msg 1 */
    }

    if (cache) {
	header = cache;
	cache = NULL;
    }
    else
	header = fgetline(mail->filep);	/* do fgets till we get it all */
    if (header == NULL)			/* if not even close, fail */
        return ((mail->last_read < num-1) ? RD_FAIL : RD_EOF);
    if (strncmp (header, key, keysize) != 0) {
        fail_msg (mail->last_read + 1);	/* aww...  on our first message!  */
	free (header);
	return (RD_FAIL);		/* doesn't have any! */
    }

    /* skip over the ones we don't want */
    while (++mail->last_read < num) {	/* starting to read next message */
        do {
	    free (header);
	    header = fgetline (mail->filep); /* try the next line */
	    if (header == NULL) {
		if (mail->last_read < num - 1) {
		    fail_msg (mail->last_read+1);
		    return (RD_FAIL);
		}
		else
		    return (RD_EOF);
	    }
	} while (strncmp (header, key, keysize) !=0); /* till new msg */
    }					/* do next message */

    /* now we have the header for the message we want */
    cp = &header[keysize];		/* skip "From " (key) */
    while (isspace (*cp) && *cp != '\0') /* get to actual name */
	cp++;
    from = cp;				/* here's the name */

    while (!isspace (*cp) && *cp != '\0') { /* break on whitespace */
	if (*cp == '"')			/* ignore inter-quote stuff */
	    while (*(++cp) != '"' && *cp != '\0');
	cp++;				/* skip non-space */
    }
    if (*cp != '\0')			/* anything left? */
	*(cp++) = '\0';			/* end from field */

    while (isspace (*cp) && *cp != '\0') /* get to date */
	cp++;

    /* about ready to parse the date, if there's any header left */
    if (*cp == 0) {
        fail_msg (mail->last_read--);	/* no good */
	free (header);
	return (RD_FAIL);
    }

/*    date = stringtotime (cp); */
    date = mbox_date(cp);
    if (date == 0)
        fprintf (stderr,"Message %d has bad date string (continuing)\n", num);

    /* now get the rest of the message */
    buflen = 0;
    text = cp = malloc (buflen += 100);	/* get some space for body of msg */
    space = buflen;			/* all empty so far */
    eof = false;			/* not the end yet */
    inheaders = true;			/* start out reading the headers */
    do {
        line = cp - text;		/* starting a line */
	if (space < 10) {
	    text = (char *) realloc (text, buflen += 100);
	    space += 100;		/* used the rest (but for '\0') */
	    cp = &text[buflen-space];	/* room to complete line */
	}
        if (fgets (cp, space, mail->filep) == NULL) { /* try to get a line */
	    eof = true;			/* end of file, that's it */
	    continue;			/* go down to the "while" */
	}
	while (index (cp, '\n') == NULL) { /* get the WHOLE line */
	    text = (char *) realloc (text, buflen +=100);
	    space = 101;		/* used the rest (but for '\0') */
	    cp = &text[buflen-space];	/* room to complete line */
	    if (fgets (cp, space, mail->filep) == NULL) {
	        eof = true;		/* end of file */
		break;			/* out of inner while */
	    }
	}

	if (eof)
	    continue;			/* get out of outer while */

	if (text[line] == '\n')		/* blank line */
	    inheaders = false;
	if (inheaders &&
	    (ustrncmp (&text[line], flagkey, flagsize) == 0)) { /* flags */
	    if (sscanf(&text[line]+flagsize,"%6lo%6lo",&keybits,&flags) !=2) {
		fail_msg (mail->last_read--);
		return (RD_FAIL);
	    }
	    text[line] = '\0';		/* forget this line */
	    space = space + (cp - &text[line]);	/* reclaim that */
	    cp = &text[line];		/* back to beginning */
	}
	else {
	    while (*cp)			/* get to null */
	        cp++;			/* keep going */
	    space = buflen - (cp - text); /* what we haven't used */
	}
    } while (!eof && (strncmp (&text[line], key, keysize) != 0));

    if (!eof) {				/* unread the from */
	cache = (char *)safe_strcpy(&text[line]);
	text[line] = '\0';		/* stop before it */
	text = (char *) realloc (text, (size = line) +1); /* snug to fit */
    }
    else
        text = (char *) realloc (text, (size = (strlen (text))) +1);

    m = &mail->msgs[num];
    m->date = date;
    m->from = malloc (strlen (from) +1); /* need space */
    strcpy (m->from, from);
    m->size = size;
    m->flags = flags;
    m->keybits = keybits;
    m->text = text;
    m->keywords = nil;
    get_incoming_keywords(mail,m);
    free (header);			/* AFTER copying from field */
    return (RD_OK);			/* got the message! */
}


/*
 * mbox_wrmsg:
 * see rd.h comments about wr_msg
 * Each mbox format message starts with a line of the form "From sender date"
 * and then has the special flags line, and then contains the text.
 *
 * We need a sender.  msg->from is best, but if we read in a
 * non-mbox format we'll have to make something up.  Sender is
 * preferable to From, but if we don't find anything, at least say
 * something.   
 */
mbox_wrmsg(mail,msg,num,flags)
msgvec *mail;
message *msg;
int num;
int flags;
{
  int err;
  char *from, *from2, *htext();
  int freep;				/* do we have to free "from"? */
  int i,j;
  char *tbeg, *tfrom;			/* "beg" of text and "From" loc */
  char *quote;

  if (!(flags & (WR_SAVE | WR_COPY))) {	/* not a save or copy */
      mail->flags |= MF_DIRTY;		/* mark it to be saved */
      return (false);			/* no error */
  }
  if ((msg->flags & M_DELETED) && (flags & WR_EXP)) {
      if (num == mail->count)
	  local_get_size(mail);
      return (false);			/* don't save deleted messages */
  }

  /* if we have a from field already, we won't have to free it */
  if (freep = (((from = msg->from) == NULL) || (from[0] == '\0')))
      /* no internal from */
      if ((from = htext ("sender", msg->text)) == NULL) /* "Sender:" field? */
	  if ((from = htext ("from", msg->text)) == NULL) { /* no, "From:"? */
	      from = "???";		/* who knows?? */
	      freep = false;		/* don't try to free that! */
	  }

  /* do we need to quote the address? read in msg->from, it was good then */
  if (freep &&				/* taken from a header, check it */
      (index(from,' ') || index(from,'\t'))) { /* needs quoting */
      from2 = malloc (strlen(from)+3); /* adding two quotes */
      from2[0] = '"';			/* going to have to quote it */
      for (i = 0, j = 1; from[i] != '\0'; i++)
	  if (from[i] != '"') {		/* don't copy internal quotes */
	      from2[j] = from[i];
	      j++;
	  }
      from2[j] = '"'; from2[j+1] = '\0';
      free (from);			/* done with old copy */
      from = from2;
  }

  while (freep && (from2 = index(from,'\n'))) {
      *from2 = ' ';
  }
      

  fprintf(mail->filep,"From %s %s",from, ctime(&msg->date));
  fprintf(mail->filep,"%s%06lo%06lo\n",flagkey,
	  msg->keybits, (msg->flags & M_INTERNAL));
  if (freep)
      free (from);			/* we malloced it */
  /* now check for "\nFrom " and write out text */
  tbeg = msg->text;			/* point to start of text */
  while ((tfrom = search ("\nFrom ", tbeg)) != NULL) {
      /* write up to and including \n first */
      if (fwrite (tbeg, tfrom - tbeg + 1, 1, mail->filep) != 1)
	  return (errno);
      fprintf (mail->filep, ">From ");	/* quote the From */
      tbeg = tfrom+6;			/* rest of string */
  }
  fwrite (tbeg, sizeof(char), strlen(tbeg), mail->filep);
  if (msg->text[msg->size-1] != '\n')	/* trailing newline required */
      fputc ('\n', mail->filep);

  if ((fflush (mail->filep) == EOF) ||	/* make sure we get it out */
      (ferror(mail->filep) != 0))
      return (true);
  if (flags & WR_SAVE) {		/* we saved it */
      msg->flags &= ~M_MODIFIED;	/* it's not modified anymore  */
      if (num == mail->count)
	  local_get_size(mail);
  }
  return (false);			/* no error */
}


/*
 * probe file to see if it's in mbox format
 * mbox format is distinctive in that the file starts with "From "
 */
mbox_probe(file)
char *file;
{
    FILE *fp;
    char p[keysize +1];		/* enough to probe it, with null */
    int i;
    int ret;

    if (cf && same_file(cf->filename, file)) {
	if (cf->type == TYPE_MBOX) 
	    return (PR_OK);
	else
	    return (PR_NOTOK);
    }
    if ((ret = local_contig_proben (file, &fp)) != PR_OK)
	return (ret);			/* couldn't open it */
    i = fread (p, sizeof (char), keysize, fp);
    fclose (fp);			/* okay, we're done looking */
    if (i == 0)
	return (PR_EMPTY);		/* empty file */
    if (i < keysize) 
        return (PR_NOTOK);		/* not ok, not enough characters */
    p[i] = '\0';			/* finish the string */
    if (strcmp (p, key) != 0)
	return (PR_NOTOK);		/* doesn't look right */
    return (PR_OK);			/* passed! */
}
