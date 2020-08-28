/*
 * Main module for hexsim.  Just do whatever startup is needed and
 * dispatch to the command decoder.
 */

#include "hexsim.h"

int main(int argc, char* argv[])
{
  /*
   *  Init all global data that needs to be inited.
   */

  cpu_init();
  mem_init();

  term_init();

  bg_init();

  /*
   *  Run the command decoder.
   */

  toploop();

  /*
   * We should not get here, ever.
   */

  return 0;
}
