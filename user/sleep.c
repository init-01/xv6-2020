#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int sleeptime;

  if(argc < 2){
    fprintf(2, "Usage: sleep <seconds>\n");
    exit(1);
  }

  sleeptime = atoi(argv[1]);
  if(sleep(sleeptime) < 0){
    fprintf(2, "Something went wrong\n");
    exit(1);
  }
  exit(0);
}
