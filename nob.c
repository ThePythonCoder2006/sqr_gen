#include <complex.h>
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
#define MDIR SRCDIR "main/"

#define TRGT BINDIR "main"

Cmd cmd = {0};
File_Paths deps = {0};
bool debug = false;
bool no_gui = false;
bool flush_tables = true;
int64_t count = -1;
bool unit = false;
bool tests = false;
bool run = false;

void usage(FILE* stream)
{
  fprintf(stream, "Usage: ./nob [OPTIONS] [TYPE] [COUNT]\n");
  fprintf(stream, "OPTIONS:\n");
  flag_print_options(stream);
  fprintf(stream, "TYPE:\n"
                  "* leave empty for release\n"
                  "* db: build with debug symbols (alias for -debug)\n"
      );
  fprintf(stream, "COUNT: integer: number of tuples to find\n");

  return;
}

char* get_trgt(const char* const name, const char* const prefix, const char* const suffix, const char* const ext)
{
  char* ret = calloc(256, sizeof(char));
  snprintf(ret, 255,"%s%s%s%s", name,
      debug ? temp_sprintf("%sdb%s", prefix, suffix) : "",
      no_gui ? temp_sprintf("%sno-gui%s", prefix, suffix) : "",
      ext);
  return ret;
}

int cc_flags(Cmd* c)
{
  cmd_append(c, "-Wall");
  cmd_append(c, "-Wextra");
  cmd_append(c, "-pedantic");

  if (debug)
    cmd_append(c, "-ggdb", "-O0");
  else
    cmd_append(c, "-O3");
  if (no_gui)
    cmd_append(c, "-D__NO_GUI__");
  if (flush_tables)
    cmd_append(c, "-D__TABLE_FLUSHING__");

  return 1;
}

int i_flags(Cmd* c)
{
  cmd_append(c, "-pthread");
  cmd_append(c, "-I" IDIR);
  cmd_append(c, "-I.");

  return 1;
}


int l_flags(Cmd* c)
{
  cmd_append(c, "-lm");
  cmd_append(c, "-lcurses");
  cmd_append(c, "-pthread");

  return 1;
}

bool deps_append_files_in_dir(Nob_Walk_Entry entry);
bool c_to_o(Nob_Walk_Entry entry);
bool o_to_elf(Nob_Walk_Entry entry);
bool build_main(const char* const name);

char* odir;

int main(int argc, char** argv)
{
  NOB_GO_REBUILD_URSELF(argc, argv);

  bool* help = flag_bool("help", false, "show this help on stdout");
  flag_bool_var(&debug, "debug", false, "enables debug build");
  flag_bool_var(&no_gui, "no-gui", false, "disables the curses based GUI");
  flag_bool_var(&run, "run", false, "executes the build");
  flag_bool_var(&flush_tables, "flush-tables", true, "enables \"experimental\" table flushing behaviour when no more rels have been found in a long time");
  flag_bool_var(&unit, "unit", false, "builds and runs unit tests");
  flag_bool_var(&tests, "test", false, "run tests");
  bool* use_multithreaded = flag_bool("mt", false, "use multithreaded for latin squares search");
  size_t* threads = flag_size("threads", 0, "number of threads");
  bool* new_taxicabs = flag_bool("new-taxicabs", false, "searches for new taxicabs, with expected number of diagonals min_proba");
  double* min_proba = flag_double("min_proba", 0, "minimum expected number of diagonals");

  if (!flag_parse(argc, argv))
  {
    usage(stderr);
    flag_print_error(stderr);
    exit(1);
  }

  argc = flag_rest_argc();
  argv = flag_rest_argv();

  if (argc > 0)
  {
    const char* arg = shift(argv, argc);

    if (strcmp(arg, "db") == 0)
      debug = true;

    // arg MUST be a number
    count = atoi(arg);
  }

  if (*help)
  {
    usage(stdout);
    exit(0);
  }

  odir = get_trgt(ODIR, "",  "/", "");

  if (!mkdir_if_not_exists(BINDIR)) return 1;
  if (!mkdir_if_not_exists(odir)) return 1;
  if (!mkdir_if_not_exists("output")) return 1;

  da_append(&deps, "nob.c");
  walk_dir(IDIR, deps_append_files_in_dir);

  if (!walk_dir(SRCDIR, c_to_o)) return 1;

  /*
   * Now the c files have been converted to o and all subsequent build depend on them:
   *   add all c files to deps
   */
  if (!walk_dir(SRCDIR, deps_append_files_in_dir)) return 1;

  if (!build_main("main")) return 1;
  if (!build_main("viewer")) return 1;

  if (unit)
  {
    cmd_append(&cmd, "gcc");
    cmd_append(&cmd, "src/unit/test.c");
    if (!walk_dir(odir, o_to_elf)) return 1;
    cmd_append(&cmd, "-o", BINDIR"unit_test");
    cc_flags(&cmd);
    i_flags(&cmd);
    l_flags(&cmd);
    cmd_append(&cmd, "-lgmp");

    if(!cmd_run(&cmd)) return 1;

    cmd_append(&cmd, BINDIR"unit_test");

    if(!cmd_run(&cmd)) return 1;

    return 0;
  }

  if (!run) return 0;

  if (debug)
    cmd_append(&cmd, "gdb");
  cmd_append(&cmd, /* */"");

  if (count > 0)
    cmd_append(&cmd, temp_sprintf("%"PRId64, count));
  if (*new_taxicabs)
    cmd_append(&cmd, "-n", temp_sprintf("%lf", *min_proba));
  if (*use_multithreaded)
    cmd_append(&cmd, "-mt","-t", temp_sprintf("%zu", *threads));
  if (!cmd_run(&cmd)) return 1;

  return 0;
}

bool deps_append_files_in_dir(Nob_Walk_Entry entry)
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

  (void) cc_flags(&cmd);
  (void) i_flags(&cmd);

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

  if (!nob_sv_end_with(nob_sv_from_cstr(entry.path), "main.o"))
    cmd_append(&cmd, strdup(entry.path));

  return true;
}

bool build_main(const char* const name)
{
  da_append(&deps, temp_sprintf(MDIR"%s.c", name));
  if (needs_rebuild(get_trgt(temp_sprintf(MDIR"%s", name),
                   "-", "", ".o"), deps.items, deps.count))
  {
    // main -> .o
    cmd_append(&cmd, "gcc");
    cmd_append(&cmd, temp_sprintf(MDIR"%s.c", name));
    cmd_append(&cmd, "-c");
    cmd_append(&cmd, "-o",
        get_trgt(temp_sprintf(MDIR"%s", name),
                     "-", "", ".o"));
    cc_flags(&cmd);
    i_flags(&cmd);
    if (!cmd_run(&cmd)) return false;
  }

  if (needs_rebuild(get_trgt(temp_sprintf(BINDIR"%s", name),
                   "-", "", ""  ), deps.items, deps.count))
  {
    // .os -> elf
    cmd_append(&cmd, "gcc");
    if (!walk_dir(odir, o_to_elf)) return false;
    cmd_append(&cmd,
        get_trgt(temp_sprintf(MDIR"%s", name),
                     "-", "", ".o"));
    cmd_append(&cmd, "-o",
        get_trgt(temp_sprintf(BINDIR"%s", name),
                     "-", "", ""  ));
    cc_flags(&cmd);
    l_flags(&cmd);
    cmd_append(&cmd, "-lgmp");

    if (!cmd_run(&cmd)) return false;
    nob_log(INFO, "%s build successfully", name);
  }
  else
    nob_log(INFO, "Nothing to be done for %s", name);

  da_remove_unordered(&deps, deps.count - 1);

  return true;
}

#define NOB_IMPLEMENTATION
#include "nob.h"
#define FLAG_IMPLEMENTATION
#include "flag.h"
