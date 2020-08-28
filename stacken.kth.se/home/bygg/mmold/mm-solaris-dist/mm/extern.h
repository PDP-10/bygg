/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifdef RCSID
#ifndef lint
static char *extern_rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/extern.h,v 2.3 90/10/04 18:24:11 melissa Exp $";
#endif
#endif /* RCSID */

/*
 * extern.h - miscellaneous external declarations
 *
 * see parse.h for other definitions used only by command-parsing routines
 */

/* globals declared in version.c */

extern char *mm_version;		/* program version */
extern char *mm_compiled;
extern int mm_major_version;
extern int mm_minor_version;
extern int mm_edit_number;
extern char *OStype;

/* global variables declared in mm.c */

extern int debug;			/* 1 if debugging */
extern int mode;			/* command mode */
extern char *progname;			/* pointer basename(argv[0]) */
extern msgvec *cf;			/* current mail file */
extern int minutes_west;

/* globals declared in init.c */

extern int PID;				/* process id */
extern int UID;				/* user id */
extern char *HOME;			/* user's home directory */
extern char *real_personal_name;	/* user's real name */

/* globals in compat.c */

extern char shorthostname[], fullhostname[], *mailhostname, *localdomain;

/* global variables declared in set.c */

extern
int	aliases_use_groups,
	auto_startup_get,
        autowrap_column,
        clear_screen,
	control_n_abort,
        control_d_automatic_send,
	control_e_editor,
	crt,
	directory_folders,
	display_flagged_messages,
  	display_outgoing_message,
	escape_automatic_send,
        fast_init_file,
  	gnuemacs_mmail,
  	list_include_headers,
	list_on_separate_pages,
    	modify_read_only,
	new_file_mode,
	prompt_for_bcc,
        reply_all,
        reply_include_me,
    	reply_initial_display,
	reply_insert,
  	send_verbose,
  	sendmail_verbose,
        suspend_on_exit,
        suspend_on_quit,
        terse_text_prompt,
	use_editor_always;
	
extern
string	crt_filter,
	header_options_file,
/*	keywords,*/
	mail_directory,
	mail_file,
        mmail_path,
	personal_name,
	print_filter,
	read_prompt,
	reply_indent,
	saved_messages_file,
	send_prompt,
	temp_directory,
	top_level_prompt,
	user_name;

extern
addresslist
	default_bcc_list,
	default_cc_list;

extern
keylist	dont_print_headers,
    	dont_type_headers,
	default_fcc_list,
	user_keywords,
    	only_print_headers,
	only_type_headers,
	user_headers, 
        incoming_mail,
	speller,
	editor;

/* miscellaneous C library routines/variables */

char *index (), *rindex (), *getenv (), *malloc ();

extern char *sys_errlist[];
extern int errno, sys_nerr;
long time ();

/* miscellaneous utility routines defined in mm modules */

char *errstr (), *ctad (), *hdate (), *cdate (), *fdate (), *daytime (),
    *skipheader (), *search (), *whoami (), *getlocalhostname ();
time_t itime (), stringtotime (), datimetogmt ();
msgvec *getfile ();

extern int allow_aborts;
extern Mail_aliases mail_aliases;
