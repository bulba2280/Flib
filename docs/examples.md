# Examples

Short standalone programs. Each one is a separate `main.c` that includes
`flib.h` and builds like this:

```sh
cc -std=c11 -Iinclude main.c -Lbuild -lflib -o main
```

## Back up a config

Before overwriting a file, keep a copy:

```c
#include "flib.h"
#include <stdio.h>

int main(void)
{
    if (copy_File("config.ini", "config.ini.bak") != 0) {
        fprintf(stderr, "backup failed: %s\n", flib_LastError());
        return 1;
    }
    return 0;
}
```

## Mini cat

Print the file passed as an argument:

```c
#include "flib.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s FILE\n", argv[0]);
        return 2;
    }
    if (print_File(argv[1]) != 0) {
        fprintf(stderr, "%s\n", flib_LastError());
        return 1;
    }
    return 0;
}
```

## Read the port from a config

Read the whole file and look for the `port=` line:

```c
#include "flib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    char *cfg = read_File("app.conf");

    if (!cfg) {
        fprintf(stderr, "%s\n", flib_LastError());
        return 1;
    }

    char *line = strstr(cfg, "port=");
    printf("port: %d\n", line ? atoi(line + 5) : 8080);

    free(cfg);
    return 0;
}
```

## Find every .log file in a directory

```c
#include "flib.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    DirList list;

    if (read_Dir("/var/log", &list) != 0) {
        fprintf(stderr, "%s\n", flib_LastError());
        return 1;
    }

    for (size_t i = 0; i < list.count; i++) {
        const char *name = list.items[i].name;
        size_t n = strlen(name);

        if (n > 4 && strcmp(name + n - 4, ".log") == 0)
            printf("%s\n", list.items[i].path);
    }

    read_Dir_Free(&list);
    return 0;
}
```

## Date-stamped directory backup

Copy `src` into a directory named after today's date:

```c
#include "flib.h"
#include <stdio.h>
#include <time.h>

int main(void)
{
    char dst[64];
    time_t now = time(NULL);

    strftime(dst, sizeof dst, "backup-%Y%m%d", localtime(&now));
    if (copy_Dir("src", dst) != 0) {
        fprintf(stderr, "%s\n", flib_LastError());
        return 1;
    }
    return 0;
}
```

## Sort reports into directories

Create the structure and put a file in it:

```c
#include "flib.h"
#include <stdio.h>

int main(void)
{
    if (make_Dir("out/2026") != 0 ||
        copy_File("report.csv", "out/2026/report.csv") != 0) {
        fprintf(stderr, "%s\n", flib_LastError());
        return 1;
    }
    return 0;
}
```

## Write a directory listing to a log

Get the directory contents as one string and append it to a file:

```c
#include "flib.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    FILE *log = fopen("uploads.log", "a");

    if (!log) {
        perror("uploads.log");
        return 1;
    }

    char *listing = read_DirText("uploads");
    if (!listing) {
        fprintf(stderr, "%s\n", flib_LastError());
        fclose(log);
        return 1;
    }

    fprintf(log, "%s", listing);
    fclose(log);
    free(listing);
    return 0;
}
```

## Walk a tree and total file sizes

`walk_Dir()` visits every entry without building a list:

```c
#include "flib.h"
#include <stdio.h>

static int add_size(const char *path, FileType type,
                    unsigned long long size, int depth, void *user)
{
    unsigned long long *total = user;

    (void)path;
    (void)depth;
    if (type == FT_REGULAR)
        *total += size;
    return 0;
}

int main(void)
{
    unsigned long long total = 0;

    if (walk_Dir("src", 0, add_size, &total) != 0) {
        fprintf(stderr, "%s\n", flib_LastError());
        return 1;
    }
    printf("%llu bytes\n", total);
    return 0;
}
```

## Check a file before reading it

There is no point in reading binary data blindly. Ask `flib` what the file is
first:

```c
#include "flib.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;

    FileInfo fi;

    if (info_File(argv[1], &fi) != 0) {
        fprintf(stderr, "%s\n", flib_LastError());
        return 1;
    }

    if (!fi.is_text) {
        printf("%s: binary data, %llu bytes\n", fi.name, fi.size);
        info_File_Free(&fi);
        return 0;
    }

    char *text = read_File(argv[1]);
    if (text) {
        printf("%s", text);
        free(text);
    }

    info_File_Free(&fi);
    return text ? 0 : 1;
}
```
