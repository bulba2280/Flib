# Changelog

## 1.2.0 (2026-11-18)

Added:

- `write_File()` and `append_File()` for writing binary data;
- `remove_File()` and recursive `remove_Dir()`;
- `move_File()` and `move_Dir()`, with a copy and remove fallback across
  filesystems.

## 1.1.0 (2026-11-14)

Added:

- directory support: `read_Dir()`, `read_DirText()`, `read_Dir_Print()`,
  `read_Dir_Free()`, `make_Dir()`, `copy_File()`, `copy_Dir()`;
- English output from the library: error messages, type names and the
  `info_File_Print()` format;
- English documentation with a static HTML build.

## 1.0.0 (2026-11-14)

First release: `print_File()`, `read_File()`, `read_FileEx()`, `info_File()`,
`info_File_Print()`, `info_File_Free()`. Plus the helpers `file_Exists()`,
`file_Language()`, `file_Type_Name()`, `time_Format()` and `flib_LastError()`.
