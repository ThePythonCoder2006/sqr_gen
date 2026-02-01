#include <dirent.h>
#include <stdio.h>
#include <string.h>

#define NOB_STRIP_PREFIX
#include "nob.h"

#include "flag.h"

#define BINDIR "bin/"
#define SRCDIR "src/"
#define ODIR SRCDIR "obj/"
#define IDIR "include"
#define LDIR "libs"

#define TRGT BINDIR "main"

Cmd cmd = {0};
File_Paths deps = {0};
bool debug = false;
bool no_gui = false;

void usage(FILE* stream)
{
  fprintf(stream, "Usage: ./nob [OPTIONS]\n");
  fprintf(stream, "OPTIONS:\n");
  flag_print_options(stream);

  return;
}

int cc_flags(void)
{
  cmd_append(&cmd, "-Wall");
  cmd_append(&cmd, "-Wextra");
  cmd_append(&cmd, "-pedantic");

  if (debug)
    cmd_append(&cmd, "-ggdb", "-O0");
  else
    cmd_append(&cmd, "-O3");
  if (no_gui)
    cmd_append(&cmd, "-D__NO_GUI__");

  return 1;
}

int l_flags(void)
{
  cmd_append(&cmd, "-I" IDIR);
  cmd_append(&cmd, "-I.");

  cmd_append(&cmd, "-lm");
  cmd_append(&cmd, "-lcurses");
  cmd_append(&cmd, "-lgmp");

  return 1;
}

bool append_h_to_deps(Nob_Walk_Entry entry);
bool c_to_o(Nob_Walk_Entry entry);
bool o_to_elf(Nob_Walk_Entry entry);

char odir[256];
char trgt[256];

int main(int argc, char** argv)
{
  NOB_GO_REBUILD_URSELF(argc, argv);

  bool* help = flag_bool("help", false, "show this help on stdout");
  flag_bool_var(&debug, "debug", false, "enables debug build");
  flag_bool_var(&no_gui, "no-gui", false, "disables the curses based GUI");
  bool* run = flag_bool("run", false, "executes the build");

  if (!flag_parse(argc, argv))
  {
    usage(stderr);
    flag_print_error(stderr);
    exit(1);
  }

  argc = flag_rest_argc();
  argv = flag_rest_argv();

  while (argc > 0)
  {
    const char* arg = shift(argv, argc);

    if (strcmp(arg, "db") == 0)
      debug = true;
  }

  if (*help)
  {
    usage(stdout);
    exit(0);
  }

  snprintf(odir, 255, ODIR"%s%s", debug ? "db/" : "", no_gui ? "no_gui/":"");
  snprintf(trgt, 255, TRGT"%s%s", debug ? "_db" : "", no_gui ? "_no-gui" : "");

  if (!mkdir_if_not_exists(BINDIR)) return 1;
  if (!mkdir_if_not_exists(odir)) return 1;

  da_append(&deps, "nob.c");
  walk_dir(IDIR, append_h_to_deps);

  walk_dir(SRCDIR, c_to_o);
  cmd_append(&cmd, "gcc");
  walk_dir(odir, o_to_elf);
  cmd_append(&cmd, "-o", trgt);
  cc_flags(); 
  l_flags();

  if (!cmd_run(&cmd)) return 1;

  if (!*run) return 0;

  if (debug)
    cmd_append(&cmd, "gdb");

  cmd_append(&cmd, trgt);
  if (!cmd_run(&cmd)) return 1;

  return 0;
}

bool append_h_to_deps(Nob_Walk_Entry entry)
{
  if (entry.type == NOB_FILE_DIRECTORY)
  {
    if (entry.level > 0)
      *entry.action = NOB_WALK_SKIP;
    return true;
  }

  da_append(&deps, strdup(entry.path));

  return true;
}

bool c_to_o(Nob_Walk_Entry entry)
{
  if (entry.type == NOB_FILE_DIRECTORY)
  {
    if (entry.level > 0)
      *entry.action = NOB_WALK_SKIP;
    return true;
  }

  char buff[256] = {0};

  // extract just the file name
  char *s = (char *)entry.path;
  while (*s) ++s;
  --s; // walk back from the '\0'
  --s; // walk back from the 'c' file extension
  char *save = s;
  *s = '\0';
  while (*s != '/') --s;
  ++s; // walk forward from the '/'

  snprintf(buff, 255, "%s%s.o", odir, s);

  *save = '.'; // restore the file extension
  
  da_append(&deps, entry.path);

  if (!needs_rebuild(buff, deps.items, deps.count)) return true;

  da_remove_unordered(&deps, deps.count - 1);

  cmd_append(&cmd, "gcc");
  cmd_append(&cmd, entry.path);
  cmd_append(&cmd, "-c");

  cmd_append(&cmd, "-o", buff);
  

  cc_flags();
  l_flags();

  if (!cmd_run(&cmd)) return false;

  return true;
}

bool o_to_elf(Nob_Walk_Entry entry)
{
  if (entry.type == NOB_FILE_DIRECTORY)
  {
    if (entry.level > 0)
      *entry.action = NOB_WALK_SKIP;
    return true;
  }

  cmd_append(&cmd, strdup(entry.path));

  return true;
}

#define NOB_IMPLEMENTATION
#include "nob.h"
#define FLAG_IMPLEMENTATION
#include "flag.h"
