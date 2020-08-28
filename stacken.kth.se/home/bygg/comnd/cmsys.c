/*
**  Copyright (c) 2001 - 2004, Johnny Eriksson
**  All rights reserved.
**
**  Redistribution and use in source and binary forms, with or without
**  modification, are permitted provided that the following conditions
**  are met:
**
**  1. Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**
**  2. Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in the
**     documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "comnd.h"

/*
** Find out what code to generate:
*/

#if defined (SYS_DUMMY)         /* Force just a dummy module? */
#  include "sys_dummy.h"
#elif defined (SYS_BSD)		/* Force BSD code? */
#  include "sys_bsd.h"
#elif defined (SYS_POSIX)	/* Force POSIX? */
#  include "sys_posix.h"
#elif defined (SYS_LINUX)       /* Thorvald-ux (tux)? */
#  include "sys_linux.h"
#elif defined (SYS_SOLARIS)	/* Force SOLARIS? */
#  include "sys_sol.h"

/*
** No forced version, try heuristics:
*/

#elif defined (__FreeBSD__) || \
      defined (__NetBSD__) || \
      defined (__OpenBSD__)     /* Known BSD? */
#  include "sys_posix.h"	/*   ... should be POSIX. */

#elif defined (__linux__)       /* Thorvald-ux (tux)? */
#  include "sys_posix.h"	/*   ... should be POSIX. */

#elif defined(__sun) && defined(__SVR4) /* Solaris? */
#  include "sys_posix.h"	/*   ... should be POSIX. */

/*
** Can't do anything, have to use a dummy.
*/

#else
#  include "sys_dummy.h"
#endif
