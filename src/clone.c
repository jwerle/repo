
#include <assert.h>
#include "repo.h"

static void 
print_progress (const git_progress_payload_t *payload) {
	int network_percent = (100 *payload->fetch_progress.received_objects) / payload->fetch_progress.total_objects;
	int index_percent = (100*payload->fetch_progress.indexed_objects) / payload->fetch_progress.total_objects;
	int checkout_percent = (payload->total_steps > 0)? (100 * payload->completed_steps) / payload->total_steps : 0.f;
	int kbytes = payload->fetch_progress.received_bytes / 1024;

	printf("net: %3d (%4d kb, %5d/%5d) \n", network_percent, kbytes, payload->fetch_progress.received_objects, payload->fetch_progress.total_objects);
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
repo_clone (repo_dir_t *repo, const char *url, const char *path) {
	int error;
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

	error = git_clone(&cloned_repo, url, path, &clone_opts);
	printf("\n");
	if (error != 0) {
		const git_error *err = giterr_last();
		if (err) printf("ERROR %d: %s\n", err->klass, err->message);
		else printf("ERROR %d: no detailed info\n", error);
	}
	else if (cloned_repo) git_repository_free(cloned_repo);
	return error;
}