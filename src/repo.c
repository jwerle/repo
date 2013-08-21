
#include "repo.h"


repo_user_t *
repo_user_new () {
  repo_user_t *user;
  struct passwd *pw = getpwuid(getuid());
  const char *homedir = pw->pw_dir;
  if (!(user = malloc(sizeof(repo_user_t)))) return NULL;
  strcpy(user->name, getlogin());
  strcpy(user->homedir, homedir);
  return user;
}


repo_t *
repo_new (char *path) {
  repo_t *repo;
  if (!(repo = malloc(sizeof(repo_t)))) return NULL;
  strcpy(repo->path, path);
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
  repo_dir_t *dir;
  DIR *dir_;
  struct dirent *fd;
  int i = 0, n = 0;

  if (!(dir = malloc(sizeof(repo_dir_t))))
    return NULL;

  strcpy(dir->path, path);
  printf("%s\n", path);
  if (!(dir_ = opendir(path)))
    return NULL;

  dir->items = malloc(sizeof(repo_dir_item_t *));

  while ((fd = readdir(dir_))) {
    printf("%s\n", fd->d_name);
    if (0 == strncmp(".", &fd->d_name[0], 1)) continue;
   // dir->items = (repo_dir_item_t*)realloc(dir->items, sizeof(repo_dir_item_t) * dir->items);
    dir->items[i++] = *repo_dir_item_new(fd, dir);
  }

  if (0 != errno) {
    perror("");
    exit(0);
  }

  dir->length = i;

  puts("biz");

  // closedir(dir_);

  puts("bif");

  //exit(0);

  return dir;
}



repo_dir_item_t *
repo_dir_item_new (struct dirent *fd, repo_dir_t *root) {
  repo_dir_item_t *item;
  char *path = malloc(sizeof(char *));
  if (!(item = malloc(sizeof(repo_dir_item_t)))) return NULL;
  item->ino = (int *)fd->d_ino;
  strcpy(item->name, fd->d_name);
  sprintf(path, "%s/%s", root->path, item->name);
  strcpy(item->path, path);
  free(path);
 // printf("--- %s\n", path);
  return item;
}






