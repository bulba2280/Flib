#define _GNU_SOURCE

#include "internal.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static char *read_all(const char *path, size_t *out_size) {
  FILE *f;
  struct stat st;
  size_t cap = 4096;
  size_t len = 0;
  char *buf = NULL;

  f = fopen(path, "rb");
  if (!f) {
    flib_set_error("cannot open '%s': %s", path, strerror(errno));
    return NULL;
  }

  if (fstat(fileno(f), &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0) {
    if ((unsigned long long)st.st_size < (unsigned long long)SIZE_MAX - 1)
      cap = (size_t)st.st_size + 1;
  }

  buf = malloc(cap);
  if (!buf) {
    flib_set_error("out of memory reading '%s'", path);
    goto fail;
  }

  for (;;) {
    size_t n;

    if (len + 1 >= cap) {
      char *bigger;

      if (cap > SIZE_MAX / 2) {
        flib_set_error("file '%s' is too large", path);
        goto fail;
      }
      cap *= 2;
      bigger = realloc(buf, cap);
      if (!bigger) {
        flib_set_error("out of memory reading '%s'", path);
        goto fail;
      }
      buf = bigger;
    }

    n = fread(buf + len, 1, cap - len - 1, f);
    len += n;

    if (n == 0) {
      if (ferror(f)) {
        flib_set_error("reading '%s' failed: %s", path, strerror(errno));
        goto fail;
      }
      break;
    }
  }

  fclose(f);
  buf[len] = '\0';
  if (out_size)
    *out_size = len;
  return buf;

fail:
  free(buf);
  fclose(f);
  return NULL;
}

char *read_FileEx(const char *path, size_t *out_size) {
  if (out_size)
    *out_size = 0;

  if (!flib_require_path(path, "file"))
    return NULL;
  return read_all(path, out_size);
}

char *read_File(const char *path) { return read_FileEx(path, NULL); }

int print_File(const char *path) {
  FILE *f;
  int rc;

  if (!flib_require_path(path, "file"))
    return -1;

  f = fopen(path, "rb");
  if (!f) {
    flib_set_error("cannot open '%s': %s", path, strerror(errno));
    return -1;
  }

  rc = flib_copy_stream(f, stdout, path, NULL);
  fclose(f);
  return rc;
}

static int write_all(FILE *f, const void *data, size_t size,
                     const char *path) {
  const unsigned char *p = data;
  size_t done = 0;

  while (done < size) {
    size_t n = fwrite(p + done, 1, size - done, f);

    if (n == 0) {
      flib_set_error("writing '%s' failed: %s", path, strerror(errno));
      return -1;
    }
    done += n;
  }
  return 0;
}

static int write_file_mode(const char *path, const void *data, size_t size,
                           const char *mode, const char *action) {
  FILE *f;
  int rc = 0;

  if (!flib_require_path(path, "file"))
    return -1;
  if (!data && size > 0) {
    flib_set_error("null data for '%s'", path);
    return -1;
  }

  f = fopen(path, mode);
  if (!f) {
    flib_set_error("cannot open '%s' for %s: %s", path, action,
                   strerror(errno));
    return -1;
  }

  if (write_all(f, data, size, path) != 0)
    rc = -1;

  if (fclose(f) != 0 && rc == 0) {
    flib_set_error("writing '%s' failed: %s", path, strerror(errno));
    rc = -1;
  }
  return rc;
}

int write_File(const char *path, const void *data, size_t size) {
  return write_file_mode(path, data, size, "wb", "writing");
}

int append_File(const char *path, const void *data, size_t size) {
  return write_file_mode(path, data, size, "ab", "appending");
}
