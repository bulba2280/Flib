# Working with directories

Everything works the same way as with files: `0` on success, `-1` on failure,
`NULL` when nothing could be created.

## read_Dir

```c
int read_Dir(const char *path, DirList *list);
void read_Dir_Free(DirList *list);
```

Reads a directory into a `DirList`: names, full paths, types, sizes. The `.`
and `..` entries are skipped, everything else is sorted by name. Memory is
released by `read_Dir_Free()`.

```c
DirList list;

if (read_Dir("src", &list) == 0) {
    for (size_t i = 0; i < list.count; i++)
        printf("%s%s\n", list.items[i].name,
               list.items[i].type == FT_DIRECTORY ? "/" : "");
    read_Dir_Free(&list);
}
```

`DirList` has two fields: `items` (an array of `DirEntry`) and `count`.

| `DirEntry` field | Meaning |
| --- | --- |
| `name` | name inside the directory |
| `path` | full path, directory plus name |
| `type` | `FT_REGULAR`, `FT_DIRECTORY`, `FT_SYMLINK`, `FT_FIFO` and the rest |
| `size` | size in bytes |

## read_DirText

```c
char *read_DirText(const char *path);
```

Returns the whole directory as one string: one name per line, directories end
with `/`. An empty directory gives `""`. The caller frees the memory.

```c
char *text = read_DirText("src");

if (!text) {
    fprintf(stderr, "error: %s\n", flib_LastError());
    return 1;
}
printf("%s", text);
free(text);
```

## read_Dir_Print

```c
void read_Dir_Print(const DirList *list, FILE *out);
```

Prints the list as "type, size, name". With `out == NULL` the output goes to
`stdout`.

## make_Dir

```c
int make_Dir(const char *path);
```

Creates a directory together with all its parents, like `mkdir -p`. An existing
directory is not an error.

```c
make_Dir("backup/2026/november");
```

## copy_File

```c
int copy_File(const char *src, const char *dst);
```

Copies a regular file: contents, permissions, modification time. If `src` and
`dst` point to the same file, it returns an error.

```c
if (copy_File("config.ini", "config.ini.bak") != 0)
    fprintf(stderr, "%s\n", flib_LastError());
```

## copy_Dir

```c
int copy_Dir(const char *src, const char *dst);
```

Recursively copies the contents of `src` into `dst`.

- If `dst` does not exist, it is created along with its parents, permissions
  and times are copied.
- If `dst` exists, entries are copied inside it, and matching files are
  overwritten.
- Regular files, subdirectories, symlinks and FIFOs are copied, a symlink stays
  a link.
- Sockets and devices are skipped.
- Copying a directory into itself is an error.

On failure it returns `-1`, and whatever was already copied stays in place.

```c
if (copy_Dir("project", "backup/project") != 0)
    fprintf(stderr, "error: %s\n", flib_LastError());
```

## remove_Dir

```c
int remove_Dir(const char *path);
```

Removes a directory with everything inside, like `rm -rf`. Symlinks are removed
as links, their targets are not touched. The function refuses `""`, `"."`,
`".."` and the filesystem root.

On failure it stops at the first error and returns `-1`. What was already removed
stays removed, this cannot be undone. A tree deeper than 1024 levels stops with
an error instead of overflowing the stack.

```c
if (remove_Dir("build") != 0)
    fprintf(stderr, "%s\n", flib_LastError());
```

## move_Dir

```c
int move_Dir(const char *src, const char *dst);
```

Moves a directory. It uses `rename()` first, so on the same filesystem the
operation is atomic. When the filesystem returns `EXDEV`, it copies the tree and
removes the source. The second case is not atomic: if it fails, a partial copy
may stay in place. Trees with sockets or devices are refused in that case,
because `copy_Dir()` skips them and removing the source would lose them.

`dst` is the new path, not a directory to move into. If `dst` already exists,
`rename()` replaces it when the rules of the system allow that, and the
cross-device fallback merges the contents with `copy_Dir()`.
