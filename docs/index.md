# flib

Маленькая библиотека на C для работы с файлами и каталогами: читает и печатает
файлы, собирает метаданные, выводит содержимое каталога и копирует его
рекурсивно. Зависимостей нет, только POSIX и стандартная библиотека.

## Сборка

```sh
make
sudo make install
```

Заголовок ставится в `/usr/local/include`, библиотеки в `/usr/local/lib`. Без
установки можно линковаться прямо из каталога сборки:

```sh
cc -std=c11 -Iinclude main.c -Lbuild -lflib -o main
```

## Быстрый пример

```c
#include "flib.h"
#include <stdio.h>

int main(void) {
    FileInfo fi;

    if (info_File("main.c", &fi) != 0) {
        fprintf(stderr, "%s\n", flib_LastError());
        return 1;
    }

    printf("%s, %llu байт, язык: %s\n", fi.name, fi.size,
           fi.language ? fi.language : "не определён");
    info_File_Free(&fi);
    return 0;
}
```

## Разделы

- [Работа с файлами](files.md): чтение, печать, `FileInfo`
- [Работа с каталогами](dirs.md): листинг, создание, копирование
- [Примеры](examples.md): готовые куски кода
- [История изменений](changelog.md)
- [Лицензия](license.md)
