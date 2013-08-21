
#include "repo.h"

void
repo_ls (repo_t *repo) {
  repo_dir_t *dir = repo_dir_new(repo->path);
  printf("len-%d\n", dir->length);
  printf("%s\n", dir->items[0].name);
  for (int i = 0; i < dir->length; ++i) {
    printf("%s\n"
        , dir->items[i].path);
        //, dir->items[i].path);
  }
}
