#include <setjmp.h>
#include <stdio.h>

struct {jmp_buf j;} x, y;

#define foo() {x = y; if (!setjmp(y.j)) longjmp(x.j, 0); }

char* p = 
  "& @M%M%M%M%M%M%ML@@$H@CRPTEB@@4$EAPT4$EAPT20U@XDEL($RHPT2#JTAL(2&<(2"
  "&<%Q&<% &,&\"2*CK',6Z0";

int main(int argc, char* argv[])
{
  int i, c, done = 0;

  if (setjmp(y.j)) {
    for(;;) {
      foo();
      if ((i = c) & 0x08) {
	foo(); c += (i << 4);
      }
      if (c & 0x40) {
	printf("\n");
	if (c == 0xc0) {
	  done = 1; foo();
	}
      }
      for (i = 0; i < (c & 0x3f); i += 1) putchar(' ');
      putchar('*');
    }
  }

  foo();

  for (;;) {
    c = (*p >> 2) & 15; foo(); if (done) break;
    c = (*p++ << 2) & 12; c += (*p >> 4) & 3; foo(); if (done) break;
    c = (*p++) & 15; foo(); if (done) break;
  }
}
