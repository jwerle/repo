
#include <unistd.h>
#include <commander.h>
#include "repo.h"

int
main (int argc, char *argv[]) {

	command_t cmd;
  command_init(&cmd, "repo", REPO_VERSION);
  

  command_parse(&cmd, argc, argv);
  command_free(&cmd);
  return 0;
}
