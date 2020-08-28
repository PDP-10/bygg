#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "comnd.h"

static bool done;

void* handle = NULL;

void errcheck(void)
{
  const char* err;

  err = dlerror();
  if (err != NULL)
    printf("%s\n", err);
}

void cmd_exit(void)
{
  cm_noise("this program");	/* Give noise words if completed. */
  cm_confirm();			/* Parse an end-of-line here. */
  done = true;			/* Set flag to exit main loop. */
}

void cmd_load(void)
{
  cm_default("module");
  cm_ptxt("item to load", 0);

  cm_confirm();

  if (handle == NULL) {
    (void) dlerror();
    handle = dlopen(atombuffer, RTLD_LAZY);
    printf("dlopen returns %p\n", handle);
    errcheck();
  } else
    printf("already loaded.\n");
}

void cmd_unload(void)
{
  int i;

  cm_confirm();

  if (handle != NULL) {
    (void) dlerror();
    i = dlclose(handle);
    printf("dlclose returns %d\n", i);
    errcheck();
    handle = NULL;
  } else
    printf("not loaded.\n");
}

static cmkeyword cmds[] = {
  { "exit",	0,	0, cmd_exit,     "exits program" },
  { "load",     0,      0, cmd_load,     "loads module" },
  { "quit",    KEY_INV, 0, cmd_exit,     NULL },
  { "unload",   0,      0, cmd_unload,   "unloads module" },
  { NULL },
};

void message(char* msg)
{
  printf("%s\n", msg);
}

int main(int argc, char* argv[])
{
  done = false;

  while (!done) {
    cm_prompt("test> ");
    cm_pcmd("Command, ", 0, cmds, 0);
  }

  return 0;
}
