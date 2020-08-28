/*
** This is the main starting point for the grand disassembler.
*/

#include "disass.h"

int main(int argc, char* argv[])
{
  UNUSED(argc);			/* We don't care about any arguments. */
  UNUSED(argv);

  sy_environment();		/* Check out the surrounding area. */

  toploop();			/* Call the main program. */

  return 0;			/* Dummy, we never get here. */
}

/* end of file */
