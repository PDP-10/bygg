/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/edit.c,v 2.2 90/10/04 18:24:03 melissa Exp $";
#endif

/*
 * edit.c:
 * edit message in mail file, edit outgoing message, ^E action in
 * paragraph parse to edit outgoing message being composed.
 */

#include "mm.h"
#include "ccmd.h"
#include "rd.h"
#include "message.h"

#define EDIT_HEADERS	0x1
#define EDIT_TEXT	0x2
#define EDIT_MESSAGE	0x4

/* prefix for creating temp files */
#ifndef HAVE_FLEXFILENAMES
#define PRE_OUTGOING ".mm-o"
#define PRE_INREPLY  ".mm-i"
#define PRE_HEADERS  ".mm-h"
#define PRE_MESSAGE  ".mm-m"
#define PRE_COMMAND  ".mm-c"
#define PRE_SPELL    ".mm-s"
#else
#define PRE_OUTGOING ".mm-outgoing."
#define PRE_INREPLY  ".mm-in-reply-to."
#define PRE_HEADERS  ".mm-headers."
#define PRE_MESSAGE  ".mm-message."
#define PRE_COMMAND  ".mm-command."
#define PRE_SPELL    ".mm-spell."
#endif

/* type of backup file to remove */
#define GEMACS	1
#define SPELL	2

#define purge(tf) { if (tf) { unlink(tf); free(tf); tf=NULL; } }

char *mktempfile();			/* create a temporary file */
char *write_to_temp();			/* write string to tempfile */
char *read_from_temp();			/* read tempfile into string */

static char *msgfile = NULL;		/* msg being replied to's temp  */
					/* file name */
static char *outfile = NULL;		/* outgoing msg's temp file */
static char *hfile = NULL;		/* header temp file */
static char *cmdfile = NULL;		/* cmd file */


/*
 * CMD_EDIT:
 */

cmd_edit (n)
int n;
{
  if (mode & MM_SEND) {
    edit_outgoing (TRUE);
  }
  else {
    edit_sequence();
  }
}

/**********************************************************************/

/*
 * EDIT_SEQUENCE:
 */

edit_sequence () {
  int do_edit_seqmsg();

  if (!check_cf (O_RDWR))		/* pre-check file existence */
    return;
  parse_sequence ("current",NULL,NULL);	/* parse the message sequence */
  if (!check_cf (O_WRONLY))		/* check writable after they CR */
    return;
  if (editor == NULL) {
    fprintf (stderr, "\
Cannot edit.  The editor variable is not set.  Use the SET EDITOR command\n\
to set it first.\n");
    return;
  }
  if (gnuemacs_mmail) {
    if (mmail_path[0] == '\0') {
      fprintf (stderr, "\
Cannot edit.  You are attempting to use gnuemacs mmail mode, but the\n\
mmail-path variable is not set.  Use the SET MMAIL-PATH command to set\n\
it first.\n");
      return;
    }
  }
  sequence_loop (do_edit_seqmsg);	/* run run the edit command over it */
}


/*
 * DO_EDIT_SEQMSG:
 */

do_edit_seqmsg(n) 
int n; 
{
  message *m;
  char *fname, *mktempfile();
  int fd;
  FILE *fp;
  char **editargv;
  char *newtext = NULL;
  keylist free_keylist();
  int count, i;

  if (n <= 0)
    return (true);			/* no initializing or ending */

  m = &cf->msgs[n];			/* the message we are editting */

  if ((fname = write_to_temp (PRE_MESSAGE, FALSE, m->text)) == NULL)
    return;				/* couldn't write to temp file */

  for (count = 0; editor[count] != NULL; count++)
    ;
  if (gnuemacs_mmail) {			/* write out command file */
    cmdfile = mktempfile (PRE_COMMAND, TRUE);
    if ((fd = open(cmdfile, O_WRONLY|O_CREAT, 0700)) < 0) {
      fprintf (stderr, "Trouble creating tempfile\n");
      purge(fname);
      purge(cmdfile);
      return;
    }
    else {
      fp = fdopen (fd, "w");
      fprintf (fp, "nil\nnil\nnil\n%s\n", fname);
      fclose (fp);
    }
    
    editargv = (char **) malloc ((count+4)*sizeof (char *));
    for (i = 0; i < count; i++)
      editargv[i] = editor[i];
    editargv[count++] = cmdfile;
    editargv[count++] = "-l";
    editargv[count++] = mmail_path;
    editargv[count++] = 0;
  }
  else {				/* not gnuemacs_mmail */
    editargv = (char **) malloc ((count+2)*sizeof (char *));
    for (i = 0; i < count; i++)
      editargv[i] = editor[i];
    editargv[count++] = fname;
    editargv[count++] = 0;
  }

  if (mm_execute (editor[0], editargv) != 0) {
    fprintf (stderr, "Edit failed\n");
    purge(fname);
    purge (cmdfile);
    free (editargv);
    return;
  }
  purge(cmdfile);
  free (editargv);

  if ((newtext = read_from_temp(fname)) == NULL) {
    purge(fname);
    return;
  }
  if (gnuemacs_mmail)
      maybe_remove_backups (fname, GEMACS);
  purge(fname);
  free (m->text);
  m->text = newtext;
  m->keywords = free_keylist(m->keywords);
  get_incoming_keywords(cf, m);
  m->size = strlen(m->text);		/* update the message length */
  if (m->hdrsum) {
    free (m->hdrsum);
    m->hdrsum = NULL;			/* cached header may be wrong */
  }
  m->flags |= M_EDITED|M_SEEN;		/* message has been edited */
  (*msg_ops[cf->type].wr_msg) (cf, m, n, 0); /* write it out (mark as dirty) */
}


/**********************************************************************/

/*
 * EDIT_OUTGOING:
 */

edit_outgoing (p) int p; {
  static keywrd editkeys[] = {
    { "all",	0,	(keyval) EDIT_HEADERS|EDIT_TEXT|EDIT_MESSAGE },
    { "headers",0,	(keyval) EDIT_HEADERS },
    { "text",	0,	(keyval) EDIT_TEXT },
  };
  static keytab edittab = { (sizeof(editkeys)/sizeof(keywrd)), editkeys };
  static fdb editfdb = { _CMKEY, 0, NULL, (pdat)&edittab, NULL, "all", NULL };
  pval parseval;
  fdb used;
  int what;

  if (p) {				/* should we parse? */
    parse (&editfdb, &parseval, &used);
    confirm();
    what = parseval._pvint;
  }
  else {
    what = EDIT_HEADERS|EDIT_TEXT|EDIT_MESSAGE;
  }

  if (editor == NULL) {
    fprintf (stderr, "\
Cannot edit.  The editor variable is not set.  Use the SET EDITOR command\n\
to set it first.\n");
    return;
  }

  if (gnuemacs_mmail) {
    if (mmail_path[0] == '\0') {
      fprintf (stderr, "\
Cannot edit.  You are attempting to use gnuemacs mmail mode, but the\n\
mmail-path variable is not set.  Use the SET MMAIL-PATH command to set\n\
it first.\n");
      return;
    }
    gnuemacs_edit_outgoing (what);
    return;
  }
  else
    other_edit_outgoing (what);
}


/*
 * GNUEMACS_EDIT_OUTGOING:
 */

gnuemacs_edit_outgoing (what) int what; {
  mail_msg *outmsg, *get_outgoing();
  headers *h;
  int fd;
  FILE *fp;
  char **editargv;
  char *newoutmsg = NULL;		/* new outgoing msg */
  char *newheaders = NULL;		/* new headers from temp file */
  message tmessage;
  mail_msg *new_mail_msg = NULL, *parse_msg();
  int count, i;

  outmsg = get_outgoing();


  /* always write out the headers */
  h = outmsg->headers;
  hfile = mktempfile (PRE_HEADERS, FALSE);
  if ((fd = open(hfile, O_WRONLY|O_CREAT, 0700)) < 0) { /* open temp file */
    fprintf (stderr, "Trouble creating tempfile\n");
    clear_edit_tempfiles();
    return;
  }
  fp = fdopen (fd, "w");		/* get a FILE * for it */
  display_header (fp, outmsg, TRUE, FALSE); /* expand, not in sendmail mode */
  fclose (fp);

  if (what & EDIT_TEXT) {		/* write out outgoing message text */
    if ((outfile = write_to_temp (PRE_OUTGOING, FALSE, outmsg->body))
	== NULL) {
      clear_edit_tempfiles();
      return;
    }
  }

  if ((mode&MM_ANSWER) && (what&EDIT_MESSAGE)) { /* we are replying to a msg */
    if ((msgfile = write_to_temp (PRE_INREPLY, FALSE,
				  cf->msgs[cf->current].text)) == NULL) {
      clear_edit_tempfiles();
      return;
    }
  }

  /* write out command file for gnuemacs mode */
  cmdfile = mktempfile (PRE_COMMAND, TRUE);
  if ((fd = open(cmdfile, O_WRONLY|O_CREAT, 0700)) < 0) { /* open temp file */
    fprintf (stderr, "Trouble creating tempfile\n");
    clear_edit_tempfiles();
    return;
  }
  else {
    fp = fdopen (fd, "w");
    fprintf (fp, "%s\n", (what & EDIT_TEXT) ? outfile : "nil");
    fprintf (fp, "%s\n", 
	     ((mode&MM_ANSWER)&&(what&EDIT_MESSAGE)) ? msgfile : "nil");    
    fprintf (fp, "%s\n", (what & EDIT_HEADERS) ? hfile : "nil");
    fprintf (fp, "nil\n");
    fclose (fp);
  }

  for (count = 0; editor[count] != NULL; count++)
    ;
  editargv = (char **) malloc ((count+4)*sizeof(char *));
  for (i = 0; i < count; i++)
    editargv[i] = editor[i];
  editargv[count++] = cmdfile;
  editargv[count++] = "-l";
  editargv[count++] = mmail_path;
  editargv[count++] = 0;

  if (mm_execute (editor[0], editargv) != 0) {
    fprintf (stderr, "Edit failed\n");
    clear_edit_tempfiles();
    free (editargv);
    return;
  }
  free (editargv);

  if (outfile) {
    if ((newoutmsg = read_from_temp (outfile)) == NULL) {
      fprintf(stderr,"Message file was null...leaving temp files\n");
      /*      clear_edit_tempfiles(); */
      return;
    }
  }

  if ((newheaders = read_from_temp (hfile)) == NULL) {
      fprintf(stderr,"Header file was null...leaving temp files\n");
      /*    clear_edit_tempfiles();*/
    return;
  }

  tmessage.text = newheaders;		/* copy into temp message struct */
  tmessage.size = strlen(newheaders);	/* in order to call parse_msg */
  new_mail_msg = parse_msg (&tmessage);	/* parse the msg (header) */
  if (new_mail_msg->to)
      files_to_fcc(new_mail_msg->to->address, new_mail_msg);
  if (new_mail_msg->cc)
      files_to_fcc(new_mail_msg->cc->address, new_mail_msg);
  if (new_mail_msg->bcc)
      files_to_fcc(new_mail_msg->bcc->address, new_mail_msg);
  free (newheaders);

  if (newoutmsg)
    new_mail_msg->body = newoutmsg;
  else {
    new_mail_msg->body = (char *) malloc (strlen(outmsg->body)+1);
    strcpy (new_mail_msg->body, outmsg->body);
  }
  
  free_msg(outmsg);			/* to prevent mem leak */
  set_outgoing (new_mail_msg);
  if (hfile)
      maybe_remove_backups (hfile, GEMACS);
  if (outfile)
      maybe_remove_backups (outfile, GEMACS);
  if (msgfile)
      maybe_remove_backups (msgfile, GEMACS);
  if (strlen(new_mail_msg->body) < 10) {
      fprintf(stderr, "Message was short.  Not removing temp files\n");
  } else {
      clear_edit_tempfiles();
  }
}



/*
 * CLEAR_EDIT_TEMPFILES:
 * clean up when edit_outgoing fails for some reason.
 */


clear_edit_tempfiles() {
  purge(hfile);				/* header file */
  purge(outfile);			/* outgoing message file */;
  purge(msgfile);			/* and/or message file */
  purge(cmdfile);			/* gnuemacs cmd file */
}



/*
 * OTHER_EDIT_OUTGOING:
 */

other_edit_outgoing (what) int what; {
  char *tfile = NULL, *hfile = NULL;
  mail_msg *outmsg, *get_outgoing();
  char *newmsg, *headerstring;
  char **editargv;
  int fd;
  FILE *fp;
  headers *h;
  message tmessage;
  mail_msg *new_mail_msg = NULL, *parse_msg();
  int count, i;

  outmsg = get_outgoing();

  if (!(what & EDIT_HEADERS)) {		/* have to save headers */
    h = outmsg->headers;
    hfile = mktempfile (PRE_HEADERS, FALSE);
    if ((fd = open(hfile, O_WRONLY|O_CREAT, 0700)) < 0) { /* open temp file */
      fprintf (stderr, "Trouble creating tempfile\n");
      purge(hfile);
      return;
    }
    else {
      fp = fdopen (fd, "w");		/* get a FILE * for it */
      display_header (fp, outmsg, TRUE, FALSE); /* not in sendmail mode */
      fclose (fp);
    }
  }

  tfile = mktempfile (PRE_OUTGOING, FALSE);
  if ((fd = open(tfile, O_WRONLY|O_CREAT, 0700)) < 0) { /* open temp file */
    fprintf (stderr, "Trouble creating tempfile\n");
    purge(hfile);
    purge(tfile);
    return;
  }
  else {
    fp = fdopen (fd, "w");		/* get a FILE * for it */
    if (what & EDIT_HEADERS) {
      display_header (fp, outmsg, TRUE, FALSE); /* not in sendmail mode */
      if (what & EDIT_TEXT)		/* both HEADERS and TEXT */
	fputc ('\n', fp);		/* then separate them */
    }
    if (what & EDIT_TEXT)
      display_text (fp, outmsg);
    fclose (fp);
  }

  for (count = 0; editor[count] != NULL; count++)
    ;
  editargv = (char **) malloc ((count+2)*sizeof (char *));
  for (i = 0; i < count; i++)
    editargv[i] = editor[i];
  editargv[count++] = tfile;
  editargv[count++] = 0;

  if (mm_execute (editor[0], editargv) != 0) {
    fprintf (stderr, "Edit failed\n");
    purge(hfile);
    purge(tfile);
    free (editargv);
    return;
  }
  free (editargv);

  if ((newmsg = read_from_temp (tfile)) == NULL) {
    purge(hfile);
    purge(tfile);
    return;
  }

  if (hfile) {
    if ((headerstring = read_from_temp (hfile)) == NULL) {
      fprintf (stderr, "Error reading tempfile\n");
      purge(hfile);
      purge(tfile);
      return;
    }
    tmessage.size = strlen(headerstring)+strlen(newmsg);
    tmessage.text = (char *) malloc (tmessage.size+2);
    sprintf (tmessage.text, "%s\n%s", headerstring, newmsg);
    free (headerstring);
  }
  else {
    tmessage.size = strlen(newmsg);
    tmessage.text = (char *) malloc (tmessage.size+1);
    strcpy (tmessage.text, newmsg);
  }
  free (newmsg);
  new_mail_msg = parse_msg (&tmessage); /* parse the msg */
  free (tmessage.text);
  if (!(what & EDIT_TEXT)) {
    new_mail_msg->body = (char *) malloc (strlen(outmsg->body)+1);
    strcpy (new_mail_msg->body, outmsg->body);
  }
  
  free_msg(outmsg);			/* to prevent mem leak */
  set_outgoing (new_mail_msg);

  purge(hfile);
  purge(tfile);
}


/**********************************************************************/

cmd_save_draft (n)
int n;
{
  char *ofile, *parse_output_file();
  mail_msg *m, *get_outgoing();
  FILE *fp;

  noise ("in file");
  ofile = parse_output_file ("file name", NULL, false);
  confirm();
  if (access(ofile, F_OK) == 0) {	/* output file exists */
    cmxprintf ("%s exists, ", ofile);
    if (!yesno("overwrite? ", "no"))	/* shall we overwrite? */
      return;
  }
  m = get_outgoing();
  if ((fp = fopen (ofile, "w")) == NULL) {
    cmpemsg ("Trouble creating output file, draft not saved",CM_SDE);
    return;
  }
  display_header (fp, m, TRUE, FALSE);
  fputc ('\n', fp);
  display_text (fp, m);
  fclose (fp);
  free (ofile);
  printf ("Draft saved in %s\n", ofile);
}


cmd_restore_draft (n)
int n;
{
  char *text;				/* whole the message text */
  char *fname;				/* file name to read */
  char *parse_input_file();
  mail_msg *msg, *get_outgoing();	/* the parsed message. */
  message tmessage;
    
  noise ("from file");
  fname = parse_input_file( nil, nil, false); /* get the filename */
  confirm();
  if ((text = read_from_temp(fname)) == NULL) { /* read the file */
    free(fname);			/* failure, clean up */
    return;
  }
  free(fname);
  tmessage.text = text;
  tmessage.size = strlen(text);
  msg = parse_msg(&tmessage);		/* parse the message. */
  free(text);				/* throw away the buffer */
  set_outgoing(msg);			/* copy in */
  send_mode(get_outgoing());
}


/**********************************************************************/

/*
 * MKTEMPFILE:
 * create a temp file using the passed prefix and tacking on this
 * process's pid.
 * NOTE: It is assumed that the string representation of a process pid 
 * is at most 9 characters long.
 */

char *
mktempfile (pref, usetmp) char *pref; int usetmp; {
  char *name;
  char *dir, *get_default_temp_dir();

  if (usetmp)
    dir = TMPDIR;
  else
    dir = (temp_directory[0] != '\0') ? temp_directory 
                                      : get_default_temp_dir();
  name = (char *) malloc (strlen(dir) + 1 + strlen(pref)+10);
  sprintf (name, "%s/%s%d", dir, pref, PID);
  return (name);
}


/*
 * WRITE_TO_TEMP:
 * open a temp file, place string in file, close temp file, and return
 * the name of the tempfile.
 * Returns the temp filename upon success, NULL otherwise.
 * Note: the filename should be free'd by the caller when done.
 */

char *
write_to_temp(pref, usetmp, str) 
char *pref;
int usetmp;
char *str; 
{
  char *fname;				/* place to create file name */
  int fd;
  int len; 

  fname = mktempfile(pref, usetmp);	/* create a temp file */
  if ((fd = open(fname, O_WRONLY|O_CREAT, 0700)) < 0) { /* open temp file */
    fprintf (stderr, "Trouble creating tempfile\n");
    free (fname);
    fname = NULL;
    return (NULL);
  }
  if (str != NULL) {
      len = strlen(str);
      if (write(fd, str, len) != len) {
	  fprintf (stderr, "Trouble writing to tempfile\n");
	  close (fd);
	  purge(fname);
	  return (NULL);
      }
  }
  close (fd);
  return (fname);
}


/*
 * READ_FROM_TEMP:
 * read temp file into string.  
 * Returns pointer to string on success and NULL on failure.
 * Note: Caller should free the returned string.
 */

char *
read_from_temp (fname) char *fname; {
  struct stat sbuf;
  int fd;
  char *newtext;
  int len, clen, r;

  if ((fd = open(fname, O_RDONLY,0)) < 0) { /* open file for read */
    fprintf (stderr, "Could not open temporary file\n");
    return (NULL);
  }
  fsync(fd);				/* some truncation problems */

  len = 0;
  clen = 0;
  newtext = (char *) calloc(clen+=BUFSIZ, 1);
  while (1) {
      if (clen == len)
	  newtext = (char *) realloc(newtext, clen*=2);
      r = read(fd, newtext+len, clen-len);
      if (r == 0)
	  break;
      else if (r == -1) {
	  perror("read");
	  free(newtext);
	  return NULL;
      } else {
	  len+=r;
      }      
  } 
  newtext = realloc(newtext, len+2);
  close (fd);
  if (newtext[len] != '\n')
      newtext[len++] = '\n';
  newtext[len] = '\0';		/* null terminate the text */
  return (newtext);
}


char *spell_text();
cmd_spell(n)
{
    char *ntext, *otext;
    mail_msg *m;

    if (mode & MM_SEND) {
	confirm();
	m = get_outgoing();
	otext = m->body;
	ntext = spell_text(otext);
	free(otext);
	m->body = ntext;
    }
    else {
	int spell_seq();
	if (!check_cf(O_RDWR))		/* pre-check we have a file */
	    return;
	parse_sequence ("current",NULL,NULL); /* parse the message sequence */
	if (!check_cf(O_WRONLY))	/* make sure we can write it */
	    return;
	sequence_loop (spell_seq);	/* run the spell command over it */
    }
}


spell_seq(n)
int n;
{
    char *otext, *ntext;
    message *m = &cf->msgs[n];
    if (n <= 0)
	return (true);			/* no initializing or ending */

    otext = m->text;
    ntext = spell_text(otext);
    free(otext);
    m->text = ntext;
    m->keywords = free_keylist(m->keywords);
    get_incoming_keywords(cf, m);
    m->size = strlen(m->text);		/* update the message length */
    if (m->hdrsum) {
	free (m->hdrsum);
	m->hdrsum = NULL;		/* cached header may be wrong */
    }
    (*msg_ops[cf->type].wr_msg) (cf, m, n, 0);/* write out (mark as dirty) */
}

char *
spell_text(txt)
char *txt;
{
    char *fname;
    char *ntxt;
    char **spellargv;
    int count, i;

    if (speller == NULL) {
        fprintf (stderr, "\
Cannot run speller.  The speller variable is not set.  Use the SET SPELLER\n\
command to set it first.\n");
	return (txt);
    }

    if ((fname = write_to_temp (PRE_SPELL, FALSE, txt)) == NULL) {
	fprintf(stderr,"Could not write spell file: %s\n", fname);
	return(txt);			/* couldn't write to temp file */
    }    
    
    for (count = 0; speller[count] != NULL; count++)
	;
    spellargv = (char **) malloc ((count+2)*sizeof(char *));
    for (i = 0; i < count; i++)
	spellargv[i] = speller[i];
    spellargv[count++] = fname;
    spellargv[count++] = 0;

    if (mm_execute (speller[0], spellargv) != 0) {
        fprintf (stderr, "Could not run speller\n");
	purge(fname);
	free (spellargv);
	return(txt);
    }
    free (spellargv);

    ntxt = read_from_temp(fname);
    if (ntxt == nil) {
	fprintf(stderr,"Could not read spell file: %s\n", fname);
	purge(fname);
	return(txt);
    }
    maybe_remove_backups (fname, SPELL);
    purge(fname);
    return(ntxt);
}


/*
 * maybe_remove_backups:
 * remove backup files created by editor, speller, etc.
 * name is the name of the file whose backups we are going to remove.
 * type is one of the following: GEMACS or SPELL.
 */

maybe_remove_backups (name, type)
char *name;
int type;
{
    static fdb backfilfdb = { _CMFIL, FIL_OLD|FIL_WLD, 
				  NULL, NULL, NULL, NULL, NULL };
    static fdb tfdb = { _CMTXT, 0, NULL, NULL, NULL, NULL, NULL };
    pval p;
    fdb *u;
    char *template;
    int plen;
    int i;

    /* XXX check a variable controlling removal of backups? */

    template = (char *) malloc (strlen(name)+10); /* sufficient space */
    switch (type) {
    case GEMACS:
	strcpy (template, name);
	strcat (template, "*~");
	break;
    case SPELL:
	strcpy (template, name);
	strcat (template, ".bak");
	break;
    default:
	return;
    }
    match (template, strlen(template), fdbchn(&backfilfdb,&tfdb,NULL), 
	   &p, &u, &plen);
    free (template);
    if (u == &tfdb)			/* did not get filenames */
	return;
    for (i = 0; p._pvfil[i] != NULL; i++)
	unlink(p._pvfil[i]);
}

