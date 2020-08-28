#include "comnd.h"
#include "xwin.h"

unsigned char reg_a = 0;
unsigned short reg_bc = 0;
unsigned short reg_de = 0;
unsigned short reg_hl = 0;

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

void cmd_open(void)
{
  int wty;

  static cmkeyword wtypes[] = {
    { "counters",   0, (void*) wty_counters },
    { "registers",  0, (void*) wty_registers },
    { "windows",    0, (void*) wty_windows },
    { NULL },
  };

  cm_pkey("window type, ", 0, wtypes, 0);
  wty = (int) pval.kw->data;
  cm_confirm();
  w_open(wty);
}

void cmd_register(void)
{
  int reg;
  int val;

  static cmkeyword regs[] = {
    { "a",  0, (void*) 1 },
    { "bc", 0, (void*) 2 },
    { "de", 0, (void*) 3 },
    { "hl", 0, (void*) 4 },
    { NULL },
  };

  cm_pkey("register, ", 0, regs, 0);
  reg = (int) pval.kw->data;
  cm_pnum("value", NUM_UNIX, 10);
  val = pval.num.number;
  cm_confirm();

  switch (reg) {
  case 1:
    reg_a = val;
    break;
  case 2:
    reg_bc = val;
    break;
  case 3:
    reg_de = val;
    break;
  case 4:
    reg_hl = val;
    break;
  }
  wc_registers();
}

static cmkeyword cmds[] = {
  { "exit",	0,	0, cmd_exit,     "exits program" },
  { "help",	0,	0, cmd_help,     "gives help" },
  { "open",     0,      0, cmd_open,     "opens a window" },
  { "quit",    KEY_INV, 0, cmd_exit,     NULL },
  { "register", 0,      0, cmd_register, "sets register" },
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
