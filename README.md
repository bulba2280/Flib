# flib

Маленькая библиотека на C для работы с файлами и каталогами. Читает и печатает
файлы, собирает метаданные (размер, строки, язык, права, даты), выводит
содержимое каталога и копирует его рекурсивно.

Зависимостей нет, только POSIX и стандартная библиотека.

## Сборка

```sh
make
```

На выходе получатся `build/libflib.a` и `build/libflib.so`. Если нужна
установка в систему:

```sh
sudo make install
```

Заголовок попадёт в `/usr/local/include`, библиотеки в `/usr/local/lib`.

Можно и не устанавливать, а линковаться прямо из каталога сборки:

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

## Пара замечаний

Проверял только на Linux. Время создания берётся через `statx()`, если система и
файловая система его отдают, иначе на месте времени создания будет `ctime`.

## Документация

- [Файлы](docs/files.md): чтение, печать, `FileInfo`
- [Каталоги](docs/dirs.md): листинг, создание, копирование
- [Примеры](docs/examples.md): готовые куски кода
- [История изменений](docs/changelog.md)

## Лицензия

MIT. Текст лежит в [LICENSE](LICENSE).
