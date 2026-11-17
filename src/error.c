#include "internal.h"

#include <stdarg.h>
#include <stdio.h>

static _Thread_local char g_error[FLIB_ERROR_SIZE];

const char *flib_LastError(void) { return g_error; }

void flib_set_error(const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  vsnprintf(g_error, sizeof g_error, fmt, ap);
  va_end(ap);
}
