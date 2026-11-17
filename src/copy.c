#define _GNU_SOURCE

#include "internal.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int same_inode(const struct stat *a, const struct stat *b) {
  return a->st_dev == b->st_dev && a->st_ino == b->st_ino;
}

static int same_file(const char *a, const char *b) {
  struct stat sa;
  struct stat sb;

  if (stat(a, &sa) != 0 || stat(b, &sb) != 0)
    return 0;
  return same_inode(&sa, &sb);
}

static void copy_times(const char *path, const struct stat *st) {
  struct timespec times[2];

  times[0] = st->st_atim;
  times[1] = st->st_mtim;
  (void)utimensat(AT_FDCWD, path, times, 0);
}

static int copy_file_impl(const char *src, const char *dst,
                          const struct stat *st) {
  FILE *in;
  FILE *out;
  int rc;

  if (same_file(src, dst)) {
    flib_set_error("source and destination are the same file: '%s'", src);
    return -1;
  }

  in = fopen(src, "rb");
  if (!in) {
    flib_set_error("cannot open '%s': %s", src, strerror(errno));
    return -1;
  }

  out = fopen(dst, "wb");
  if (!out) {
    flib_set_error("cannot create '%s': %s", dst, strerror(errno));
    fclose(in);
    return -1;
  }

  rc = flib_copy_stream(in, out, src, dst);

  if (fclose(in) != 0 && rc == 0) {
    flib_set_error("reading '%s' failed: %s", src, strerror(errno));
    rc = -1;
  }
  if (fclose(out) != 0 && rc == 0) {
    flib_set_error("writing '%s' failed: %s", dst, strerror(errno));
    rc = -1;
  }

  if (rc != 0) {
    unlink(dst);
    return -1;
  }

  (void)chmod(dst, st->st_mode & 07777);
  copy_times(dst, st);
  return 0;
}

int copy_File(const char *src, const char *dst) {
  struct stat st;

  if (!flib_require_path(src, "file") || !flib_require_path(dst, "file"))
    return -1;

  if (stat(src, &st) != 0) {
    flib_set_error("cannot open '%s': %s", src, strerror(errno));
    return -1;
  }
  if (!S_ISREG(st.st_mode)) {
    flib_set_error("'%s' is not a regular file", src);
    return -1;
  }
  return copy_file_impl(src, dst, &st);
}

// Read the symlink target.
static int copy_symlink(const char *src, const char *dst,
                        const struct stat *st) {
  size_t size = st->st_size > 0 ? (size_t)st->st_size + 1 : 256;
  char *target = NULL;
  ssize_t n;

  for (;;) {
    char *bigger = realloc(target, size);

    if (!bigger) {
      flib_set_error("out of memory");
      free(target);
      return -1;
    }
    target = bigger;

    n = readlink(src, target, size - 1);
    if (n < 0) {
      flib_set_error("cannot read link '%s': %s", src,
                     strerror(errno));
      free(target);
      return -1;
    }
    if ((size_t)n < size - 1)
      break;
    if (size > (size_t)-1 / 2) {
      flib_set_error("symbolic link '%s' is too long", src);
      free(target);
      return -1;
    }
    size *= 2;
  }
  target[n] = '\0';

  if (symlink(target, dst) != 0) {
    if (errno != EEXIST || unlink(dst) != 0 || symlink(target, dst) != 0) {
      flib_set_error("cannot create link '%s': %s", dst,
                     strerror(errno));
      free(target);
      return -1;
    }
  }

  free(target);
  return 0;
}

static int copy_fifo(const char *dst, mode_t mode) {
  if (mkfifo(dst, mode) == 0)
    return 0;
  if (errno == EEXIST && unlink(dst) == 0 && mkfifo(dst, mode) == 0)
    return 0;
  return -1;
}

static int copy_dir_contents(const char *src, const char *dst,
                             const struct stat *dstst);

// Copy one directory entry.
static int copy_entry(const char *src, const char *dst, const char *name,
                      const struct stat *dstst) {
  char *src_path = flib_join_path(src, name);
  char *dst_path = flib_join_path(dst, name);
  struct stat st;
  int rc = -1;

  if (!src_path || !dst_path)
    goto done;

  if (lstat(src_path, &st) != 0) {
    flib_set_error("cannot stat '%s': %s", src_path,
                   strerror(errno));
    goto done;
  }

  if (S_ISDIR(st.st_mode)) {
    if (same_inode(&st, dstst)) {
      flib_set_error("cannot copy directory '%s' into itself",
                     src_path);
      goto done;
    }
    if (make_Dir(dst_path) != 0)
      goto done;
    if (copy_dir_contents(src_path, dst_path, dstst) != 0)
      goto done;
    (void)chmod(dst_path, st.st_mode & 07777);
    copy_times(dst_path, &st);
  } else if (S_ISREG(st.st_mode)) {
    if (copy_file_impl(src_path, dst_path, &st) != 0)
      goto done;
  } else if (S_ISLNK(st.st_mode)) {
    if (copy_symlink(src_path, dst_path, &st) != 0)
      goto done;
  } else if (S_ISFIFO(st.st_mode)) {
    if (copy_fifo(dst_path, st.st_mode & 07777) != 0) {
      flib_set_error("cannot create FIFO '%s': %s", dst_path,
                     strerror(errno));
      goto done;
    }
  }

  rc = 0;

done:
  free(src_path);
  free(dst_path);
  return rc;
}

// Copy src into an existing dst.
static int copy_dir_contents(const char *src, const char *dst,
                             const struct stat *dstst) {
  DIR *d;
  struct dirent *de;
  int rc = 0;
  int saved_errno;

  d = opendir(src);
  if (!d) {
    flib_set_error("cannot open directory '%s': %s", src, strerror(errno));
    return -1;
  }

  errno = 0;
  while ((de = readdir(d)) != NULL) {
    if (flib_is_dot_entry(de->d_name)) {
      errno = 0;
      continue;
    }
    if (copy_entry(src, dst, de->d_name, dstst) != 0) {
      rc = -1;
      break;
    }
    errno = 0;
  }

  saved_errno = errno;
  closedir(d);

  if (rc == 0 && saved_errno != 0) {
    flib_set_error("reading directory '%s' failed: %s", src,
                   strerror(saved_errno));
    rc = -1;
  }
  return rc;
}

int copy_Dir(const char *src, const char *dst) {
  struct stat src_st;
  struct stat dst_st;
  int created = 0;

  if (!flib_require_path(src, "directory") ||
      !flib_require_path(dst, "directory"))
    return -1;

  if (stat(src, &src_st) != 0) {
    flib_set_error("cannot open directory '%s': %s", src, strerror(errno));
    return -1;
  }
  if (!S_ISDIR(src_st.st_mode)) {
    flib_set_error("'%s' is not a directory", src);
    return -1;
  }

  if (stat(dst, &dst_st) == 0) {
    if (!S_ISDIR(dst_st.st_mode)) {
      flib_set_error("'%s' already exists and is not a directory", dst);
      return -1;
    }
    if (same_inode(&src_st, &dst_st)) {
      flib_set_error("source and destination are the same directory: '%s'",
                     src);
      return -1;
    }
  } else if (errno != ENOENT) {
    flib_set_error("cannot stat '%s': %s", dst, strerror(errno));
    return -1;
  } else {
    if (make_Dir(dst) != 0)
      return -1;
    if (stat(dst, &dst_st) != 0) {
      flib_set_error("cannot stat '%s': %s", dst, strerror(errno));
      return -1;
    }
    created = 1;
  }

  if (copy_dir_contents(src, dst, &dst_st) != 0)
    return -1;

  if (created) {
    (void)chmod(dst, src_st.st_mode & 07777);
    copy_times(dst, &src_st);
  }
  return 0;
}
