
#include <repo.h>
#include <assert.h>
#include <commander.h>

// global current session
static repo_session_t *current_session = NULL;

repo_session_t *
repo_session_get_current () {
	return current_session;
}

void
on_set_repo (command_t *self) {
	puts("dir set");
	// printf("repos directory set to %s\n", self->arg);
}


repo_session_t *
repo_session_init (int argc, char *argv[]) {
	// free current session
	if (NULL != current_session)
		repo_session_free(current_session);

	repo_session_t *sess = malloc(sizeof(repo_session_t));
	
	if (!sess) {
		repo_error("Failed to initialize session");
	}

	repo_user_t *user = repo_user_new();
  assert(user);

  repo_t *repo = repo_set(user, "repos");
  assert(repo);

  command_t program;

  sess->user = user;
  sess->argc = argc;
	sess->program = &program;
	// init with version
  command_init(sess->program, "repo", REPO_VERSION);
	// options
  command_option(sess->program, "-R", "--repos [repos]", "Directory that holds git repositories", on_set_repo);

  // copy string
  for (int i = 0; i < argc; ++i) {
  	sess->argv[i] = argv[i];
  }

  current_session = sess;
	return sess;
}

repo_session_t *
repo_session_start (repo_session_t *sess) {
	// parse
  
  return sess;
}

void
repo_session_free (repo_session_t *sess) {
	// command_free(sess->program);
  repo_free(sess->user);
}