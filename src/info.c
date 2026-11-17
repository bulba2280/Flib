#define _GNU_SOURCE

#include "internal.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

static void format_perms(mode_t mode, char out[11]) {
  char first = flib_type_letter(flib_type_from_mode(mode));
  size_t i;

  out[0] = first == '?' ? '-' : first;
  for (i = 0; i < 9; i++)
    out[i + 1] = (mode & (0400u >> i)) ? "rwxrwxrwx"[i] : '-';

  if (mode & S_ISUID)
    out[3] = (mode & S_IXUSR) ? 's' : 'S';
  if (mode & S_ISGID)
    out[6] = (mode & S_IXGRP) ? 's' : 'S';
  if (mode & S_ISVTX)
    out[9] = (mode & S_IXOTH) ? 't' : 'T';

  out[10] = '\0';
}

// Count lines and guess whether the data is text or binary.
static void scan_file_content(const char *path, FileInfo *info) {
  FILE *f;
  unsigned char buf[FLIB_IO_CHUNK];
  unsigned long long newlines = 0;
  size_t checked = 0;
  size_t suspicious = 0;
  int binary = 0;
  int last = '\n';
  size_t n;

  info->is_text = 1;
  info->lines = 0;

  if (info->type != FT_REGULAR) {
    info->is_text = 0;
    return;
  }
  if (info->size == 0)
    return;

  f = fopen(path, "rb");
  if (!f) {
    info->is_text = 0;
    return;
  }

  while ((n = fread(buf, 1, sizeof buf, f)) > 0) {
    size_t i;

    last = buf[n - 1];
    for (i = 0; i < n; i++) {
      unsigned char c = buf[i];

      if (c == '\n')
        newlines++;

      if (checked < FLIB_TEXT_SAMPLE_SIZE) {
        checked++;
        if (c == 0)
          binary = 1;
        else if (c < 0x20 && c != '\t' && c != '\n' && c != '\r' && c != '\f' &&
                 c != '\v' && c != 0x1b)
          suspicious++;
      }
    }
  }

  fclose(f);

  if (checked > 0 && suspicious * 100 > checked * FLIB_TEXT_CONTROL_PERCENT)
    binary = 1;

  if (binary) {
    info->is_text = 0;
    info->lines = 0;
    return;
  }

  info->lines = newlines + (last != '\n' ? 1 : 0);
}

int info_File(const char *path, FileInfo *info) {
  struct stat st;
  const char *lang;

  if (!info) {
    flib_set_error("info == NULL");
    return -1;
  }
  if (!flib_require_path(path, "file"))
    return -1;

  memset(info, 0, sizeof *info);

  if (stat(path, &st) != 0) {
    flib_set_error("cannot stat '%s': %s", path,
                   strerror(errno));
    return -1;
  }

  lang = file_Language(path);

  info->path = strdup(path);
  info->name = flib_path_name(path);
  info->extension = info->name ? strdup(flib_ext_of(info->name)) : NULL;
  info->language = lang ? strdup(lang) : NULL;

  if (!info->path || !info->name || !info->extension ||
      (lang && !info->language)) {
    info_File_Free(info);
    flib_set_error("out of memory");
    return -1;
  }

  info->type = flib_type_from_mode(st.st_mode);
  info->size = (unsigned long long)st.st_size;
  info->is_empty = (info->size == 0);

  info->mode = (unsigned)st.st_mode & 07777;
  format_perms(st.st_mode, info->perms);

  info->uid = (unsigned long)st.st_uid;
  info->gid = (unsigned long)st.st_gid;
  info->inode = (unsigned long)st.st_ino;
  info->links = (unsigned long)st.st_nlink;

  info->modified = st.st_mtime;
  info->accessed = st.st_atime;
  info->changed = st.st_ctime;

#if defined(__linux__) && defined(__GLIBC__) && defined(STATX_BTIME)
  {
    struct statx stx;

    if (statx(AT_FDCWD, path, 0, STATX_BTIME, &stx) == 0 &&
        (stx.stx_mask & STATX_BTIME) && stx.stx_btime.tv_sec > 0) {
      info->created = (time_t)stx.stx_btime.tv_sec;
      info->created_is_birth = 1;
    }
  }
#endif
  if (!info->created_is_birth)
    info->created = st.st_ctime;

  scan_file_content(path, info);
  return 0;
}

void info_File_Free(FileInfo *info) {
  if (!info)
    return;
  free(info->path);
  free(info->name);
  free(info->extension);
  free(info->language);
  memset(info, 0, sizeof *info);
}

// Width of a UTF-8 string in characters.
static int utf8_width(const char *s) {
  int width = 0;
  const unsigned char *p;

  for (p = (const unsigned char *)s; *p; p++)
    if ((*p & 0xC0) != 0x80)
      width++;
  return width;
}

static void print_field(FILE *out, const char *label, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

static void print_field(FILE *out, const char *label, const char *fmt, ...) {
  va_list ap;
  int width;

  if (!fmt)
    fmt = "";

  fputs("  ", out);
  fputs(label, out);
  for (width = utf8_width(label); width < 13; width++)
    fputc(' ', out);

  va_start(ap, fmt);
  vfprintf(out, fmt, ap);
  va_end(ap);
  fputc('\n', out);
}

void info_File_Print(const FileInfo *info, FILE *out) {
  char sizebuf[64];
  char timebuf[64];
  char octbuf[16];

  if (!out)
    out = stdout;
  if (!info) {
    fprintf(out, "  (no data)\n");
    return;
  }

  flib_human_size(info->size, sizebuf, sizeof sizebuf);
  snprintf(octbuf, sizeof octbuf, "%04o", info->mode);

  print_field(out, "Path:", "%s", info->path ? info->path : "?");
  print_field(out, "Name:", "%s", info->name ? info->name : "?");
  print_field(out, "Type:", "%s", file_Type_Name(info->type));

  if (info->extension && info->extension[0])
    print_field(out, "Extension:", "%s", info->extension);
  else
    print_field(out, "Extension:", "(none)");

  print_field(out, "Language:", "%s",
              info->language ? info->language : "unknown");

  print_field(out, "Size:", "%s (%llu bytes)", sizebuf, info->size);

  if (info->type != FT_REGULAR)
    print_field(out, "Contents:", "-");
  else if (info->is_empty)
    print_field(out, "Contents:", "empty");
  else if (info->is_text)
    print_field(out, "Contents:", "text, %llu lines", info->lines);
  else
    print_field(out, "Contents:", "binary data");

  print_field(out, "Permissions:", "%s (%s)", info->perms, octbuf);
  print_field(out, "Owner:", "uid %lu, gid %lu", info->uid, info->gid);
  print_field(out, "Links:", "%lu", info->links);
  print_field(out, "Inode:", "%lu", info->inode);

  if (info->created_is_birth)
    print_field(out, "Created:", "%s",
                time_Format(info->created, timebuf, sizeof timebuf));
  else
    print_field(out, "Created:", "unknown (filesystem does not store btime)");

  print_field(out, "Modified:", "%s",
              time_Format(info->modified, timebuf, sizeof timebuf));
  print_field(out, "Accessed:", "%s",
              time_Format(info->accessed, timebuf, sizeof timebuf));
  print_field(out, "Metadata:", "%s",
              time_Format(info->changed, timebuf, sizeof timebuf));
}
