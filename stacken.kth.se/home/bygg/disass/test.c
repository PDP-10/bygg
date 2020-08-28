#include <stdio.h>

void* foo;
void* bar;

#define S2VP(arg) ((void*) (((char*) 0) + arg))
#define VP2S(arg) ((short) (((char*) arg) - ((char*) 0)))

int main(int argc, char* argv[])
{
  short allan = 17, kaka = 42;

  foo = S2VP(allan);
  bar = (void*) kaka;

  allan = (short) bar;
  kaka = VP2S(foo);

  printf("allan = %d, kaka = %d\n", allan, kaka);

  allan = ((unsigned short) bar) % 11;
  kaka = ((unsigned short) VP2S(foo)) % 11;

  printf("allan = %d, kaka = %d\n", allan, kaka);
}
