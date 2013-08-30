
#include <assert.h>
#include <repo.h>
#include <libgen.h>

static void 
print_progress (const git_progress_payload_t *payload) {
	int network_percent = (100 *payload->fetch_progress.received_objects) / payload->fetch_progress.total_objects;
	int index_percent = (100*payload->fetch_progress.indexed_objects) / payload->fetch_progress.total_objects;
	int checkout_percent = (payload->total_steps > 0)? (100 * payload->completed_steps) / payload->total_steps : 0.f;
	int kbytes = payload->fetch_progress.received_bytes / 1024;

  usleep(20000);
	printf("\rnet: %3d (%4d kb, %5d/%5d) ", network_percent, kbytes, payload->fetch_progress.received_objects, payload->fetch_progress.total_objects);
  fflush(stdout);
}


static int 
on_fetch_progress(const git_transfer_progress *stats, void *data) {
	git_progress_payload_t *payload = (git_progress_payload_t*)data;
	payload->fetch_progress = *stats;
	print_progress(payload);
	return 0;
}


static void 
on_checkout_progress (const char *path, size_t current, size_t toal, void *data) {
	git_progress_payload_t *payload = (git_progress_payload_t*)data;
	payload->completed_steps = current;
	payload->total_steps = toal;
	payload->path = path;
	print_progress(payload);
}


static int 
on_cred_acquire(git_cred **out,
             const char * url, 
             const char * username_from_url, 
             unsigned int allowed_types, 
             void * payload) {

	fprintf(stderr, "%s\n", "expecting authentication");
	exit(1);
	return 1;
}


int
repo_clone (repo_t *repo, const char *url, const char *path) {
	int error;
	char dest_path[256];
	sprintf(dest_path, "%s/%s", repo->path, path);

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
	
	puts("");

	if (error != 0) {
		const git_error *err = giterr_last();
		if (err) printf("ERROR %d: %s\n", err->klass, err->message);
		else printf("ERROR %d: no detailed info\n", error);
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

  puts("");
  puts("cloning..");
  printf("  %s -> %s\n", remote, dest);
  puts("");

  repo_clone(repo, remote, dest);

  repo_session_free(sess);
  exit(0);
}
