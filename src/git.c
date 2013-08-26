
#include "repo.h"

void 
repo_git_check (int error, const char *message, const char *extra) {
	const git_error *err;
	const char *msg = "", *separator = "";

	if (!error)
		return;

	if ((err = giterr_last()) && err->message) {
		msg = err->message;
		separator = " - ";
	}

	if (extra) {
		fprintf(stderr, "%s '%s' [%d]%s%s\n",
			message, extra, error, separator, msg);
	} else {
		fprintf(stderr, "%s [%d]%s%s\n",
			message, error, separator, msg);
	}

	exit(1);
}


void
repo_git_init (repo_dir_item_t *item) {
	git_repository *git_repo = item->git_repo;
	int error = 0;
	const char *branch = item->git_branch;
	git_reference *head = item->git_head;

	// open repo and check for integrity
	repo_git_check(
		git_repository_open_ext(&git_repo, item->path, 0, NULL),
		"Failed to open git repository", item->path
	);

	item->is_git_repo = true;

	// retrieve head
	error = git_repository_head(&head, git_repo);

	if (error == GIT_EORPHANEDHEAD) {
		item->is_git_orphan = true;
		branch = NULL;
	} else if (error == GIT_ENOTFOUND) {
		item->is_git_repo = false;
		branch = NULL;
	} else if (!error) {
		branch = git_reference_name(head);
		if (!strncmp(branch, "refs/heads/", strlen("refs/heads/"))) {
			branch += strlen("refs/heads/");
		}

		item->git_branch = branch;
	} else {
		repo_git_check(error, "failed to get current branch", NULL);
	}
}

bool
repo_is_git_repo (repo_dir_item_t *item) {
	struct stat s;
	char path[REPO_PATH_MAX + 5];
	sprintf(path, "%s/.git", item->path);
	int err = stat(path, &s);

	if (-1 == err && errno != ENOENT) return false;
	else if (S_ISDIR(s.st_mode)) return true;
  else return false;
}




