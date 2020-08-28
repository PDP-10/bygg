#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <dirent.h>
#include <stdio.h>

struct group* g;
struct passwd* p;
struct dirent* d;
DIR* dir;

int main(int argc, char* argv[])
{
  setgrent();
  for (;;) {
    g = getgrent();
    if (g == NULL) break;
    printf("group %s, gid = %d\n", g->gr_name, g->gr_gid);
  }
  endgrent();

  setpwent();
  for (;;) {
    p = getpwent();
    if (p == NULL) break;
    printf("user %s, uid = %d, gid = %d\n", p->pw_name, p->pw_uid, p->pw_gid);
  }
  endpwent();

  dir = opendir(".");
  for (;;) {
    d = readdir(dir);
    if (d == NULL) break;
    printf("file %s, fileno = %d\n", d->d_name, d->d_fileno);
  }
  (void) closedir(dir);
}
