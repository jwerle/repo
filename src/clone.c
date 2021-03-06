
#include <assert.h>
#include <repo.h>
#include <libgen.h>
#include <progress.h>

progress_t *fetch_progress, *checkout_progress;

static void
on_progress_start (progress_data_t *data) {
  repo_log("clone: fetching..");
}

static void
on_progress (progress_data_t *data) {
  progress_write(data->holder);
}

static void
on_progress_end (progress_data_t *data) {
  // add new line from progress bar
  puts("");
  repo_log("clone: complete");
}

static int
on_fetch_progress (const git_transfer_progress *stats, void *data) {
  git_progress_payload_t *payload = (git_progress_payload_t*)data;
  payload->fetch_progress = *stats;

  if (0 == fetch_progress->total) {
    fetch_progress->total = (int) stats->total_objects;
  }

  progress_tick(fetch_progress, (int)stats->received_objects);

  if (fetch_progress->value != fetch_progress->total) {
    usleep(20000);
  }

  return 0;
}

static void
on_checkout_progress (const char *path, size_t current, size_t toal, void *data) {

  // do nothing here..

}


static int
on_cred_acquire (git_cred **out, const char * url, const char * username_from_url,
                 unsigned int allowed_types, void * payload) {

  repo_ferror("%s\n", "expecting authentication");
  exit(1);
  return 1;

}


int
repo_clone (repo_t *repo, const char *url, const char *path) {
  int error;
  char dest_path[256];
  sprintf(dest_path, "%s/%s", repo->path, path);

  progress_t *fprogress = progress_new(0, 50);
  fetch_progress = fprogress;
  checkout_progress = progress_new(0, 20);

  progress_on(fetch_progress, PROGRESS_EVENT_START, on_progress_start);
  progress_on(fetch_progress, PROGRESS_EVENT_PROGRESS, on_progress);
  progress_on(fetch_progress, PROGRESS_EVENT_END, on_progress_end);

  fetch_progress->fmt = "repo: clone: :percent {:bar} (:elapsed)";
  fetch_progress->bar_char = "#";
  fetch_progress->bg_bar_char = ".";

  git_progress_payload_t payload = {{ 0 }};
  git_repository *cloned_repo = NULL;
  git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
  git_checkout_opts checkout_opts = GIT_CHECKOUT_OPTS_INIT;

  checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE_CREATE;
  checkout_opts.progress_cb = on_checkout_progress;
  checkout_opts.progress_payload = &payload;

  clone_opts.checkout_opts = checkout_opts;
  clone_opts.fetch_progress_cb = &on_fetch_progress;
  clone_opts.fetch_progress_payload = &payload;
  clone_opts.cred_acquire_cb = on_cred_acquire;

  error = git_clone(&cloned_repo, url, dest_path, &clone_opts);

  if (0 != error) {
    const git_error *err = giterr_last();
    if (err){
      repo_ferror("clone: (%d) %s\n", err->klass, err->message);
    } else {
      repo_ferror("clone: (%d) unknown error\n", error);
    }
  } else if (cloned_repo) {
    git_repository_free(cloned_repo);
  }

  return error;
}


void
repo_cmd_clone (repo_session_t *sess) {
  int n = repo_args_index("clone");

  if (-1 == n)
    n = 0;

  char *remote, *tmp_dest, dest[256], abspath[256];
  repo_t *repo = sess->user->repo;

  if (NULL != (remote = sess->argv[n + 1])) {
    if (repo_cmd_needs_help(sess)) {
      repo_help(sess, false);
      exit(0);
    }

    repo_session_start(sess);
  }



  if (NULL != (tmp_dest = sess->argv[n + 2])) {
    sprintf(dest, "%s", tmp_dest);
  } else {
    char *base = basename(remote);
    sprintf(dest, "%s", repo_str_replace(base, ".git", "", 4));
  }

  sprintf(abspath, "%s/%s", repo->path, dest);

  if (repo_is_dir(abspath)) {
    repo_ferror("clone: Destination '%s' already exists", dest);
  }

  repo_printf("clone: cloning into: %s => %s\n", remote, dest);
  repo_clone(repo, remote, dest);

  repo_session_free(sess);
  exit(0);
}
