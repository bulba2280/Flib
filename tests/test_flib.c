#define _GNU_SOURCE

#include "flib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int failures;

static void check(int ok, const char *what, int line) {
  if (!ok) {
    printf("FAIL line %d: %s\n", line, what);
    failures++;
  }
}

#define CHECK(cond) check((cond), #cond, __LINE__)

static void check_str(const char *got, const char *want, int line) {
  if (!got || strcmp(got, want) != 0) {
    printf("FAIL line %d: got '%s', want '%s'\n", line,
           got ? got : "(null)", want);
    failures++;
  }
}

#define CHECK_STR(got, want) check_str((got), (want), __LINE__)

static void die(const char *what) {
  fprintf(stderr, "test setup failed: %s\n", what);
  exit(2);
}

static char *join(const char *base, const char *name) {
  char *path;

  if (asprintf(&path, "%s/%s", base, name) < 0)
    die("asprintf");
  return path;
}

static void ensure_dir(const char *base, const char *rel) {
  char *path = join(base, rel);

  if (make_Dir(path) != 0)
    die(flib_LastError());
  free(path);
}

static void put(const char *base, const char *rel, const void *data,
                size_t size) {
  char *path = join(base, rel);

  if (write_File(path, data, size) != 0)
    die(flib_LastError());
  free(path);
}

static char *read_text(const char *base, const char *rel) {
  char *path = join(base, rel);
  char *text = read_File(path);

  free(path);
  if (!text)
    die(flib_LastError());
  return text;
}

static char *read_bin(const char *base, const char *rel, size_t *size) {
  char *path = join(base, rel);
  char *data = read_FileEx(path, size);

  free(path);
  if (!data)
    die(flib_LastError());
  return data;
}

static int exists_rel(const char *base, const char *rel) {
  char *path = join(base, rel);
  int ok = access(path, F_OK) == 0;

  free(path);
  return ok;
}

static void make_link(const char *target, const char *base,
                      const char *rel) {
  char *path = join(base, rel);

  if (symlink(target, path) != 0)
    die("symlink");
  free(path);
}

static int write_rel(const char *base, const char *rel, const void *data,
                     size_t size) {
  char *path = join(base, rel);
  int rc = write_File(path, data, size);

  free(path);
  return rc;
}

static int append_rel(const char *base, const char *rel, const void *data,
                      size_t size) {
  char *path = join(base, rel);
  int rc = append_File(path, data, size);

  free(path);
  return rc;
}

static int remove_rel(const char *base, const char *rel) {
  char *path = join(base, rel);
  int rc = remove_File(path);

  free(path);
  return rc;
}

static int remove_dir_rel(const char *base, const char *rel) {
  char *path = join(base, rel);
  int rc = remove_Dir(path);

  free(path);
  return rc;
}

static int move_file_rel(const char *base, const char *src,
                         const char *dst) {
  char *src_path = join(base, src);
  char *dst_path = join(base, dst);
  int rc = move_File(src_path, dst_path);

  free(src_path);
  free(dst_path);
  return rc;
}

static int move_dir_rel(const char *base, const char *src,
                        const char *dst) {
  char *src_path = join(base, src);
  char *dst_path = join(base, dst);
  int rc = move_Dir(src_path, dst_path);

  free(src_path);
  free(dst_path);
  return rc;
}

static int walk_rel(const char *base, const char *rel, unsigned flags,
                    flib_WalkFn fn, void *user) {
  char *path = join(base, rel);
  int rc = walk_Dir(path, flags, fn, user);

  free(path);
  return rc;
}

static void test_write_append(const char *base) {
  char *text;
  char *data;
  size_t size;
  unsigned char bin[4] = {0, 1, 2, 3};

  ensure_dir(base, "write");

  CHECK(write_rel(base, "write/a.txt", "hello", 5) == 0);
  text = read_text(base, "write/a.txt");
  CHECK_STR(text, "hello");
  free(text);

  CHECK(append_rel(base, "write/a.txt", " world", 6) == 0);
  text = read_text(base, "write/a.txt");
  CHECK_STR(text, "hello world");
  free(text);

  CHECK(append_rel(base, "write/new.txt", "abc", 3) == 0);
  text = read_text(base, "write/new.txt");
  CHECK_STR(text, "abc");
  free(text);

  CHECK(write_rel(base, "write/bin.dat", bin, sizeof bin) == 0);
  data = read_bin(base, "write/bin.dat", &size);
  CHECK(size == sizeof bin);
  CHECK(memcmp(data, bin, sizeof bin) == 0);
  free(data);

  CHECK(write_rel(base, "write/empty.txt", NULL, 0) == 0);
  data = read_bin(base, "write/empty.txt", &size);
  CHECK(size == 0);
  CHECK(data[0] == '\0');
  free(data);

  CHECK(write_rel(base, "write/none/x.txt", "x", 1) == -1);
  CHECK(flib_LastError()[0] != '\0');

  CHECK(write_rel(base, "write/a.txt", NULL, 1) == -1);
  CHECK(append_rel(base, "write/none/y.txt", "x", 1) == -1);
  CHECK(write_rel(base, "write", "x", 1) == -1);

  CHECK(remove_rel(base, "write/a.txt") == 0);
  CHECK(!exists_rel(base, "write/a.txt"));
}

static void test_remove(const char *base) {
  char *target;
  char *fifo;

  ensure_dir(base, "rm/sub");
  ensure_dir(base, "outside");
  put(base, "rm/a.txt", "a", 1);
  put(base, "rm/sub/b.txt", "b", 1);
  put(base, "outside/keep.txt", "keep", 4);

  target = join(base, "outside");
  make_link(target, base, "rm/link");
  free(target);

  CHECK(remove_dir_rel(base, "rm") == 0);
  CHECK(!exists_rel(base, "rm"));
  CHECK(exists_rel(base, "outside/keep.txt"));

  target = join(base, "outside");
  make_link(target, base, "outside-link");
  free(target);

  CHECK(remove_dir_rel(base, "outside-link") == -1);
  CHECK(exists_rel(base, "outside/keep.txt"));
  CHECK(remove_rel(base, "outside-link") == 0);

  CHECK(remove_dir_rel(base, "outside/keep.txt") == -1);
  CHECK(remove_rel(base, "outside/keep.txt") == 0);
  CHECK(!exists_rel(base, "outside/keep.txt"));
  CHECK(remove_Dir("") == -1);
  CHECK(remove_dir_rel(base, "no-such-dir") == -1);
  CHECK(remove_dir_rel(base, "outside") == 0);

  fifo = join(base, "fifo");
  if (mkfifo(fifo, 0600) != 0)
    die("mkfifo");
  CHECK(remove_rel(base, "fifo") == 0);
  CHECK(access(fifo, F_OK) != 0);
  free(fifo);
}

static void test_move(const char *base) {
  char *target;
  char *text;

  ensure_dir(base, "move/d1");
  put(base, "move/a.txt", "data", 4);
  put(base, "move/d1/inner.txt", "inner", 5);

  CHECK(move_file_rel(base, "move/a.txt", "move/b.txt") == 0);
  CHECK(!exists_rel(base, "move/a.txt"));
  CHECK(exists_rel(base, "move/b.txt"));

  text = read_text(base, "move/b.txt");
  CHECK_STR(text, "data");
  free(text);

  CHECK(move_file_rel(base, "move/b.txt", "move/b.txt") == 0);
  CHECK(exists_rel(base, "move/b.txt"));

  CHECK(move_file_rel(base, "move/b.txt", "move/none/c.txt") == -1);
  CHECK(exists_rel(base, "move/b.txt"));

  CHECK(move_file_rel(base, "move/d1", "move/file-target") == -1);
  CHECK(exists_rel(base, "move/d1/inner.txt"));

  CHECK(move_dir_rel(base, "move/d1", "move/d2") == 0);
  CHECK(!exists_rel(base, "move/d1"));
  CHECK(exists_rel(base, "move/d2/inner.txt"));

  CHECK(move_dir_rel(base, "move/d2", "move/none/d3") == -1);
  CHECK(exists_rel(base, "move/d2/inner.txt"));

  target = join(base, "move/b.txt");
  make_link(target, base, "move/link");
  free(target);

  CHECK(move_file_rel(base, "move/link", "move/link2") == -1);
  CHECK(exists_rel(base, "move/link"));
}

typedef struct WalkStats {
  int count;
  int files;
  int dirs;
  int symlinks;
  int hidden;
  int max_depth;
  int stop_after;
} WalkStats;

static int walk_stats(const char *path, FileType type,
                      unsigned long long size, int depth, void *user) {
  WalkStats *s = user;
  const char *name = strrchr(path, '/');

  (void)size;
  name = name ? name + 1 : path;

  if (name[0] == '.')
    s->hidden++;
  if (type == FT_DIRECTORY)
    s->dirs++;
  else if (type == FT_SYMLINK)
    s->symlinks++;
  else
    s->files++;

  if (depth > s->max_depth)
    s->max_depth = depth;

  s->count++;
  if (s->stop_after && s->count >= s->stop_after)
    return 7;
  return 0;
}

static void test_walk(const char *base) {
  WalkStats stats;
  char *tree;
  char *loop;

  ensure_dir(base, "tree/sub/deep");
  put(base, "tree/a.txt", "a", 1);
  put(base, "tree/.hidden", "h", 1);
  put(base, "tree/sub/b.txt", "b", 1);
  put(base, "tree/sub/deep/c.txt", "c", 1);

  make_link("sub", base, "tree/link");
  tree = join(base, "tree");
  loop = join(base, "tree/loop");
  if (symlink(tree, loop) != 0)
    die("symlink loop");
  free(loop);

  memset(&stats, 0, sizeof stats);
  CHECK(walk_Dir(tree, 0, walk_stats, &stats) == 0);
  CHECK(stats.count == 7);
  CHECK(stats.files == 3);
  CHECK(stats.dirs == 2);
  CHECK(stats.symlinks == 2);
  CHECK(stats.hidden == 0);
  CHECK(stats.max_depth == 2);

  memset(&stats, 0, sizeof stats);
  CHECK(walk_Dir(tree, FLIB_WALK_HIDDEN, walk_stats, &stats) == 0);
  CHECK(stats.count == 8);
  CHECK(stats.hidden == 1);

  memset(&stats, 0, sizeof stats);
  CHECK(walk_Dir(tree, FLIB_WALK_FOLLOW, walk_stats, &stats) == 0);
  CHECK(stats.count == 10);
  CHECK(stats.files == 5);
  CHECK(stats.dirs == 5);
  CHECK(stats.symlinks == 0);
  CHECK(stats.max_depth == 2);

  memset(&stats, 0, sizeof stats);
  stats.stop_after = 1;
  CHECK(walk_Dir(tree, 0, walk_stats, &stats) == 7);
  CHECK(stats.count == 1);

  CHECK(walk_rel(base, "no-such-tree", 0, walk_stats, &stats) == -1);
  CHECK(walk_rel(base, "tree/a.txt", 0, walk_stats, &stats) == -1);
  CHECK(walk_Dir(tree, 0, NULL, &stats) == -1);

  ensure_dir(base, "broken");
  make_link("nowhere", base, "broken/bad");
  memset(&stats, 0, sizeof stats);
  CHECK(walk_rel(base, "broken", FLIB_WALK_FOLLOW, walk_stats, &stats) == 0);
  CHECK(stats.count == 1);
  CHECK(stats.symlinks == 1);

  free(tree);
}

int main(void) {
  const char *tmp = getenv("TMPDIR");
  char *template;
  char *base;

  if (!tmp || !*tmp)
    tmp = "/tmp";

  if (asprintf(&template, "%s/flib-test-XXXXXX", tmp) < 0)
    die("asprintf");
  base = mkdtemp(template);
  if (!base)
    die("mkdtemp");

  CHECK(strcmp(FLIB_VERSION, "1.2.0") == 0);

  test_write_append(base);
  test_remove(base);
  test_move(base);
  test_walk(base);

  CHECK(remove_Dir(base) == 0);
  free(base);

  printf("failures: %d\n", failures);
  return failures ? 1 : 0;
}
