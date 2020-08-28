/*
 Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 the City of New York.  Permission is granted to any individual or
 institution to use, copy, or redistribute this software so long as it
 is not sold for profit, provided this copyright notice is retained.

 Authors: Andrew Lowry, Howie Kaye
*/

#include <stdio.h>

main(argc,argv) 
char *argv[];
{
  int c,x;

  while (1) {
    printf("%s",argv[1]);
    c = getchar();
    if (c != '\n' && c != EOF)
      while ((x = getchar()) != '\n')
	if (x == EOF) exit(-1);
    if ((c == 'Y') || (c == 'y')) exit(0);
    if ((c == 'N') || (c == 'n') || (c == EOF)) exit(1);
    printf("Please answer 'Y' or 'N'\n");
  }
}
