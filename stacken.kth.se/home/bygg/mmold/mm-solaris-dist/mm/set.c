/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/set.c,v 2.9 90/10/04 18:26:40 melissa Exp $";
#endif

/*
 * set.c - support for the "set" command
 *
 * All the internal variables which can be explicitly changed by the
 * user are declared here, along with the code implementing the "set" 
 * command itself.
 */

#include "mm.h"
#include "parse.h"
#include "set.h"
#include "cmds.h"

#define CHAR_SQ '\''			/* \047 */
#define CHAR_BS '\\'			/* \0134 */

#define isquote(x) (x == CHAR_SQ)
#define isbackslash(x) (x == CHAR_BS)


extern char **parse_filelist();
char *show_filename();

int	aliases_use_groups = SET_NO,	/* aliases != groups */
        append_new_mail = SET_NO,       /* use old style fetchmail() */
        append_signature = SET_NO,	/* don't append .signature file */
	auto_create_files = SET_ASK,	/* ask about new files */
	auto_startup_get = SET_YES,	/* get your mailfile on startup */
        autowrap_column,		/* set up in initialize */
	browse_clear_screen = SET_YES,	/* clear before showing each header */
        browse_pause = SET_YES,		/* pause between messages on browse */
  	check_interval = 300,		/* interval for newmail alarm() */
        clear_screen = SET_YES,		/* clear screen by default */
        continuous_check = SET_NO,	/* check constantly for new mail */
        control_d_automatic_send = SET_NO, /* don't send automatically on ^D */
	control_e_editor = SET_YES,	/* ^E invokes editor */
    	control_l_confirm = SET_NO,	/* don't confirm on ^L */
	control_n_abort = SET_ASK,	/* ask about abort */
	directory_folders = SET_NO,	/* can folders be directories */
	display_flagged_messages = SET_YES, /* show flagged messages */
  	display_outgoing_message = SET_NO, /* show msg when prompting */
        escape_automatic_send = SET_NO,	/* send automatically on esc */
	expunge_on_bye = SET_ASK,	/* expunge when they say bye */
	fast_init_file = SET_YES,	/* use fast init file */
  	gnuemacs_mmail = SET_YES,	/* we have gnuemacs mmail mode */
    	handle_changed_modtime = SET_YES, /* recover from mtime change */
  	list_include_headers = SET_YES,	/* put index before messages */
  	list_on_separate_pages = SET_NO, /* don't put formfeed between msgs */
        modify_read_only = SET_YES,	/* don't ask whether to modify */
	new_file_mode = 0600,		/* default mode */
	prompt_for_bcc = SET_NO,	/* prompt for bcc on send */
	prompt_for_cc = SET_YES,	/* prompt for cc on send */
	prompt_for_fcc = SET_NO,	/* prompt for fcc on send */
        prompt_rcpt_always = SET_NO;	/* prompt even when "send foo" */
	reply_all = SET_NO,		/* don't reply to everyone */
        reply_include_me = SET_NO,	/* replies go to self too */
    	reply_initial_display = SET_NO,	/* display To, Cc, etc when replying */
	reply_insert = SET_NO,		/* don't include original in reply */
  	send_verbose = SET_YES,		/* show all recipients when sending */
  	sendmail_verbose = SET_NO,	/* don't be verbose when sending */
  	sendmail_background = SET_YES,	/* Send mail in the background */
        suspend_on_exit = SET_NO,	/* suspend when a quit is done */
        suspend_on_quit = SET_NO,	/* suspend when a quit is done */
        terse_text_prompt = SET_NO,	/* don't be terse when prompting */
	use_editor_always = SET_NO,	/* don't use the editor by default */
    	use_crt_filter_always = SET_NO;	/* don't always filter things */
	use_invalid_address = SET_ASK;	/* ask about invalid addresses */

string	crt_filter = { PAGER },
    	default_from = { NULL },
  	default_read_command = { "next" },
	default_reply_to = { NULL },
  	default_send_command = { NULL },
        finger_command = { "finger" },
  	header_options_file = { "/dev/null" },
  	help_file = { HELPFILE },
  	help_dir = { HELPDIR },
	mail_directory = { "." },
	mail_file = { NULL},
        mmail_path = { MMAIL_PATH },
  	movemail_path = { MOVEMAIL },
	personal_name = { 0 },
        print_filter = { "lpr" },
	read_prompt = { "R>" },
	reply_indent = { "> " },
	saved_messages_file = { "/dev/null" },
	send_prompt = { "S>" },
  	temp_directory = { NULL },	/* set in init.c:initialize() */
	top_level_prompt = { "MM>" },
        user_name = { NULL };

addresslist
	default_bcc_list = { nil, nil },
	default_cc_list = { nil, nil };

keylist	dont_print_headers = nil,
    	dont_type_headers = nil,
	user_keywords = nil,
    	only_print_headers = nil,
	only_type_headers = nil,
	user_headers = nil,
	incoming_mail = nil,
  	editor = NULL,
	default_fcc_list = nil,
        speller = NULL;



extern Mail_aliases mail_aliases;

keywrd user_levels[] = {
    { "novice", 0, NOVICE },
    { "expert", 0, EXPERT },
};

keytab user_levtab = { sizeof(user_levels) / sizeof(keywrd), user_levels };
fdb user_level_fdb = { _CMKEY, 0, nil, (pdat) &user_levtab, "user level" };

extern keytab send_keytab, read_keytab;
setkey default_mail_type = { &formattab, "mbox" },
       user_level = { &user_levtab, "expert" };


keywrd set_cmds[] = {
    { "aliases-use-groups",	0, (keyval) SET_ALIASES_USE_GROUPS },
    { "append-new-mail",        0, (keyval) SET_APPEND_NEW_MAIL },
    { "append-signature",	0, (keyval) SET_APPEND_SIGNATURE },
    { "auto-create-files",	0, (keyval) SET_AUTO_CREATE_FILES },
    { "auto-startup-get", 	0, (keyval) SET_AUTO_STARTUP_GET },
    { "autowrap-column", 	0, (keyval) SET_AUTOWRAP_COLUMN },
    { "browse-clear-screen",	0, (keyval) SET_BROWSE_CLEAR_SCREEN },
    { "browse-pause", 		0, (keyval) SET_BROWSE_PAUSE },
    { "check-interval",		0, (keyval) SET_CHECK_INTERVAL },
    { "clear-screen", 		0, (keyval) SET_CLEAR_SCREEN },
    { "continuous-check",	0, (keyval) SET_CONTINUOUS_CHECK },
    { "control-d-automatic-send", 0, (keyval) SET_CONTROL_D_AUTOMATIC_SEND },
    { "control-e-editor", 	0, (keyval) SET_CONTROL_E_EDITOR },
    { "control-l-confirm", 	0, (keyval) SET_CONTROL_L_CONFIRM },
    { "control-n-abort", 	0, (keyval) SET_CONTROL_N_ABORT },
    { "crt-filter", 		0, (keyval) SET_CRT_FILTER },
    { "default-bcc-list",	0, (keyval) SET_DEFAULT_BCC_LIST },
    { "default-cc-list",	0, (keyval) SET_DEFAULT_CC_LIST },
    { "default-fcc-list",	0, (keyval) SET_DEFAULT_FCC_LIST },
    { "default-from",		0, (keyval) SET_DEFAULT_FROM },
    { "default-mail-type",	0, (keyval) SET_DEFAULT_MAIL_TYPE },
    { "default-read-command",	0, (keyval) SET_DEFAULT_READ_COMMAND },
    { "default-reply-to",	0, (keyval) SET_DEFAULT_REPLY_TO },
    { "default-send-command",	0, (keyval) SET_DEFAULT_SEND_COMMAND },
    { "directory-folders",	0, (keyval) SET_DIRECTORY_FOLDERS },
    { "display-flagged-messages",0, (keyval) SET_DISPLAY_FLAGGED_MESSAGES },
    { "display-outgoing-message",0, (keyval) SET_DISPLAY_OUTGOING_MESSAGE },
    { "dont-print-headers",	0, (keyval) SET_DONT_PRINT_HEADERS },
    { "dont-type-headers", 	0, (keyval) SET_DONT_TYPE_HEADERS },
    { "editor",		 	0, (keyval) SET_EDITOR },
    { "escape-automatic-send",	0, (keyval) SET_ESCAPE_AUTOMATIC_SEND },
    { "expunge-on-bye",		0, (keyval) SET_EXPUNGE_ON_BYE },
    { "fast-init-file",		0, (keyval) SET_FAST_INIT_FILE },
    { "finger-command",		0, (keyval) SET_FINGER_COMMAND },
    { "gnuemacs-mmail",		0, (keyval) SET_GNUEMACS_MMAIL },
    { "handle-changed-modtime", KEY_INV, (keyval) SET_HANDLE_CHANGED_MODTIME },
    { "header-options-file", 	0, (keyval) SET_HEADER_OPTIONS_FILE },
    { "help-file",	  KEY_INV, (keyval) SET_HELP_FILE },
    { "help-dir",	  KEY_INV, (keyval) SET_HELP_DIR },
    { "incoming-mail", 		0, (keyval) SET_INCOMING_MAIL },
    { "keywords", 		0, (keyval) SET_KEYWORDS },
    { "list-include-headers",	0, (keyval) SET_LIST_INCLUDE_HEADERS },
    { "list-on-separate-pages", 0, (keyval) SET_LIST_ON_SEPARATE_PAGES },
    { "mail-aliases", 	  	0, (keyval) SET_MAIL_ALIASES },
    { "mail-directory", 	0, (keyval) SET_MAIL_DIRECTORY },
    { "mail-file", 		0, (keyval) SET_MAIL_FILE },
    { "mmail-path",	  KEY_INV, (keyval) SET_MMAIL_PATH },
    { "modify-read-only",	0, (keyval) SET_MODIFY_READ_ONLY },
    { "movemail-path",	  KEY_INV, (keyval) SET_MOVEMAIL_PATH },
    { "new-file-mode", 		0, (keyval) SET_NEW_FILE_MODE },
    { "only-print-headers",	0, (keyval) SET_ONLY_PRINT_HEADERS },
    { "only-type-headers", 	0, (keyval) SET_ONLY_TYPE_HEADERS },
    { "personal-name", 		0, (keyval) SET_PERSONAL_NAME },
    { "print-filter", 		0, (keyval) SET_PRINT_FILTER },
    { "prompt-for-bcc", 	0, (keyval) SET_PROMPT_FOR_BCC },
    { "prompt-for-cc",	 	0, (keyval) SET_PROMPT_FOR_CC },
    { "prompt-for-fcc", 	0, (keyval) SET_PROMPT_FOR_FCC },
    { "prompt-rcpt-always",	0, (keyval) SET_PROMPT_RCPT_ALWAYS },
    { "read-prompt", 		0, (keyval) SET_READ_PROMPT },
    { "reply-all", 		0, (keyval) SET_REPLY_ALL },
    { "reply-include-me",	0, (keyval) SET_REPLY_INCLUDE_ME },
    { "reply-indent", 		0, (keyval) SET_REPLY_INDENT },
    { "reply-initial-display",	0, (keyval) SET_REPLY_INITIAL_DISPLAY },
    { "reply-insert", 		0, (keyval) SET_REPLY_INSERT },
    { "saved-messages-file", 	0, (keyval) SET_SAVED_MESSAGES_FILE },
    { "send-prompt", 		0, (keyval) SET_SEND_PROMPT },
    { "send-verbose",		0, (keyval) SET_SEND_VERBOSE },
    { "sendmail-background",	0, (keyval) SET_SENDMAIL_BACKGROUND },
    { "sendmail-verbose",	0, (keyval) SET_SENDMAIL_VERBOSE },
    { "speller",		0, (keyval) SET_SPELLER },
    { "suspend-on-exit", 	0, (keyval) SET_SUSPEND_ON_EXIT },
    { "suspend-on-quit", 	0, (keyval) SET_SUSPEND_ON_QUIT },
    { "temp-directory",		0, (keyval) SET_TEMP_DIRECTORY },
    { "terse-text-prompt",	0, (keyval) SET_TERSE_TEXT_PROMPT },
    { "top-level-prompt", 	0, (keyval) SET_TOP_LEVEL_PROMPT },
    { "use-crt-filter-always",	0, (keyval) SET_USE_CRT_FILTER_ALWAYS },
    { "use-editor-always", 	0, (keyval) SET_USE_EDITOR_ALWAYS },
    { "use-invalid-address", 	0, (keyval) SET_USE_INVALID_ADDRESS },    
    { "user-headers", 		0, (keyval) SET_USER_HEADERS },
    { "user-level",		0, (keyval) SET_USER_LEVEL },
    { "user-name",		0, (keyval) SET_USER_NAME },
};

keytab set_keytab = { sizeof (set_cmds) / sizeof (keywrd), set_cmds };
fdb set_cmd_fdb = { _CMKEY, 0, nil, (pdat) &set_keytab, "variable, " };

/*
 * The following structure is a gross hack, since all of the variable
 * addresses are coerced to (char *) and back.  Need to rewrite someday.
 *
 * Anyway, the purpose is to encode in a single structure the name of
 * the variable, it's address, size, and a brief description, to make
 * it easy to set/dump all the variables, etc.
 */
#ifdef __STDC__
#define XXs(name, type) VAR_##type,           name, sizeof (name)
#define  XX(name, type) VAR_##type, (char *) &name, sizeof (name)
#else
#define XXs(name, type) VAR_/**/type,           name, sizeof (name)
#define  XX(name, type) VAR_/**/type, (char *) &name, sizeof (name)
#endif

variable set_variables[] = {
    { "aliases-use-groups", XX(aliases_use_groups,BOOLEAN)},
    { "append-new-mail", XX(append_new_mail,BOOLEAN)},
    { "append-signature", XX(append_signature,MAYBE)},
    { "auto-create-files", XX(auto_create_files,MAYBE)},
    { "auto-startup-get", XX(auto_startup_get,BOOLEAN)},
    { "autowrap-column", XX(autowrap_column,INTEGER)},
    { "browse-clear-screen", XX(browse_clear_screen,BOOLEAN)},
    { "browse-pause", XX(browse_pause,BOOLEAN)},
    { "check-interval", XX(check_interval,INTEGER)},
    { "clear-screen", XX(clear_screen,BOOLEAN)},
    { "continuous-check", XX(continuous_check,BOOLEAN)},
    { "control-d-automatic-send", XX(control_d_automatic_send,BOOLEAN)},
    { "control-e-editor", XX(control_e_editor,BOOLEAN)},
    { "control-l-confirm", XX(control_l_confirm,BOOLEAN)},
    { "control-n-abort", XX(control_n_abort,MAYBE)},
    { "crt-filter", XXs(crt_filter,COMMAND)},
    { "default-bcc-list", XX(default_bcc_list,ADDRLIST)},
    { "default-cc-list", XX(default_cc_list,ADDRLIST)},
    { "default-fcc-list", XX(default_fcc_list,FILES)},
    { "default-from", XXs(default_from,TEXT)},
    { "default-mail-type", XX(default_mail_type,KEYWORDS)},
    { "default-read-command", XXs(default_read_command,CMDKEYS)},
    { "default-reply-to", XXs(default_reply_to,TEXT)},
    { "default-send-command", XXs(default_send_command,CMDKEYS)},
    { "directory-folders", XX(directory_folders,BOOLEAN)},
    { "display-flagged-messages", XX(display_flagged_messages,BOOLEAN)},
    { "display-outgoing-message", XX(display_outgoing_message,BOOLEAN)},
    { "dont-print-headers", XX(dont_print_headers,KEYLIST)},
    { "dont-type-headers", XX(dont_type_headers,KEYLIST)},
    { "editor", XX(editor,CMDARGS)},
    { "escape-automatic-send",	XX(escape_automatic_send,BOOLEAN)},
    { "expunge-on-bye", XX(expunge_on_bye,MAYBE)},
    { "fast-init-file", XX(fast_init_file,MAYBE)},
    { "finger-command", XXs(finger_command,COMMAND)},
    { "gnuemacs-mmail", XX(gnuemacs_mmail,BOOLEAN)},
    { "handle-changed-modtime", XX(handle_changed_modtime,MAYBE)},
    { "header-options-file", XXs(header_options_file,FILE)},
    { "help-file", XXs(help_file,FILE)},
    { "help-dir", XXs(help_dir,DIRECTORY)},
    { "incoming-mail", XX(incoming_mail,FILES)},
    { "keywords", XX(user_keywords,KEYLIST)},
    { "list-include-headers", XX(list_include_headers,BOOLEAN)},
    { "list-on-separate-pages", XX(list_on_separate_pages,BOOLEAN)},
    { "mail-aliases", XX(mail_aliases,ALIAS)},
    { "mail-directory", XXs(mail_directory,DIRECTORY)},
    { "mail-file", XXs(mail_file,OFILE)},
    { "mmail-path", XXs(mmail_path,FILE)},
    { "modify-read-only", XX(modify_read_only,MAYBE)},
    { "movemail-path", XXs(movemail_path,COMMAND)},
    { "new-file-mode", XX(new_file_mode,INTEGER)},
    { "only-print-headers", XX(only_print_headers,KEYLIST)},
    { "only-type-headers", XX(only_type_headers,KEYLIST)},
    { "personal-name", XXs(personal_name,TEXT)},
    { "print-filter", XXs(print_filter,COMMAND)},
    { "prompt-for-bcc", XX(prompt_for_bcc,BOOLEAN)},
    { "prompt-for-cc", XX(prompt_for_cc,BOOLEAN)},
    { "prompt-for-fcc", XX(prompt_for_fcc,BOOLEAN)},
    { "prompt-rcpt-always", XX(prompt_rcpt_always ,BOOLEAN)},
    { "read-prompt", XXs(read_prompt,PROMPT)},
    { "reply-all", XX(reply_all,BOOLEAN)},
    { "reply-include-me", XX(reply_include_me,BOOLEAN)},
    { "reply-indent", XXs(reply_indent,QUOTEDSTR)},
    { "reply-initial-display", XX(reply_initial_display,BOOLEAN)},
    { "reply-insert", XX(reply_insert,BOOLEAN)},
    { "saved-messages-file", XXs(saved_messages_file,OFILE)},
    { "send-prompt", XXs(send_prompt,PROMPT)},
    { "send-verbose", XX(send_verbose,BOOLEAN)},
    { "sendmail-background", XX(sendmail_background,BOOLEAN)},
    { "sendmail-verbose", XX(sendmail_verbose,BOOLEAN)},
    { "speller", XX(speller,CMDARGS)},
    { "suspend-on-exit", XX(suspend_on_exit,BOOLEAN)},
    { "suspend-on-quit", XX(suspend_on_quit,BOOLEAN)},
    { "temp-directory", XXs(temp_directory,DIRECTORY)},
    { "terse-text-prompt", XX(terse_text_prompt,BOOLEAN)},
    { "top-level-prompt", XXs(top_level_prompt,PROMPT)},
    { "use-crt-filter-always", XX(use_crt_filter_always,BOOLEAN)},
    { "use-editor-always", XX(use_editor_always,BOOLEAN)},
    { "use-invalid-address", XX(use_invalid_address,MAYBE)},
    { "user-headers", XX(user_headers,KEYLIST)},
    { "user-level", XX(user_level,KEYWORDS)},
    { "user-name", XXs(user_name,USERNAME)},
};
#undef XX
#undef XXs

int set_variable_count = sizeof (set_variables) / sizeof (set_variables[0]);

int
set_variable (n)
int n;
{
    int x;
    char *p, **q;
    char **ap;
    variable *v = &set_variables[n];
    caddr_t dangling = 0;
    char **split_args();

    switch (v->type) {
      case VAR_BOOLEAN:
	x = parse_yesno ("yes");
	*((int *) v->addr) = x;
	break;
      case VAR_MAYBE:
	*((int *) v->addr) = parse_yesnoask ("ask");
	break;
      case VAR_INTEGER:
	if (n == SET_NEW_FILE_MODE)
	    x = parse_number (8, "octal mode", nil);
	else
	    x = parse_number (10, "decimal number", nil);
	confirm ();
	*((int *) v->addr) = x;
	if (n == SET_AUTOWRAP_COLUMN)
	    cmcsb._cmwrp = autowrap_column;
	if (n == SET_NEW_FILE_MODE)
	    umask (~(new_file_mode&04777));
	break;
      case VAR_ADDRLIST:
	dangling = nil;
	parse_addresses((addresslist *)v->addr);
	break;
      case VAR_KEYLIST:
	dangling = *((char **) v->addr);
	*((char ***) v->addr) = parse_keylist (0, 0, "header keyword");
	break;
      case VAR_DIRECTORY:
	p = parse_directory ("directory name", v->addr);
	strcpy (v->addr, p);
	break;
      case VAR_FILES:
	dangling = *((char **) v->addr);
	*((char ***) v->addr) = parse_filelist (0, 0, "filename", true);
	break;
      case VAR_FILE:
	p = parse_input_file ("filename", v->addr, false);
	confirm();
	strcpy(v->addr, p);
	break;
      case VAR_OFILE:
	p = parse_output_file("filename", v->addr, true);
	confirm();
	strcpy(v->addr, p);
	if (n == SET_MAIL_FILE) {	/* changed their primary mail file */
	    if (cf != NULL) {
		if (strcmp(cf->filename, p) == 0)
		    cf->flags |= MF_MAILBOX; /* primary mailbox? */
		else
		    cf->flags &= ~MF_MAILBOX; /* not any more */
	    }
	}
	break;
      case VAR_CMDARGS:
	{
	    char **cp;
	    
	    p = parse_text ("text string", nil);
	    for (cp = *((char ***) v->addr); cp && *cp; cp++)
	        safe_free (*cp);
	    safe_free (*((char ***) v->addr));
	    *((char ***) v->addr) = split_args(p);	
	}
	break;
      case VAR_COMMAND: /* should parse for an executable file, but too slow */
      case VAR_TEXT:
	p = parse_text ("text string", nil);
	if (strlen (p) >= v->size)
	    cmerr ("Variable \"%s\" can be no more than %d characters long",
		   v->name, v->size - 1);
	strcpy (v->addr, p);
	break;
      case VAR_PROMPT:
	p = (char *) parse_prompt("string to prompt with");
	if (strlen(p) >= v->size)
	    cmerr ("Variable \"%s\" can be no more than %d characters long",
		   v->name, v->size -1);
	strcpy (v->addr, p);
	break;
      case VAR_QUOTEDSTR:
	p = parse_quoted("quoted string", nil);
	confirm();
	if (strlen (p) >= v->size)
	    cmerr ("Variable \"%s\" can be no more than %d characters long",
		   v->name, v->size - 1);
	strcpy (v->addr, p);
	break;
      case VAR_USERNAME:
	p = parse_username ("username", user_name);
	confirm();
	if (n == SET_USER_NAME) {
	  struct passwd *p1,*getpwnam();
	  p1 = getpwnam(p);
	  if (p1 == NULL || p1->pw_uid != getuid())
	      cmerr("Username \"%s\" does not match your uid", p1->pw_name);
	  sethome(p);
        }
	strcpy(v->addr, p);
	break;
      case VAR_KEYWORDS:
	p = parse_keyword(((setkey *)v->addr)->keytab, FALSE);
	if (p != NULL) {
	    confirm();
	    strcpy(((setkey *)v->addr)->current,p);
	}
	else
	    ((setkey *)v->addr)->current[0] = '\0'; /* no default */
	break;
      case VAR_CMDKEYS:	{
	fdb *fdbs;
	static fdb cfmfdb = { _CMCFM, CM_SDH|CM_NLH, NULL, NULL, 
				"confirm to unset", NULL, NULL, NULL };

	switch (n) {
	case SET_DEFAULT_READ_COMMAND:
	  fdbs = fdbchn (&mm_read_fdb_abbr, &mm_read_fdb_1, 
			 &mm_read_fdb_2, &mm_read_fdb_3, &mm_read_fdb_4, 
			 &mm_read_fdb_5, &mm_read_fdb_6, &mm_read_fdb_7, 
			 &mm_read_fdb_inv, 
			 &cfmfdb, nil);

	  break;
	case SET_DEFAULT_SEND_COMMAND:
	  fdbs = fdbchn (&mm_send_fdb_abbr, &mm_send_fdb_1, 
			 &mm_send_fdb_2, &mm_send_fdb_3, &mm_send_fdb_4, 
			 &mm_send_fdb_5, &mm_send_fdb_inv, 
			 &cfmfdb, nil);
	  break;
	}
	parse (fdbs, &pv, &used);
	if (used == &cfmfdb)
	  *v->addr = '\0';
	else {
	  confirm();
	  strcpy (v->addr, ((keywrd *) pv._pvkey)->_kwkwd);
	}
	break;				/* dont lose this! */
      }
      case VAR_ALIAS:
	cmd_define(CMD_DEFINE);
	break;
      default:
	printf ("set_variable called to set unknown variable type %d\n");
    }
    v->changed = TRUE;
    if (dangling) {
	safe_free (dangling);
    }
    return true;
}

int
show_variable (fp, n, verbose)
FILE *fp;
int n, verbose;
{
    char **ap;
    variable *v = &set_variables[n];
    int i;

    if (verbose && n != SET_MAIL_ALIASES)
	fprintf (fp, "set %s ", v->name);
    switch (set_variables[n].type) {
      case VAR_BOOLEAN:
	fprintf (fp, "%s", (*((int *) v->addr) ? "yes" : "no"));
	break;
      case VAR_MAYBE:
	switch (*((int *) v->addr)) {
	  case SET_NEVER:
	    fprintf (fp, "never");
	    break;
	  case SET_ALWAYS:
	    fprintf (fp, "always");
	    break;
	  case SET_ASK:
	    fprintf (fp, "ask");
	    break;
	}
	break;
      case VAR_INTEGER:
	if (n == SET_NEW_FILE_MODE)
	    fprintf (fp, "%o", *((int *) v->addr));
	else
	    fprintf (fp, "%d", *((int *) v->addr));
	break;
      case VAR_TEXT:
      case VAR_USERNAME:
      case VAR_CMDKEYS:
	fprintf (fp, "%s", v->addr);
	break;
      case VAR_DIRECTORY:
      case VAR_COMMAND:
      case VAR_FILE:
      case VAR_OFILE:
	show_filename (fp, v->addr, true);
	break;
      case VAR_QUOTEDSTR:
      case VAR_PROMPT:
	fprintf(fp, "\"%s\"", v->addr);
	break;
      case VAR_FILES:			/* these aren't comma-separated */
	for (ap = *((char ***) v->addr); ap && *ap; ap++) {
	    show_filename (fp, *ap,true);
	    if (*(ap+1))
		putc (' ',fp);
	}
	break;
      case VAR_KEYLIST:
	for (ap = *((char ***) v->addr); ap && *ap; ap++)
	    fprintf (fp, "%s%s", *ap, (*(ap+1) ? ", " : ""));
	break;
      case VAR_CMDARGS:
	for (ap = *((char ***) v->addr); ap && *ap; ap++)
	    fprintf (fp, "%s ", *ap);
	break;
      case VAR_ADDRLIST:
	disp_addresses(fp, nil, v->addr, true, false, false, true);
	break;
      case VAR_KEYWORDS:
	fprintf(fp,"%s", ((setkey *)v->addr)->current);
	break;
      case VAR_ALIAS:
	for(i = 0; i < mail_aliases.count; i++)
	    disp_alias(fp, i, true,true);
	break;
    }
    if (verbose)
	putc ('\n', fp);
    return true;
}

int
cmd_set (n)
int n;
{
    static fdb cfmfdb = { _CMCFM, CM_SDH, nil, nil, 
			      "confirm to show all variable settings",
			      nil, nil, nil };

    noise ("variable");
    parse (fdbchn (&cfmfdb,&set_cmd_fdb, nil), &pv, &used);
    if (used == &cfmfdb)
	show_all_variables(cmcsb._cmoj ? cmcsb._cmoj : stdout);
    else
	(void) set_variable (pv._pvint);
    return true;
}

int
cmd_show (n)
int n;
{
    int i;
    extern fdb aliasfdb;
    FILE *out;				/* for output */

    noise ("settings");
    
    aliasfdb._cmdat = (pdat) mk_alias_keys();
    aliasfdb._cmdef = nil;
    parse (fdbchn(&cfm_fdb, &aliasfdb, &set_cmd_fdb, nil), &pv, &used);
    out = cmcsb._cmoj ? cmcsb._cmoj : stdout;
    if (used == &cfm_fdb) {
	show_all_variables(out);
    }
    else if (used == &set_cmd_fdb) {
	i = pv._pvint;
	confirm ();
	show_variable (out, i, true);
    }
    else {
	i = pv._pvint;
	confirm();
	disp_alias(out, i, true,true);
    }
    return true;
}

/*
 * show_filename:
 * show a filename, abbreviating to ~ where appropriate
 */
char *
show_filename (fp, longname, disp)
FILE *fp;
char *longname;
int disp;
{
    int len = strlen (HOME);
    static char buf[100];
    if ((strcmp(HOME, "/") != 0) &&	/* don't abbreviate "/" */
	(strncmp (longname, HOME, len) == 0)) { /* something in homedir */
	if (longname[len] != '\0')
	    sprintf (buf, "~%s", &longname[len]);
	else				/* exactly homedir */
	    sprintf (buf, "~");
    }
    else
	sprintf (buf, "%s", longname);
    if (disp)
	fprintf(fp,"%s",buf);
    return(buf);
}


static char *
tilde_expand(dir)
char *dir;
{
  static char buf[MAXPATHLEN];
  if (*dir != '~') return(dir);
  strcpy(buf,HOME);
  strcat(buf,&dir[1]);
  return(buf);
}


/* The following routines handle a fast start up by directly reading
 * junk in from an .mmfast file.  They are here instead of in init.c
 * or doinit.c because the code there merely calls them.  Since they
 * are, in a way, parsing commands, it makes partial sense to put them
 * in here.  If fast init files turn out to be a bad idea (horrors!),
 * then this code can be easily changed to something like what MM-20
 * does (ie, a fast hacky text parser).
 */
#define fail(fp) {fclose(fp); return(false);}
int
read_fast_init_file()		/* Reads the fast init file */
{
  register int icount;		/* Number of guys we got */
  int vindex;			/* Variable index--not register, guess why! */
  int ccount;			/* Character count */
  register char ** ap;		/* A pointer */
  FILE *fp;
  buffer fastname,		/* Fast init file name */
  	 initname;		/* Regular init file name */
  struct stat faststat,		/* struct to stat time of fast init file */
  	      initstat;		/* Also to stat the init file */
  char filetext[BUFSIZ];
  char *sstr(),*name, *cp;
  char **p,*tilde_expand(),*safe_strcpy();
  int count,i;
  variable *v;
  addresslist *al,al1;
  char *show_filename();
  int vlen, olen;

  if (HOME == NULL)		/* Can't do much without a home */
    return false;

  sprintf (initname, "%s/.mminit", HOME);
  sprintf (fastname, "%s/.mmfast", HOME);
  if (stat(fastname,&faststat) != 0)	/* Is the fast file even there? */
    return false;		/* Not there, so can't read it */

  if (fast_init_file == SET_ASK)
    if (!yesno("Fast init file exists -- read it? ", "yes"))
      return (false);

  if (stat(initname,&initstat) == 0)	/* Is the init file even there? */
      if (faststat.st_mtime <	/* Is the fast file older then the */
	  initstat.st_mtime)	/* fast file? */
	  return false;		/* The fast init file is out of date */

  fp = fopen(fastname, "r");

  if (fp == NULL)
    return false;		/* Can't parse it if can't open it */

  vlen = strlen(mm_version);
#ifdef FASTBIN
  olen = strlen(OStype);
#else
  olen = -1;
#endif

  if (fgets(filetext,vlen+olen+1+2,fp) == NULL) /* Chew newline */
    return false;		/* It's bad if we can't read version string */

  if (strncmp(filetext, mm_version, vlen) != 0) /* Ignore newline */
    return false;		/* Version has changed */
#ifdef FASTBIN
  if (strncmp(&filetext[vlen+1],OStype,olen) != 0) /* OSTYPE matters if */
    return false;			/* binary fastfile */
#endif  

  while (gnum(fp,&vindex)) {
    v = &set_variables[vindex];
    switch(v->type) {
      case VAR_BOOLEAN:	/* Simple variables just want numbers */
      case VAR_MAYBE:
      case VAR_INTEGER:
	if (!gnum(fp,v->addr)) fail(fp);
	break;
      case VAR_TEXT:		/* All text is counted */
      case VAR_CMDKEYS:
      case VAR_QUOTEDSTR:
      case VAR_PROMPT:
      case VAR_USERNAME:
      case VAR_COMMAND:
	if (!gsstr(fp, v->addr)) fail(fp);
	  break;
      case VAR_DIRECTORY:
      case VAR_FILE:
      case VAR_OFILE:
	if (!gstr(fp,&cp)) fail(fp);
	strcpy(v->addr,
	       tilde_expand(cp)); /* De~ the filename */
	safe_free(cp);
	break;
	
      case VAR_FILES:
	if (!gnum(fp,&count)) fail(fp);
	if (count == 0) {
	    *((char ***) v->addr) = nil;
	    break;
	}
	p = (char **) malloc((count+1)*sizeof(char *));
	*(char ***)v->addr = p;
	for(i = 0; i < count; i++) {
	  char *cp,*cp1;
	  if (!gstr(fp,&cp)) fail(fp);
	  cp1 = tilde_expand(cp);
	  safe_free(cp);
	  p[i] = safe_strcpy(cp1);
	}
	p[count] = nil;
	break;

      case VAR_KEYLIST:
      case VAR_CMDARGS:
	if (!gnum(fp,&count)) fail(fp);
	if (count == 0) {
	  *((char ***) v->addr) = nil;
	  break;
	}
	p = (char **) malloc((count+1)*sizeof(char *));
	*(char ***)v->addr = p;
	for(i = 0; i < count; i++) {
	  char *cp;
	  if (!gstr(fp,&cp)) fail(fp);
	  p[i] = cp;
	}
	p[count] = nil;
	break;

      case VAR_ADDRLIST:
	if (!gnum(fp,&count)) fail(fp);
	al = (addresslist *)v->addr;
	al->first = al->last = nil;
	for(i = 0; i < count; i++) {
	  char *cp;
	  int type;
	  if (!gnum(fp,&type)) fail(fp);
	  if (!gstr(fp,&cp)) fail(fp);
	  add_addresslist(al,cp,type);
	  safe_free(cp);
	}
	break;
	
      case VAR_ALIAS:
	if (!gstr(fp,&name)) fail(fp);
	if (!gnum(fp,&count)) fail(fp);
	al1.first = al1.last = nil;
	for(i = 0; i < count; i++) {
	  char *cp;
	  int type;
	  if (!gnum(fp,&type)) fail(fp);
	  if (!gstr(fp,&cp)) fail(fp);
	  add_addresslist(&al1,cp,type);
	  safe_free(cp);
	}
	set_alias(name,&al1,MA_USER);
	break;		/* Read it real fast */
	
      case VAR_KEYWORDS:
	if (!gstr(fp,&cp)) fail(fp);
	strcpy(((setkey *)v->addr)->current, cp);
	safe_free(cp);
	break;
	
      default:		/* This is very bad and should not happen */
	fprintf(stderr,"? %d,**UNKNOWN Variable index**\n",
		vindex);
	fail(fp);
	break;
      }				/* End of switch */
    geol(fp);
    v->changed = TRUE;
  }
  if (!feof(fp)) fail(fp);
  fclose(fp);			/* Done parsing the file */
  return true;
}


/* Much like the set command nonsense except that we write out the
 * index of the variable in the set_variables array and follow it with
 * the data in the variable.  We then use that integer to index into
 * an array to set the data instead wasting time parsing.  This is
 * pretty straightforward except for lists of objects.  All non-unary
 * objects are preceeded by a length count.  Lists of objects are also
 * preceeded by an object count.
 */

int
write_fast_init_file()		/* Writes the fast init file */
{
  register int n;		/* Current index in set_variables array */
  char **ap;
  variable * v;
  char * shortname, *cp;
  FILE *fp;
  buffer filename;
  int count;
  addresslist *al;
  addr_unit *a;

  if (HOME == NULL)		/* Can't do much without a home */
   return false;

  sprintf (filename, "%s/.mmfast", HOME);
  fp = fopen(filename, "w");

#ifdef FASTBIN
  fprintf(fp,"%s,%s\n",mm_version, OStype); /* Stick out version and OSTYPE */
#else
  fprintf(fp,"%s\n",mm_version);	/* Stick out the version */
#endif

  for (n=0,v = &set_variables[n]; /* Loop through all the variables */
       n < (sizeof (set_variables) / sizeof (variable)); /* End of table */
       n++,v = &set_variables[n]) {
    if (!v->changed)
      continue;
    if (v->type != VAR_ALIAS)
      pnum(fp,n);
    switch (set_variables[n].type) {

      case VAR_BOOLEAN:		/* Simple variables just want numbers */
      case VAR_MAYBE:
      case VAR_INTEGER:
	pnum(fp, *((int *)v->addr));
	break;

      case VAR_TEXT:		/* All text is counted */
      case VAR_CMDKEYS:
      case VAR_QUOTEDSTR:
      case VAR_PROMPT:
      case VAR_USERNAME:
      case VAR_COMMAND:
	pstr(fp,v->addr);
	break;

      case VAR_DIRECTORY:
      case VAR_FILE:
      case VAR_OFILE:
	cp = show_filename(fp,v->addr,false);
	pstr(fp,cp);
	break;

      case VAR_FILES:
	for (count=0,ap = *((char ***) v->addr); ap && *ap; ap++,count++);
	pnum(fp,count);
	  for (ap = *((char ***) v->addr); ap && *ap; ap++) {
	    cp = show_filename(fp, *ap, false);
	    pstr(fp,cp);
	  }
	break;

      case VAR_KEYLIST:
      case VAR_CMDARGS:
	for (count=0,ap = *((char ***) v->addr); ap && *ap; ap++,count++);
	pnum(fp,count);
	for (ap = *((char ***) v->addr); ap && *ap; ap++)
	  pstr(fp, *ap);
	break;

      case VAR_ALIAS:			/* an alias */
	for (count = 0; count < mail_aliases.count; count++) {
	  if (mail_aliases.aliases[count].type == MA_USER) { 
	    al = &mail_aliases.aliases[count].alias; /* save if user set */
	    pnum(fp,n);
	    if (al->first){
	      int naliases;
	      for(naliases = 0, a = al->first; 
		  a != nil;
		  naliases++, a = a->next );
	      pstr(fp,mail_aliases.aliases[count].name);
	      pnum(fp,naliases);
	      for(a = al->first; ; a = a->next) {
		if (a->type == ADR_MLIST)
		  pnum(fp,ADR_LISTFILE);
		else
		  pnum(fp,a->type);
		pstr(fp,a->data);
		if (a == al->last) break;
	      }
	    }
	    peol(fp);
	  }
        }
	break;
      case VAR_ADDRLIST:
	al = (addresslist *) v->addr;
	for(count = 0, a = al->first; a != nil; count++, a = a->next );
	pnum(fp, count);
	if (al->first) {
	  for(a = al->first; ; a = a->next) {
	    if (a->type == ADR_MLIST)
	      pnum(fp,ADR_LISTFILE);
	    else
	      pnum(fp,a->type);
	    pstr(fp,a->data);
	    if (a == al->last) break;
	  }
	}
	break;
      case VAR_KEYWORDS:
	cp = ((setkey *)v->addr)->current;
	pstr(fp,cp);
	break;


      default:			/* This is very bad and should not happen */
/*	fprintf(fp,"%d,%d,**UNKNOWN**\n",n,set_variables[n].type);*/
	break;
      }
    if (v->type != VAR_ALIAS)
      peol(fp);
  }
  fflush(fp);			/* Get everything out of there */
  fclose(fp);
  return true;
}


pnum(fp, n)
FILE *fp;
int n;
{
#ifdef FASTBIN
  int i;
  char *cp = (char *)&n;
  for(i = 0; i < sizeof(int); i++,cp++)
    putc(*cp,fp);
#else
  fprintf(fp,"%d,",n);
#endif
}

pstr(fp, str)
FILE *fp;
char *str;
{
    int n = strlen(str);
    pnum(fp,n);
#ifdef FASTBIN
    fprintf(fp,"%s",str);
#else
    fprintf(fp,"%s,",str);
#endif
}

peol(fp)
FILE *fp;
{
#ifndef FASTBIN
    fprintf(fp,"\n");
#endif
}

int
gnum(fp,num)
FILE *fp;
int *num;
{
    int n;
#ifdef FASTBIN
    int i;
    int k;
    char *cp = (char *)&n;

    for(i = 0; i < sizeof(int); i++) {
      k = getc(fp);
      if (k == EOF) return(false);
      *cp++ = k;
    }
    *num = n;
    return(true);
#else
    if (fscanf(fp,"%d,",&n) != 1) return(false);
    *num = n;
    return(true);
#endif
}


int
gstr(fp,cp) 
FILE *fp;
char **cp;
{
    int n,c;
    char *buf;

#ifdef FASTBIN
    if (gnum(fp,&n) == false) return(false);
    buf = malloc(n+1);
    if (n != 0)
      if (fread(buf,sizeof(char),n,fp) <= 0) return(false);
    buf[n] = '\0';
    *cp = buf;
    return(true);
#else
    if (fscanf(fp,"%d,",&n) != 1) return(false);
    buf = malloc(n+1);
    if (n != 0)
      if (fread(buf,sizeof(char),n,fp) <= 0) return(false);
    c = getc(fp);
    if (c == EOF) return(false);
    if (c != ',') {
	ungetc(c,fp);
	return(false);
    }
    buf[n] = '\0';
    *cp = buf;
    return(true);
#endif
}


int
gsstr(fp,cp) 
FILE *fp;
char *cp;
{
    int n,c;
    char *buf;

#ifdef FASTBIN
    if (gnum(fp,&n) == false) return(false);
    if (n != 0)
      if (fread(cp,sizeof(char),n,fp) <= 0) return(false);
    cp[n] = '\0';
    return(true);
#else
    if (fscanf(fp,"%d,",&n) != 1) return(false);
    if (n != 0)
      if (fread(cp,sizeof(char),n,fp) <= 0) return(false);
    cp[n] = '\0';
    c = getc(fp);
    if (c == EOF) return(false);
    if (c != ',') {
	ungetc(c,fp);
	return(false);
    }
    return(true);
#endif
}

int 
geol(fp) 
FILE *fp;
{
#ifndef FASTBIN
    int c;
    c = getc(fp);
    if (c == EOF) return(false);
    if (c != '\n') {
	ungetc(c,fp);
	return(false);
    }
#endif
    return(true);
}

/*
 * split_args:
 * takes a string containing a command and splits it up into
 * individual arg's and returns a char ** (argv) pointing at them.
 * note: arguments in single quotes are taken literally.  The
 * backslash character makes the following character be taken literally.
 */
 
char **
split_args (str)
char *str;
{
    char *cp;
    char *temp = NULL;			/* temp buffer to build arguments */
    int i = 0;				/* current position in temp buffer */
    int count = 0;			/* number of slots in args[] */
    int anum = 0;			/* slots in use in args[] */
    char **args = NULL;			/* argument list to be returned */

    if (str == NULL)			/* null string, no args */
	return (NULL);
    if ((temp = (char *) malloc (strlen (str) + 1)) == NULL)
	return (NULL);

    cp = str;
    while (*cp) {			/* count number of spaces */
	if (isspace(*cp))		/* this should give us an */
	    count++;			/* idea of the number of args */
	cp++;
    }
    count += 2;				/* and a couple more */
    /* get space for arg list */
    if ((args = (char **) malloc (count*sizeof(char *))) == NULL)
	return (NULL);

    cp = str;
    while (*cp) {			/* break up into args */
	while (*cp && isspace(*cp))	/* skip initial spaces */
	    cp++;
	if (!*cp)
	    break;			/* no more args */

	if (isquote(*cp)) {		/* starts with quote */
	    cp++;			/* skip the quote */
	    while (*cp && !isquote(*cp)) { /* till matching quote is found */
		if (isbackslash(*cp))
		    cp++;		/* skip over the backslash */
		temp[i++] = *cp;
		cp++;
	    }
	    if (*cp)			/* string didn't end  */
		cp++;			/* skip the end quote */
	}
	else {				/* begins with something else */
	    while (*cp && !isspace(*cp)) { /* go till end of word (space) */
		if (isbackslash(*cp))
		    cp++;		/* skip over the backslash */
		temp[i++] = *cp;
		cp++;
	    }
	}
	temp[i] = '\0';			/* tie off an arg */

	if (anum >= count) {		/* need more space? */
	    if ((args = (char **) 
		 realloc (args, (++count)*sizeof(char *))) == NULL)
		return (NULL);
	}
	if ((args[anum] = (char *) malloc (strlen(temp)+1)) == NULL) {
	    anum--;			/* back up */
	    while (anum >= 0)
		safe_free (args[anum--]);
	    safe_free (args);
	    return (NULL);
	}
	strcpy (args[anum++], temp);	/* save this argument */
	i = 0;				/* reset to beginning of buffer */
    }
    args[anum] = NULL;			/* tie off arg list */
    return (args);			/* return list */
}

/*
 * show_all_variables:
 * show the values of all variables
 */
show_all_variables(out)
FILE *out;
{
    int i,nv;
    FILE *fp, *more_pipe_open();	/* to pipe long output */

    nv = sizeof (set_variables) / sizeof (variable);
    if ((nv + mail_aliases.count) >= cmcsb._cmrmx)
	fp = more_pipe_open(out);	/* 1 line per variable or alias... */
    else
	fp = out;
    for (i = 0; i < nv; i++)
	if (i != SET_MAIL_ALIASES)
	    show_variable (fp, i, true);
    for (i = 0; i < mail_aliases.count; i++)
	disp_alias (fp, i, true,true);
    if (fp == out)			/* not a pipe */
	fflush (fp);
    else
	more_pipe_close(fp);	/* really a pipe */
}
