/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/argsort.c,v 2.1 90/10/04 18:23:28 melissa Exp $";
#endif

main(argc,argv)
int argc;
char **argv;
{
    int xstrcmp();
    int i;

    argc--, argv++;
    qsort(argv,argc,sizeof(char *), xstrcmp);
    for(i = 0; i < argc; i++)
	printf("%s ", argv[i]);
    printf("\n");
    return(0);
}

xstrcmp(a,b)
char **a,**b;
{
    return(strcmp(*a,*b));
}
