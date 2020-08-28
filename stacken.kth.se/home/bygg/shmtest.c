#include <machine/param.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
  int shmid;

  shmid = shmget(IPC_PRIVATE, 1024, 0600);

  printf("return value = %d\n", shmid);
}
