


#ifndef __REPO_H__
#define __REPO_H__

#include <unistd.h> // getlogin(), getcwd(), getuid()
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h> // getpwuid()
#include <dirent.h> // readdir(), opendir(), scandir()
#include <errno.h>

#include <commander.h>
#include <json.h>
#include <git2.h>

#define REPO_VERSION "0.0.1"


#define REPO_PATH_MAX 4096
#define REPO_NAME_MAX 256
#define REPO_MAX_DIR_SIZE 500


#if __GNUC__ >= 4
# define REPO_EXTERN(type) extern                \
  __attribute__((visibility("default"))) type    
#elif defined(_MSC_VER)
# define REPO_EXTERN(type) __declspec(dllexport) type
#else
# define REPO_EXTERN(type) extern "C" type
#endif


#ifdef _MSC_VER
# define REPO_INLINE(type) static __inline type
#else
# define REPO_INLINE(type) static inline type
#endif


#define out(s) printf("%s\n", s);


#define repo_error(s)                            \
 fprintf(stderr, "repo: error: %s\n", s);


#define repo_ferror(fmt, s)                      \
 char t[256];                                    \
 sprintf(t, "repo: error: %s\n", fmt);           \
 fprintf(stderr, t, s);                          \
 exit(1);                                                               






/**
 * Type structure that represents a repos directory
 *
 * @typedef `repo_t`
 * @struct `repo`
 */

typedef struct repo {
  char *path;
} repo_t;


/**
 * Type structure that represents a user
 *
 * @typedef `repo_user_t`
 * @struct `repo_user`
 */

typedef struct repo_user {
  char *name;
  char *cwd;
  const char *homedir;
  repo_t *repo;
} repo_user_t;



typedef struct repo_dir_item {
  int ino;
  bool is_git_repo;
  bool is_git_orphan;
  bool is_bare;
  char *name;
  char *path;
  const char *git_branch;
  struct dirent *fd_;
  git_repository *git_repo;
  git_reference *git_head;
} repo_dir_item_t;


/**
 * Type structure that represents a directory
 *
 * @typedef `repo_dir_t`
 * @struct `repo_dir`
 */

typedef struct repo_dir {
  char *path;
  int length;
  repo_dir_item_t items[REPO_MAX_DIR_SIZE];
} repo_dir_t;


// git

typedef struct git_progress_payload {
  git_transfer_progress fetch_progress;
  size_t completed_steps;
  size_t total_steps;
  const char *path;
} git_progress_payload_t;


typedef struct repo_session {
  repo_user_t *user;
  command_t program;
  int argc;
  char *argv[];
} repo_session_t;

#ifdef __cplusplus
extern "C" {
#endif


repo_session_t *
repo_session_init (int argc, char *argv[]);

repo_session_t *
repo_session_start (repo_session_t *sess);

void
repo_session_free (repo_session_t *sess);

repo_session_t *
repo_session_get_current ();

repo_user_t *
repo_user_new ();

repo_t *
repo_new (char *path);

void
repo_free (repo_user_t *user);

repo_t *
repo_set (repo_user_t *user, char *path);


// dir
void
repo_dir_ls (repo_t *repo);

repo_dir_t *
repo_dir_new (char *path);

repo_dir_item_t *
repo_dir_item_new(char *root, struct dirent *fd, repo_dir_t *dir);

// util

bool
repo_is_dir (char *path);

void
repo_printf (const char *format, const char *str);

void
repo_help_commands ();

void
repo_help (repo_session_t *sess, bool show_commands);

// git
void
repo_git_check (int error, const char *message, const char *extra);

void
repo_git_init (repo_dir_item_t *item);

bool
repo_is_git_repo (repo_dir_item_t *item);

int
repo_clone (repo_t *repo, const char *url, const char *path);

// commands
bool
repo_cmd_is (const char * cmd);

bool
repo_has_cmds (repo_session_t *sess);




// command line commands
void
repo_cmd_ls (repo_session_t *sess);

void
repo_cmd_clone (repo_session_t *sess);





bool
repo_cmd_needs_help (repo_session_t *sess);

bool
repo_cmd_is_flag (char *flag);

bool
repo_cmd_has_flag (const char *flag);

void
repo_cmd_cmd (repo_session_t *sess);

bool
repo_cmd_has (const char *cmd);

void
repo_cmd_parse (repo_session_t *sess);

int
repo_args_index (const char *str);

char *
repo_str_replace (char str[], const char *search, const char *replacement, size_t size);

#ifdef __cplusplus
}
#endif

#endif