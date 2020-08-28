#include <stdio.h>

int main(int argc, char* argv[])
{
  union x {
    short s;
    void* p;
  } x;

  x.p = (void*) 0x12345678;
  printf("s=%x\n", x.s);
}
