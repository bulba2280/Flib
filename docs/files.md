# Работа с файлами

Правила одинаковые почти для всех функций:

- что-то делает: вернула `0`, значит получилось, `-1` значит нет;
- что-то создаёт: вернула указатель, а при ошибке `NULL`;
- причину ошибки всегда можно прочитать через `flib_LastError()`.

## print_File

```c
int print_File(const char *path);
```

Печатает файл в `stdout`, по сути `cat` внутри программы. Данные идут кусками
по 64 КиБ, поэтому память не зависит от размера файла.

```c
if (print_File("hello.txt") != 0)
    fprintf(stderr, "%s\n", flib_LastError());
```

## read_File

```c
char *read_File(const char *path);
char *read_FileEx(const char *path, size_t *out_size);
```

`read_File()` читает файл целиком и возвращает строку с `'\0'` в конце.
Память выделяется через `malloc`, освобождать её вызывающему.

`read_FileEx()` делает то же самое, но ещё записывает размер в `*out_size`
(терминирующий ноль не считается). Если не вышло, вернётся `NULL`, а
`*out_size` станет `0`.

```c
size_t len;
char *text = read_FileEx("main.c", &len);

if (!text) {
    fprintf(stderr, "ошибка: %s\n", flib_LastError());
    return 1;
}
printf("прочитано %zu байт\n", len);
free(text);
```

## info_File

```c
int info_File(const char *path, FileInfo *info);
void info_File_Free(FileInfo *info);
```

Собирает в структуру всё, что можно узнать о файле: размер, число строк, язык,
права, даты, тип.

```c
FileInfo fi;

if (info_File("photo.png", &fi) == 0) {
    printf("размер: %llu байт\n", fi.size);
    printf("язык:   %s\n", fi.language ? fi.language : "не определён");
    info_File_Free(&fi);
}
```

`info_File_Free()` освобождает строки внутри структуры и обнуляет её. Вызывать
после каждого успешного `info_File()`.

Строки считаются по всему файлу, поэтому на большом логе `info_File()` будет
думать ровно столько, сколько нужно, чтобы его прочитать. У FIFO, сокетов и
устройств содержимое не читается: заполняются только метаданные.

Что лежит в `FileInfo`:

| Поле | Значение |
| --- | --- |
| `path`, `name`, `extension` | путь, имя, расширение без точки |
| `language` | язык или формат: по расширению, имени или shebang |
| `type` | `FT_REGULAR`, `FT_DIRECTORY`, `FT_SYMLINK`, `FT_FIFO`, `FT_SOCKET`, `FT_BLOCK_DEVICE`, `FT_CHAR_DEVICE` |
| `is_text` | эвристика «текст или двоичные данные» |
| `is_empty` | размер 0 байт |
| `size` | размер в байтах |
| `lines` | число строк, только для текста |
| `mode`, `perms` | права числом (`0644`) и строкой (`-rw-r--r--`) |
| `uid`, `gid`, `inode`, `links` | владелец, группа, inode, число жёстких ссылок |
| `created`, `created_is_birth` | время создания (btime), либо `ctime`, если ФС не хранит btime |
| `modified`, `accessed`, `changed` | mtime, atime, ctime |

## info_File_Print

```c
void info_File_Print(const FileInfo *info, FILE *out);
```

Печатает структуру в `out`. Если передать `NULL`, вывод пойдёт в `stdout`.
Удобно в консольных утилитах и при отладке.

```c
FileInfo fi;

if (info_File("notes.txt", &fi) == 0) {
    info_File_Print(&fi, stdout);
    info_File_Free(&fi);
}
```

Выглядит примерно так:

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

## Служебные функции

```c
int file_Exists(const char *path);
const char *file_Language(const char *path);
const char *file_Type_Name(FileType type);
const char *time_Format(time_t t, char *buf, size_t bufsize);
const char *flib_LastError(void);
```

`file_Exists()` возвращает `1`, если путь есть, иначе `0`.

`file_Language()` определяет язык по расширению, имени файла или shebang.
Возвращает `NULL`, если ничего не подошло. Строка статическая, `free()` не
нужен.

`file_Type_Name()` даёт человекочитаемое имя типа: `regular file`, `directory` и
так далее.

`time_Format()` пишет время в `buf` в виде `ГГГГ-ММ-ДД ЧЧ:ММ:СС` и возвращает
сам `buf`.

```c
char when[32];
printf("изменён: %s\n", time_Format(fi.modified, when, sizeof when));
```

`flib_LastError()` отдаёт текст последней ошибки. Буфер свой у каждого потока
(`_Thread_local`), текст в UTF-8.

Функции, которые ничего не создают, ошибку не выставляют. Так что смотреть на
`flib_LastError()` имеет смысл только после `-1` или `NULL`.
