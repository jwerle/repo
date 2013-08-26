
#ifndef __REPO_H__
#define __REPO_H__


#if __GNUC__ >= 4
# define REPO_EXTERN(type) extern \
   __attribute__((visibility("default"))) \
 type
#elif define(_MSC_VER)
# define REPO_EXTERN(type) __declspec(dllexport) type
#else
# define REPO_EXTERN(type) extern type
#endif


#ifdef _MSC_VER
# define REPO_INLINE(type) static __inline type
#else
# define REPO_INLINE(type) static inline type
#endif


#define REPO_PATH_MAX 4096
#define REPO_NAME_MAX 256
#define REPO_MAX_DIR_SIZE 500

#ifdef __cplusplus
extern "C" {
#endif

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

#include "json.h"
#include "git2.h"

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


REPO_EXTERN(repo_user_t *)
repo_user_new ();

REPO_EXTERN(repo_t *)
repo_new (char *path);

REPO_EXTERN(void)
repo_free (repo_user_t *user);

REPO_EXTERN(repo_t *)
repo_set (repo_user_t *user, char *path);


// dir
REPO_EXTERN(void)
repo_dir_ls (repo_t *repo);

REPO_EXTERN(repo_dir_t *)
repo_dir_new (char *path);

REPO_EXTERN(repo_dir_item_t *)
repo_dir_item_new(char *root, struct dirent *fd, repo_dir_t *dir);


// git
REPO_EXTERN(void)
repo_git_check (int error, const char *message, const char *extra);

REPO_EXTERN(void)
repo_git_init (repo_dir_item_t *item);

REPO_EXTERN(bool)
repo_is_git_repo (repo_dir_item_t *item);


#ifdef __cplusplus
}
#endif


#endif
