#include <stdio.h>
#include <sys/time.h>

#include "comnd.h"

static bool done;

void cmd_exit(void)
{
  cm_noise("this program");	/* Give noise words if completed. */
  cm_confirm();			/* Parse an end-of-line here. */
  done = true;			/* Set flag to exit main loop. */
}

void cmd_loop(void)
{
  struct timeval starttime, stoptime;
  struct timespec sleeptime;
  int i, limit;

  cm_noise("count");
  cm_pnum("number of iterations", 0, 10);
  limit = (int) pval.num.number;
  cm_confirm();

  (void) gettimeofday(&starttime, NULL);
  /*
  sleeptime.tv_sec = 0;
  sleeptime.tv_nsec = 1;
  (void) nanosleep(&sleeptime, NULL);
  */
  for (i = 0; i < limit; i++);
  (void) gettimeofday(&stoptime, NULL);

  printf("start: %d.%6d\n", starttime.tv_sec, starttime.tv_usec);
  printf("stop:  %d.%6d\n", stoptime.tv_sec, stoptime.tv_usec);
}

void cmd_system(void)
{
  cm_ptxt("system command", 0);
  system(atombuffer);
}

static cmkeyword cmds[] = {
  { "exi",   KEY_ABR+KEY_INV, "exit" },
  { "exit",  KEY_EMO+KEY_NOC, 0, cmd_exit,     "exits program" },
  { "loop",     0,            0, cmd_loop,     "performs a loop test" },
  { "qui",   KEY_ABR+KEY_INV, "quit" },
  { "quit",  KEY_INV+KEY_EMO+KEY_NOC, 0, cmd_exit },
  { "system",   0,            0, cmd_system,   "do a system command" },
  { NULL },
};

int main(int argc, char* argv[])
{
  done = false;

  while (!done) {
    cm_prompt("kaka> ");
    cm_parse(cm_chain(cm_fdb(_CMKEY, "Command, ", 0, cm_ktab(cmds, 0)),
		      cm_fdb(_CMTOK, "! for shell escape", 0, "!"),
		      NULL));
    if (pval.used->function == _CMKEY) {
      cm_dispatch();
    } else {
      cmd_system();
    }
  }
}
