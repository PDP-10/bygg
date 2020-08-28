#include <sys/types.h>
#include <grp.h>
#include <stdio.h>

struct group* g;

int main(int argc, char* argv[])
{
  setgrent();
  for (;;) {
    g = getgrent();
    if (g == NULL) break;
    printf("group %s, gid = %d\n", g->gr_name, g->gr_gid);
  }
  endgrent();
}
