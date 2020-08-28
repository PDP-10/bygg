/*
** This module contains routines that might be more or less dependent of
** the operating system.
*/

#include "disass.h"

/*
** sy_getenv() returns the value of the specified environment variable.
*/

char* sy_getenv(char* variable)
{
  return(getenv(variable));
}

/*
** sy_setenv() sets the value of the specified environment variable.
*/

void sy_setenv(char* variable, char* value)
{
  (void) setenv(variable, value, 1);
}

/*
** sy_environment() will be called at startup.  The task is to scan
** all the environment variables of this process, and if any are
** found with a name like "DISASS_foo", then the symbol foo is to
** be created and given the value of that variable.
*/

void sy_environment(void)
{
  extern char** environ;
  
  /* some code missing here. */
}
