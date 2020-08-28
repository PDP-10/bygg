#include <stdio.h>
#include <stdlib.h>

#include "comnd.h"

static bool done;

void cmd_default(void)
{
  static cmkeyword test[] = {
    { "foo", 0, 0, 0, "test foo" },
    { "bar", 0, 0, 0, "test bar" },
    { NULL },
  };

  cm_default("foo");
  cm_pkey("Command, ", 0, test, 0);
  cm_confirm();

  printf("Used: %s\n", pval.kw->key);
}

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

void cmd_nd(void)
{
  cm_default("100");
  cmd_number();
}

void cmd_history(void)
{
  cm_confirm();
  cm_phist();
}

void cmd_test(void)
{
  static cmkeyword test[] = {
    { "foo", 0, 0, 0, "test foo" },
    { "bar", 0, 0, 0, "test bar" },
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
    cm_ptxt("shell command", 0);
    system(atombuffer);
    break;
  case _CMCFM:
    printf("confirmed.\n");
    break;
  }
}

static cmkeyword cmds[] = {
  { "default",  0,      0, cmd_default,  "tests defaulting" },
  { "exit",	0,	0, cmd_exit,     "exits program" },
  { "help",	0,	0, cmd_help,     "gives help" },
  { "nd",       0,      0, cmd_nd,       "test number defaulting" },
  { "number",   0,      0, cmd_number,   "parse a number" },
  { "history",  0,      0, cmd_history,  "print command history" },
  { "quit",    KEY_INV, 0, cmd_exit,     NULL },
  { "test",     0,      0, cmd_test,     "tests parser" },
  { NULL },
};

int main(int argc, char* argv[])
{
  done = false;

  while (!done) {
    cm_prompt("test> ");
    cm_pcmd("Command, ", 0, cmds, 0);
  }

  return 0;
}
