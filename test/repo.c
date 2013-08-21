
#include <assert.h>
#include "repo.h"

int
main (int argc, char *argv[]) {
  repo_user_t *user = repo_user_new();
  assert(user);
  repo_set(user, "repos");
  repo_ls(user->repo);
  //printf("repo->repo->path = %s\n", user->repo->path);
  repo_free(user);
  puts("pass +");
  return 0;
}
