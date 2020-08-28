/*
 Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 the City of New York.  Permission is granted to any individual or
 institution to use, copy, or redistribute this software so long as it
 is not sold for profit, provided this copyright notice is retained.

*/

/*
 * Define HAVE_VOIDSIG if your <signal.h> defines signal(2) as
 * void (*signal())(). This seems to be true in System V Release 3
 * and SunOS 4.0.
 */

#if defined(SVR3)
#define HAVE_VOIDSIG
#endif
