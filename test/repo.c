
#include <assert.h>
#include <repo.h>

int
main (int argc, char *argv[]) {
  repo_session_t *sess = repo_sesssion_init(argc, argv);



  repo_session_start(sess);
  repo_clone(sess->user->repo, "https://github.com/humanshell/assembly.git", "assembly");
  repo_session_free(sess);
  puts("pass +");
  return 0;
}

