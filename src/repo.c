
#include "repo.h"


repo_user_t *
repo_user_new () {
  repo_user_t *user;
  struct passwd *pw = getpwuid(getuid());
  const char *homedir = pw->pw_dir;
  if (!(user = malloc(sizeof(repo_user_t)))) return NULL;
  else if (!(user->name = getlogin())) return NULL;
  user->homedir = (char *)homedir;
  return user;
}


repo_t *
repo_new (char *path) {
  repo_t *repo;
  if (!(repo = malloc(sizeof(repo_t)))) return NULL;
  repo->path = path;
  return repo;
}


repo_t *
repo_set (repo_user_t *user, char *path) {
  char *abspath;
  sprintf(abspath, "%s/%s", user->homedir, path);
  user->repo = repo_new(abspath);
  return user->repo;
}


void
repo_free (repo_user_t *user) {
  free(user->repo);
  free(user);
}
