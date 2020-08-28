/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/prepatch.c,v 2.1 90/10/04 18:25:34 melissa Exp $";
#endif

/*
 * see if patch exists on this system
 * if patch exists, return false.
 * if argc == 1, return true.
 * if patch doesn't exist, return true.
 */
#include "config.h"
#include "osfiles.h"
#include "compat.h"

main(argc, argv, envp) 
int argc;
char **argv, **envp;
{
    int i;
    struct stat sb;

    if (argc == 1)			/* success.   no need to do more */
	return(0);

    if (argc == 2)
	if (strcmp(argv[1], "patch.00") == 0 || stat(argv[1], &sb) < 0)
	    return(0);

    if (inpath("patch"))		/* found it.   fail, so patch can */
	return(dopatch(argc-1,argv+1));
    
    fprintf(stderr,"\
Your system does not seem to have the 'patch' program installed (or at\n\
least it is not in your path.  Hence, the following patch files cannot\n\
be applied.  You should apply them by hand before compiling MM, or\n\
pick up a copy of the patch program.  It is probably available from\n\
the same place you got MM.\n");
    for(i = 1; i < argc; i++)
	fprintf(stderr,"\t%s\n", argv[i]);
    return(1);
}


inpath(fname)
char *fname;
{
    extern char *getenv(), *index();
    char *path = getenv("PATH");
    char *cp = path;
    char *cp1,c;
    struct stat sb;
    int buf[MAXPATHLEN];
    
    while((cp1 = index(cp,':')) != NULL) {
	c = *cp1;
	*cp1 = '\0';
	sprintf(buf,"%s/%s",cp,fname);
	if (stat(buf,&sb) == 0)		/* does it exist? */
	    if (sb.st_mode & 0111)
		return(1);
	*cp1 = c;
	cp = cp1+1;
    }
    return(0);
}

dopatch(count,files)
int count;
char **files;
{
    int i;
    int ret=0;
    char buf[MAXPATHLEN+20];
    for(i = 0; i < count; i++) {
	sprintf(buf,"patch < %s", files[i]);
	fprintf(stderr, "%s\n", buf);
	ret |= system(buf);
    }
    return(ret);
}
