#include <dirent.h>
#include <stdio.h>
#include <string.h>

#define NOB_STRIP_PREFIX
#include "nob.h"

#define BINDIR "bin/"
#define SRCDIR "src/"
#define ODIR SRCDIR "obj/"
#define IDIR "include"
#define LDIR "libs"

#define TRGT BINDIR "main"
#define TRGT_DB BINDIR "main_db"

Cmd cmd = {0};
File_Paths deps = {0};

int cc_flags(void)
{
  cmd_append(&cmd, "-Wall");
  cmd_append(&cmd, "-Wextra");
  cmd_append(&cmd, "-pedantic");

  return 1;
}

int l_flags(void)
{
  cmd_append(&cmd, "-I" IDIR);

#ifdef _WIN32
  cmd_append(&cmd, "-L" LDIR);
#else
  cmd_append(&cmd, "-lm");
#endif
  cmd_append(&cmd, "-lcurses");
  cmd_append(&cmd, "-lgmp");

  return 1;
}

int function_on_all_files_in_directory(const char* const dir_path, int (*f)(char *))
{
  DIR* d;
  struct dirent* dir;
  d = opendir(dir_path);

  if (!d) 
    return 0;

  while ((dir = readdir(d)))
  {
    size_t n = strlen(dir->d_name);
    if (dir->d_type == DT_REG && dir->d_name[n-1] != '~')
      if (!f(dir->d_name)) return 0;
  }

  closedir(d);

  return 1;
}

int append_to_deps(char *fname)
{
  da_append(&deps, fname);
  return 1;
}

int main(int argc, char** argv)
{
  NOB_GO_REBUILD_URSELF(argc, argv);

  if (!mkdir_if_not_exists(BINDIR)) return 1;
  if (!mkdir_if_not_exists(ODIR)) return 1;

  da_append(&deps, "nob.c");

  function_on_all_files_in_directory(IDIR, append_to_deps);

  return 0;
}

int c_to_o(const char* const name)
{
  cmd_append(&cmd, "gcc");
  cmd_append(&cmd, temp_sprintf(SRCDIR "%s.c", name));
  cmd_append(&cmd, "-o", temp_sprintf(ODIR "%s.o", name));
  cc_flags();
  l_flags();

  if (!cmd_run(&cmd)) return 0;

  return 1;
}

#define NOB_IMPLEMENTATION
#include "nob.h"
