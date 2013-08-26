
#include <assert.h>
#include "repo.h"

void
repo_dir_ls (repo_t *repo) {
  repo_dir_t *dir = repo_dir_new(repo->path);
  
  assert(dir->path);
  assert(dir->items);
  assert(dir->length);

  for (int i = 0; i < dir->length; ++i) {
    repo_dir_item_t *item = &dir->items[i];
    if (item->is_git_repo && !item->is_git_orphan) {
      printf("%s (%s)\n", item->name, item->git_branch);
    }
  }
}
