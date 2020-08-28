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
  { "exit",     0, 0, cmd_exit,     "exits program" },
  { "file",     0, 0, cmd_file,     "parses a file name" },
  { "group",    0, 0, cmd_group,    "parses a group name" },
  { "help",	0, 0, cmd_help,     "gives help" },
  { "history",  0, 0, cmd_history,  "lists the command history" },
  { "ip",       0, 0, cmd_ip,       "parses an IP address" },
  { "number",   0, 0, cmd_number,   "parses a number" },
  { "password", 0, 0, cmd_password, "asks for a password" },
  { "q",       KEY_INV+KEY_ABR, "quoted" },
  { "qu",      KEY_INV+KEY_ABR, "quoted" },
  { "quit",     0, 0, cmd_exit,     "exits program" },
  { "quoted",   0, 0, cmd_quoted,   "parses a quoted string" },
  { "t",       KEY_INV+KEY_ABR, "table" },
  { "table",    0, 0, cmd_table,    "test table ops" },
  { "toggle",   0, 0, cmd_toggle,   "toggles help style" },
  { "token",    0, 0, cmd_token,    "parses a token" },
  { "user",     0, 0, cmd_user,     "parses a user name" },
  { "word",     0, 0, cmd_word,     "parses a word" },
  { NULL },
};

keyword tblops[] = {
  { "add",      0, 0, tbl_add,      "add a keyword" },
  { "delete",   0, 0, tbl_delete,   "delete a keyword" },
  { "enter",    0, 0, tbl_enter,    "enter table subcommand mode" },
  { "lookup",   0, 0, tbl_lookup,   "lookup a keyword" },
  { "modify",   0, 0, tbl_modify,   "modify flags on a keyword" },
  { "print",    0, 0, tbl_print,    "show the current keyword table" },
  { "show",     0, 0, tbl_print,    "show the current keyword table" },
  { "reset",  KEY_EMO+KEY_NOC,
		   0, tbl_reset,    "reset the table to initial state" },
  { "test",     0, 0, tbl_test,     "parse a keyword from the table" },
  { NULL },
};
