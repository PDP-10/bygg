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
void cmd_number(void);
void cmd_password(void);
void cmd_quoted(void);
void cmd_swedish(void);
void cmd_table(void);
void cmd_toggle(void);
void cmd_token(void);
void cmd_user(void);
void cmd_word(void);

/*
** table of top level commands:
*/

static keyword cmds[] = {
  { "exit",	0,	(keyval) cmd_exit,     "exits program" },
  { "file",     0,      (keyval) cmd_file,     "parses a file name" },
  { "group",    0,      (keyval) cmd_group,    "parses a group name" },
  { "help",	0,	(keyval) cmd_help,     "gives help" },
  { "history",  0,      (keyval) cmd_history,  "lists the command history" },
  { "ip",       0,      (keyval) cmd_ip,       "parses an IP address" },
  { "number",   0,      (keyval) cmd_number,   "parses a number" },
  { "password", 0,      (keyval) cmd_password, "asks for a password" },
  { "quit",     0,      (keyval) cmd_exit,     "exits program" },
  { "quoted",   0,      (keyval) cmd_quoted,   "parses a quoted string" },
  { "swedish",  0,      (keyval) cmd_swedish,  "swedish character test" },
  { "table",    0,      (keyval) cmd_table,    "test table ops" },
  { "toggle",   0,      (keyval) cmd_toggle,   "toggles help style" },
  { "token",    0,      (keyval) cmd_token,    "parses a token" },
  { "user",     0,      (keyval) cmd_user,     "parses a user name" },
  { "word",     0,      (keyval) cmd_word,     "parses a word" },
};

static keytab cmdtab = { (sizeof(cmds)/sizeof(keyword)), cmds, 0, 0};

/************************************************************************/

typedef void (parsefunc)(void);	/* Parse function. */

static void execute(parsefunc f) {(*f)();}

static bool done;
static bool helpflag;

void commandloop(void)
{
  static fdb topfdb = { _CMKEY, 0, 0, (char*) &(cmdtab), "Command, ",
			0, 0 };
  static fdb shellfdb = { _CMTOK, 0, 0, (char*) "!", "! for shell escape",
			  0, 0 };
  done = false;

  while (!done) {
    seterror();
    prompt("test> ");
    setreparse();
    parse(fdbchn(&topfdb, &shellfdb, NULL));
    helpflag = false;
    if (pval.used == &shellfdb) {
      confirm();
      printf("Sorry, the ! command does not work.\n");
    } else {
      execute((parsefunc*) pval.kw->value);
    }
  }
}

static bool help(void)
{
  if (helpflag) {
    confirm();
  }
  return(helpflag);
}

static void notyet(void)
{
  printf("Sorry, this function is not yet implemented.\n");
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
    noise("this program");
    confirm();
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
    static fdb filfdb = { _CMFIL, 0, 0, 0, 0, 0, 0 };

    parse(&filfdb);
    confirm();
    
    printf("file name given: '%s'\n", atombuffer);
  }
}

/**********************************************************************/

void cmd_group(void)
{
  if (help()) {
    printf("\
\n\
Syntax:  GROUP <user name>\n\
\n\
This command parses a group name with the _CMGRP function.\n\
\n\
");
  } else {
    confirm();
    
    notyet();
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
    static fdb topfdb = { _CMKEY, 0, 0, (char*) &(cmdtab),
			  "Command to get help for, ", 0, 0 };
    static fdb cfmfdb = { _CMCFM, 0, 0, 0, 0, 0, 0 };

    noise("me with");
    parse(fdbchn(&topfdb, &cfmfdb, NULL));
    if (pval.used == &topfdb) {	/* Subcommand given? */
      helpflag = true;
      execute((parsefunc*) pval.kw->value);
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
    confirm();

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
This command parses an IP address, using the _CMIP4/CMIP6 functions.\n\
\n\
");
  } else {
    static fdb ip4fdb = { _CMIP4, 0, 0, 0, 0, 0, 0 };
    static fdb ip6fdb = { _CMIP6, 0, 0, 0, 0, 0, 0 };
    fdb* foo;

    parse(fdbchn(&ip4fdb, &ip6fdb, NULL));
    foo = pval.used;

    confirm();
    
    if (foo == &ip4fdb) {
      printf("IPv4 address = %s\n", atombuffer);
    } else {
      printf("IPv6 address = %s\n", atombuffer);
    }
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
    static fdb numfdb = { _CMNUM, NUM_UNIX, 0, (char*) 10, 0, 0, 0 };
    int number;

    parse(&numfdb);
    number = pval.number;
    confirm();

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

    confirm();

    p = password("Password: ");
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
    static fdb qstfdb = { _CMQST, 0, 0, 0, 0, 0, 0 };

    parse(&qstfdb);
    confirm();

    printf("string given: \"%s\"\n", atombuffer);
  }
}

/**********************************************************************/

void cmd_swedish(void)
{
  if (help()) {
    printf("\
\n\
Syntax:  SWEDISH\n\
\n\
This is just a stupid test.\n\
\n\
");
  } else {
    static keyword swkeys[] = {
      { "blå",  0, 0, "swedish colour blue" },
      { "grön", 0, 0, "swedish colour green" },
      { "skär", 0, 0, "swedish colour pink" },
    };
    static keytab swtab = { (sizeof(swkeys)/sizeof(keyword)), swkeys,
			    KT_MWL, 0};
    static fdb swfdb = { _CMKEY, 0, 0, (char* ) &swtab, "Colour, ", 0, 0 };
    keyword* kw;

    noise("colour is");

    parse(&swfdb);
    kw = pval.kw;

    confirm();

    printf("You selected the %s\n", kw->descr);
  }
}

/**********************************************************************/

void tbl_add(void);
void tbl_delete(void);
void tbl_lookup(void);
void tbl_print(void);

void cmd_table(void)
{
  if (help()) {
    printf("\
\n\
Syntax:  TABLE [ADD | DELETE | LOOKUP] <word>\n\
\n\
This command test the routines that modify keyword tables.\n\
\n\
");
  } else {
    static keyword tblops[] = {
      { "add",    0, (keyval) tbl_add,    "add a keyword" },
      { "delete", 0, (keyval) tbl_delete, "delete a keyword" },
      { "lookup", 0, (keyval) tbl_lookup, "lookup a keyword" },
      { "print",  0, (keyval) tbl_print,  "show the current keyword table" },
      { "show",   0, (keyval) tbl_print,  "show the current keyword table" },
    };
    static keytab tbltab = { (sizeof(tblops)/sizeof(keyword)), tblops, 0, 0 };
    static fdb tblfdb = { _CMKEY, 0, 0, (char*) &tbltab, "table op, ", 0, 0 };

    parse(&tblfdb);
    execute((parsefunc*) pval.kw->value);
  }
}

static char* parse_word(void)
{
  static char buffer[100];	/* UGH! */

  static fdb wordfdb = { _CMFLD, 0, 0, 0, 0, 0, 0 };

  parse(&wordfdb);
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
	 bval(b), bval(pval.tbl_ambig), bval(pval.tbl_exact));
  printf("offset = %d, mcount = %d\n", pval.tbl_offset, pval.tbl_mcount);
}

static keyword tbl_kw[] = {
  { "bar", 0, 0, NULL },
  { "foo", 0, 0, NULL },
};

static keytab tbl_kt = { sizeof(tbl_kw)/sizeof(keyword), tbl_kw, 0, 0 };

/**********************************************************************/

void tbl_add(void)
{
  static fdb numfdb = { _CMNUM, NUM_US /* + CM_SDH */ , 0, (char*) 10,
			"position in table", 0, 0 };
  char* word;
  int pos;

  word = parse_word();
  parse(&numfdb);
  pos = pval.number;

  confirm();

  p_tblop(tbadd(&tbl_kt, word, pos));
}

/**********************************************************************/

void tbl_delete(void)
{
  char* word;

  word = parse_word();
  confirm();

  p_tblop(tbdel(&tbl_kt, word));
}

/**********************************************************************/

void tbl_lookup(void)
{
  char* word;

  word = parse_word();
  confirm();

  p_tblop(tblook(&tbl_kt, word));
}

/**********************************************************************/

void tbl_print(void)
{
  int i;
  keyword* kw;

  confirm();

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
      if (kw->flags & KEY_DYN) {
	printf(", KEY_DYN");
      }
      printf("\n");
    }
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
    confirm();

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
    static fdb tk1fdb = { _CMTOK, 0, 0, "allan", 0, 0, 0 };
    static fdb tk2fdb = { _CMTOK, 0, 0, "kaka", 0, 0, 0 };
    char* token;

    parse(fdbchn(&tk1fdb, &tk2fdb, NULL));
    token = pval.used->data;

    confirm();

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
    static fdb usrfdb = { _CMUSR, 0, 0, 0, 0, 0, 0 };

    parse(&usrfdb);
    confirm();
    
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
    static fdb wrdfdb = { _CMFLD, 0, 0, 0, 0, 0, 0 };

    parse(&wrdfdb);
    confirm();
    /*
    ** note that confirm() does not modify the atom buffer contents.
    ** otherwise we would have had to save the string before the confirm().
    */
    printf("Word: '%s'\n", atombuffer);
  }
}

/**********************************************************************/

int main(int argc, char* argv[])
{
  commandloop();
}
