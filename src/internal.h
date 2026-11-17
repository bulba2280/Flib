// Internal declarations for flib.
// Not installed, not part of the public API.

#ifndef FLIB_INTERNAL_H
#define FLIB_INTERNAL_H

#include "../include/flib.h"

#include <sys/types.h>

#define FLIB_ERROR_SIZE 1024
#define FLIB_IO_CHUNK (64 * 1024)
#define FLIB_TEXT_SAMPLE_SIZE 8192
#define FLIB_TEXT_CONTROL_PERCENT 5

void flib_set_error(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

char *flib_path_name(const char *path);
const char *flib_ext_of(const char *name);
int flib_require_path(const char *path, const char *kind);
char *flib_join_path(const char *dir, const char *name);
int flib_is_dot_entry(const char *name);

FileType flib_type_from_mode(mode_t mode);
char flib_type_letter(FileType type);

void flib_human_size(unsigned long long bytes, char *buf, size_t bufsize);

int flib_copy_stream(FILE *in, FILE *out, const char *src, const char *dst);

#endif
