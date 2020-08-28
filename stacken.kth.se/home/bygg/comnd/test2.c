#include <stdio.h>
#include <stdlib.h>

#include "comnd.h"

/*
** forward declaration of command handlers:
*/

void cmd_exit(void);
void cmd_file(void);
void cmd_help(void);
void cmd_history(void);
void cmd_group(void);
void cmd_ip(void);
void cmd_mac(void);
void cmd_number(void);
void cmd_password(void);
void cmd_quoted(void);
void cmd_table(void);
void cmd_text(void);
void cmd_toggle(void);
void cmd_token(void);
void cmd_user(void);
void cmd_word(void);

/*
** table of top level commands:
*/

static cmkeyword cmds[] = {
  { "exit",     0, 0, cmd_exit,     "exits program" },
  { "file",     0, 0, cmd_file,     "parses a file name" },
  { "group",    0, 0, cmd_group,    "parses a group name" },
  { "help",	0, 0, cmd_help,     "gives help" },
  { "history",  0, 0, cmd_history,  "lists the command history" },
  { "ip",       0, 0, cmd_ip,       "parses an IP address" },
  { "mac",      0, 0, cmd_mac,      "parses a MAC address" },
  { "number",   0, 0, cmd_number,   "parses a number" },
  { "password", 0, 0, cmd_password, "asks for a password" },
  { "q",       KEY_INV+KEY_ABR, "quoted" },
  { "qu",      KEY_INV+KEY_ABR, "quoted" },
  { "quit",     0, 0, cmd_exit,     "exits program" },
  { "quoted",   0, 0, cmd_quoted,   "parses a quoted string" },
  { "t",       KEY_INV+KEY_ABR, "table" },
  { "table",    0, 0, cmd_table,    "test table ops" },
  { "text",     0, 0, cmd_text,     "parses a line of text" },
  { "toggle",   0, 0, cmd_toggle,   "toggles help style" },
  { "token",    0, 0, cmd_token,    "parses a token" },
  { "user",     0, 0, cmd_user,     "parses a user name" },
  { "word",     0, 0, cmd_word,     "parses a word" },
};

static cmkeytab cmdtab = { (sizeof(cmds)/sizeof(cmkeyword)), cmds, 0, 0};

/************************************************************************/

static bool done;
static bool helpflag;

void commandloop(void)
{
  static cmfdb topfdb = { _CMKEY, 0, 0, &(cmdtab), "Command, " };
  static cmfdb shellfdb = { _CMTOK, 0, 0, "!", "! for shell escape" };
  done = false;

  while (!done) {
    cm_prompt("test> ");
    cm_parse(cm_chain(&topfdb, &shellfdb, NULL));
    helpflag = false;
    if (pval.used == &shellfdb) {
      cm_ptxt("shell command", 0);
      cm_confirm();
      system(atombuffer);
    } else {
      cm_dispatch();
    }
  }
}

static bool help(void)
{
  if (helpflag) {
    cm_confirm();
  }
  return(helpflag);
}

/**********************************************************************/
/*
** Top level command handlers:
*/
/**********************************************************************/

void cmd_exit(void)
{
  if (help()) {
    printf("\
\n\
Syntax:  EXIT\n\
    or:  QUIT\n\
\n\
This command exits the program.\n\
\n\
");
  } else {
    cm_noise("this program");
    cm_confirm();
    done = true;
  }
}

/**********************************************************************/

void cmd_file(void)
{
  if (help()) {
    printf("\
\n\
Syntax:  FILE <file name>\n\
\n\
This command parses a file name with the _CMFIL function.\n\
\n\
");
  } else {
    static cmfdb filfdb = { _CMFIL, 0, 0, 0, 0, 0, 0 };

    cm_parse(&filfdb);
    cm_confirm();
    
    printf("file name given: '%s'\n", atombuffer);
  }
}

/**********************************************************************/

void cmd_group(void)
{
  if (help()) {
    printf("\
\n\
Syntax:  GROUP <group name>\n\
\n\
This command parses a group name with the _CMGRP function.\n\
\n\
");
  } else {
    cm_pgrp(NULL, 0);
    cm_confirm();
    
    printf("group name: '%s'\n", atombuffer);
  }
}

/**********************************************************************/

void cmd_help(void)
{
  if (help()) {
    printf("\
\n\
Syntax:  HELP [command]\n\
\n\
This command is used to get help.  What did you think?\n\
\n\
");
  } else {
    static cmfdb topfdb = { _CMKEY, 0, 0, &(cmdtab),
			  "Command to get help for, ", 0, 0 };
    static cmfdb cfmfdb = { _CMCFM, 0, 0, 0, 0, 0, 0 };

    cm_noise("me with");
    cm_parse(cm_chain(&topfdb, &cfmfdb, NULL));
    if (pval.used == &topfdb) {	/* Subcommand given? */
      helpflag = true;
      cm_dispatch();
    } else {
      printf("\
\n\
This is a program to test the comnd parsing package.\n\
\n\
");
    }
  }
}

/**********************************************************************/

void cmd_history(void)
{
  extern void phist(void);

  if (help()) {
    printf("\
\n\
Syntax:  HISTORY\n\
\n\
This command prints out the current command history.\n\
\n\
");
  } else {
    cm_confirm();

    phist();
  }
}

/**********************************************************************/

void cmd_ip(void)
{
  if (help()) {
    printf("\
\n\
Syntax:  IP <address>\n\
\n\
This command parses an IP address, using the _CMIP4/_CMIP6 functions.\n\
\n\
");
  } else {
    static cmfdb ip4fdb = { _CMIP4, 0, 0, 0, 0, 0, 0 };
    static cmfdb ip6fdb = { _CMIP6, 0, 0, 0, 0, 0, 0 };
    cmfdb* foo;

    cm_parse(cm_chain(&ip4fdb, &ip6fdb, NULL));
    foo = pval.used;

    cm_confirm();
    
    if (foo == &ip4fdb) {
      printf("IPv4 address = %s\n", atombuffer);
    } else {
      printf("IPv6 address = %s\n", atombuffer);
    }
  }
}

/**********************************************************************/

void cmd_mac(void)
{
  if (help()) {
    printf("\
\n\
Syntax:  MAC <address>\n\
\n\
This command parses a MAC address, using the _CMMAC function.\n\
\n\
");
  } else {
    cm_pmac(NULL, 0);
    cm_confirm();

    printf("address given: %02x:%02x:%02x:%02x:%02x:%02x\n",
	   pval.mac.addr[0],
	   pval.mac.addr[1],
	   pval.mac.addr[2],
	   pval.mac.addr[3],
	   pval.mac.addr[4],
	   pval.mac.addr[5]);
  }
}

/**********************************************************************/

void cmd_number(void)
{
  if (help()) {
    printf("\
\n\
Syntax:  NUMBER <number>\n\
\n\
This command tries to parse a number in base 10.  If the number starts\n\
with \"0\", \"0x\" or \"0b\" it will be in base 8, 16 or 2 instead.\n\
The NUM_UNIX flag to the parser turns this last feature on.\n\
\n\
");
  } else {
    static cmfdb numfdb = { _CMNUM, NUM_UNIX, 0, (char*) 10, 0, 0, 0 };
    int number;

    cm_parse(&numfdb);
    number = (int) pval.num.number;
    cm_confirm();

    printf("The string \"%s\" parsed as %d\n", atombuffer, number);
  }
}

/**********************************************************************/

void cmd_password(void)
{
  if (help()) {
    printf("\
\n\
Syntax:  PASSWORD\n\
\n\
This command will ask you for a password, using the built-in routine\n\
to read passwords without echo.\n\
\n\
");
  } else {
    char* p;

    cm_confirm();

    p = cm_password("Password: ");
    if (p == NULL) {
      printf("Aborted!\n");
    } else {
      printf("You typed the password '%s'\n", p);
    }
  }
}

/**********************************************************************/

void cmd_quoted(void)
{
  if (help()) {
    printf("\
\n\
Syntax:  QUOTED <quoted string>\n\
\n\
This command parses a quoted string.  The quote characters can be\n\
either \" or '.  Doubling the quote character inside the string counts\n\
as a single quote character.\n\
\n\
");
  } else {
    static cmfdb qstfdb = { _CMQST, 0, 0, 0, 0, 0, 0 };

    cm_parse(&qstfdb);
    cm_confirm();

    printf("string given: \"%s\"\n", atombuffer);
  }
}

/**********************************************************************/

void tbl_add(void);
void tbl_delete(void);
void tbl_enter(void);
void tbl_modify(void);
void tbl_lookup(void);
void tbl_print(void);
void tbl_reset(void);
void tbl_test(void);

cmkeyword tblops[] = {
  { "add",    0, 0, tbl_add,    "add a keyword" },
  { "delete", 0, 0, tbl_delete, "delete a keyword" },
  { "enter",  0, 0, tbl_enter,  "enter table subcommand mode" },
  { "lookup", 0, 0, tbl_lookup, "lookup a keyword" },
  { "modify", 0, 0, tbl_modify, "modify flags on a keyword" },
  { "print",  0, 0, tbl_print,  "show the current keyword table" },
  { "show",   0, 0, tbl_print,  "show the current keyword table" },
  { "rese",  KEY_INV+KEY_ABR, "reset" },
  { "reset", KEY_EMO+KEY_NOC,
		 0, tbl_reset,  "reset the table to initial state" },
  { "test",   0, 0, tbl_test,   "parse a keyword from the table" },
  { NULL },
};

void cmd_table(void)
{
  if (help()) {
    printf("\
\n\
Syntax:  TABLE [<action>] {<keyword> {<argument(s)}}\n\
\n\
This command test the routines that modify keyword tables.\n\
\n\
");
  } else {
    cm_pcmd("table op, ", 0, tblops, 0);
  }
}

static char* parse_word(void)
{
  static char buffer[100];	/* UGH! */

  cm_pwrd(NULL, 0);
  sprintf(buffer, atombuffer);
  return (buffer);
}

static char* bval(bool b)
{
  if (b) {
    return ("true");
  }
  return ("false");
}

static void p_tblop(bool b)
{
  printf("retval = %s, ambig = %s, exact = %s\n",
	 bval(b), bval(pval.tbl.ambig), bval(pval.tbl.exact));
  printf("offset = %d, count = %d\n", pval.tbl.offset, pval.tbl.count);
  if (pval.tbl.mlen > 0) {
    printf("mptr = \"%s\", mlen = %d\n", pval.tbl.mptr, pval.tbl.mlen);
  }
}

static cmkeyword tbl_kw[] = {
  { "bar", 0, NULL, NULL, NULL },
  { "foo", 0, NULL, NULL, NULL },
};

static cmkeytab tbl_kt = { sizeof(tbl_kw)/sizeof(cmkeyword), tbl_kw, 0, 0 };

/**********************************************************************/

void tbl_add(void)
{
  char* word;
  int pos;

  word = parse_word();

  cm_pnum("position in table", NUM_US, 10);
  pos = pval.num.number;

  cm_confirm();

  p_tblop(cm_tbadd(&tbl_kt, word, pos));
}

/**********************************************************************/

void tbl_delete(void)
{
  char* word;

  word = parse_word();
  cm_confirm();

  p_tblop(cm_tbdel(&tbl_kt, word));
}

/**********************************************************************/

bool tbl_done;

void tbl_return(void)
{
  cm_confirm();
  tbl_done = true;
}

void tbl_enter(void)
{
  cmkeyword exitop[] = {
    { "return", 0, 0, tbl_return, "return to main command level" },
    { NULL },
  };
  static volatile cmkeyword* fixup;

  cm_confirm();

  if (cm_tbfnd(cm_ktab(tblops, 0), "enter")) {
    fixup = pval.kw;
    fixup->flags |= (KEY_NOR | KEY_INV);
  } else {
    fixup = NULL;
  }

  tbl_done = false;
  while (!tbl_done) {
    cm_prompt("table> ");
    cm_parse(cm_chain(cm_fdb(_CMKEY, "table op, ", 0, cm_ktab(tblops, 0)),
		      cm_fdb(_CMKEY, NULL, 0, cm_ktab(exitop, 0)),
		      NULL));
    cm_dispatch();
  }

//  if (cm_tbfnd(cm_ktab(tblops, 0), "enter")) {
//    fixup = pval.kw;
//    fixup->flags &= ~(KEY_NOR | KEY_INV);
//  }

  if (fixup) {
    fixup->flags &= ~(KEY_NOR | KEY_INV);
  }
}

/**********************************************************************/

void tbl_modify(void)
{
  cmkeyword* kw;
  int op;
  int flag;
  char* abstr = NULL;

  cmkeyword setorclear[] = {
    { "set",   0, (void*) 1 },
    { "clear", 0, (void*) 0 },
    { NULL },
  };

  cmkeyword flags[] = {
    { "abbrev",     0, (void*) KEY_ABR },
    { "exact",      0, (void*) KEY_EMO },
    { "invisible",  0, (void*) KEY_INV },
    { "nocomplete", 0, (void*) KEY_NOC },
    { "norecog",    0, (void*) KEY_NOR },
    { NULL },
  };

  cm_parse(cm_chain(cm_fdb(_CMKEY, NULL, 0, &tbl_kt),
		    cm_fdb(_CMQST, NULL, 0, NULL),
		    NULL));
  if (pval.used->function == _CMQST) {
    if (!cm_tbfnd(&tbl_kt, atombuffer)) {
      pval.kw = NULL;
    }
  }
  kw = pval.kw;

  cm_pkey("operation, ", 0, setorclear, 0);
  op = (int) pval.kw->data;

  cm_pkey("flag, ", 0, flags, 0);
  flag = (int) pval.kw->data;

  if ((op == 1) && (flag == KEY_ABR)) {
    cm_noise("for keyword");
    cm_parse(cm_fdb(_CMKEY, NULL, 0, &tbl_kt));
    abstr = pval.kw->key;
  }

  cm_confirm();

  if (kw == NULL) {
    printf("Sorry, the string given does not match a keyword.\n");
    return;
  }

  if (op == 0) {
    kw->flags &= ~flag;
  } else {
    kw->flags |= flag;
    if (flag == KEY_ABR) {
      kw->data =  abstr;
    }
  }
}

/**********************************************************************/

void tbl_lookup(void)
{
  char* word;

  word = parse_word();
  cm_confirm();

  p_tblop(cm_tbluk(&tbl_kt, word));
}

/**********************************************************************/

void tbl_print(void)
{
  int i;
  cmkeyword* kw;

  cm_confirm();

  printf("keytab: count = %d, size = %d", tbl_kt.count, tbl_kt.size);
  if (tbl_kt.flags & KT_DYN) {
    printf(", KT_DYN");
  }
  printf("\n  table at: %p\n", tbl_kt.keys);

  if (tbl_kt.count > 0) {
    printf("keywords:\n");
    for (i = 0; i < tbl_kt.count; i += 1) {
      kw = &tbl_kt.keys[i];
      printf("  %p: %s", kw, kw->key);
      if (kw->flags & KEY_DYN) printf(", DYN");
      if (kw->flags & KEY_EMO) printf(", EMO");
      if (kw->flags & KEY_INV) printf(", INV");
      if (kw->flags & KEY_NOC) printf(", NOC");
      if (kw->flags & KEY_NOR) printf(", NOR");
      if (kw->flags & KEY_ABR) printf(", ABR(%s)", (char*) kw->data);
    printf("\n");
    }
  }
}

/**********************************************************************/

void tbl_reset(void)
{
  int i;

  cm_confirm();

  if (tbl_kt.flags & KT_DYN) {
    free(tbl_kt.keys);
  }
  tbl_kt.count = sizeof(tbl_kw)/sizeof(cmkeyword);
  tbl_kt.keys = tbl_kw;
  tbl_kt.flags = 0;
  tbl_kt.size = 0;
  for (i = 0; i < tbl_kt.count; i += 1) {
    tbl_kw[i].flags = 0;
  }
}

/**********************************************************************/

void tbl_test(void)
{
  cm_parse(cm_fdb(_CMKEY, NULL, 0, &tbl_kt));
  cm_confirm();
  printf("selected keyword: %s\n", pval.kw->key);
}

/**********************************************************************/

void cmd_text(void)
{
  if (help()) {
    printf("\
\n\
Syntax:  TEXT <line of text>\n\
\n\
This command parses a line of text with the _CMTXT function.\n\
\n\
");
  } else {
    cm_ptxt("line of text", 0);
    cm_confirm();

    printf("Line: %s\n", atombuffer);
  }
}

/**********************************************************************/

void cmd_toggle(void)
{
  if (help()) {
    printf("\
\n\
Syntax:  TOGGLE\n\
\n\
This command toggles the KT_MWL flag in the main command table.\n\
\n\
");
  } else {
    cm_confirm();

    cmdtab.flags ^= KT_MWL;
  }
}

/**********************************************************************/

void cmd_token(void)
{
  if (help()) {
    printf("\
\n\
Syntax:  TOKEN <token string>\n\
\n\
This command parses one token out of a short list.\n\
\n\
");
  } else {
    static cmfdb tk1fdb = { _CMTOK, 0, 0, "allan", 0, 0, 0 };
    static cmfdb tk2fdb = { _CMTOK, 0, 0, "kaka", 0, 0, 0 };
    char* token;

    cm_parse(cm_chain(&tk1fdb, &tk2fdb, NULL));
    token = pval.used->data;

    cm_confirm();

    printf("selected token: %s\n", token);
  }
}

/**********************************************************************/

void cmd_user(void)
{
  if (help()) {
    printf("\
\n\
Syntax:  USER <user name>\n\
\n\
This command parses a user name with the _CMUSR function.\n\
\n\
");
  } else {
    static cmfdb usrfdb = { _CMUSR, 0, 0, 0, 0, 0, 0 };

    cm_parse(&usrfdb);
    cm_confirm();
    
    printf("user name: '%s'\n", atombuffer);
  }
}

/**********************************************************************/

void cmd_word(void)
{
  if (help()) {
    printf("\
\n\
Syntax:  WORD <word>\n\
\n\
This command parses a word with the _CMFLD function.\n\
\n\
");
  } else {
    static cmfdb wrdfdb = { _CMFLD, 0, 0, 0, 0, 0, 0 };

    cm_parse(&wrdfdb);
    cm_confirm();
    /*
    ** note that cm_confirm() does not modify the atom buffer contents.
    ** otherwise we would have had to save the string before the cm_confirm().
    */
    printf("Word: '%s'\n", atombuffer);
  }
}

/**********************************************************************/

int main(int argc, char* argv[])
{
  commandloop();

  return 0;
}
