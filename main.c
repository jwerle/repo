
#include <unistd.h>
#include <assert.h>
#include <commander.h>
#include <repo.h>


int
main (int argc, char *argv[]) {
	// initialize session
	repo_session_t *sess = repo_session_start(repo_session_init(argc, argv));

  // process commands
	if (argc > 1) {
		if (repo_cmd_has("ls")) {
			repo_cmd_ls(sess);
		} else if (repo_cmd_has("cmd")) {
			repo_cmd_cmd(sess);
		} else if (repo_cmd_has("help")) {
			repo_help(sess, true);
		} else if (!repo_cmd_is_flag(argv[1])) {
			printf("repo: '%s' is not a repo command. See 'repo --help'.\n", argv[1]);
			repo_help(sess, true);
		} else if (repo_cmd_needs_help(sess)) {
			repo_help(sess, true);
		}

		// exit(0);
  // 	repo_session_free(sess);

	} else if (repo_cmd_needs_help(sess) || !repo_has_cmds(sess)) {
		repo_help(sess, true);
	}

	repo_cmd_parse(sess);

  // freedom
  repo_session_free(sess);
  return 0;
}
