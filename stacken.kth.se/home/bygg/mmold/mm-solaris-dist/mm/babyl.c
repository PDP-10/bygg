/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /src/sos5.bin/cucca/mm/mm/RCS/babyl.c,v 2.0.1.3 1997/10/21 19:33:32 howie Exp $";
#endif

/* babyl.c: routines to handle babyl format mail files */
/*
 * cache stuff may break if there are two open files.
 * when opening second file, maybe rewind "cf", and free the cache.
 */

#include "mm.h"				/* good stuff! */
#include "rd.h"
#include "babyl.h"			/* babylish stuff */

#define key "BABYL OPTIONS:"		/* key to babyl format */
#define keysize (sizeof(key)/sizeof(char))-1 /* don't count the null */
#define MSGEND '\037'
#define MSGBEG '\014'
#define EOOH  "*** EOOH ***"
#define FROM  "From:"
#define BABYLHEADER "BABYL OPTIONS:\n\
Version: 5\n\
Note:   This is the header of an rmail file.\n\
Note:   If you are seeing it in rmail,\n\
Note:    it means the file has no messages in it.\n\
Labels:"


extern int local_close();
char *fgetline(), *safe_strcpy();
u_long flaglookup();
static char *cache = 0;

static struct flagword flagnames[] = {
    { "deleted", M_DELETED },
    { "answered", M_ANSWERED },
    { "filed", M_FILED },
    { "forwarded", M_FORWARDED },
    { "unseen", M_SEEN },
    { "edited", M_EDITED },
    { NULL, 0 },
};


static struct flagword keynames[] = {
    { "flagged", M_FLAGGED },
    { NULL, 0 },
};

/*
 * babyl_open:
 * see rd.h comments about open
 */
babyl_open(mail,flags)
msgvec *mail;
int flags;
{
    int error;
    struct stat sbuf;
    keylist k;
    struct flagword *fl;

    if (!(flags & OP_APP))		/* don't worry if just an append */
	mail->keywords = nil;
    error = local_contig_open (mail,flags);
    if (error != 0 || !(flags & OP_APP))
	return(error);
    if (mail->filep == NULL)		/* reuse cf, file lock downgraded */
	return(0);
    if (fstat(fileno(mail->filep), &sbuf) != 0) {
	perror("fstat");
	return(errno);
    }
    if (sbuf.st_size != 0) 
	return(0);
    fprintf(mail->filep,"%s", BABYLHEADER);
    for(fl = keynames; fl && fl->name; fl++)
	fprintf(mail->filep, "%s%s", fl->name, (fl+1)->name ? "," : "");
    if (mail->keywords && keynames[0].name)
        fputc (',', mail->filep);
    if (mail->keywords)
	for(k = mail->keywords; *k != NULL; k++) 
	    fprintf(mail->filep, "%s%s", *k, *(k+1) ? "," : "\n");
    else 
        fputc ('\n', mail->filep);
    fputc (MSGEND, mail->filep);
    return(0);
}

/*
 * see rd.h comments about close
 */
babyl_close (fp)
FILE *fp;
{
    local_close (fp);
    if (cache) {
	free(cache);
	cache = NULL;
    }
}

/*
 * babyl_rdmsg:
 * see rd.h comments about rd_msg
 */
babyl_rdmsg(mail,num)
msgvec *mail;
int num;
{
    char *babyl_banner();
    char *header, *cp, *text, *htext();
    int buflen, space, eof, line, seen_eooh = false, which_headers = 0;
    message *m;
    u_long size;

    if (num > mail->count)		/* much more than we have */
       return (RD_FAIL);		/* not there */

    if (mail->last_read > num) { 	/* gotta go back to it */
        rewind (mail->filep);		/* have to start from beginning */
	if (cache) {
	    free(cache);
	    cache = NULL;
	}
	mail->last_read = 0;		/* haven't read msg 1 */
	header = babyl_banner(mail);
	if (header == NULL)
	    return(RD_FAIL);		/* no banner, bad file */
    }
    else {				/* no need to rewind */
	if (cache) {
	    header = cache;
	    cache = NULL;
	}
	else
	    header = fgetline(mail->filep);
	if (header == NULL)
	    return (RD_EOF);		/* this is the end... */
	if (strcmp(header, key) == 0) {	/* looking at babyl header */
	    rewind(mail->filep);
	    mail->last_read = 0;
	    header = babyl_banner(mail); /* get past banner */
	    if (header == NULL) {
		fprintf(stderr,
		       "Babyl banner has bad format, ignoring rest of file\n");
		return(RD_FAIL);	/* bad banner, bad file */
	    }
	}
	if (header[0] != MSGEND) {
	    fail_msg (mail->last_read);
	    fseek(mail->filep, -strlen(header), SEEK_CUR);
	    return(RD_FAIL);		/* bad previous message */
	}
    }
    
    while (++mail->last_read < num) {	/* skip up to the num'th message */
	if (header[1] != MSGBEG) {
	    fseek(mail->filep, -strlen(header), SEEK_CUR);
	    free(header);
	    fail_msg (mail->last_read--); /* couldn't read it */
	    return (RD_FAIL);		/* earlier message missing */
	}
	while (true) {			/* skip one message */
	    free(header);
	    header = fgetline(mail->filep);
	    if (header == NULL) {
		fail_msg (mail->last_read--); /* couldn't read it */
		return (RD_FAIL);	/* bad file -- ended w/out MSGEND */
	    }
	    if (header[0] == MSGEND)
		break;
	}
    }
    /* should be looking at our msg now */
    if (header[1] != MSGBEG) {		/* no next message.... */
	fseek(mail->filep, -strlen(header), SEEK_CUR);
	free(header);
	if (header[1] != '\0') {	/* not just end of file */
	    fail_msg (mail->last_read--); /* couldn't read it */
	    return (RD_FAIL);
	}
	return(RD_EOF);			/* just missed it */
    }
    free(header);			/* free "MSGEND MSGBEG \n" */
    header = fgetline(mail->filep);	/* get line with flags and keywords */
					/* handle babyl flags/keywords */
    if (header[0] == '0')		/* no headers before EOOH */
	which_headers = 2;
    else if (header[0] == '1')
	which_headers = 1;
    else {
	free (header);
	fail_msg (mail->last_read--);
	return (RD_FAIL);		/* bad format */
    }
    mail->msgs[num].keywords = nil;
    if (!babyl_flags(mail,header,&mail->msgs[num])) {
	free(header);
	fail_msg (mail->last_read--);
	return(RD_FAIL);
    }
    free(header);
					/* now get the rest of the message */
    buflen = 0;
    text = cp = malloc (buflen += 100);	/* get some space for body of msg */
    space = buflen;			/* all empty so far */
    eof = false;			/* not the end yet */
    
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
	while (index (cp, '\n') == NULL) {
	    text = (char *) realloc (text, buflen +=100);
	    space = 101;		/* used the rest (but for '\0') */
	    cp = &text[buflen-space];	/* room to complete line */
	    if (fgets (cp, space, mail->filep) == NULL) {
	        eof = true;		/* end of file */
		break;			/* out of while */
	    }
	}
					/* end of first header? */
	if (eof)
	    continue;
	if (text[line] == MSGEND)
	    continue;

	if (!seen_eooh && (ustrncmp (&text[line], EOOH, strlen(EOOH)) == 0)) {
	    seen_eooh = true;
	    if (which_headers == 1) {
		text[line] = '\0';		/* forget this line */
		space = space + (cp - &text[line]);	/* reclaim that */
		cp = &text[line];		/* back to beginning */
		while(true) {
		    header = fgetline(mail->filep);
		    if (strlen(header) == 0) {
			free(header);
			break;
		    }
		    free(header);
		}
	    }
	    else {
		text[line] = '\0';		/* forget this line */
		space = space + (cp - &text[line]);	/* reclaim that */
		cp = &text[line];		/* back to beginning */
/* 		free(header); */
		continue;
	    }
	}
	else if (!seen_eooh && which_headers == 2) {
	    text[line] = '\0';		/* forget this line */
	    space = space + (cp - &text[line]);	/* reclaim that */
	    cp = &text[line];		/* back to beginning */
/* 	    free(header); */
	    continue;
	}
	else {
	    while (*cp)			/* get to null */
	        cp++;			/* keep going */
	    space = buflen - (cp - text); /* what we haven't used */
	}
    } while (!eof && (text[line] != MSGEND));

    if (!eof) {
	cache = (char *)safe_strcpy(&text[line]);
	if (text[line] == MSGEND)
	    text[line] = '\0';		/* throw it out */
	text = (char *) realloc (text, (size = line) +1); /* snug to fit */
    }
    else {
	if (text[line] == MSGEND)
	    text[line] = '\0';		/* throw it out */
        text = (char *) realloc (text, (size = (strlen (text))) +1);
    }


    m = &mail->msgs[num];
    cp = htext("date", text);
    if (cp) {
	m->date = babyl_date(cp);
	free(cp);
    }
    else 
	m->date = 0;
    if (m->date == 0)
        fprintf (stderr,"Message %d has bad date string (continuing)\n", num);

    m->from = NULL;
    m->size = size;
    m->text = text;
    set_msg_keywords(m);
    return (RD_OK);			/* got the message! */
}


/*
 * babyl_wrmsg:
 * see rd.h comments about wr_msg
 * Each babyl format message starts with a special MSGBEG character.
 * The next line has a "0" to signify that what comes next is
 * unprocessed (as far as true babyl is concerned) headers, with any
 * keywords and flags (as text strings) on the same line as the "0".
 *
 * Next is a "Summary-line:" header if that was in the text, then the
 * special EOOH (End Of, Oh... Headers) string, then the entire text of
 * the message (except for the "Summary-line:"). 
 *
 * Last comes the special MSGEND character.
 *
 */
babyl_wrmsg(mail,msg,num,flags)
msgvec *mail;
message *msg;
int num;
int flags;
{
  int err;
  char *text;
  char *htext();
  struct flagword *fl;
  char **k;
  char *sl, *sl_end;

  if (!(flags & (WR_SAVE | WR_COPY))) {	/* not a save or copy */
      mail->flags |= MF_DIRTY;		/* mark it to be saved */
      return (false);			/* no error */
  }
  if ((msg->flags & M_DELETED) && (flags & WR_EXP)) {
      if (num == mail->count)
	  local_get_size(mail);
      return (false);			/* don't save deleted messages */
  }

  fprintf(mail->filep, "%c\n", MSGBEG);
  fprintf(mail->filep,"0,");

  msg->flags ^= M_SEEN;			/* this one works backwards */
  for(fl = flagnames; fl->name != NULL; fl++) {
      if (msg->flags & fl->flag)
#ifdef BROKEN_BABYL
	  fprintf(mail->filep, "%s,", fl->name);
#else
	  fprintf(mail->filep, " %s,", fl->name); /* space before keyword */
#endif /* BROKEN_BABYL */
  }
  msg->flags ^= M_SEEN;			/* this one works backwards */

  fputc(',', mail->filep);
  for(fl = keynames; fl->name != NULL; fl++) {
      if (msg->flags & fl->flag)
#ifdef BROKEN_BABYL
	  fprintf(mail->filep, "%s,", fl->name);
#else
	  fprintf(mail->filep, " %s,", fl->name); /* space */
#endif /* BROKEN_BABYL */
  }
  for(k = msg->keywords; k && *k; k++)
#ifdef BROKEN_BABYL
      fprintf(mail->filep,"%s,",*k);
#else
      fprintf(mail->filep," %s,",*k);	/* space */
#endif /* BROKEN_BABYL */
  fputc('\n', mail->filep);
  text = msg->text;
  if ((sl = (char *) hfind("summary-line",text)) != NULL) {
      sl_end = index(sl,'\n');
      if (sl_end) *sl_end = '\0';
      fprintf(mail->filep,"%s\n", sl);
      if (sl_end) *sl_end = '\n';
  }
  fprintf(mail->filep,"%s\n",EOOH);
  while(true) {
      char *cp;
      if (text == sl)			/* don't print summary-line */
	  text = sl_end+1;
      cp = index(text,'\n');
      if (cp) {
	  *cp = '\0';
	  fprintf(mail->filep,"%s\n",text);
	  if (strlen(text) == 0) {
	      *cp = '\n';
	      text = cp+1;
	      break;
	  }
	  *cp = '\n';
	  text = cp+1;
      }
      else {
	  fprintf(mail->filep,"%s",text);
	  break;
      }
  }

  fwrite(text, sizeof(char), strlen(text), mail->filep);
  fputc(MSGEND, mail->filep);
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
 * probe file to see if it's in babyl format
 * babyl format files are distinguished by starting with "BABYL OPTIONS"
 */
babyl_probe (file)
char *file;
{
    FILE *fp;
    char p[keysize +1];		/* enough to probe it, with null */
    int i;
    int ret;

    if (cf && same_file(cf->filename, file)) {
	if (cf->type == TYPE_BABYL) 
	    return (PR_OK);
	else
	    return (PR_NOTOK);
    }
    if ((ret = local_contig_proben (file, &fp)) != PR_OK)
	return (ret);			/* couldn't open it */
    i = fread (p, sizeof (char), keysize, fp);
    fclose (fp);			/* okay, we're done looking */
    if (i == 0)
	return (PR_EMPTY);		/* nothing in it */
    if (i < keysize) 
        return (PR_NOTOK);		/* not enough characters */
    p[i] = '\0';			/* finish the string */
    if (strcmp (p, key) != 0)
        return (PR_NOTOK);		/* doesn't look right */
    return (PR_OK);			/* passed! */
}


babyl_flags(mail,str, msg)
char *str;
message *msg;
msgvec *mail;
{
    int num;
    u_long f;
    char *ptr;
    keylist add_keyword();
    keylist kw;
    u_long flags;
    int keycount;
    
    keycount = 0;
    flags=0;
    kw = NULL;
    if (!(ptr = index(str, ',')))
	return(false);
    *ptr = '\0';
    num = atoi(str);
    if (num == 0 && str[0] != '0')
	return(false);
    *ptr = ',';
    str = ptr + 1;

    while(ptr = index(str,',')) {
	if (*str != ',') {
	    *ptr = '\0';
	    if (*str == ' ')		/* ignore leading space */
	        str++;			/*   see BROKEN_BABYL stuff */
	    f = flaglookup(str,flagnames);
	    if (f == 0)
		f = flaglookup(str,keynames);
	    if (f == 0) {
		kw = add_keyword(str,kw);
		mail->keywords = add_keyword(str,mail->keywords);
	    }
	    else
		flags |= f;
	    *ptr = ',';
	}
	str = ptr + 1;
    }
    msg->flags = flags ^ M_SEEN;	/* this one's backwards */
    msg->keybits = 0;			/* no keybits in babyl format */
    msg->keywords = kw;
    return(true);
}

u_long
flaglookup(str,table) 
char *str;
struct flagword *table;
{
    while(table->name) {
	if (ustrcmp(table->name, str) == 0)
	    return(table->flag);
	table++;
    }
    return(0);
}

/*
 * babyl_banner:
 * skip past the banner, and return a pointer to the MSGEND MSGBEG line
 * before the first message
 * return NULL if something goes wrong...
 */
char *
babyl_banner(mail) 
msgvec *mail;
{
    char *h;
    rewind(mail->filep);
    h = fgetline(mail->filep);
    if (strcmp(h,key) != 0) {
	free(h);
	return(NULL);
    }
    while(true) {
	free(h);
	h = fgetline(mail->filep);
	if (h == NULL) 
	    return(NULL);
	if (h[0] == MSGEND)
	    return(h);
    }
}
