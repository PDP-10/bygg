#include <stdio.h>
#include <stdbool.h>

bool flag;

int main(int argc, char* argv[])
{
  int i;

  flag = false;

  for (i = 1; i <= 5; i++) {
    flag ^= ~(bool)0;
    printf("%s %08x\n", flag? "true" : "false", (int) flag);
  }
}
