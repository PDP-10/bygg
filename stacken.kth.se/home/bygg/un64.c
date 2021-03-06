
#include <stdio.h>

int offset = 0;

int state = 0;

int foo;

int un64[256] = {
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

void range(unsigned char c, int count)
{
  while (count > 0) {
    un64[(int) c] = offset;
    c += 1;
    offset += 1;
    count -= 1;
  }
}

void store(int c)
{
  state += 1;

  switch (state) {
    case 1:
      foo = (c << 2);
      break;
    case 2:
      foo |= (c >> 4);
      putchar(foo);
      foo = (c << 4);
      break;
    case 3:
      foo |= (c >> 2);
      putchar(foo);
      foo = (c << 6);
      break;
    case 4:
      foo |= c;
      putchar(foo);
      foo = 0;
      state = 0;
      break;
  }
}

void main(void)
{
  int c;

  range('A', 26);
  range('a', 26);
  range('0', 10);
  range('+', 1);
  range('/', 1);

  while ((c = getchar()) != EOF) {
    if ((c = un64[c]) >= 0) {
      store(c);
    }
  }
}
