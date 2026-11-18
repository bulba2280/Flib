#define _GNU_SOURCE

#include "internal.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define FLIB_REMOVE_MAX_DEPTH 1024

// Refuse to move or delete the filesystem root.
static int path_is_root(const char *path) {
  char *real = realpath(path, NULL);
  int root;

  if (!real)
    return 0;

  root = strcmp(real, "/") == 0;
  free(real);
  return root;
}

// copy_Dir() does not copy sockets or devices, so removing the source after a
// partial copy would lose them. Report whether such entries exist.
static int tree_has_unsupported(const char *path) {
  DIR *d;
  struct dirent *de;
  int found = 0;

  d = opendir(path);
  if (!d)
    return 0;

  while (!found && (de = readdir(d)) != NULL) {
    char *child;
    struct stat st;

    if (flib_is_dot_entry(de->d_name))
      continue;

    child = flib_join_path(path, de->d_name);
    if (!child)
      continue;

    if (lstat(child, &st) == 0) {
      if (S_ISDIR(st.st_mode))
        found = tree_has_unsupported(child);
      else if (!S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode) &&
               !S_ISFIFO(st.st_mode))
        found = 1;
    }

    free(child);
  }

  closedir(d);
  return found;
}

int remove_File(const char *path) {
  struct stat st;

  if (!flib_require_path(path, "file"))
    return -1;

  if (lstat(path, &st) != 0) {
    flib_set_error("cannot stat '%s': %s", path, strerror(errno));
    return -1;
  }
  if (S_ISDIR(st.st_mode)) {
    flib_set_error("'%s' is a directory, use remove_Dir()", path);
    return -1;
  }

  if (unlink(path) != 0) {
    flib_set_error("cannot remove '%s': %s", path, strerror(errno));
    return -1;
  }
  return 0;
}

// Remove the contents of a directory, then the directory itself.
static int remove_tree(const char *path, int depth) {
  DIR *d;
  struct dirent *de;
  int rc = 0;
  int saved_errno;

  if (depth >= FLIB_REMOVE_MAX_DEPTH) {
    flib_set_error("directory tree is too deep at '%s'", path);
    return -1;
  }

  d = opendir(path);
  if (!d) {
    flib_set_error("cannot open directory '%s': %s", path, strerror(errno));
    return -1;
  }

  errno = 0;
  while ((de = readdir(d)) != NULL) {
    char *child;
    struct stat st;

    if (flib_is_dot_entry(de->d_name)) {
      errno = 0;
      continue;
    }

    child = flib_join_path(path, de->d_name);
    if (!child) {
      rc = -1;
      break;
    }

    if (lstat(child, &st) != 0) {
      flib_set_error("cannot stat '%s': %s", child, strerror(errno));
      rc = -1;
    } else if (S_ISDIR(st.st_mode)) {
      rc = remove_tree(child, depth + 1);
    } else if (unlink(child) != 0) {
      flib_set_error("cannot remove '%s': %s", child, strerror(errno));
      rc = -1;
    }

    free(child);
    if (rc != 0)
      break;
    errno = 0;
  }

  saved_errno = errno;
  closedir(d);

  if (rc == 0 && saved_errno != 0) {
    flib_set_error("reading directory '%s' failed: %s", path,
                   strerror(saved_errno));
    rc = -1;
  }
  if (rc != 0)
    return -1;

  if (rmdir(path) != 0) {
    flib_set_error("cannot remove directory '%s': %s", path,
                   strerror(errno));
    return -1;
  }
  return 0;
}

int remove_Dir(const char *path) {
  struct stat st;

  if (!flib_require_path(path, "directory"))
    return -1;

  if (strcmp(path, ".") == 0 || strcmp(path, "..") == 0) {
    flib_set_error("refusing to remove '%s'", path);
    return -1;
  }

  if (lstat(path, &st) != 0) {
    flib_set_error("cannot stat '%s': %s", path, strerror(errno));
    return -1;
  }
  if (!S_ISDIR(st.st_mode)) {
    flib_set_error("'%s' is not a directory", path);
    return -1;
  }
  if (path_is_root(path)) {
    flib_set_error("refusing to remove '%s'", path);
    return -1;
  }

  return remove_tree(path, 0);
}

int move_File(const char *src, const char *dst) {
  struct stat st;

  if (!flib_require_path(src, "file") || !flib_require_path(dst, "file"))
    return -1;

  if (lstat(src, &st) != 0) {
    flib_set_error("cannot stat '%s': %s", src, strerror(errno));
    return -1;
  }
  if (!S_ISREG(st.st_mode)) {
    flib_set_error("'%s' is not a regular file", src);
    return -1;
  }

  if (rename(src, dst) == 0)
    return 0;

  if (errno != EXDEV) {
    flib_set_error("cannot move '%s' to '%s': %s", src, dst,
                   strerror(errno));
    return -1;
  }

  if (copy_File(src, dst) != 0)
    return -1;

  if (unlink(src) != 0) {
    flib_set_error("copied '%s' to '%s' but cannot remove the source: %s",
                   src, dst, strerror(errno));
    return -1;
  }
  return 0;
}

int move_Dir(const char *src, const char *dst) {
  struct stat st;

  if (!flib_require_path(src, "directory") ||
      !flib_require_path(dst, "directory"))
    return -1;

  if (lstat(src, &st) != 0) {
    flib_set_error("cannot stat '%s': %s", src, strerror(errno));
    return -1;
  }
  if (!S_ISDIR(st.st_mode)) {
    flib_set_error("'%s' is not a directory", src);
    return -1;
  }
  if (path_is_root(src)) {
    flib_set_error("refusing to move '%s'", src);
    return -1;
  }

  if (rename(src, dst) == 0)
    return 0;

  if (errno != EXDEV) {
    flib_set_error("cannot move '%s' to '%s': %s", src, dst,
                   strerror(errno));
    return -1;
  }

  // copy_Dir() skips sockets and devices, so removing the source after a
  // partial copy would lose them. Refuse the move instead.
  if (tree_has_unsupported(src)) {
    flib_set_error("cannot move '%s' across filesystems: it contains "
                   "sockets or devices", src);
    return -1;
  }

  if (copy_Dir(src, dst) != 0)
    return -1;

  if (remove_Dir(src) != 0) {
    char saved[FLIB_ERROR_SIZE];

    snprintf(saved, sizeof saved, "%s", flib_LastError());
    flib_set_error("copied '%s' to '%s' but cannot remove the source: %s",
                   src, dst, saved);
    return -1;
  }
  return 0;
}
