
#include <assert.h>
#include "repo.h"


repo_user_t *
repo_user_new () {
  repo_user_t *user;
  long size;
  char cwd[256];
  struct passwd *pw = getpwuid(getuid());
  const char *homedir = pw->pw_dir;
  
  if (!(user = malloc(sizeof(repo_user_t)))) 
    return NULL;

  user->name = getlogin();
  user->homedir = homedir;

  if (!getcwd(cwd, sizeof(cwd)) && ERANGE != errno) {
    perror("repo: user: error: failed to set cwd");
    exit(0);
  }
  
  return user;
}


repo_t *
repo_new (char *path) {
  repo_t *repo;
  
  if (!(repo = malloc(sizeof(repo_t)))) 
    return NULL;

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



repo_dir_t *
repo_dir_new (char *path) {
  struct dirent *fd;
  repo_dir_t *dir;
  DIR *dir_;

  if (!(dir = malloc(sizeof(repo_dir_t))))
    return NULL;
  
  if (!(dir_ = opendir(path)))
    return NULL;

  dir->length = 0;
  dir->path = path;

  while ((fd = readdir(dir_))) {
    if (0 == strncmp(".", &fd->d_name[0], 1)) continue;
    repo_dir_item_t *item = repo_dir_item_new(path, fd, dir);
  }
  
  closedir(dir_);
  
  return dir;
}



repo_dir_item_t *
repo_dir_item_new (char *root, struct dirent *fd, repo_dir_t *dir) {
  int i = dir->length++;
  repo_dir_item_t *item = &dir->items[i];
  if (!item) return NULL;
  
  char *name = malloc(REPO_NAME_MAX * sizeof(char *));
  sprintf(name, "%s", fd->d_name);
  
  char *path = malloc(REPO_PATH_MAX * sizeof(char *));
  sprintf(path, "%s/%s", root, fd->d_name);

  item->fd_ = fd;
  item->ino = (int)fd->d_ino;
  item->name = name;
  item->path = path;
  item->is_git_repo = false;

  if (true == repo_is_git_repo(item)) {
    repo_git_init(item);
  }

  assert((int) strlen(item->name) == (int) strlen(fd->d_name));

  return item;
}





