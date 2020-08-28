#include <setjmp.h>
#include <stdio.h>

struct {jmp_buf j;} x, y;

#define foo() {x = y; if (!setjmp(y.j)) longjmp(x.j, 0);}

char* p =
  "0X8CB&A<FOR'_A#4)70Q@D@DQ@BLL @!@CP@CA@2L4L8,1LPL#OAP @@MCNKDQLWL"
  "<EB]CNKDQLWL<EBL4L3A!DSE3L 0TL@L@H@L@RHD0@&@C@@HNG@";

int main(int argc, char* argv[])
{
  int c, i = 1;

  if (setjmp(y.j)) {
    foo();
    for(;;) {
      if ((i = c) & 0x08) {
	foo();
	c += (i << 4);
      }
      if (c & 0x40) {
	printf("\n");
	if (c == 192) {
	  i -= 12;
	  foo();
	}
      }
      while (c-- & 0x3f) putchar(' ');
      i = putchar('*');
      foo();
    }
  } else {
    foo();
    for (;;) {
      c = (*p >> 2) & 15; foo(); if (!i) break;
      c = (*p++ << 2) & 12; c += (*p >> 4) & 3; foo(); if (!i) break;
      c = (*p++) & 15; foo(); if (!i) break;
    }
  }
}
