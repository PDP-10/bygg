/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /src/sos5.bin/cucca/mm/mm/RCS/mtxt.c,v 2.0.1.3 1997/10/21 19:33:32 howie Exp $";
#endif

/* mtxt.c: routines to handle mail.txt format files */

#include "mm.h"
#include "rd.h"

#define hdrlen 81			/* the header's less than 80 chars */

char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
		   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

/*
 * mtxt_open:
 * see rd.h comments about open
 * mtxt_open is just your average local, contiguous mail file
 */
mtxt_open (mail,flags)
msgvec *mail;
int flags;
{
    return (local_contig_open (mail, flags)); /* just your avg contig file */
}


/*
 * mtxt_close:
 * see rd.h comments about close
 */
mtxt_close (fp)
FILE *fp;
{
    local_close (fp);
}


/*
 * mtxt_rdmsg:
 * see rd.h comments about rd_msg
 * an mtxt format message has a header line, which contains the date
 * the message was received, the number of bytes in the remainder of
 * the message (the text), and flags written as a 12-char octal
 * number.  Then comes the text.  The end of the message is determined
 * from the byte count.
 */
mtxt_rdmsg (mail,num)
msgvec *mail;
int num;
{
    char *mesg;				/* the body of the message */
    message *m;				/* message pointer */
    char *cp;				/* pointer into the header */
    char header[hdrlen];		/* just the first line */
    time_t date;			/* date the message was received */
    u_long size, flags = 0, keybits = 0;

    if (num > mail->count)		/* can't read what we don't have */
       return (RD_FAIL);		/* not there */

    if (mail->last_read > num) { 	/* gotta go back to it */
        rewind (mail->filep);		/* have to start from beginning */
	mail->last_read = 0;		/* haven't read msg 1 */
    }
    /* skip over the ones we don't want */
    while (++mail->last_read < num) {	/* starting to read next message */
        /* get first line of the msg */
        if (fgets (header, hdrlen, mail->filep) == NULL) {
	    mail->last_read--;		/* missed it */
	    return (RD_FAIL);		/* previous messages missing */
	}

        if (!chkhdr(header, mail, &size)) {
	    fail_msg(mail->last_read--); /* didn't get that one */
	    return (RD_FAIL);
	}
	fseek (mail->filep, size, SEEK_CUR); /* skip over the body */
    }
	
    /* now get the message we want */

    /* get first line of the msg */
    if (fgets (header, hdrlen, mail->filep) == NULL) {
        mail->last_read--;		/* didn't read it */
        return (RD_EOF);		/* end of file */
    }
    if (!chkhdr(header, &size)) {
        fail_msg(mail->last_read--);	/* didn't read it */
	return (RD_FAIL);		/* bad message */
    }

    cp = index (header, ',');		/* find end of date */
    if (cp == NULL) {
        fail_msg(mail->last_read--);
	return (RD_FAIL);
    }
    *cp++ = '\0';			/* null-terminate the date */
/*    date = stringtotime (header); */
    date = mtxt_date(header);
    if (date == (time_t) 0)
	fprintf (stderr, "Message %d has bad date string (continuing)\n",
		 num);
    
    cp = index (cp, ';');		/* find flags field */
    if (cp == NULL) {
        fail_msg(mail->last_read--);
	return (RD_FAIL);
    }
    cp++;
    if (sscanf(cp,"%6lo%6lo", &keybits, &flags) != 2) { /* written in octal */
        fail_msg (mail->last_read--);
	return (RD_FAIL);
    }

    mesg = malloc (size+1);		/* get space for the whole mesg */
    if (mesg == NULL) {			/* **************** well? ***** */
        fprintf (stderr, "Couldn't malloc space for message.\n");
	mail->last_read--;
        return (RD_FAIL);		/* couldn't read it */
    }

    if (fread (mesg, sizeof (char), size, mail->filep) != size) {
        fail_msg(mail->last_read--);	/* tell them */
	free (mesg);			/* free the space */
	return (RD_FAIL);
    }
    mesg[size] = '\0';
    m = &mail->msgs[num];
    m->size = size;
    m->flags = flags;
    m->keybits = keybits;
    m->from = NULL;			/* nothing here */
    m->date = date;
    m->text = mesg;
    m->next = m->prev = 0;
    m->keywords = nil;
    get_incoming_keywords(mail,m);
    return (RD_OK);
}

/*
 * chkhdr:
 * check out the header line of the current message, and parse the size
 * return true if it has a size field and a \n at the end, false otherwise
 */
chkhdr(header, psize)
char *header;
int *psize;
{
    char *cp;

    cp = index (header, '\n');		/* make sure we got the \n */
    if (cp == NULL)			/* uh-oh, first line too long */
        return (false);
    *cp = '\0';				/* blank out the newline */
    cp = index (header, ',');		/* find the comma */
    if (cp == NULL)			/* didn't find it? */
	    return (false);		/* too bad... */
    *psize = atoi (++cp);
    /* we want to see if we got a negative number or a non-number */
    /* a non-number gives value 0 */
    if ((*psize < 1) && (*cp != '0'))	/* really zero is okay */
        return (false);
    return (true);			/* we passed! */
}

/*
 * mtxt_wrmsg:
 * see rd.h comments about wr_msg
 * specifics of writing mtxt format: 
 * each mtxt format message starts with a line like 
 * "17-May-86 13:52:06-GMT,710;000000000005", containing a
 * fixed-format date in GMT, byte count, and flags (in octal).  After
 * this line is the text of the message.
 */
mtxt_wrmsg(mail,msg,num,flags)
msgvec *mail;
message *msg;
int num;
int flags;
{
  int err;

  if (!(flags & (WR_SAVE | WR_COPY))) { /* not a save or copy */
    mail->flags |= MF_DIRTY;		/* mark it to be saved */
    return (false);			/* no error */
  }
  if ((msg->flags & M_DELETED) && (flags & WR_EXP)) {
      if (num == mail->count)
	  local_get_size(mail);
      return (false);			/* don't save deleted messages */
  }

  /* print date, size & flags: "17-May-86 13:52:06-GMT,710;000000000005" */
  fprintf(mail->filep, "%s,%d;%06lo%06lo\n",
	  fdate(msg->date), msg->size, 
	  msg->keybits, (msg->flags & M_INTERNAL));
  if (fwrite (msg->text, msg->size, 1, mail->filep) != 1)
      return (errno);
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
 * mtxt_probe:
 * probe file to see if it's a mail.txt
 * a mail.txt starts with a date, either "dd-mon-yy" or " d-mon-yy"
 * (where d is a digit).  Return true if it's this format, false o.w.
 */
mtxt_probe(file)
char *file;
{
    FILE *fp;
    char p[2];				/* enough to probe it */
    int i;
    int ret;

    if (cf && same_file(cf->filename, file)) {
	if (cf->type == TYPE_MTXT) 
	    return (PR_OK);
	else
	    return (PR_NOTOK);
    }
    if ((ret = local_contig_proben (file, &fp)) != PR_OK)
	return (ret);			/* couldn't open it */
    i = fread (p, sizeof (char), 2, fp);
    fclose (fp);			/* okay, we're done looking */
    if (i == 0)
	return (PR_EMPTY);		/* empty file */
    if (i < 2) 
        return (PR_NOTOK);		/* not ok, not enough characters */
    if (isdigit (*p) || (*p == ' ' && isdigit (p[1])))
        return (PR_OK);
    return (PR_NOTOK);
}
