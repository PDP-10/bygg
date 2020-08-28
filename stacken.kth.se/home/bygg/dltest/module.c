/*
 *  Module test:
 */

extern void message(char* msg);

void _init(void)
{
  message("loading...");
}

void _fini(void)
{
  message("unloading...");
}

/*
 *  Should be more code here.
 */
