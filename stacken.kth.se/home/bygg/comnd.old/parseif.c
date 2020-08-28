#include "comnd.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <net/if_dl.h>
#include <net/if.h>

static bool done = false;

void cmd_exit(void);
void cmd_interface(void);

static keyword cmds[] = {
  { "exit",	0,	(keyval) cmd_exit,      "exits program" },
  { "interface", 0,     (keyval) cmd_interface, "parse an interface name" },
  { "quit",    KEY_INV, (keyval) cmd_exit,     NULL },
  { NULL },
};

static keytab ifktab = { 0 };

int main(int argc, char* argv[])
{
  struct ifaddrs* ifap;
  struct ifaddrs* ifa;

  if (getifaddrs(&ifap) == 0) {	/* FSCKing stupid calling convention */
    for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
      (void) (cm_tbadd(&ifktab, ifa->ifa_name, 0));
    }
  }

  freeifaddrs(ifap);

  done = false;
  while (!done) {
    cm_prompt("test> ");
    cm_pcmd("Command, ", 0, cmds, 0);
  }
}

void cmd_exit(void)
{
  cm_noise("this program");	/* Give noise words if completed. */
  cm_confirm();			/* Parse an end-of-line here. */
  done = true;			/* Set flag to exit main loop. */
}

void cmd_interface(void)
{
  keyword* kw;

  ifktab.flags |= KT_MWL;
  cm_parse(cm_fdb(_CMKEY, "interface, ", 0, &ifktab));
  kw = pval.kw;
  cm_confirm();

  printf("interface given: %s\n", kw->key);
}
