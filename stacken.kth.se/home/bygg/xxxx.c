#include <stdio.h>

void x(int i)
{
  int j = i;
  int b = 0;
  if (i & 0xffff0000) { i >>= 16; b += 16; }
  if (i & 0x0000ff00) { i >>= 8; b += 8; }
  if (i & 0x000000f0) { i >>= 4; b += 4; }
  b += ((char[16]) { 0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4 })[i];
  printf("%3d: %d\n", j, b);
}

int main(int argc, char* argv[])
{
  int i;

  for (i = 0; i < 40; i++) {
    x(i);
  }
  x(4711);
  x(1234567);
  x(0x00345001);
}
