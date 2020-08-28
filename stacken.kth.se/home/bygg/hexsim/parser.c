/*
 * Command parser for hexsim.
 */

#include "hexsim.h"
#include "comnd.h"

int argwinindex;

hexaword parse_hw(char* help)
{
  cm_pnum(help, NUM_US, 16);
  return pval.num.magnitude;
}

void cmd_close(void)
{
  int win;

  cm_pnum("window index", 0, 10);
  win = (int) pval.num.number;
  cm_confirm();
  w_close(win);
}

void cmd_exit(void)
{
  cm_noise("this program");
  cm_confirm();

  exit(0);
}

void cmd_halt(void)
{
  cm_confirm();

  bg_stop();
}

void cmd_help(void)
{
  cm_confirm();

  printf("help string...\n");
}

void cmd_load(void)
{
  cm_noise("file");
  cm_default("hex.out");
  cm_pfil("input file", 0);
  cm_confirm();

  mem_load_file(atombuffer);
}

void cmd_run(void)
{
  cm_confirm();

  bg_run();
}

void set_ips(void)
{
  cm_pnum("instructions/second", NUM_US, 10);
  cm_confirm();

  bg_ips_set((int) pval.num.number);
}

void set_mem(void)
{
  hexaword addr;
  uint8_t buf[256];
  int pos = 0;
  int i;
  cmfdb* f;

  addr = parse_hw("address");

  cm_noise("value");

  cm_default("0");

  for (;;) {
    f = cm_chain(cm_fdb(_CMCFM, NULL, 0, NULL),
		 cm_fdb(_CMNUM, "byte to set",
			NUM_US+NUM_UNIX, (void*) 16),
		 cm_fdb(_CMQST, "text string", 0, NULL),
		 NULL);
    if (pos == 0)
      f = f->next;
    cm_parse(f);
    if (pval.used->function == _CMCFM)
      break;
    if (pval.used->function == _CMQST) {
      for (i = 0; atombuffer[i] != 0; i++) {
	if (pos < 256) {
	  buf[pos++] = atombuffer[i];
	}
      }
    } else {
      if (pos < 256)
	buf[pos++] = (int) pval.num.number;
    }
  }

  for (i = 0; i < pos; i++) {
    (void) mem_deposit(ASP_PHYS, addr, buf[i]);
    addr += 1;
  }
  wc_memory();
}

void set_pc(void)
{
  hexaword newpc;

  newpc = parse_hw("new PC");
  cm_confirm();

  pc_deposit(newpc);
}

void set_psw(void)
{
  hexaword bits;

  bits = parse_hw("new value");
  cm_confirm();

  psw_write(bits);
}

void set_reg(void)
{
  int reg;
  hexaword newval;

  cm_pnum("register", NUM_US, 10);
  reg = (int) pval.num.number;

  cm_noise("value");
  newval = parse_hw("register value");

  cm_confirm();

  reg_deposit(reg, newval);
}

void win_address(void)
{
  hexaword addr;

  addr = parse_hw("new address");
  cm_confirm();

  w_setaddress(argwinindex, addr);
}

void win_name(void)
{
  /*
   *  Set window name.
   */

  cm_confirm();

  printf("not quite yet...\n");
}

void set_win(void)
{
  static cmkeyword wincmds[] = {
    { "address",   0, 0, win_address, "set window address" },
    { "name",      0, 0, win_name,    "set window name" },
    { NULL },
  };

  cm_pnum("window index", NUM_US, 10);
  argwinindex = (int) pval.num.number;

  cm_pcmd("window parameter, ", 0, wincmds, 0);
}

void cmd_set(void)
{
  static cmkeyword setcmds[] = {
    { "ips",       0, 0, set_ips,    "set instr/second" },
    { "memory",    0, 0, set_mem,    "set memory contents" },
    { "pc",        0, 0, set_pc,     "set new PC value" },
    { "psw",       0, 0, set_psw,    "set new PSW value" },
    { "register",  0, 0, set_reg,    "set register" },
    { "window",    0, 0, set_win,    "set window params" },
    { NULL },
  };

  cm_pcmd("Subcommand, ", 0, setcmds, 0);
}

void show_cpu(void)
{
  cm_confirm();

  cpu_show_hw_regs();
}

void show_ips(void)
{
  cm_confirm();

  printf("current ips = %u\n", bg_ips_get());
}

void cmd_show(void)
{
  static cmkeyword showcmds[] = {
    { "cpu",       0, 0, show_cpu,   "show cpu registers" },
    { "ips",       0, 0, show_ips,   "show ips" },
    { NULL },
  };

  cm_pcmd("Subcommand, ", 0, showcmds, 0);
}

void cmd_window(void)
{
  int winindex;
  int wintype;

  static cmkeyword wtypes[] = {
    { "context",    0, (void*) wty_context },
    { "cpu-regs",   0, (void*) wty_cpu },
    { "memory",     0, (void*) wty_memory },
    { "registers",  0, (void*) wty_registers },
    { "terminal",   0, (void*) wty_terminal },
    { "windows",    0, (void*) wty_windows },
    { NULL },
  };

  cm_pkey("window type, ", 0, wtypes, 0);
  wintype = VP2I(pval.kw->data);

  /* XXX should parse subinfo for some windows here. */

  cm_confirm();
  winindex = w_open(wintype, 0);
  if (winindex != 0) {
    switch (wintype) {
    case wty_registers:
      w_setinput(winindex, wi_reg);
      break;
    case wty_terminal:
      w_setinput(winindex, uart_input);
      break;
    }
  }
}

void cmd_step(void)
{
  int i, stat;

  cm_default("1");
  cm_pnum("number of instructions", 0, 10);
  i = (int) pval.num.number;

  cm_confirm();

  if (i > 0) {
    while (i-- > 0) {
      stat = cpu_execute();
      if (stat != 0)
	break;
    }

    if (stat != 0)
      printf("  return = %d\n", stat);
  }
}

static cmkeyword cmds[] = {
  { "close",    0,      0, cmd_close,    "close a window" },
  { "exit",	0,	0, cmd_exit,     "exits program" },
  { "halt",     0,      0, cmd_halt,     "halt the cpu" },
  { "help",	0,	0, cmd_help,     "gives help" },
  { "load",     0,      0, cmd_load,     "load file into mem" },
  { "quit",    KEY_INV, 0, cmd_exit,     NULL },
  { "run",      0,      0, cmd_run,      "start the cpu" },
  { "set",      0,      0, cmd_set,      "sets things" },
  { "show",     0,      0, cmd_show,     "shows things" },
  { "step",     0,      0, cmd_step,     "single-step cpu" },
  { "window",   0,      0, cmd_window,   "manipulate windows" },
  { NULL },
};

void toploop(void)
{
  for (;;) {
    cm_prompt("hexsim> ");
    cm_pcmd("Command, ", 0, cmds, 0);
  }
}
