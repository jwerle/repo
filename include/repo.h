
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


#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h> // getlogin(), getcwd(), getuid()
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <pwd.h> // getpwuid()
#include <dirent.h> // readdir(), opendir(), scandir()

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
  char *cwd;
  char *homedir;
  char *name;
  repo_t *repo;
} repo_user_t;


REPO_EXTERN(repo_user_t *)
repo_user_new ();

REPO_EXTERN(repo_t *)
repo_new (char *path);

REPO_EXTERN(void)
repo_free (repo_user_t *user);

REPO_EXTERN(repo_t *)
repo_set (repo_user_t *user, char *path);

#ifdef __cplusplus
}
#endif


#endif
