/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/sendmail.c,v 2.2 90/10/04 18:26:27 melissa Exp $";
#endif

/**
 **  sendmail.c - mm interface to sendmail(8)
 **/

#include "mm.h"
#include "message.h"
#include "rd.h"
#include "parse.h"

extern int append_signature;		/* VAR_MAYBE */

#ifndef FALSE
#define FALSE (0)
#define TRUE  (!FALSE)
#endif 

#ifdef HAVE_FLEXFILENAMES
#define PRE_SENDMAIL ".mm-sendmail."
#define PRE_DEADLETTER ".mm-deadletter."
#else
#define PRE_SENDMAIL ".mm-sm"
#define PRE_DEADLETTER ".mm-dl"
#endif

#define INDENT 8			/* amount to indent when folding */
#define ALLOCINC 20			/* increment for realloc */

#define SIGNATURE ".signature"
#define sendmail_who_cmd "/usr/lib/sendmail -bv"

/*
 * sendmail:
 * accepts a mail_msg and forks up sendmail(8) and sends the message.
 */

sendmail (m)
mail_msg *m;
{
  FILE *tfp, *message_tempfile ();
  char **argv = NULL,			/* arguments to sendmail(8) */
       **Files = NULL;			/* file recipients */
  int i, pid, mailit, fileit, saveit, failed = FALSE;
  headers *new_header ();
  char *safe_strcpy ();
  char *signature = NULL, *maybe_append_signature();

  /*
   * Make sure the headers are complete.  This is probably the wrong
   * place to do it, but...
   */

  if (!m->date) {
      headers *h = m->headers, *t = m->last, *x;
      m->date = x = new_header (DATE, "Date", HEAD_KNOWN, m);
      x->string = safe_strcpy (rfctime ((time_t *)0));
      if (t) {
	  /*
	   * XXX kludge to make Date: header appear first.
	   */
	  t->next = x->next;		/* unlink new header */
	  m->last = t;
	  x->next = h;			/* cons it to front of list */
	  m->headers = x;
      }
  }
  if (!m->message_id) {
      char temp[256];
      extern int mm_patch_level;

      sprintf (temp, "<CMM.%d.%d.%d.%ld.%s@%s>",
	       mm_major_version, mm_minor_version, mm_patch_level, time(0), 
	       user_name, fullhostname);
      m->message_id = new_header (MESSAGE_ID, "Message-ID", HEAD_KNOWN, m);
      m->message_id->string = safe_strcpy (temp);
  }

  /* build recipient lists */
  mailit = build_recipients (&argv, &Files, m);
  /* do we need to file this? */
  saveit = *saved_messages_file &&
      strcmp (saved_messages_file, "/dev/null") != 0;
  fileit = Files || saveit;
  signature = maybe_append_signature();

  /*
   * Remind the user of who's on the recipient list
   */
  if (send_verbose && !sendmail_verbose) {
    int i;

    for (i = 1; argv[i]; i++)
	if (argv[i][0] != '-')
	    break;

    while (argv[i] != NULL) {
      printf ("%s... Queued\n", argv[i]);
      i++;
    }
  }

  if (!sendmail_verbose && !send_verbose) {
    printf("Sending... "); fflush(stdout);
  }

  /*
   * Fire up sendmail for non-file recipients
   */
  if (mailit) {
      extern int sendmail_background;

      tfp = message_tempfile (m, signature, true);
      if (tfp) {
	  
	  fix_signals_for_fork (true);	/* reset SIGCHLD */
	  
	  pid = vfork ();
	  if (pid == 0) {
	      int n;
	      
	      (void) close (0);
	      (void) dup (fileno (tfp));
	      
	      for (n = 3; n < 20; ++n)
		  (void) close (n);
	      
	      new_process_group ();
	      
	      (void) execv (SENDMAIL, argv);
	      _exit (errno);
	  }
	  if (pid > 0)
	      if (sendmail_background)
		  maybe_wait_for_process (pid);
	      else
		  wait_for_process (pid);
	  else
	      fprintf (stderr, "?Could not invoke %s: %s",
		       SENDMAIL, errstr (errno));
	  
	  fix_signals_for_fork (false);	/* reenable SIGCHLD handler */
	  
	  fclose (tfp);
      }
      else
	  failed = true;
  }

  /*
   * Deliver the message to any specified files
   */
  if (fileit) {
      tfp = message_tempfile (m, NULL, false);

      if (tfp) {
	  if (Files != NULL) {
	      if (!deliver_to_files(tfp, m, Files))
		  failed = true;
	      rewind(tfp);
	  }

	  if (saveit) {
	      char *av[2];
	      av[0] = saved_messages_file;
	      av[1] = nil;

	      if (!deliver_to_files (tfp, m, av))
		  failed = true;
	  }
	  fclose (tfp);
      }
      else
	  failed = true;
  }

  if (!sendmail_verbose && !send_verbose && !failed)
      printf ("Done.\n");

  if (argv) {
      for (i = 0; argv[i]; i++)
	  free (argv[i]);
      free (argv);
  }
  if (signature)
      free (signature);
  return (!failed);
}

    


/*
 * write_addresses:
 * write the header name (i.e "To:", "Cc:") followed by a folded list of 
 * recipients to the temp file
 */

write_addresses (tfp, h, expand) 
FILE *tfp;
headers *h;
int expand;
{
  int c = 0;				/* count what column we are up to */
  addresslist *a = h->address;		/* point at addresses */
  addr_unit *oau,*au = a->first;	/* point at first address unit */
  
  char *addr;				/* the address string */
  int i;
  addr_unit *addr_to_str();
  int first = TRUE;			/* true if this is the first address */
  int wrote_header = FALSE;

  
  if (au) {				/* we have an address unit */
    fprintf (tfp, "%s:", h->name);	/* write header name */
    c += strlen(h->name) + 1;		/* update column count */
    wrote_header = TRUE;
  }

  while (au) {
    oau = au;
    au = addr_to_str (au, &addr, expand); /* get next address string and */
					/* update au to next address unit */

    if (addr) {
	if (!first &&			/* if not first address */
	    !(oau->prev->type == ADR_GROUP ||
	      oau->prev->type == ADR_AL_EXPAND ||
	      oau->type == ADR_GROUPEND))
	{
	    fputc (',', tfp);		/* put a ',' at end of the prev addr */
	    c++;
	}
	else 
	    first = FALSE;
					/* >>> use term width instead <<< */
	if (c + (i = strlen(addr)) + 1 > 78) {
	    fprintf (tfp, "\n%*s", INDENT, ""); /* end this line */
	    c = INDENT;			/* reset to beginning of line */
					/* (plus indentation) */
	}

	fprintf (tfp, " %s", addr);	/* display the address */
	c += i+1;			/* increment by address length */
    }
  }
  if (wrote_header)
    fputc('\n', tfp);			/* terminate with new line */
}


/*
 * addr_to_str:
 * Address takes a pointer to an address unit, and a pointer to a char
 * pointer.  It creates a string containing the address as it should be 
 * written in the header and sets addr to point to it.
 * It returns an updated address unit pointer (skips past a group end
 * if the au was pointing at a group head originally).
 */

addr_unit *
addr_to_str (au, addr, expand)
addr_unit *au;
char **addr; 
int expand;
{
  static int saw_alias = false;
  switch (au->type) {
  case ADR_ADDRESS:			/* simple address */
    *addr = (char *) malloc(strlen(au->data)+1);/* allocate space for addr */
    strcpy (*addr, au->data);		/* copy the address string */
    au = au->next;			/* move on to the next address unit */
    return (au);
  case ADR_GROUP:			/* address group */
    *addr = (char *) malloc (strlen(au->data)+4); /* space for group name */
					/* and ": ;" */
    if (!expand) {
	sprintf (*addr, "%s: ;", au->data);	/* write group name */
	while (au->next && au->next->type != ADR_GROUPEND)
	    au = au->next;			/* skip all group members */
	au = au->next->next;		/* skip over group end */
	return (au);
    }
    sprintf(*addr, "%s: ", au->data);
    return(au->next);
  case ADR_GROUPEND:
    if (saw_alias && !expand) {
	saw_alias = false;
	*addr = NULL;
	return(au->next);
    }
    if (!expand) {
	fprintf (stderr, "address: Found unexpected ADR_GROUPEND\n");
	return (NULL);
    }
    *addr = (char *) malloc(strlen(";") + 1);
    strcpy(*addr, ";");
    return(au->next);
 case ADR_AL_EXPAND:
    if (!expand) {
	return(au->next);
    }
    else {
	*addr = (char *) malloc (strlen(au->data)+3);/* space for group name */
					/* and ": " */
	sprintf(*addr, "%s: ", au->data);
	saw_alias = true;
	return(au->next);
    }
  case ADR_FILE:
    *addr = (char *) malloc(strlen(au->data)+2);
    sprintf (*addr, "*%s", au->data);
    au = au->next;
    return (au);
  default:
    fprintf (stderr, "address: bogus type\n");
    return (NULL);
  }
}


/*
 * write_sender:
 * add a sender field of the form:
 * Sender: Personal Name <username@hostname>
 */

write_sender(tfp) FILE *tfp; {
  char *sender, *create_sender();

  sender = create_sender();		/* create the sender header */
  fprintf (tfp, "Sender: %s\n", sender); /* write out the sender */
}

/*
 * create_sender:
 * create the sender field
 */

char *
create_sender()
{
  struct passwd *p, *getpwuid();
  static char buf[256];			/* this should be large enough */
					/* (famous last words) */
  if (real_personal_name != NULL)
    sprintf (buf, "%s <%s@%s>", real_personal_name, user_name, 
	     fullhostname);
  else
    sprintf (buf, "%s@%s", user_name, fullhostname);
  return (buf);
}



/*
 * build_recipients:
 * Create an argv for sendmail(8) and a list of file recipients
 * from the mail msg passed.
 * Arguments:
 *	char ***argvp: Pointer to argv (char **)
 *	char ***Filesp: Pointer to array of file names
 *	mail_msg *m: The message we are sending
 * Returns:
 *	Nothing.  Always returns.
 */

build_recipients
(argvp, Filesp, m)
char ***argvp, ***Filesp;
mail_msg *m;
{
  int argcount = 0;			/* argument count */
  int filecount = 0;			/* file recipient count */
  char **add_string();			/* add a string to the argv */
  int i;
  int smargs;

  *argvp = add_string (*argvp, &argcount, SENDMAIL); /* argv[0] prog name */
  i = 0;

  /*
   * Tell sendmail not to treat "." by itself as EOF
   */
  *argvp = add_string (*argvp, &argcount, "-oi");

  /*
   * include ourself in any alias expansion
   */
  *argvp = add_string (*argvp, &argcount, "-om");

  /*
   * If sendmail-verbose is true, add the debugging option
   */
  if (sendmail_verbose)
    *argvp = add_string (*argvp, &argcount, "-v");

  smargs = argcount;			/* remember number of args */
  if (m->resent_to) {			/* the Resent-To: list */
    add_recipients (argvp, Filesp, &argcount, &filecount, m->resent_to); 
  } else {
    if (m->to)				/* the To: list */
      add_recipients (argvp, Filesp, &argcount, &filecount, m->to); 
    if (m->cc)				/* the Cc: list */
      add_recipients (argvp, Filesp, &argcount, &filecount, m->cc); 
    if (m->bcc)				/* the Bcc: list */
      add_recipients (argvp, Filesp, &argcount, &filecount, m->bcc); 
    if (m->fcc)				/* the Bcc: list */
      add_recipients (argvp, Filesp, &argcount, &filecount, m->fcc); 
  }

  if (argcount % ALLOCINC == 0)		/* no room for null */
    *argvp = (char **) safe_realloc (*argvp, /* get more space */
				  (argcount+1)*sizeof(char *)); 
  (*argvp)[argcount] = NULL;		/* tie off with a null */
  if (*Filesp) {
    if (filecount % ALLOCINC == 0)	/* no room for null */
      *Filesp = (char **) safe_realloc (*Filesp, /* get more space */
				     (filecount+1)*sizeof(char *)); 
    (*Filesp)[filecount] = NULL;	/* tie off with a null */
  }

  return (argcount != smargs);		/* did we have recipients? */
}


/*
 * add_string:
 * Add a string to the argv array and update the count c.
 * Returns a possibly updated char **argv.
 */

char **
add_string (argv, c, s) char **argv; int *c; char *s; {
  if (*c % ALLOCINC == 0)		/* need more room */
    argv = (char **) safe_realloc (argv,	/* get more space */
				(*c+ALLOCINC)*sizeof(char *)); 
  argv[*c] = (char *) malloc (strlen(s)+1); /* space for the string */
  strcpy (argv[*c], s);			/* copy the string into arg list */
  (*c)++;				/* increment arg count */
  return (argv);
}


/*
 * add_recipients:
 * Add recipients in header h to argv and update the count c
 * Returns a possibly different char **argv (if it needs to realloc 
 * more for more arguments).
 */

add_recipients (argvp, Filesp, argcount, filecount, h) 
char ***argvp, ***Filesp; 
int *argcount, *filecount; headers *h; 
{
  addresslist *a = h->address;		/* point at addresses */
  addr_unit *au = a->first;		/* point at first address unit */

  while (au) {				/* step through all addr_units */
    switch (au->type) {
    case ADR_ADDRESS:			/* real address */
      if (*argcount % ALLOCINC == 0)	/* need more room */
	*argvp = (char **) safe_realloc (*argvp, 
				      (*argcount+ALLOCINC)*sizeof(char *));

      (*argvp)[*argcount] = (char *) malloc (strlen(au->data)+1);
      if (au->data[0] == '\\')
	strcpy ((*argvp)[*argcount], au->data+1); /* copy addr into arg list */
      else
	strcpy ((*argvp)[*argcount], au->data);
      (*argcount)++;			/* increment arg count */
      break;
    case ADR_GROUP:			/* group head */
    case ADR_AL_EXPAND:
    case ADR_GROUPEND:			/* group end */
      break;				/* do nothing, skip over */
    case ADR_FILE:			/* recipient is file */
      if (*filecount % ALLOCINC == 0)	/* need more room */
	*Filesp = (char **) safe_realloc (*Filesp, 
				       (*filecount+ALLOCINC)*sizeof(char *));
      (*Filesp)[*filecount] = (char *) malloc (strlen(au->data)+1);
      strcpy ((*Filesp)[*filecount], au->data); /* copy fname into list */
      (*filecount)++;			/* increment file count */
      break;
    default:
      fprintf (stderr, "add_recipients: invalid addr_unit type\n");
      return;
    }
    au = au->next;			/* go on to the next addr_unit */
  }
}


/*
 * deliver_to_files:
 * deliver outgoing message to files by 'copying' the message
 * Note: The sendmail temp file is read into a string and a
 * message struct is created.  This message struct is copied to
 * all specified files.
 */

deliver_to_files (tfp, m, Files)
FILE *tfp;
mail_msg *m;
char **Files;
{
  int i = 0;
  struct stat sbuf;
  int fsize;
  char *msgtext;
  message msg;
  int failed = 0;

  if (fstat(fileno(tfp), &sbuf) != 0) {
    perror ("fstat");
    return (FALSE);
  }
  fsize = sbuf.st_size;			/* get file size */
  if ((msgtext = (char *) malloc (fsize+1)) == NULL) {
    fprintf (stderr, "?Out of memory\n");
    return (FALSE);
  }

  /* read temp file into string */
  if (fread(msgtext, sizeof(char), fsize, tfp) != fsize) {
    fprintf (stderr, "?Could not reread message\n");
    free (msgtext);
    return (FALSE);
  }
  msgtext[fsize] = '\0';		/* tie off with a null */

  /* fill in required parts of message struct */
  msg.text = msgtext;			/* of course! */
  time (&msg.date);			/* need a date */
  msg.size = strlen(msg.text);		/* message size */
  msg.from = NULL;			/* no from */
  msg.keywords = NULL;			/* init keywords */
  get_incoming_keywords(NULL, &msg);	/* fill in keywords */
  msg.keybits = 0;			/* no keybits set */
  msg.flags = 0;			/* no flags */

  while (Files[i] != NULL) {		/* for all recipient files */
    msgvec *df, *getdestfile();

    if (sendmail_verbose || send_verbose) {
      printf ("*%s... ", Files[i]); fflush(stdout);
    }
    if ((df = getdestfile(Files[i],-1,false)) == NULL) { /* open the file */
      failed++;
      if (sendmail_verbose || send_verbose) 
	printf ("Failed - file may be busy, or not in proper format\n");
    }
    else {
      (*msg_ops[df->type].wr_msg) (df, &msg, 0, WR_COPY); /* copy the msg */
      (*msg_ops[df->type].close) (df->filep); /* close the file */
      free (df);
      if (sendmail_verbose || send_verbose)
	printf ("Sent\n");
    }
    i++;				/* on to the next file */
  }
  if (failed) {				/* failed on delivery to a file */
    char dlname[MAXPATHLEN+1];
    msgvec *dlf, *getdestfile();
    
    sprintf (dlname, "%s/%s%d", HOME, PRE_DEADLETTER, PID);
    if ((dlf = getdestfile(dlname,-1,false)) == NULL) { /* open the file */
      if (sendmail_verbose || send_verbose) 
	printf ("Dead letter file may be busy, or not in proper format\n");
    }
    else {
      (*msg_ops[dlf->type].wr_msg) (dlf, &msg, 0, WR_COPY);
      (*msg_ops[dlf->type].close) (dlf->filep); /* close the file */
      free (dlf);
      printf ("Failed message saved in %s\n", dlname);
    }
  }
  return (TRUE);
}


/*
 * maybe_append_signature:
 * check variable append_signature, and existence of signature file
 * and possibly read it in, returning the text.
 */

char *
maybe_append_signature()
{
  char sigfile[MAXPATHLEN+1];
  struct stat sbuf;
  int sigsize;
  char *signature = NULL;
  int sfd;

  if (append_signature == SET_NO)	/* they don't want it */
    return (NULL);
  sprintf (sigfile, "%s/%s", HOME, SIGNATURE); /* create path */
  if (stat (sigfile, &sbuf) != 0) {	/* stat the file */
    if (errno != ENOENT)
      perror (sigfile);
    return (NULL);
  }
  if (append_signature == SET_ASK) {
    if (!yesno ("Append signature? ", "yes"))
      return (NULL);
  }
  /* if we get here, it was SET_ASK and they said YES or it was SET_YES */
  sigsize = sbuf.st_size;		/* get size of file */
  if ((signature = (char *) malloc (sigsize+2)) == NULL) { /* get mem */
    fprintf (stderr, "?out of memory trying to read %s\n", sigfile);
    return (NULL);
  }
  if ((sfd = open (sigfile, O_RDONLY, 0)) < 0) { /* open for read */
    perror (sigfile);
    free (signature);
    return (NULL);
  }
  if (read (sfd, signature, sigsize) != sigsize) {
    perror (sigfile);
    close (sfd);
    free (signature);
    return (NULL);
  }
  if ((sigsize > 0) && (signature[sigsize-1] != '\n')) {
    signature[sigsize++] = '\n';
  }
  signature[sigsize] = '\0';
  close (sfd);
  return (signature);
}


/*
 * valid_from_field:
 * check to see if the supplied string is a valid From: field.
 */ 

valid_from_field (str)
char *str;
{
    int result = true;
    char *cp, *sender, *from, *host, *create_sender();

    sender = create_sender();
    str = (char *) safe_strcpy (str);	/* our personal copy */

    if (ustrcmp (sender, str) == 0)	/* from is identical to sender  */
	goto accept;

    if ((from = index (str, '<')) == NULL) /* strip out up to '<' */
	from = str;
    else {
	from++;
	if ((cp = index (from, '>')) != NULL)
	    *cp = '\0';			/* chop of at '>' */
    }
    if ((host = index (from, '@')) != NULL)
	*host++ = '\0';			/* point at host string */

    if (ustrcmp (from, user_name) != 0)
	goto reject;			/* incorrect user name */

    if (host == NULL)
	goto accept;			/* right user, no host specified */

    if (ustrcmp (host, fullhostname) == 0 ||
	ustrcmp (host, shorthostname) == 0 ||
	ustrcmp (host, mailhostname) == 0)
	goto accept;

 reject:
  result = false;
 accept:
  free (str);
  return result;
}


/*
 * cmd_who:
 */

cmd_who(n)
int n;
{
  static fdb text_fdb = { _CMTXT, CM_SDH, nil, nil,
			      "recipient name, text string" };
  extern fdb aliasfdb;

  noise ("is");
  aliasfdb._cmdat = (pdat) mk_alias_keys();
  aliasfdb._cmdef = nil;
  parse (fdbchn(&aliasfdb, &text_fdb, nil), &pv, &used);

  if (used == &text_fdb)
    sendmail_who(atmbuf);
  else {				/* an alias */
    int i;

    i = pv._pvint;
    confirm();
    disp_alias(stdout, i, false, true);
  }
  return (true);
}


/*
 * sendmail_who:
 * run sendmail(8) to expand mail address 'str'
 */

sendmail_who (str) char *str; {
  FILE *pp, *mm_popen();
  char cmd[BUFSIZ];
  char buf[BUFSIZ];
  int len, choplen;
  
  if (strlen (str) == 0)
    cmerr ("No name to expand");
  sprintf (cmd, "%s %s", sendmail_who_cmd, 
	   (strcmp (str, ".") == 0) ? user_name : str);
  if ((pp = mm_popen (cmd, "r")) == NULL) { /* start up sendmail(8) */
    perror ("sendmail");
    return;
  }
  choplen = strlen("... deliverable");
  while (fgets(buf, BUFSIZ, pp) != NULL) { /* while we have output from */
    len = strlen(buf);			/* sendmail(8) */
    buf[--len] = '\0';			/* get rid of the newline */
    if (strcmp(&buf[len-choplen], "... deliverable") == 0)
    buf[len-choplen] = '\0';		/* chop of deliverable msg */
    printf ("%s\n", buf);
  }
  mm_pclose(pp);
}


/*
 * cmd_route:
 * set or unset mail routing (forwarding) for user.
 */

cmd_route (n) 
int n; 
{
#ifndef FORWARD_FILE
  cmerr ("The ROUTE command is not supported on this system");
#else
  char fname[MAXPATHLEN];
  char *text, *parse_text();
  char *cp, *index();
  FILE *fp;
  int err;
  extern int errno;

  noise ("mail to");
  /* XXX - should probably use the address parser for this */
  text = parse_text ("text string\n  or confirm to remove current routing", 
		     NULL);
  sprintf (fname, "%s/%s", HOME, FORWARD_FILE);
  if (text[0] == '\0') {		/* null text parsed */
    if ((unlink (fname) != 0) && (errno != ENOENT)) {
      perror (fname);
      return;
    }
    /*
     * Note this can be misleading--MM should simply call a separate
     * program, to allow other databases (e.g. /usr/lib/aliases) to
     * be updated and/or to determine whether some other form of mail
     * forwarding is in effect.
     */
    printf ("Mail forwarding for %s removed.\n", user_name);
    return;
  }

  if ((cp = index (text, '\n')) != NULL)
    *cp = '\0';
  if ((fp = fopen(fname, "w")) == NULL) {
    fprintf (stderr, "Could not open %s - mail forwarding not modified.\n", 
	     fname);
    return;
  }
  fwrite(text, sizeof(char), strlen(text), fp);
  fputc('\n', fp);
  err = ferror (fp);
  err |= (fclose (fp) == EOF);
  if (!err) {
    printf ("Mail for %s will be forwarded to %s\n", user_name, text);
    return;
  }
  fprintf (stderr, "?Error writing %s: %s\n", fname, errstr (errno));
  if (unlink (fname) != 0)
      fprintf (stderr, "Could not unlink %s: %s", fname, errstr (errno));
  return;
#endif
}

/*
 * show_route:
 * display current mail routing
 */

show_route (verbose) 
int verbose;
{
#ifdef FORWARD_FILE
  FILE *fp;
  struct stat sbuf;
  char buf[BUFSIZ];
  char fname[MAXPATHLEN];

  sprintf (fname, "%s/%s", HOME, FORWARD_FILE);
  fp = fopen (fname, "r");
  if (fp) {
      int c, lastchar;
      printf (" Mail for %s is forwarded to ", user_name);
      
      while ((c = getc (fp)) != EOF)
	  putchar (lastchar = c);
      if (lastchar == '\n')
	  fflush (stdout);
      else
	  printf ("\n");
      (void) fclose (fp);
  }
#endif /* FORWARD_FILE */
}

FILE *
message_tempfile (m, signature, for_sendmail)
msgvec *m;
char *signature;
int for_sendmail;
{
    char *mktempfile();
    char *tfile = mktempfile (PRE_SENDMAIL, TRUE);
    FILE *tfp;

    if ((tfp = fopen (tfile, "w+")) == NULL) {
	fprintf (stderr, "Cannot create temp file %s: %s\n",
		 tfile, errstr (errno));
	return 0;
    }
    if (chmod (tfile, 0600) != 0)
	fprintf (stderr, "Cannot change permissions of %s: %s\n",
		 tfile, errstr (errno));

    display_header (tfp, m, FALSE, for_sendmail);
    fputc ('\n', tfp);
    display_text (tfp, m);		/* write out the body of the message */
    if (signature) {
        fputc('\n', tfp);
	fwrite(signature, sizeof(char), strlen(signature), tfp);
    }
    fflush (tfp);
    rewind (tfp);			/* rewind to beginning of file */
    unlink (tfile);
    return tfp;
}
