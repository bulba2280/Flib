#define _GNU_SOURCE

#include "internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

typedef struct MapEntry {
  const char *key;
  const char *value;
} MapEntry;

#define ARRAY_LEN(a) (sizeof a / sizeof a[0])

static const char *lookup(const MapEntry *map, size_t n, const char *key) {
  size_t i;

  for (i = 0; i < n; i++)
    if (strcasecmp(key, map[i].key) == 0)
      return map[i].value;
  return NULL;
}

static const MapEntry g_ext_langs[] = {
    {"c", "C"},
    {"h", "C header"},
    {"cpp", "C++"},
    {"cc", "C++"},
    {"cxx", "C++"},
    {"c++", "C++"},
    {"hpp", "C++ header"},
    {"hh", "C++ header"},
    {"hxx", "C++ header"},
    {"cs", "C#"},
    {"java", "Java"},
    {"kt", "Kotlin"},
    {"kts", "Kotlin"},
    {"swift", "Swift"},
    {"py", "Python"},
    {"pyw", "Python"},
    {"pyi", "Python"},
    {"js", "JavaScript"},
    {"mjs", "JavaScript"},
    {"cjs", "JavaScript"},
    {"jsx", "JavaScript (JSX)"},
    {"ts", "TypeScript"},
    {"tsx", "TypeScript (TSX)"},
    {"rs", "Rust"},
    {"go", "Go"},
    {"rb", "Ruby"},
    {"php", "PHP"},
    {"pl", "Perl"},
    {"pm", "Perl"},
    {"lua", "Lua"},
    {"sh", "Shell"},
    {"bash", "Bash"},
    {"zsh", "Zsh"},
    {"fish", "Fish"},
    {"ksh", "Ksh"},
    {"ps1", "PowerShell"},
    {"bat", "Batch"},
    {"cmd", "Batch"},
    {"asm", "Assembly"},
    {"s", "Assembly"},
    {"html", "HTML"},
    {"htm", "HTML"},
    {"css", "CSS"},
    {"scss", "SCSS"},
    {"sass", "Sass"},
    {"less", "Less"},
    {"json", "JSON"},
    {"jsonc", "JSON with comments"},
    {"xml", "XML"},
    {"yaml", "YAML"},
    {"yml", "YAML"},
    {"toml", "TOML"},
    {"ini", "INI"},
    {"conf", "Config"},
    {"cfg", "Config"},
    {"md", "Markdown"},
    {"markdown", "Markdown"},
    {"rst", "reStructuredText"},
    {"tex", "LaTeX"},
    {"sql", "SQL"},
    {"db", "Database"},
    {"sqlite", "SQLite"},
    {"csv", "CSV"},
    {"tsv", "TSV"},
    {"txt", "Text"},
    {"log", "Log"},
    {"png", "PNG image"},
    {"jpg", "JPEG image"},
    {"jpeg", "JPEG image"},
    {"gif", "GIF image"},
    {"bmp", "BMP image"},
    {"svg", "SVG image"},
    {"webp", "WebP image"},
    {"ico", "Icon"},
    {"mp3", "MP3 audio"},
    {"wav", "WAV audio"},
    {"flac", "FLAC audio"},
    {"ogg", "OGG audio"},
    {"mp4", "MP4 video"},
    {"mkv", "Matroska video"},
    {"avi", "AVI video"},
    {"mov", "QuickTime video"},
    {"webm", "WebM video"},
    {"pdf", "PDF"},
    {"zip", "ZIP archive"},
    {"tar", "TAR archive"},
    {"gz", "GZip archive"},
    {"7z", "7-Zip archive"},
    {"rar", "RAR archive"},
    {"xz", "XZ archive"},
    {"bz2", "BZip2 archive"},
    {"woff", "WOFF font"},
    {"woff2", "WOFF2 font"},
    {"ttf", "TrueType font"},
    {"otf", "OpenType font"},
    {"exe", "Windows executable"},
    {"dll", "Windows library"},
    {"so", "Shared library"},
    {"a", "Static library"},
    {"o", "Object file"},
    {"gitignore", "Git ignore"},
    {"env", "Environment variables"},
};

// Names with no extension, or ones that need special handling.
static const MapEntry g_name_langs[] = {
    {"Makefile", "Makefile"},
    {"makefile", "Makefile"},
    {"GNUmakefile", "Makefile"},
    {"Dockerfile", "Dockerfile"},
    {"CMakeLists.txt", "CMake"},
    {"Rakefile", "Ruby"},
    {"Gemfile", "Ruby"},
    {"Vagrantfile", "Ruby"},
    {"configure", "Shell"},
    {".bashrc", "Bash"},
    {".bash_profile", "Bash"},
    {".zshrc", "Zsh"},
    {".profile", "Shell"},
    {".gitignore", "Git ignore"},
    {".gitattributes", "Git attributes"},
    {".editorconfig", "EditorConfig"},
    {".env", "Environment variables"},
};

// Interpreters found in shebang lines.
static const MapEntry g_interp_langs[] = {
    {"python", "Python"},   {"python2", "Python 2"},  {"python3", "Python 3"},
    {"bash", "Bash"},       {"sh", "Shell"},          {"dash", "Shell"},
    {"zsh", "Zsh"},         {"fish", "Fish"},         {"ksh", "Ksh"},
    {"node", "JavaScript"}, {"nodejs", "JavaScript"}, {"deno", "TypeScript"},
    {"bun", "JavaScript"},  {"perl", "Perl"},         {"ruby", "Ruby"},
    {"php", "PHP"},         {"lua", "Lua"},           {"tclsh", "Tcl"},
    {"wish", "Tcl"},        {"awk", "AWK"},           {"Rscript", "R"},
};

static const char *lang_by_shebang(const char *path) {
  FILE *f;
  struct stat st;
  char line[256];
  char *save = NULL;
  char *tok;
  const char *result = NULL;

  // Regular files only: fopen() blocks on FIFOs and some devices.
  if (stat(path, &st) != 0 || !S_ISREG(st.st_mode))
    return NULL;

  f = fopen(path, "rb");
  if (!f)
    return NULL;

  if (fgets(line, sizeof line, f) && line[0] == '#' && line[1] == '!') {
    tok = strtok_r(line + 2, " \t\r\n", &save);

    if (tok && strcasecmp(tok, "/usr/bin/env") == 0) {
      tok = strtok_r(NULL, " \t\r\n", &save);
      if (tok && strcmp(tok, "-S") == 0)
        tok = strtok_r(NULL, " \t\r\n", &save);
    }

    if (tok) {
      const char *base = strrchr(tok, '/');

      base = base ? base + 1 : tok;
      result = lookup(g_interp_langs, ARRAY_LEN(g_interp_langs), base);
      if (!result && strncmp(base, "python", 6) == 0)
        result = "Python";
    }
  }

  fclose(f);
  return result;
}

const char *file_Language(const char *path) {
  char *name;
  const char *lang;

  if (!path)
    return NULL;

  name = flib_path_name(path);
  if (!name)
    return NULL;

  lang = lookup(g_name_langs, ARRAY_LEN(g_name_langs), name);
  if (!lang)
    lang = lookup(g_ext_langs, ARRAY_LEN(g_ext_langs), flib_ext_of(name));
  if (!lang)
    lang = lang_by_shebang(path);

  free(name);
  return lang;
}
