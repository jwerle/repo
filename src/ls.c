
#include <assert.h>
#include <repo.h>

void
repo_cmd_ls (repo_session_t *sess) {
  if (sess->argc > 1) {
    if (repo_cmd_needs_help(sess)) {
      repo_help(sess, false);
      exit(0);
    }

    if (repo_cmd_has_flag("file")) {
      puts("file");
      exit(0);
    }
  }

  repo_dir_ls(sess->user->repo);

  // repo_session_free(sess);
  // exit(0);
}

void
repo_dir_ls (repo_t *repo) {
  repo_dir_t *dir = repo_dir_new(repo->path);
  
  assert(dir->path);
  assert(dir->items);
  assert(dir->length);

  for (int i = 0; i < dir->length; ++i) {
    repo_dir_item_t *item = &dir->items[i];
    if (item->is_git_repo && !item->is_git_orphan) {
      printf(" (%s) %s\n"
        , item->git_branch
        , item->name
      );
    }
  }

  free(dir);
  exit(0);
}
