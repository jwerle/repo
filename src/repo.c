
#include <assert.h>
#include <repo.h>

void
repo_help_commands () {
  // commands
  out("");
  out("commands:");
  out("   ls           List all git repositories");
  out("   clone <url>  Clone a repo into your repos path");
}


void
repo_help (repo_session_t *sess, bool show_commands) {
  command_help(&sess->program);
  if (false != show_commands) repo_help_commands();
  repo_session_free(sess);
  exit(0);
}

int
repo_args_index (const char *str) {
  repo_session_t *sess = repo_session_get_current();

  for (int i = 0; i < sess->argc; ++i) {
    // printf("%s %s %d\n", str, (char *)sess->argv[i], strcmp(str, (char *)sess->argv[i]));
    if (0 == strcmp(str, (char *)sess->argv[i])) {
      return i;
    }
  }

  return -1;
}

repo_user_t *
repo_user_new () {
  repo_user_t *user;
  long size;
  char cwd[256];
  struct passwd *pw = getpwuid(getuid());
  const char *homedir = pw->pw_dir;
  
  if (!(user = malloc(sizeof(repo_user_t)))) 
    return NULL;

  user->name = getlogin();
  user->homedir = homedir;

  if (!getcwd(cwd, sizeof(cwd)) && ERANGE != errno) {
    perror("repo: user: error: failed to set cwd");
    exit(0);
  }
  
  return user;
}


repo_t *
repo_new (char *path) {
  repo_t *repo;
  
  if (!(repo = malloc(sizeof(repo_t)))) 
    return NULL;

  repo->path = path;
  return repo;
}


repo_t *
repo_set (repo_user_t *user, char *path) {
  char *abspath;
  sprintf(abspath, "%s/%s", user->homedir, path);
  user->repo = repo_new(abspath);
  return user->repo;
}


void
repo_free (repo_user_t *user) {
  free(user->repo);
  free(user);
}



repo_dir_t *
repo_dir_new (char *path) {
  struct dirent *fd;
  repo_dir_t *dir;
  DIR *dir_;

  if (!repo_is_dir(path))
    return NULL;

  if (!(dir = malloc(sizeof(repo_dir_t))))
    return NULL;
  
  if (!(dir_ = opendir(path)))
    return NULL;

  dir->length = 0;
  dir->path = path;

  while ((fd = readdir(dir_))) {
    if (0 == strncmp(".", &fd->d_name[0], 1)) continue;
    repo_dir_item_t *item = repo_dir_item_new(path, fd, dir);
  }
  
  closedir(dir_);
  
  return dir;
}



repo_dir_item_t *
repo_dir_item_new (char *root, struct dirent *fd, repo_dir_t *dir) {
  int i = dir->length++;
  repo_dir_item_t *item = &dir->items[i];
  if (!item) return NULL;
  
  char *name = malloc(REPO_NAME_MAX * sizeof(char *));
  sprintf(name, "%s", fd->d_name);
  
  char *path = malloc(REPO_PATH_MAX * sizeof(char *));
  sprintf(path, "%s/%s", root, fd->d_name);

  item->fd_ = fd;
  item->ino = (int)fd->d_ino;
  item->name = name;
  item->path = path;
  item->is_git_repo = false;

  if (true == repo_is_git_repo(item)) {
    repo_git_init(item);
  }

  assert((int) strlen(item->name) == (int) strlen(fd->d_name));

  return item;
}

bool
repo_is_dir (char *path) {
  struct stat s;
  int err = stat(path, &s);
  
  if (-1 == err) return false;
  else if (S_ISDIR(s.st_mode)) return true;
  else return false;
}

// commands
void
repo_cmd_parse (repo_session_t *sess) {
  command_t cmd = sess->program;
  command_parse(&cmd, sess->argc, sess->argv);
}

bool
repo_cmd_is (const char *cmd) {
  repo_session_t *sess = repo_session_get_current();
  if (0 == strcmp(cmd, sess->argv[1])) return true;
  else return false;
}

bool
repo_has_cmds (repo_session_t *sess) {
  if (sess->argc > 1 && 0 != strncmp("-", sess->argv[1], 1)) 
    return true;
  return false;
}

bool
repo_cmd_has (const char *cmd) {
  repo_session_t *sess = repo_session_get_current();
  if (1 == sess->argc) return false;
  for (int i = 0; i < sess->argc; ++i) {
    if (0 == strcmp(cmd, sess->argv[i])) {
      return true;
    }
  }

  return false;
}


bool
repo_cmd_needs_help (repo_session_t *sess) {
  if (1 == sess->argc) return false;
  for (int i = 0; i < sess->argc; ++i) {
    if (0 == strcmp("--help", sess->argv[i]) || 0 == strcmp("-h", sess->argv[i])) {
      return true;
    }
  }

  return false;
}

bool
repo_cmd_is_flag (char *cmd) {
  return 0 == strncmp("-", cmd, 1)? true : false;
}

bool
repo_cmd_has_flag (const char *flag) {
  repo_session_t *sess = repo_session_get_current();
  
  if (1 == sess->argc)
    return false;

  for (int i = 0; i < sess->argc; ++i) {
    char longname[64], shortname[64];
    sprintf(longname, "--%s", flag);
    snprintf(shortname, 3, "-%s", flag);

    if (0 == strcmp(longname, sess->argv[i])) 
      return true;
    
    if (0 == strcmp(shortname, sess->argv[i])) 
      return true;
  }

  return false;
}


char *
repo_str_replace (char str[], 
                  const char *search, 
                  const char *replacement, 
                  size_t size) {

  char *pch;
  pch = strstr(str, search);
  strncpy (pch, replacement, (int) size);
  return str;

}
