#ifndef FLIB_H
#define FLIB_H

#define FLIB_VERSION "1.2.0"
#define FLIB_VERSION_MAJOR 1
#define FLIB_VERSION_MINOR 2
#define FLIB_VERSION_PATCH 0

#include <stddef.h>
#include <stdio.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum FileType {
    FT_UNKNOWN = 0,
    FT_REGULAR,
    FT_DIRECTORY,
    FT_SYMLINK,
    FT_FIFO,
    FT_SOCKET,
    FT_BLOCK_DEVICE,
    FT_CHAR_DEVICE
} FileType;

typedef struct FileInfo {
    char *path;
    char *name;
    char *extension;
    char *language;

    FileType type;
    int is_text;
    int is_empty;
    unsigned long long size;
    unsigned long long lines;

    unsigned mode;
    char perms[11];

    unsigned long uid;
    unsigned long gid;
    unsigned long inode;
    unsigned long links;

    time_t created;
    int created_is_birth;
    time_t modified;
    time_t accessed;
    time_t changed;
} FileInfo;

typedef struct DirEntry {
    char *name;
    char *path;
    FileType type;
    unsigned long long size;
} DirEntry;

typedef struct DirList {
    DirEntry *items;
    size_t count;
} DirList;

int print_File(const char *path);

char *read_File(const char *path);

char *read_FileEx(const char *path, size_t *out_size);

int info_File(const char *path, FileInfo *info);

void info_File_Free(FileInfo *info);

void info_File_Print(const FileInfo *info, FILE *out);

int read_Dir(const char *path, DirList *list);

char *read_DirText(const char *path);

void read_Dir_Free(DirList *list);

void read_Dir_Print(const DirList *list, FILE *out);

int make_Dir(const char *path);

int copy_File(const char *src, const char *dst);

int copy_Dir(const char *src, const char *dst);

int write_File(const char *path, const void *data, size_t size);

int append_File(const char *path, const void *data, size_t size);

int remove_File(const char *path);

int remove_Dir(const char *path);

int move_File(const char *src, const char *dst);

int move_Dir(const char *src, const char *dst);

#define FLIB_WALK_FOLLOW (1u << 0)
#define FLIB_WALK_HIDDEN (1u << 1)

typedef int (*flib_WalkFn)(const char *path, FileType type,
                           unsigned long long size, int depth, void *user);

int walk_Dir(const char *path, unsigned flags, flib_WalkFn fn, void *user);

int file_Exists(const char *path);

const char *file_Language(const char *path);

const char *file_Type_Name(FileType type);

const char *time_Format(time_t t, char *buf, size_t bufsize);

const char *flib_LastError(void);

#ifdef __cplusplus
}
#endif

#endif
