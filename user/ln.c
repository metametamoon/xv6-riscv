#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if(argc != 3 && argc != 4){
    fprintf(2, "Usage: ln old new\n");
    exit(1);
  }
  if(argc == 3 && link(argv[1], argv[2]) < 0)
    fprintf(2, "link %s %s: failed\n", argv[1], argv[2]);
  else if(argc == 4 && strcmp(argv[1], "-s") == 0 && symlink(argv[2], argv[3]) < 0)
    fprintf(2, "symlink %s %s %s: failed\n", argv[1], argv[2], argv[3]);
  exit(0);
}
