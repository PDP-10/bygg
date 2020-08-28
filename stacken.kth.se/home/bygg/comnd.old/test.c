#include "comnd.h"

static bool done;

void cmd_exit(void)
{
  cm_noise("this program");	/* Give noise words if completed. */
  cm_confirm();			/* Parse an end-of-line here. */
  done = true;			/* Set flag to exit main loop. */
}

void cmd_help(void)
{
  cm_confirm();			/* Parse an end-of-line here. */
  printf("\
\n\
This is a program to test the comnd parsing package.\n\
\n\
");
}

void cmd_number(void)
{
  cm_pnum(NULL, NUM_UNIX, 10);
  cm_confirm();
  printf("number given: %d\n", (int) pval.num.number);
}

void cmd_history(void)
{
  cm_confirm();
  phist();
}

void cmd_test(void)
{
  static keyword test[] = {
    { "foo", 0, 0, "test foo" },
    { "bar", 0, 0, "test bar" },
    { NULL },
  };

  cm_parse(cm_chain(cm_fdb(_CMKEY, "Keyword, ", 0, cm_ktab(test, 0)),
		    cm_fdb(_CMTOK, "! for shell escape", 0, "!"),
		    cm_fdb(_CMCFM, "confirm", 0, 0),
		    NULL));
  switch (pval.used->function) {
  case _CMKEY:
    cm_confirm();
    printf("keyword (%s) used.\n", pval.kw->key);
    break;
  case _CMTOK:
    cm_confirm();
    printf("shell escape.\n");
    break;
  case _CMCFM:
    printf("confirmed.\n");
    break;
  }
}

static keyword cmds[] = {
  { "exit",	0,	(keyval) cmd_exit,     "exits program" },
  { "help",	0,	(keyval) cmd_help,     "gives help" },
  { "number",   0,      (keyval) cmd_number,   "parse a number" },
  { "history",  0,      (keyval) cmd_history,  "print command history" },
  { "quit",    KEY_INV, (keyval) cmd_exit,     NULL },
  { "test",     0,      (keyval) cmd_test,     "tests parser" },
  { NULL },
};

int main(int argc, char* argv[])
{
  done = false;

  while (!done) {
    cm_prompt("test> ");
    cm_pcmd("Command, ", 0, cmds, 0);
  }
}
