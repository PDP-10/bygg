/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /src/sos5.bin/cucca/mm/mm/RCS/cmds.c,v 2.1 1990/10/04 18:23:41 melissa Exp $";
#endif

/*
 * cmds.c - top level command tables for mm
 *
 * All of the top-level command tables for mm are declared here.
 */

#include "mm.h"
#include "parse.h"
#include "cmds.h"
#include "rd.h"

#define MAXCOL 16
#define CMKEYTAB(t,k) keytab t = { (sizeof(k)/sizeof(keywrd)), k, MAXCOL }


/*
 * mm top level commands
 */


keywrd mm_top_keys_1[] = {
    { "exit", 0, (keyval) CMD_EXIT },
    { "help", 0, (keyval) CMD_HELP },
    { "headers", 0, (keyval) CMD_HEADERS },
    { "quit", 0, (keyval) CMD_QUIT },
    { "read", 0, (keyval) CMD_READ },
    { "review", 0, (keyval) CMD_REVIEW },
    { "send", 0, (keyval) CMD_SEND },
    { "suspend", 0, (keyval) CMD_SUSPEND },
};
CMKEYTAB(mm_top_keytab_1,mm_top_keys_1);
fdb mm_top_fdb_1 = { _CMKEY, CM_NLH|KEY_PTR|KEY_WID, NULL, 
			 (pdat)&mm_top_keytab_1, 
			 "\n  BASIC command, ",
			 NULL, NULL, NULL };


keywrd mm_top_keys_2[] = {
    { "answer", 0, (keyval) CMD_ANSWER },
    { "delete", 0, (keyval) CMD_DELETE },
    { "forward", 0, (keyval) CMD_FORWARD },
    { "print", 0, (keyval) CMD_PRINT },
    { "remail", 0, (keyval) CMD_REMAIL },
    { "reply", 0, (keyval) CMD_REPLY },
    { "type", 0, (keyval) CMD_TYPE },
    { "undelete", 0, (keyval) CMD_UNDELETE },
};
CMKEYTAB(mm_top_keytab_2,mm_top_keys_2);
fdb mm_top_fdb_2 = { _CMKEY, CM_NLH|KEY_PTR|KEY_WID, NULL, 
			 (pdat)&mm_top_keytab_2, 
			 "MESSAGE-HANDLING command, ",
			 NULL, NULL, NULL };

keywrd mm_top_keys_3[] = {
    { "flag", 0, (keyval) CMD_FLAG },
    { "keyword", 0, (keyval) CMD_KEYWORD },
    { "mark", 0, (keyval) CMD_MARK },
    { "unanswer", 0, (keyval) CMD_UNANSWER },
    { "unflag", 0, (keyval) CMD_UNFLAG },
    { "unkeyword", 0, (keyval) CMD_UNKEYWORD },
    { "unmark", 0, (keyval) CMD_UNMARK },
};
CMKEYTAB(mm_top_keytab_3,mm_top_keys_3);
fdb mm_top_fdb_3 = { _CMKEY, CM_NLH|KEY_PTR|KEY_WID, NULL, 
			 (pdat)&mm_top_keytab_3, 
			 "MESSAGE-TAGGING command, ",
			 NULL, NULL, NULL };

keywrd mm_top_keys_4[] = {
    { "copy", 0, (keyval) CMD_COPY },
    { "examine", 0, (keyval) CMD_EXAMINE },
    { "expunge", 0, (keyval) CMD_EXPUNGE },
    { "get", 0, (keyval) CMD_GET },
    { "move", 0, (keyval) CMD_MOVE },
    { "restore-draft", 0, (keyval) CMD_RESTORE_DRAFT },
    { "sort", 0, (keyval) CMD_SORT },
    { "write", 0, (keyval) CMD_WRITE },
};
CMKEYTAB(mm_top_keytab_4,mm_top_keys_4);
fdb mm_top_fdb_4 = { _CMKEY, CM_NLH|KEY_PTR|KEY_WID, NULL, 
			 (pdat)&mm_top_keytab_4, 
			 "FILING command, ",
			 NULL, NULL, NULL };

keywrd mm_top_keys_5[] = {
    { "define", 0, (keyval) CMD_DEFINE },
    { "profile", 0, (keyval) CMD_PROFILE },
    { "save-init", 0, (keyval) CMD_CREATE_INIT },
    { "set", 0, (keyval) CMD_SET },
    { "show", 0, (keyval) CMD_SHOW },
};
CMKEYTAB(mm_top_keytab_5,mm_top_keys_5);
fdb mm_top_fdb_5 = { _CMKEY, CM_NLH|KEY_PTR|KEY_WID, NULL, 
			 (pdat)&mm_top_keytab_5, 
			 "CUSTOMIZATION command, ",
			 NULL, NULL, NULL };

keywrd mm_top_keys_6[] = {
    { "check", 0, (keyval) CMD_CHECK },
    { "count", 0, (keyval) CMD_COUNT },
    { "daytime", 0, (keyval) CMD_DAYTIME },
    { "finger", 0, (keyval) CMD_FINGER },
    { "pwd", 0, (keyval) CMD_PWD },
    { "status", 0, (keyval) CMD_STATUS },
    { "version", 0, (keyval) CMD_VERSION },
    { "who", 0, (keyval) CMD_WHO },
};
CMKEYTAB(mm_top_keytab_6,mm_top_keys_6);
fdb mm_top_fdb_6 = { _CMKEY, CM_NLH|KEY_PTR|KEY_WID, NULL, 
			 (pdat)&mm_top_keytab_6, 
			  "INFORMATION command, ",
			 NULL, NULL, NULL };

keywrd mm_top_keys_7[] = {
    { "backtrack", 0, (keyval) CMD_BACKTRACK },
    { "blank", 0, (keyval) CMD_BLANK },
    { "browse", 0, (keyval) CMD_BROWSE },
    { "bug", 0, (keyval) CMD_BUG },
    { "bye", 0, (keyval) CMD_QQUIT },
    { "cd", 0, (keyval) CMD_CD },
    { "continue", 0, (keyval) CMD_CONTINUE },
    { "echo", 0, (keyval) CMD_ECHO },
    { "edit", 0, (keyval) CMD_EDIT },
    { "follow", 0, (keyval) CMD_FOLLOW },
    { "jump", 0, (keyval) CMD_JUMP },
    { "list", 0, (keyval) CMD_LIST },
    { "literal", 0, (keyval) CMD_LITERAL },
    { "next", 0, (keyval) CMD_NEXT },
    { "previous", 0, (keyval) CMD_PREVIOUS },
    { "push", 0, (keyval) CMD_PUSH },
    { "route", 0 , (keyval) CMD_ROUTE },
    { "smail", KEY_INV, (keyval) CMD_SMAIL },
    { "spell", 0, (keyval) CMD_SPELL },
    { "take", 0, (keyval) CMD_TAKE },
};
CMKEYTAB(mm_top_keytab_7,mm_top_keys_7);
fdb mm_top_fdb_7 = { _CMKEY, CM_NLH|KEY_PTR|KEY_WID, NULL, 
			 (pdat)&mm_top_keytab_7, 
			 "some OTHER command, ",
			 NULL, NULL, NULL };

keywrd mm_top_keys_abbr[] = {
    { "delete", KEY_INV, (keyval) CMD_DELETE },
    { "headers", KEY_INV, (keyval) CMD_HEADERS },
    { "literal", KEY_INV, (keyval) CMD_LITERAL },
    { "previous", KEY_INV, (keyval) CMD_PREVIOUS },
    { "quit", KEY_INV, (keyval) CMD_QUIT },
    { "read", KEY_INV, (keyval) CMD_READ },
    { "send", KEY_INV, (keyval) CMD_SEND },
    { "type", KEY_INV, (keyval) CMD_TYPE },
    { "undelete", KEY_INV, (keyval) CMD_UNDELETE },
};
CMKEYTAB(mm_top_keytab_abbr,mm_top_keys_abbr);
fdb mm_top_fdb_abbr = { _CMKEY, CM_NLH|CM_SDH|KEY_PTR|KEY_WID, NULL, 
			    (pdat)&mm_top_keytab_abbr, NULL,
			    NULL, NULL, "Invalid command" };

keywrd mm_top_keys_inv[] = {
    { "alias", 0, (keyval) CMD_ALIAS },
    { "create-init", KEY_INV, (keyval) CMD_CREATE_INIT },
    { "debug", KEY_INV, (keyval) CMD_DEBUG },
    { "debug-memory", KEY_INV, CMD_MEMDEBUG },
    { "mail", 0, (keyval) CMD_SEND },
    { "qquit", 0, (keyval) CMD_QQUIT },
    { "z", KEY_INV, (keyval) CMD_Z },
};
CMKEYTAB(mm_top_keytab_inv,mm_top_keys_inv);
fdb mm_top_fdb_inv = { _CMKEY, CM_NLH|CM_SDH|KEY_PTR|KEY_WID, NULL, 
			   (pdat)&mm_top_keytab_inv, NULL,
			   NULL, NULL, NULL };



/*
 * mm send mode commands (send + answer modes)
 */

keywrd mm_send_keys_1[] = {
    { "display", 0, (keyval) CMD_DISPLAY },
    { "edit", 0, (keyval) CMD_EDIT },
    { "header", 0, (keyval) CMD_HEADERS },
    { "help", 0, (keyval) CMD_HELP },
    { "insert", 0, (keyval) CMD_INSERT },
    { "quit", 0, (keyval) CMD_QUIT },
    { "save-draft", 0, (keyval) CMD_SAVE_DRAFT },
    { "send", 0, (keyval) CMD_SEND },
    { "suspend", 0, (keyval) CMD_SUSPEND },
    { "text", 0, (keyval) CMD_TEXT },
    { "type", 0, (keyval) CMD_TYPE },
};
CMKEYTAB(mm_send_keytab_1,mm_send_keys_1);
fdb mm_send_fdb_1 = { _CMKEY, CM_NLH|KEY_PTR|KEY_WID, NULL, 
			  (pdat)&mm_send_keytab_1, 
			  "\n  BASIC send-mode command, ",
			  NULL, NULL, NULL };

keywrd mm_send_keys_2[] = {
    { "bcc", 0, (keyval) CMD_BCC },
    { "cc", 0, (keyval) CMD_CC },
    { "erase", 0, (keyval) CMD_ERASE },
    { "fcc", 0, (keyval) CMD_FCC },
    { "from", 0, (keyval) CMD_FROM },
    { "keyword", 0, (keyval) CMD_KEYWORD },
    { "in-reply-to", 0, (keyval) CMD_IN_REPLY_TO },
    { "remove", 0, (keyval) CMD_REMOVE },
    { "reply-to", 0, (keyval) CMD_REPLY_TO },
    { "subject", 0, (keyval) CMD_SUBJECT },
    { "to", 0, (keyval) CMD_TO },
    { "unkeyword", 0, (keyval) CMD_UNKEYWORD },
    { "user-header", 0, (keyval) CMD_USER_HEADER },
};
CMKEYTAB(mm_send_keytab_2,mm_send_keys_2);
fdb mm_send_fdb_2 = { _CMKEY, CM_NLH|KEY_PTR|KEY_WID, NULL, 
			  (pdat)&mm_send_keytab_2, 
			  "HEADER-FIELD command, ",
			  NULL, NULL, NULL };

keywrd mm_send_keys_3[] = {
    { "define", 0, (keyval) CMD_DEFINE },
    { "profile", 0, (keyval) CMD_PROFILE },
    { "save-init", 0, (keyval) CMD_CREATE_INIT },
    { "set", 0, (keyval) CMD_SET },
    { "show", 0, (keyval) CMD_SHOW },
};
CMKEYTAB(mm_send_keytab_3,mm_send_keys_3);
fdb mm_send_fdb_3 = { _CMKEY, CM_NLH|KEY_PTR|KEY_WID, NULL, 
			  (pdat)&mm_send_keytab_3, 
			  "CUSTOMIZATION command, ",
			  NULL, NULL, NULL };

keywrd mm_send_keys_4[] = {
    { "check", 0, (keyval) CMD_CHECK },
    { "daytime", 0, (keyval) CMD_DAYTIME },
    { "finger", 0, (keyval) CMD_FINGER },
    { "pwd", 0, (keyval) CMD_PWD },
    { "status", 0, (keyval) CMD_STATUS },
    { "version", 0, (keyval) CMD_VERSION },
    { "who", 0, (keyval) CMD_WHO },
};
CMKEYTAB(mm_send_keytab_4,mm_send_keys_4);
fdb mm_send_fdb_4 = { _CMKEY, CM_NLH|KEY_PTR|KEY_WID, NULL, 
			  (pdat)&mm_send_keytab_4, 
			  "INFORMATION command, ",
			  NULL, NULL, NULL };

keywrd mm_send_keys_5[] = {
    { "blank", 0, (keyval) CMD_BLANK },
    { "cd", 0, (keyval) CMD_CD },
    { "echo", 0, (keyval) CMD_ECHO },
    { "literal", 0, (keyval) CMD_LITERAL },
    { "push", 0, (keyval) CMD_PUSH },
    { "route", 0 , (keyval) CMD_ROUTE },
    { "spell", 0, (keyval) CMD_SPELL },
    { "take", 0, (keyval) CMD_TAKE },
};
CMKEYTAB(mm_send_keytab_5,mm_send_keys_5);
fdb mm_send_fdb_5 = { _CMKEY, CM_NLH|KEY_PTR|KEY_WID, NULL, 
			  (pdat)&mm_send_keytab_5, 
			  "some OTHER command, ",
			  NULL, NULL, NULL };

keywrd mm_send_keys_abbr[] = {
    { "display", KEY_INV, (keyval) CMD_DISPLAY },
    { "edit", KEY_INV, (keyval) CMD_EDIT },
    { "headers", KEY_INV, (keyval) CMD_HEADERS },
    { "literal", KEY_INV, (keyval) CMD_LITERAL },
    { "remove", KEY_INV, (keyval) CMD_REMOVE },
    { "send", KEY_INV, (keyval) CMD_SEND },
    { "type", KEY_INV, (keyval) CMD_TYPE },
};
CMKEYTAB(mm_send_keytab_abbr,mm_send_keys_abbr);
fdb mm_send_fdb_abbr = { _CMKEY, CM_NLH|CM_SDH|KEY_PTR|KEY_WID, NULL, 
			     (pdat)&mm_send_keytab_abbr, 
			     NULL, NULL, NULL, "Invalid command" };

keywrd mm_send_keys_inv[] = {
    { "copy", 0, (keyval) CMD_COPY },
    { "create-init", 0, (keyval) CMD_CREATE_INIT },
    { "debug", KEY_INV, (keyval) CMD_DEBUG },
    { "debug-memory", KEY_INV, (keyval) CMD_MEMDEBUG },
    { "flag", 0, (keyval) CMD_FLAG },
    { "headers", 0, (keyval) CMD_HEADERS },
    { "list", 0, (keyval) CMD_LIST },
    { "mail", 0, (keyval) CMD_SEND },
    { "mark", 0, (keyval) CMD_MARK },
    { "move", 0, (keyval) CMD_MOVE },
    { "print", 0, (keyval) CMD_PRINT },
    { "unanswer", 0, (keyval) CMD_UNANSWER },
    { "unflag", 0, (keyval) CMD_UNFLAG },
    { "unmark", 0, (keyval) CMD_UNMARK },
    { "write", 0, (keyval) CMD_WRITE },
    { "z", KEY_INV, (keyval) CMD_Z },
};
CMKEYTAB(mm_send_keytab_inv,mm_send_keys_inv);
fdb mm_send_fdb_inv = { _CMKEY, CM_NLH|CM_SDH|KEY_PTR|KEY_WID, NULL, 
			    (pdat)&mm_send_keytab_inv,
			    NULL, NULL, NULL, NULL };



/*
 * mm read mode commands 
 */

keywrd mm_read_keys_1[] = {
    { "backtrack", 0, (keyval) CMD_BACKTRACK },
    { "browse", 0, (keyval) CMD_BROWSE },
    { "follow", 0, (keyval) CMD_FOLLOW },
    { "header", 0, (keyval) CMD_HEADERS },
    { "help", 0, (keyval) CMD_HELP },
    { "next", 0, (keyval) CMD_NEXT },
    { "previous", 0, (keyval) CMD_PREVIOUS },
    { "quit", 0, (keyval) CMD_QUIT },
    { "suspend", 0, (keyval) CMD_SUSPEND },
    { "type", 0, (keyval) CMD_TYPE },
};
CMKEYTAB(mm_read_keytab_1,mm_read_keys_1);
fdb mm_read_fdb_1 = { _CMKEY, CM_NLH|KEY_PTR|KEY_WID, NULL, 
			  (pdat)&mm_read_keytab_1, 
			  "\n  BASIC read-mode command, ",
			  NULL, NULL, NULL };

keywrd mm_read_keys_2[] = {
    { "answer", 0, (keyval) CMD_ANSWER },
    { "delete", 0, (keyval) CMD_DELETE },
    { "forward", 0, (keyval) CMD_FORWARD },
    { "kill", 0, (keyval) CMD_KILL },
    { "print", 0, (keyval) CMD_PRINT },
    { "remail", 0, (keyval) CMD_REMAIL },
    { "reply", 0, (keyval) CMD_REPLY },
    { "undelete", 0, (keyval) CMD_UNDELETE },
};
CMKEYTAB(mm_read_keytab_2,mm_read_keys_2);
fdb mm_read_fdb_2 = { _CMKEY, CM_NLH|KEY_PTR|KEY_WID, NULL, 
			  (pdat)&mm_read_keytab_2, 
			  "MESSAGE-HANDLING command, ",
			  NULL, NULL, NULL };

keywrd mm_read_keys_3[] = {
    { "flag", 0, (keyval) CMD_FLAG },
    { "keyword", 0, (keyval) CMD_KEYWORD },
    { "mark", 0, (keyval) CMD_MARK },
    { "unanswer", 0, (keyval) CMD_UNANSWER },
    { "unflag", 0, (keyval) CMD_UNFLAG },
    { "unkeyword", 0, (keyval) CMD_UNKEYWORD },
    { "unmark", 0, (keyval) CMD_UNMARK },
};
CMKEYTAB(mm_read_keytab_3,mm_read_keys_3);
fdb mm_read_fdb_3 = { _CMKEY, CM_NLH|KEY_PTR|KEY_WID, NULL, 
			  (pdat)&mm_read_keytab_3, 
			  "MESSAGE-TAGGING command, ",
			  NULL, NULL, NULL };

keywrd mm_read_keys_4[] = {
    { "copy", 0, (keyval) CMD_COPY },
    { "move", 0, (keyval) CMD_MOVE },
    { "restore-draft", 0, (keyval) CMD_RESTORE_DRAFT },
    { "write", 0, (keyval) CMD_WRITE },
};
CMKEYTAB(mm_read_keytab_4,mm_read_keys_4);
fdb mm_read_fdb_4 = { _CMKEY, CM_NLH|KEY_PTR|KEY_WID, NULL, 
			  (pdat)&mm_read_keytab_4, 
			  "FILING command, ",
			  NULL, NULL, NULL };

keywrd mm_read_keys_5[] = {
    { "define", 0, (keyval) CMD_DEFINE },
    { "profile", 0, (keyval) CMD_PROFILE },
    { "save-init", 0, (keyval) CMD_CREATE_INIT },
    { "set", 0, (keyval) CMD_SET },
    { "show", 0, (keyval) CMD_SHOW },
};
CMKEYTAB(mm_read_keytab_5,mm_read_keys_5);
fdb mm_read_fdb_5 = { _CMKEY, CM_NLH|KEY_PTR|KEY_WID, NULL, 
			  (pdat)&mm_read_keytab_5, 
			  "CUSTOMIZATION command, ",
			  NULL, NULL, NULL };

keywrd mm_read_keys_6[] = {
    { "check", 0, (keyval) CMD_CHECK },
    { "daytime", 0, (keyval) CMD_DAYTIME },
    { "finger", 0, (keyval) CMD_FINGER },
    { "pwd", 0, (keyval) CMD_PWD },
    { "status", 0, (keyval) CMD_STATUS },
    { "version", 0, (keyval) CMD_VERSION },
    { "who", 0, (keyval) CMD_WHO },
};
CMKEYTAB(mm_read_keytab_6,mm_read_keys_6);
fdb mm_read_fdb_6 = { _CMKEY, CM_NLH|KEY_PTR|KEY_WID, NULL, 
			  (pdat)&mm_read_keytab_6, 
			  "INFORMATION command, ",
			  NULL, NULL, NULL };

keywrd mm_read_keys_7[] = {
    { "blank", 0, (keyval) CMD_BLANK },
    { "bug", 0, (keyval) CMD_BUG },
    { "cd", 0, (keyval) CMD_CD },
    { "continue", 0, (keyval) CMD_CONTINUE },
    { "echo", 0, (keyval) CMD_ECHO },
    { "edit", 0, (keyval) CMD_EDIT },
    { "jump", 0, (keyval) CMD_JUMP },
    { "list", 0, (keyval) CMD_LIST },
    { "literal", 0, (keyval) CMD_LITERAL },
    { "push", 0, (keyval) CMD_PUSH },
    { "route", 0 , (keyval) CMD_ROUTE },
    { "send", 0, (keyval) CMD_SEND },
    { "spell", 0, (keyval) CMD_SPELL },
    { "take", 0, (keyval) CMD_TAKE },
};
CMKEYTAB(mm_read_keytab_7,mm_read_keys_7);
fdb mm_read_fdb_7 = { _CMKEY, CM_NLH|KEY_PTR|KEY_WID, NULL, 
			  (pdat)&mm_read_keytab_7, 
			  "some OTHER command, ",
			  NULL, NULL, NULL };

keywrd mm_read_keys_abbr[] = {
    { "delete", KEY_INV, (keyval) CMD_DELETE },
    { "headers", KEY_INV, (keyval) CMD_HEADERS },
    { "kill", KEY_INV, (keyval) CMD_KILL },
    { "literal", KEY_INV, (keyval) CMD_LITERAL },
    { "move", KEY_INV, (keyval) CMD_MOVE },
    { "previous", KEY_INV, (keyval) CMD_PREVIOUS },
    { "reply", KEY_INV, (keyval) CMD_REPLY },
    { "send", KEY_INV, (keyval) CMD_SEND },
    { "type", KEY_INV, (keyval) CMD_TYPE },
    { "undelete", KEY_INV, (keyval) CMD_UNDELETE },
};
CMKEYTAB(mm_read_keytab_abbr,mm_read_keys_abbr);
fdb mm_read_fdb_abbr = { _CMKEY, CM_NLH|CM_SDH|KEY_PTR|KEY_WID, NULL, 
			     (pdat)&mm_read_keytab_abbr,
			     NULL, NULL, NULL, "Invalid command" };

keywrd mm_read_keys_inv[] = {
    { "create-init", 0, (keyval) CMD_CREATE_INIT },
    { "debug", KEY_INV, (keyval) CMD_DEBUG },
    { "debug-memory", KEY_INV, (keyval) CMD_MEMDEBUG },
    { "headers", 0, (keyval) CMD_HEADERS },
    { "mail", 0, (keyval) CMD_SEND },
    { "z", KEY_INV, (keyval) CMD_Z },
};
CMKEYTAB(mm_read_keytab_inv,mm_read_keys_inv);
fdb mm_read_fdb_inv = { _CMKEY, CM_NLH|CM_SDH|KEY_PTR|KEY_WID, NULL, 
			    (pdat)&mm_read_keytab_inv,
			    NULL, NULL, NULL, NULL };




/* 
 * Keyword table for displaying header fields
 */

keywrd hdr_keys[] = {
    { "bcc", 0, (keyval) CMD_BCC },
    { "cc", 0, (keyval) CMD_CC },
    { "fcc", 0, (keyval) CMD_FCC },
    { "from", 0, (keyval) CMD_FROM },
    { "in-reply-to", 0, (keyval) CMD_IN_REPLY_TO },
    { "reply-to", 0, (keyval) CMD_REPLY_TO },
    { "subject", 0, (keyval) CMD_SUBJECT },
    { "text", 0, (keyval) CMD_TEXT },
    { "to", 0, (keyval) CMD_TO },
    { "user-header", 0, (keyval) CMD_USER_HEADER }
};

keytab hdr_keytab = { sizeof (hdr_keys) / sizeof (keywrd), hdr_keys };
fdb hdr_cmd_fdb = { _CMKEY, 0, nil, (pdat) &hdr_keytab, "header field, " };


keywrd disp_keys[] = {
    { "all", 0, (keyval) CMD_ALL },
    { "header", 0, (keyval) CMD_HEADER },
};

keytab disp_keytab = { sizeof (disp_keys) / sizeof (keywrd), disp_keys};
fdb disp_cmd_fdb = { _CMKEY, 0, nil, (pdat) &disp_keytab, NULL, "all" };

keywrd erase_keys[] = {
    { "all", 0, (keyval) CMD_ALL },
};
keytab erase_keytab = { sizeof (erase_keys) / sizeof (keywrd), erase_keys};
fdb erase_cmd_fdb = { _CMKEY, 0, nil, (pdat) &erase_keytab, NULL, "text" };

keywrd reply_to_keys[] = {
    { "all", 0, (keyval) CMD_ALL },
    { "none", 0, (keyval) CMD_QUIT },	/* temporary way to abort */
    { "sender", 0, (keyval) CMD_SENDER },
};
keytab reply_to_keytab = { sizeof(reply_to_keys)/sizeof(keywrd),
			       reply_to_keys};
fdb reply_to_fdb = { _CMKEY, 0, nil,  (pdat) &reply_to_keytab, NULL, "sender"};

keywrd include_keys[] = {
    { "including", 0, (keyval) CMD_INCLUDE },
    { "not-including", 0, (keyval) CMD_NOINCLUDE },
};
keytab include_keytab = { sizeof(include_keys)/sizeof(keywrd),
			       include_keys};
fdb include_fdb = { _CMKEY, 0, nil,  (pdat) &include_keytab, NULL,
			"not-including"};

int
nocmd(n)
int n;
{
    confirm ();
    printf ("Sorry, this command is not yet implemented.\n");
}

/*
 * This array is indexed by the main parse loop to dispatch to the
 * appropriate handler routine for each routine.
 */

int nocmd (),
    cmd_alias(),
    cmd_bcc (),
    cmd_blank (),
    cmd_browse (),
    cmd_bug (),
    cmd_cc (),
    cmd_cd (),
    cmd_check (),
    cmd_continue(),
    cmd_copy(),
    cmd_count(),
    cmd_create_init(),
    cmd_next(),
    cmd_daytime (),
    cmd_debug (),
    cmd_debug_memory (),
    cmd_define (),
    cmd_display (),
    cmd_echo (),
    cmd_edit(),
    cmd_erase(),
    cmd_exit (),
    cmd_expunge (),
    cmd_fcc(),
    cmd_finger(),
    cmd_forward(), 
    cmd_from (),
    cmd_get (),
    cmd_headers (),
    cmd_help(),
    cmd_in_reply_to(),
    cmd_insert(),
    cmd_jump(),
    cmd_keyword(),
    cmd_list(),
    cmd_literal(),
    cmd_mark (),
    cmd_next (),
    cmd_previous (),
    cmd_print(),
    cmd_profile (),
    cmd_push (),
    cmd_pwd(),
    cmd_quit (),
    cmd_read (),
    cmd_remail(),
    cmd_remove (),
    cmd_reply(),
    cmd_reply_to(),
    cmd_restore_draft(),
    cmd_route(),
    cmd_save_draft(),
    cmd_send (),
    cmd_set (),
    cmd_show (),
    cmd_sort (),
    cmd_smail(),
    cmd_spell (),
    cmd_status (), 
    cmd_subject (),
    cmd_suspend (),
    cmd_take (), 
    cmd_text (),
    cmd_to (),
    cmd_type (),
    cmd_unkeyword (),
    cmd_user_header (),
    cmd_version (),
    cmd_who(),
    cmd_write();

int (*mm_cmds[])() = {
    cmd_alias,			/* CMD_ALIAS */
    cmd_reply,			/* CMD_ANSWER */
    nocmd,			/* CMD_BACKTRACK */
    cmd_bcc,			/* CMD_BCC */
    cmd_blank,			/* CMD_BLANK */
    cmd_browse,			/* CMD_BROWSE */
    cmd_bug,			/* CMD_BUG */
    cmd_cc,			/* CMD_CC */
    cmd_cd,			/* CMD_CD */
    cmd_check,			/* CMD_CHECK */
    cmd_continue,		/* CMD_CONTINUE */
    cmd_copy,			/* CMD_COPY */
    cmd_count,			/* CMD_COUNT */
    cmd_create_init,		/* CMD_CREATE_INIT */
    cmd_daytime,		/* CMD_DAYTIME */
    cmd_debug,			/* CMD_DEBUG */
    cmd_debug_memory,		/* CMD_MEMDEBUG */
    cmd_define,			/* CMD_DEFINE */
    cmd_mark,			/* CMD_DELETE */
    cmd_display,		/* CMD_DISPLAY */
    cmd_echo,			/* CMD_ECHO */
    cmd_edit,			/* CMD_EDIT */
    cmd_erase,			/* CMD_ERASE */
    cmd_get,			/* CMD_EXAMINE */
    cmd_exit,			/* CMD_EXIT */
    cmd_expunge,		/* CMD_EXPUNGE */
    cmd_fcc,			/* CMD_FCC */
    cmd_finger,			/* CMD_FINGER */
    cmd_mark,			/* CMD_FLAG */
    nocmd,			/* CMD_FOLLOW */
    cmd_forward,		/* CMD_FORWARD */
    cmd_from,			/* CMD_FROM */
    cmd_get,			/* CMD_GET */
    cmd_headers,		/* CMD_HEADERS */
    cmd_help,			/* CMD_HELP */
    cmd_insert,			/* CMD_INSERT */
    cmd_in_reply_to,		/* CMD_IN_REPLY_TO */
    cmd_jump,			/* CMD_JUMP */
    cmd_keyword,		/* CMD_KEYWORD */
    cmd_next,			/* CMD_KILL */
    cmd_list,			/* CMD_LIST */
    cmd_literal,		/* CMD_LITERAL */
    cmd_mark,			/* CMD_MARK */
    cmd_copy,			/* CMD_MOVE */
    cmd_next,			/* CMD_NEXT */
    cmd_previous,		/* CMD_PREVIOUS */
    cmd_print,			/* CMD_PRINT */
    cmd_profile,		/* CMD_PROFILE */
    cmd_push,			/* CMD_PUSH */
    cmd_pwd,			/* CMD_PWD */
    cmd_quit,			/* CMD_QUIT */
    cmd_quit,			/* CMD_QQUIT */
    cmd_read,			/* CMD_READ */
    cmd_remail,			/* CMD_REMAIL */
    cmd_remove,			/* CMD_REMOVE */
    cmd_reply,			/* CMD_REPLY */
    cmd_reply_to,		/* CMD_REPLY_TO */
    cmd_restore_draft,		/* CMD_RESTORE_DRAFT */
    cmd_read,			/* CMD_REVIEW */
    cmd_route,			/* CMD_ROUTE */
    cmd_save_draft,		/* CMD_SAVE_DRAFT */
    cmd_send,			/* CMD_SEND */
    cmd_set,			/* CMD_SET */
    cmd_show,			/* CMD_SHOW */
    cmd_smail,			/* CMD_SMAIL */
    cmd_sort,			/* CMD_SORT */
    cmd_spell,			/* CMD_SPELL */
    cmd_status,			/* CMD_STATUS */
    cmd_subject,		/* CMD_SUBJECT */
    cmd_suspend,		/* CMD_SUSPEND */
    cmd_take,			/* CMD_TAKE */
    cmd_text,			/* CMD_TEXT */
    cmd_to,			/* CMD_TO */
    cmd_type,			/* CMD_TYPE */
    cmd_mark,			/* CMD_UNANSWER */
    cmd_mark,			/* CMD_UNDELETE */
    cmd_mark,			/* CMD_UNFLAG */
    cmd_unkeyword,		/* CMD_UNKEYWORD */
    cmd_mark,			/* CMD_UNMARK */
    cmd_user_header,		/* CMD_USER_HEADER */
    cmd_version,		/* CMD_VERSION */
    cmd_who,			/* CMD_WHO */
    cmd_write,			/* CMD_WRITE */
    cmd_suspend,		/* CMD_Z */
};
