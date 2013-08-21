
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

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h> // getlogin(), getcwd(), getuid()
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
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
  char path[REPO_PATH_MAX];
} repo_t;


/**
 * Type structure that represents a user
 *
 * @typedef `repo_user_t`
 * @struct `repo_user`
 */

typedef struct repo_user {
  char cwd[REPO_PATH_MAX];
  char homedir[REPO_PATH_MAX];
  char name[REPO_NAME_MAX];
  repo_t *repo;
} repo_user_t;



typedef struct repo_dir_item {
  int *ino;
  char name[REPO_NAME_MAX];
  char path[REPO_PATH_MAX];
  int *length;
  char *type;
  struct dirent *dirent_;
} repo_dir_item_t;


/**
 * Type structure that represents a directory
 *
 * @typedef `repo_dir_t`
 * @struct `repo_dir`
 */

typedef struct repo_dir {
  char path[REPO_PATH_MAX];
  repo_dir_item_t *items;
  int length;
  DIR *dir_;
} repo_dir_t;




REPO_EXTERN(repo_user_t *)
repo_user_new ();

REPO_EXTERN(repo_t *)
repo_new (char *path);

REPO_EXTERN(void)
repo_free (repo_user_t *user);

REPO_EXTERN(repo_t *)
repo_set (repo_user_t *user, char *path);

REPO_EXTERN(void)
repo_ls (repo_t *repo);

REPO_EXTERN(repo_dir_t *)
repo_dir_new (char *path);

REPO_EXTERN(repo_dir_item_t *)
repo_dir_item_new (struct dirent *fd, repo_dir_t *root);

#ifdef __cplusplus
}
#endif


#endif
