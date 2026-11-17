# Работа с каталогами

Здесь всё так же, как с файлами: `0` при успехе, `-1` при ошибке, `NULL`,
если создать ничего не удалось.

## read_Dir

```c
int read_Dir(const char *path, DirList *list);
void read_Dir_Free(DirList *list);
```

Читает каталог в `DirList`: имена, полные пути, типы, размеры. Точки `.` и
`..` пропускаются, остальное сортируется по имени. Память освобождает
`read_Dir_Free()`.

```c
DirList list;

if (read_Dir("src", &list) == 0) {
    for (size_t i = 0; i < list.count; i++)
        printf("%s%s\n", list.items[i].name,
               list.items[i].type == FT_DIRECTORY ? "/" : "");
    read_Dir_Free(&list);
}
```

В `DirList` два поля: `items` (массив `DirEntry`) и `count`.

| Поле `DirEntry` | Значение |
| --- | --- |
| `name` | имя внутри каталога |
| `path` | полный путь, каталог плюс имя |
| `type` | `FT_REGULAR`, `FT_DIRECTORY`, `FT_SYMLINK`, `FT_FIFO` и остальные |
| `size` | размер в байтах |

## read_DirText

```c
char *read_DirText(const char *path);
```

Отдаёт всё содержимое каталога одной строкой: по имени на строку, у каталогов
на конце `/`. Пустой каталог даёт `""`. Память освобождает вызывающий.

```c
char *text = read_DirText("src");

if (!text) {
    fprintf(stderr, "ошибка: %s\n", flib_LastError());
    return 1;
}
printf("%s", text);
free(text);
```

## read_Dir_Print

```c
void read_Dir_Print(const DirList *list, FILE *out);
```

Печатает список в формате «тип, размер, имя». При `out == NULL` вывод идёт в
`stdout`.

## make_Dir

```c
int make_Dir(const char *path);
```

Создаёт каталог вместе со всеми родителями, как `mkdir -p`. Если каталог уже
есть, это не ошибка.

```c
make_Dir("backup/2026/november");
```

## copy_File

```c
int copy_File(const char *src, const char *dst);
```

Копирует обычный файл: содержимое, права, время изменения. Если `src` и `dst`
указывают на один и тот же файл, вернёт ошибку.

```c
if (copy_File("config.ini", "config.ini.bak") != 0)
    fprintf(stderr, "%s\n", flib_LastError());
```

## copy_Dir

```c
int copy_Dir(const char *src, const char *dst);
```

Рекурсивно копирует содержимое `src` в `dst`.

- Если `dst` нет, он создаётся вместе с родителями, права и время переносятся.
- Если `dst` есть, файлы копируются внутрь, совпадающие перезаписываются.
- Обычные файлы, подкаталоги, симлинки и FIFO копируются, симлинк остаётся
  ссылкой.
- Сокеты и устройства пропускаются.
- Копирование каталога внутрь самого себя считается ошибкой.

При ошибке вернётся `-1`, уже скопированная часть останется на месте.

```c
if (copy_Dir("project", "backup/project") != 0)
    fprintf(stderr, "ошибка: %s\n", flib_LastError());
```
