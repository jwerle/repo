
#ifndef __REPO_H__
#define __REPO_H__


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


repo_user_t *
repo_user_new ();

repo_t *
repo_new (char *path);

void
repo_free (repo_user_t *user);

repo_t *
repo_set (repo_user_t *user, char *path);

#ifdef __cplusplus
}
#endif


#endif
