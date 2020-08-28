#include <stdio.h>
#include <stdbool.h>

struct foo {
  void* x;
  char* y;
  bool b;
  bool c;
  short s;
};

int main(int argc, char* argv[])
{
  printf("size = %d\n", sizeof(struct foo));
}
