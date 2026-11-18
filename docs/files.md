# Working with files

The rules are the same for almost every function:

- it does something: `0` means success, `-1` means failure;
- it creates something: it returns a pointer, or `NULL` on error;
- the reason is always available from `flib_LastError()`.

## print_File

```c
int print_File(const char *path);
```

Prints a file to `stdout`, basically `cat` inside your program. Data goes in
64 KiB chunks, so memory use does not depend on the file size.

```c
if (print_File("hello.txt") != 0)
    fprintf(stderr, "%s\n", flib_LastError());
```

## read_File

```c
char *read_File(const char *path);
char *read_FileEx(const char *path, size_t *out_size);
```

`read_File()` reads the whole file and returns a NUL-terminated string. The
memory comes from `malloc`, so the caller frees it.

`read_FileEx()` does the same, but also stores the size in `*out_size` (the
terminator is not counted). On failure it returns `NULL` and sets `*out_size`
to `0`.

```c
size_t len;
char *text = read_FileEx("main.c", &len);

if (!text) {
    fprintf(stderr, "error: %s\n", flib_LastError());
    return 1;
}
printf("read %zu bytes\n", len);
free(text);
```

## write_File and append_File

```c
int write_File(const char *path, const void *data, size_t size);
int append_File(const char *path, const void *data, size_t size);
```

`write_File()` truncates the file and writes exactly `size` bytes. `append_File()`
writes at the end and creates the file when it does not exist. Both work with
binary data and never add a NUL terminator.

```c
if (write_File("notes.txt", "hello\n", 6) != 0)
    fprintf(stderr, "%s\n", flib_LastError());

if (append_File("notes.txt", "world\n", 6) != 0)
    fprintf(stderr, "%s\n", flib_LastError());
```

On failure the function returns `-1`. `write_File()` is not atomic: if the disk
fills up, part of the file may already be written.

## move_File

```c
int move_File(const char *src, const char *dst);
```

Moves a regular file. It uses `rename()` first, so on the same filesystem the
operation is atomic. When the filesystem returns `EXDEV`, it copies the file and
removes the source.

`dst` is the new path, not a directory to move into. Moving a symlink or a
directory returns an error. If `dst` already exists, `rename()` replaces it when
the rules of the system allow that.

## remove_File

```c
int remove_File(const char *path);
```

Removes a file, symlink or FIFO. A directory returns an error, use
`remove_Dir()` for that.

```c
if (remove_File("old.log") != 0)
    fprintf(stderr, "%s\n", flib_LastError());
```

## info_File

```c
int info_File(const char *path, FileInfo *info);
void info_File_Free(FileInfo *info);
```

Collects everything it can find out about a file into the struct: size, line
count, language, permissions, timestamps, type.

```c
FileInfo fi;

if (info_File("photo.png", &fi) == 0) {
    printf("size: %llu bytes\n", fi.size);
    printf("language: %s\n", fi.language ? fi.language : "unknown");
    info_File_Free(&fi);
}
```

`info_File_Free()` releases the strings inside the struct and zeroes it. Call it
after every successful `info_File()`.

Line counting reads the whole file, so on a large log `info_File()` takes as
long as reading it. FIFOs, sockets and devices are not read at all: only
metadata is filled in.

What `FileInfo` contains:

| Field | Meaning |
| --- | --- |
| `path`, `name`, `extension` | path, name, extension without the dot |
| `language` | language or format from the extension, file name or shebang |
| `type` | `FT_REGULAR`, `FT_DIRECTORY`, `FT_SYMLINK`, `FT_FIFO`, `FT_SOCKET`, `FT_BLOCK_DEVICE`, `FT_CHAR_DEVICE` |
| `is_text` | "text or binary data" heuristic |
| `is_empty` | size is 0 bytes |
| `size` | size in bytes |
| `lines` | line count, text files only |
| `mode`, `perms` | permissions as a number (`0644`) and as a string (`-rw-r--r--`) |
| `uid`, `gid`, `inode`, `links` | owner, group, inode, hard link count |
| `created`, `created_is_birth` | creation time (btime), or `ctime` when the filesystem has no btime |
| `modified`, `accessed`, `changed` | mtime, atime, ctime |

## info_File_Print

```c
void info_File_Print(const FileInfo *info, FILE *out);
```

Prints the struct to `out`. Pass `NULL` to write to `stdout`. Handy in console
tools and while debugging.

```c
FileInfo fi;

if (info_File("notes.txt", &fi) == 0) {
    info_File_Print(&fi, stdout);
    info_File_Free(&fi);
}
```

The output looks roughly like this:

```
Path:        examples/demo.c
Name:        demo.c
Type:        regular file
Extension:   c
Language:    C
Size:        1.3 KB (1291 bytes)
Contents:    text, 51 lines
Permissions: -rw-r--r-- (0644)
Owner:       uid 1000, gid 1000
Links:       1
Inode:       4186096
Created:     2026-11-14 16:23:13
Modified:    2026-11-14 16:23:13
Accessed:    2026-11-14 16:23:13
Metadata:    2026-11-14 16:23:13
```

## Helper functions

```c
int file_Exists(const char *path);
const char *file_Language(const char *path);
const char *file_Type_Name(FileType type);
const char *time_Format(time_t t, char *buf, size_t bufsize);
const char *flib_LastError(void);
```

`file_Exists()` returns `1` if the path exists, `0` otherwise.

`file_Language()` detects the language from the extension, file name or
shebang. It returns `NULL` when nothing matches. The string is static, no
`free()` needed.

`file_Type_Name()` gives a readable type name: `regular file`, `directory` and
so on.

`time_Format()` writes the time into `buf` as `YYYY-MM-DD HH:MM:SS` and returns
`buf`.

```c
char when[32];
printf("modified: %s\n", time_Format(fi.modified, when, sizeof when));
```

`flib_LastError()` returns the text of the last error. The buffer is per thread
(`_Thread_local`), the text is UTF-8.

Not every function sets an error. Checking `flib_LastError()` only makes sense
after a function returned `-1` or `NULL` because it failed.
