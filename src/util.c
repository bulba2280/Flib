#define _GNU_SOURCE

#include "internal.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

int flib_require_path(const char *path, const char *kind) {
  if (path && *path)
    return 1;

  flib_set_error("empty path to %s", kind);
  return 0;
}

// Basename of a path.
char *flib_path_name(const char *path) {
  size_t len = strlen(path);
  const char *start;
  char *name;

  if (len == 0)
    return strdup("");

  while (len > 1 && path[len - 1] == '/')
    len--;

  start = path + len;
  while (start > path && start[-1] != '/')
    start--;

  if (start == path + len) // root path: "/" or "///"
    return strdup("/");

  name = strndup(start, (size_t)(path + len - start));
  if (!name)
    flib_set_error("out of memory");
  return name;
}

const char *flib_ext_of(const char *name) {
  const char *dot = strrchr(name, '.');

  if (!dot || dot == name)
    return "";
  return dot + 1;
}

char *flib_join_path(const char *dir, const char *name) {
  size_t dir_len = strlen(dir);
  int need_slash = !(dir_len > 0 && dir[dir_len - 1] == '/');
  size_t size = dir_len + (size_t)need_slash + strlen(name) + 1;
  char *path = malloc(size);
  size_t offset;

  if (!path) {
    flib_set_error("out of memory");
    return NULL;
  }

  memcpy(path, dir, dir_len);
  offset = dir_len;
  if (need_slash)
    path[offset++] = '/';
  strcpy(path + offset, name);
  return path;
}

int flib_is_dot_entry(const char *name) {
  return strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
}

FileType flib_type_from_mode(mode_t mode) {
  if (S_ISREG(mode))
    return FT_REGULAR;
  if (S_ISDIR(mode))
    return FT_DIRECTORY;
  if (S_ISLNK(mode))
    return FT_SYMLINK;
  if (S_ISFIFO(mode))
    return FT_FIFO;
  if (S_ISSOCK(mode))
    return FT_SOCKET;
  if (S_ISBLK(mode))
    return FT_BLOCK_DEVICE;
  if (S_ISCHR(mode))
    return FT_CHAR_DEVICE;
  return FT_UNKNOWN;
}

// Type letter, ls style.
char flib_type_letter(FileType type) {
  static const char letters[] = {
      [FT_REGULAR] = '-', [FT_DIRECTORY] = 'd', [FT_SYMLINK] = 'l',
      [FT_FIFO] = 'p',    [FT_SOCKET] = 's',    [FT_BLOCK_DEVICE] = 'b',
      [FT_CHAR_DEVICE] = 'c',
  };

  return (unsigned)type <= FT_CHAR_DEVICE && letters[type] ? letters[type] : '?';
}

const char *file_Type_Name(FileType type) {
  static const char *const names[] = {
      [FT_UNKNOWN] = "unknown",
      [FT_REGULAR] = "regular file",
      [FT_DIRECTORY] = "directory",
      [FT_SYMLINK] = "symbolic link",
      [FT_FIFO] = "FIFO (named pipe)",
      [FT_SOCKET] = "socket",
      [FT_BLOCK_DEVICE] = "block device",
      [FT_CHAR_DEVICE] = "character device",
  };

  return (unsigned)type <= FT_CHAR_DEVICE ? names[type] : names[FT_UNKNOWN];
}

void flib_human_size(unsigned long long bytes, char *buf, size_t bufsize) {
  static const char *const units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
  double value = (double)bytes;
  size_t unit = 0;

  while (value >= 1024.0 && unit + 1 < sizeof units / sizeof units[0]) {
    value /= 1024.0;
    unit++;
  }

  if (unit == 0)
    snprintf(buf, bufsize, "%llu B", bytes);
  else
    snprintf(buf, bufsize, "%.1f %s", value, units[unit]);
}

// Copy from in to out.
int flib_copy_stream(FILE *in, FILE *out, const char *src, const char *dst) {
  char buf[FLIB_IO_CHUNK];
  size_t n;

  while ((n = fread(buf, 1, sizeof buf, in)) > 0) {
    if (fwrite(buf, 1, n, out) != n) {
      if (dst)
        flib_set_error("writing '%s' failed: %s", dst, strerror(errno));
      else
        flib_set_error("writing to stdout failed: %s", strerror(errno));
      return -1;
    }
  }

  if (ferror(in)) {
    flib_set_error("reading '%s' failed: %s", src, strerror(errno));
    return -1;
  }
  return 0;
}

int file_Exists(const char *path) {
  struct stat st;

  return path && *path && stat(path, &st) == 0;
}

const char *time_Format(time_t t, char *buf, size_t bufsize) {
  struct tm tmv;

  if (!buf || bufsize == 0)
    return buf;

  if (!localtime_r(&t, &tmv)) {
    snprintf(buf, bufsize, "?");
    return buf;
  }

  if (strftime(buf, bufsize, "%Y-%m-%d %H:%M:%S", &tmv) == 0)
    snprintf(buf, bufsize, "?");
  return buf;
}
