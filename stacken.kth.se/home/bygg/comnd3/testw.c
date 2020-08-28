#include <stdio.h>
#include <sys/ioctl.h>

#include "comnd.h"
#include "xwin.h"
#include "terminal.h"

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

void cmd_go(void)
{
  int ivalue;

  cm_confirm();

  ivalue = 1;
  if (ioctl(fileno(stdin), FIONBIO, &ivalue) < 0) {
    perror("FIONBIO");
  }

  for (;;) {
    if ((ivalue = getchar()) >= 0) {
      while (ivalue >= 0) {
	ivalue = getchar();
      }
      break;
    }
    w_background();

    /* XXX emulate something here. */

    sleep(1);
  }

  ivalue = 0;
  if (ioctl(fileno(stdin), FIONBIO, &ivalue) < 0) {
    perror("FIONBIO");
  }
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
  winindex w;
  int wty;

  static cmkeyword wtypes[] = {
    { "counters",   0, (void*) wty_counters },
    { "registers",  0, (void*) wty_registers },
    { "terminal",   0, (void*) wty_terminal },
    { "windows",    0, (void*) wty_windows },
    { NULL },
  };

  cm_pkey("window type, ", 0, wtypes, 0);
  wty = (int) pval.kw->data;
  cm_confirm();
  w = w_open(wty);
  if ((w != 0) && (wty == wty_terminal)) {
    w_setinput(w, term_char);
  }
}

void cmd_close(void)
{
  int win;

  cm_pnum("window index", 0, 10);
  win = (int) pval.num.number;
  cm_confirm();
  w_close(win);
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
  { "close",    0,      0, cmd_close,    "closes a window" },
  { "exit",	0,	0, cmd_exit,     "exits program" },
  { "go",       0,      0, cmd_go,       "starts execution" },
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
