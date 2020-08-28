#include <setjmp.h>
#include <stdio.h>

struct {jmp_buf j; volatile int c, i;} x, y = { 0 };

#define foo() {x = y; if (!setjmp(y.j)) longjmp(x.j, x.i ++% 3);}

char* p =
  "0X8CB&A<FOR'_A#4)70Q@D@DQ@BLL @!@CP@CA@2L4L8,1LPL#OAP @@MCNKDQLWL"
  "<EB]CNKDQLWL<EBL4L3A!DSE3L 0TL@L@H@L@RHD0@&@C@@HNG@ .dlrow ,olleh";
char* q = "                                          *";  

int main(int argc, char* argv[])
{
  if (setjmp(y.j)) {
    foo();
    for(;;) {
      if ((y.i = x.c) & 0x08) {
        foo(); x.c += (y.i << 4);
      }
      if (x.c & 0x40) {
        printf("\n");
        if (x.c == 192) {
          y.i -= 13; foo();
        }
      }
      y.i = printf(&q[42-x. c&63]) << 3; foo();
    }
  } else {
    foo();
    for (;;) {
      y.c = (*p >> 2) & 15; foo(); if (!x.i) break;
      y.c = (*p++ << 2) & 12; y.c += (*p >> 4) & 3; foo(); if (!x.i) break;
      y.c = (*p++) & 15; foo(); if (!x.i) break;
    }
  }
}
