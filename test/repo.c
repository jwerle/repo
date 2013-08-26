
#include <assert.h>
#include "repo.h"

int
main (int argc, char *argv[]) {
  repo_user_t *user = repo_user_new();
  assert(user);
  assert(user->name);
  assert(user->homedir);
  assert(user->cwd);

  repo_t *repo = repo_set(user, "repos");
  assert(repo->path);

  repo_dir_ls(user->repo);
  //printf("repo->repo->path = %s\n", user->repo->path);
  repo_free(user);
  puts("pass +");
  return 0;
}

