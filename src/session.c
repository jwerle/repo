
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
on_set_repos_dir (command_t *self) {
	repo_session_t *sess = repo_session_get_current();
	char *path = sess->user->repo->path;

	if (NULL != self->arg) {
		char tmp_path[256];

		if (repo_is_dir((char *)self->arg)) {
			sess->user->repo->path = path = (char *)self->arg;
		} else {
			repo_ferror("'%s' is not a valid path", path);
		}

		repo_printf("Root directory set to '%s'\n"
				, sess->user->repo->path);
	}
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

  sess->user = user;
  sess->argc = argc;

  command_t *program = &sess->program;
	// sess->program = program;
	// init with version
  command_init(program, "repo", REPO_VERSION);
	
	// defualt options
  command_option(program, "-R", "--repos [repos]", "Directory that holds git repositories", on_set_repos_dir);

  // copy string
  for (int i = 0; i < argc; ++i) {
  	sess->argv[i] = argv[i];
  }

  current_session = sess;
	return sess;
}

repo_session_t *
repo_session_start (repo_session_t *sess) {
  command_parse(&sess->program, sess->argc, sess->argv);
  return sess;
}

void
repo_session_free (repo_session_t *sess) {
	// command_free(sess->program);
  repo_free(sess->user);
}