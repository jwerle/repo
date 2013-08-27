
#include <repo.h>
#include <assert.h>

int
main (int argc, char *argv[]) {
	// initialize session
	repo_session_t *sess = repo_session_init(argc, argv);
	repo_session_start(sess);
	repo_cmd_clone(sess);
	repo_session_free(sess);
	return 0;
}