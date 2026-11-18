# flib

A small C library for files and directories: it reads and prints file contents,
collects metadata, writes, moves and removes files, lists directories and copies
them recursively. POSIX only, no dependencies.

## Build

```sh
make
sudo make install
```

The header goes to `/usr/local/include`, the libraries to `/usr/local/lib`.
Without installation you can link straight from the build directory:

```sh
cc -std=c11 -Iinclude main.c -Lbuild -lflib -o main
```

## Quick example

```c
#include "flib.h"
#include <stdio.h>

int main(void) {
    FileInfo fi;

    if (info_File("main.c", &fi) != 0) {
        fprintf(stderr, "%s\n", flib_LastError());
        return 1;
    }

    printf("%s, %llu bytes, language: %s\n", fi.name, fi.size,
           fi.language ? fi.language : "unknown");
    info_File_Free(&fi);
    return 0;
}
```

## Sections

- [Working with files](files.md): reading, writing, `FileInfo`, moving and removing
- [Working with directories](dirs.md): listing, creating, copying and removing
- [Examples](examples.md): ready to use snippets
- [Changelog](changelog.md)
- [License](license.md)
