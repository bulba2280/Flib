# flib

A small C-lang library for files and directories. It reads and prints file contents,
reports metadata (size, line count, language, permissions, timestamps), writes,
moves and removes files, lists directories and copies them recursively.

POSIX only, no dependencies.

Live documentation: https://bulba2280.github.io/Flib/

## Build

```sh
make
```

This produces `build/libflib.a` and `build/libflib.so`. To install into the
system:

```sh
sudo make install
```

The header goes to `/usr/local/include`, the libraries to `/usr/local/lib`.

You can also skip installation and link straight from the build directory:

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

## A couple of notes

Tested on Linux only. Creation time comes from `statx()` when the system and
filesystem provide it, otherwise `ctime` is used in its place.

## Documentation

- [Files](docs/files.md): reading, writing, `FileInfo`, moving and removing
- [Directories](docs/dirs.md): listing, creating, copying and removing
- [Examples](docs/examples.md): ready to use snippets
- [Changelog](docs/changelog.md)

The same pages are available as static HTML, starting at
[docs/index.html](docs/index.html). To rebuild them after editing the
Markdown:

```sh
python3 -m pip install -r docs/requirements.txt
make docs
```

## License

MIT. See [LICENSE](LICENSE).
