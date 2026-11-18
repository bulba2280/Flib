#define _GNU_SOURCE

#include "internal.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define FLIB_WALK_MAX_DEPTH 1024

// Directories on the current branch of the walk, used to catch symlink loops.
typedef struct WalkStack {
  dev_t dev[FLIB_WALK_MAX_DEPTH];
  ino_t ino[FLIB_WALK_MAX_DEPTH];
  size_t len;
} WalkStack;

static int stack_has(const WalkStack *stack, dev_t dev, ino_t ino) {
  size_t i;

  for (i = 0; i < stack->len; i++)
    if (stack->dev[i] == dev && stack->ino[i] == ino)
      return 1;
  return 0;
}

static int stack_push(WalkStack *stack, dev_t dev, ino_t ino) {
  if (stack->len >= FLIB_WALK_MAX_DEPTH) {
    flib_set_error("directory tree is too deep");
    return -1;
  }

  stack->dev[stack->len] = dev;
  stack->ino[stack->len] = ino;
  stack->len++;
  return 0;
}

static void free_namelist(struct dirent **list, int n) {
  int i;

  for (i = 0; i < n; i++)
    free(list[i]);
  free(list);
}

static int walk_contents(const char *path, int depth, unsigned flags,
                         flib_WalkFn fn, void *user, WalkStack *stack) {
  struct dirent **namelist = NULL;
  int n;
  int i;
  int rc = 0;

  n = scandir(path, &namelist, NULL, alphasort);
  if (n < 0) {
    flib_set_error("cannot open directory '%s': %s", path, strerror(errno));
    return -1;
  }

  for (i = 0; i < n; i++) {
    const char *name = namelist[i]->d_name;
    char *child;
    struct stat st;
    FileType type;
    int have = 0;

    if (flib_is_dot_entry(name))
      continue;
    if (!(flags & FLIB_WALK_HIDDEN) && name[0] == '.')
      continue;

    child = flib_join_path(path, name);
    if (!child) {
      rc = -1;
      break;
    }

    if (flags & FLIB_WALK_FOLLOW) {
      if (stat(child, &st) == 0)
        have = 1;
      else if (lstat(child, &st) == 0)
        have = 1;
    } else if (lstat(child, &st) == 0) {
      have = 1;
    }

    if (!have) {
      flib_set_error("cannot stat '%s': %s", child, strerror(errno));
      free(child);
      rc = -1;
      break;
    }

    type = flib_type_from_mode(st.st_mode);
    rc = fn(child, type, (unsigned long long)st.st_size, depth, user);
    if (rc != 0) {
      free(child);
      break;
    }

    if (type == FT_DIRECTORY) {
      if (depth + 1 >= FLIB_WALK_MAX_DEPTH) {
        flib_set_error("directory tree is too deep at '%s'", child);
        free(child);
        rc = -1;
        break;
      }
      if (!stack_has(stack, st.st_dev, st.st_ino)) {
        if (stack_push(stack, st.st_dev, st.st_ino) != 0) {
          free(child);
          rc = -1;
          break;
        }
        rc = walk_contents(child, depth + 1, flags, fn, user, stack);
        stack->len--;
        if (rc != 0) {
          free(child);
          break;
        }
      }
    }

    free(child);
  }

  free_namelist(namelist, n);
  return rc;
}

int walk_Dir(const char *path, unsigned flags, flib_WalkFn fn, void *user) {
  struct stat st;
  WalkStack stack;

  if (!flib_require_path(path, "directory"))
    return -1;
  if (!fn) {
    flib_set_error("walk callback is NULL");
    return -1;
  }

  if (flags & FLIB_WALK_FOLLOW) {
    if (stat(path, &st) != 0) {
      flib_set_error("cannot stat '%s': %s", path, strerror(errno));
      return -1;
    }
  } else if (lstat(path, &st) != 0) {
    flib_set_error("cannot stat '%s': %s", path, strerror(errno));
    return -1;
  }

  if (!S_ISDIR(st.st_mode)) {
    flib_set_error("'%s' is not a directory", path);
    return -1;
  }

  stack.len = 0;
  if (stack_push(&stack, st.st_dev, st.st_ino) != 0)
    return -1;

  return walk_contents(path, 0, flags, fn, user, &stack);
}
