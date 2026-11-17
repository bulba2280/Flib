#define _GNU_SOURCE

#include "internal.h"

#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void read_Dir_Free(DirList *list) {
  size_t i;

  if (!list)
    return;
  for (i = 0; i < list->count; i++) {
    free(list->items[i].name);
    free(list->items[i].path);
  }
  free(list->items);
  list->items = NULL;
  list->count = 0;
}

static int cmp_dir_entry(const void *a, const void *b) {
  const DirEntry *ea = (const DirEntry *)a;
  const DirEntry *eb = (const DirEntry *)b;

  return strcmp(ea->name, eb->name);
}

int read_Dir(const char *path, DirList *list) {
  DIR *d;
  struct dirent *de;
  char *entry_path = NULL;
  size_t cap = 0;
  int saved_errno = 0;
  int rc = -1;

  if (!list) {
    flib_set_error("list == NULL");
    return -1;
  }
  list->items = NULL;
  list->count = 0;

  if (!flib_require_path(path, "directory"))
    return -1;

  d = opendir(path);
  if (!d) {
    flib_set_error("cannot open directory '%s': %s", path,
                   strerror(errno));
    return -1;
  }

  errno = 0;
  while ((de = readdir(d)) != NULL) {
    char *entry_name;
    struct stat st;

    if (flib_is_dot_entry(de->d_name)) {
      errno = 0;
      continue;
    }

    entry_path = flib_join_path(path, de->d_name);
    if (!entry_path)
      goto done;

    if (lstat(entry_path, &st) != 0) {
      flib_set_error("cannot stat '%s': %s", entry_path,
                     strerror(errno));
      goto done;
    }

    if (list->count == cap) {
      size_t new_cap;
      DirEntry *items;

      if (cap > SIZE_MAX / 2) {
        flib_set_error("too many entries in '%s'", path);
        goto done;
      }

      new_cap = cap ? cap * 2 : 16;
      if (new_cap > SIZE_MAX / sizeof *items) {
        flib_set_error("too many entries in '%s'", path);
        goto done;
      }

      items = realloc(list->items, new_cap * sizeof *items);
      if (!items) {
        flib_set_error("out of memory");
        goto done;
      }
      list->items = items;
      cap = new_cap;
    }

    entry_name = strdup(de->d_name);
    if (!entry_name) {
      flib_set_error("out of memory");
      goto done;
    }

    list->items[list->count++] = (DirEntry){entry_name, entry_path,
                                            flib_type_from_mode(st.st_mode),
                                            (unsigned long long)st.st_size};
    entry_path = NULL;
    errno = 0;
  }

  saved_errno = errno;
  rc = 0;

done:
  free(entry_path);
  closedir(d);

  if (rc != 0) {
    read_Dir_Free(list);
    return -1;
  }

  if (saved_errno != 0) {
    flib_set_error("reading directory '%s' failed: %s", path,
                   strerror(saved_errno));
    read_Dir_Free(list);
    return -1;
  }

  if (list->count > 1)
    qsort(list->items, list->count, sizeof list->items[0], cmp_dir_entry);

  return 0;
}

char *read_DirText(const char *path) {
  DirList list;
  char *text;
  char *p;
  size_t total = 1;
  size_t i;

  if (read_Dir(path, &list) != 0)
    return NULL;

  for (i = 0; i < list.count; i++) {
    total += strlen(list.items[i].name) + 1;
    if (list.items[i].type == FT_DIRECTORY)
      total++;
  }

  text = malloc(total);
  if (!text) {
    flib_set_error("out of memory");
    read_Dir_Free(&list);
    return NULL;
  }

  p = text;
  for (i = 0; i < list.count; i++) {
    size_t n = strlen(list.items[i].name);

    memcpy(p, list.items[i].name, n);
    p += n;
    if (list.items[i].type == FT_DIRECTORY)
      *p++ = '/';
    *p++ = '\n';
  }
  *p = '\0';

  read_Dir_Free(&list);
  return text;
}

void read_Dir_Print(const DirList *list, FILE *out) {
  size_t i;

  if (!out)
    out = stdout;
  if (!list) {
    fprintf(out, "  (no data)\n");
    return;
  }
  if (list->count == 0) {
    fprintf(out, "  (empty)\n");
    return;
  }

  for (i = 0; i < list->count; i++) {
    const DirEntry *e = &list->items[i];
    char sizebuf[64];

    if (e->type == FT_REGULAR || e->type == FT_FIFO ||
        e->type == FT_BLOCK_DEVICE || e->type == FT_CHAR_DEVICE)
      flib_human_size(e->size, sizebuf, sizeof sizebuf);
    else
      snprintf(sizebuf, sizeof sizebuf, "-");

    fprintf(out, "  %c %9s  %s%s\n", flib_type_letter(e->type), sizebuf,
            e->name, e->type == FT_DIRECTORY ? "/" : "");
  }
}

// Create a single directory.
static int mkdir_one(const char *path) {
  struct stat st;

  if (mkdir(path, 0777) == 0)
    return 0;

  if (errno == EEXIST && stat(path, &st) == 0 && S_ISDIR(st.st_mode))
    return 0;

  if (errno == EEXIST)
    flib_set_error("'%s' already exists and is not a directory", path);
  else
    flib_set_error("cannot create directory '%s': %s", path,
                   strerror(errno));
  return -1;
}

int make_Dir(const char *path) {
  char *tmp;
  char *p;
  size_t len;
  int rc;

  if (!flib_require_path(path, "directory"))
    return -1;

  tmp = strdup(path);
  if (!tmp) {
    flib_set_error("out of memory");
    return -1;
  }

  len = strlen(tmp);
  while (len > 1 && tmp[len - 1] == '/')
    tmp[--len] = '\0';

  for (p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      if (mkdir_one(tmp) != 0) {
        free(tmp);
        return -1;
      }
      *p = '/';
    }
  }

  rc = mkdir_one(tmp);
  free(tmp);
  return rc;
}
