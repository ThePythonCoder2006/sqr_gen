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

int cc_flags(void)
{
  cmd_append(&cmd, "-Wall");
  cmd_append(&cmd, "-Wextra");
  cmd_append(&cmd, "-pedantic");
  cmd_append(&cmd, "-pthread");

  if (debug)
    cmd_append(&cmd, "-ggdb", "-O0");
  else
    cmd_append(&cmd, "-O3");
  if (no_gui)
    cmd_append(&cmd, "-D__NO_GUI__");
  if (flush_tables)
    cmd_append(&cmd, "-D__TABLE_FLUSHING__");

  return 1;
}

int l_flags(void)
{
  cmd_append(&cmd, "-I" IDIR);
  cmd_append(&cmd, "-I.");

  cmd_append(&cmd, "-lm");
  cmd_append(&cmd, "-lcurses");
  cmd_append(&cmd, "-lgmp");
  cmd_append(&cmd, "-pthread");

  return 1;
}

bool append_h_to_deps(Nob_Walk_Entry entry);
bool c_to_o(Nob_Walk_Entry entry);
bool o_to_elf(Nob_Walk_Entry entry);
bool run_tests(void);
bool build_and_run_mt_tests(bool use_valgrind, bool use_tsan);
bool build_mt_test_suite(bool use_tsan);
bool run_mt_test(const char *test_name, const char *test_exe, bool use_valgrind, bool use_tsan);

char odir[256];
char trgt[256];

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
  bool* mt_tests = flag_bool("mt-test", false, "build and run multithreaded unit tests");
  bool* use_multithreaded = flag_bool("mt", false, "use multithreaded for latin squares search");
  size_t* threads = flag_size("threads", 0, "number of threads");
  bool* new_taxicabs = flag_bool("new-taxicabs", false, "searches for new taxicabs, with expected number of diagonals min_proba");
  double* min_proba = flag_double("min_proba", 0, "minimum expected number of diagonals");
  bool* valgrind = flag_bool("valgrind", false, "run tests with valgrind memory checker");
  bool* tsan = flag_bool("tsan", false, "build and run with ThreadSanitizer");

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

  snprintf(odir, 255, ODIR"%s%s", debug ? "db/" : "", no_gui ? "no_gui/":"");
  snprintf(trgt, 255, TRGT"%s%s", debug ? "_db" : "", no_gui ? "_no-gui" : "");

  if (!mkdir_if_not_exists(BINDIR)) return 1;
  if (!mkdir_if_not_exists(odir)) return 1;
  if (!mkdir_if_not_exists("output")) return 1;

  da_append(&deps, "nob.c");
  walk_dir(IDIR, append_h_to_deps);

  // Handle multithreaded tests
  if (*mt_tests)
  {
    if (*tsan)
    {
      nob_log(INFO, "Building with ThreadSanitizer...");
    }

    if (!build_and_run_mt_tests(*valgrind, *tsan))
    {
      nob_log(ERROR, "Multithreaded tests failed");
      return 1;
    }
    return 0;
  }

  if (!walk_dir(SRCDIR, c_to_o)) return 1;
  cmd_append(&cmd, "gcc");
  if (!walk_dir(odir, o_to_elf)) return 1;
  cmd_append(&cmd, temp_sprintf("%smain.o", odir));
  cmd_append(&cmd, "-o", trgt);
  cc_flags();
  l_flags();

  if (!cmd_run(&cmd)) return 1;

  if (unit)
  {
    cmd_append(&cmd, "gcc");
    cmd_append(&cmd, "src/unit/test.c");
    if (!walk_dir(odir, o_to_elf)) return 1;
    cmd_append(&cmd, "-o", BINDIR"unit_test");
    cc_flags();
    l_flags();

    if(!cmd_run(&cmd)) return 1;

    cmd_append(&cmd, BINDIR"unit_test");

    if(!cmd_run(&cmd)) return 1;

    return 0;
  }

  if (tests)
  {
    if (!run_tests()) return 1;
    return 0;
  }

  if (!run) return 0;

  if (debug)
    cmd_append(&cmd, "gdb");

  cmd_append(&cmd, trgt);
  if (count > 0)
    cmd_append(&cmd, temp_sprintf("%"PRId64, count));
  if (*new_taxicabs)
    cmd_append(&cmd, "-n", temp_sprintf("%lf", *min_proba));
  if (*use_multithreaded)
    cmd_append(&cmd, "-mt","-t", temp_sprintf("%zu", *threads));
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

  if (!nob_sv_end_with(nob_sv_from_cstr(entry.path), "main.o"))
    cmd_append(&cmd, strdup(entry.path));

  return true;
}

bool run_tests()
{
  const char *test_exe = BINDIR "test_permut";

  // List of source files to compile
  const char *sources[] = {
    SRCDIR "permut.c",
    SRCDIR "pow_m_sqr.c",
    SRCDIR "taxicab.c",
    SRCDIR "latin_squares.c",
    SRCDIR "arithmetic.c",
    SRCDIR "print_grid.c",
    SRCDIR "unit/test_permut.c"
  };

  if (!mkdir_if_not_exists(temp_sprintf("%sunit", odir))) return false;

  const char* objects[] = {
    temp_sprintf("%spermut.o", odir),
    temp_sprintf("%spow_m_sqr.o", odir),
    temp_sprintf("%staxicab.o", odir),
    temp_sprintf("%slatin_squares.o", odir),
    temp_sprintf("%sarithmetic.o", odir),
    temp_sprintf("%sprint_grid.o", odir),
    temp_sprintf("%sunit/test_permut.o", odir)
  };

  size_t num_sources = sizeof(sources) / sizeof(sources[0]);

  // Compile each source file
  for (size_t i = 0; i < num_sources; ++i)
  {
    // Check if recompilation is needed
    bool rebuild = nob_needs_rebuild(objects[i], sources, num_sources);
    rebuild |= needs_rebuild(objects[i], deps.items, deps.count);

    if (rebuild)
    {
      nob_log(INFO, "Compiling %s", sources[i]);

      cmd_append(&cmd, "gcc");
      cmd_append(&cmd, sources[i]);
      cmd_append(&cmd, "-c");

      cmd_append(&cmd, "-o", objects[i]);

      cc_flags();
      l_flags();

      if (!cmd_run(&cmd)) return false;
    }
    else
      nob_log(INFO, "Up to date: %s", objects[i]);
  }

  // Link all objects into the test executable
  bool needs_link = nob_needs_rebuild(test_exe, objects, sizeof(objects) / sizeof(objects[0]));

  if (needs_link) {
    nob_log(INFO, "Linking %s", test_exe);

    cmd_append(&cmd, "gcc");

    if (!cc_flags()) return 0;

    cmd_append(&cmd, "-o", test_exe);

    for (size_t i = 0; i < num_sources; ++i)
      cmd_append(&cmd, objects[i]);

    if (!l_flags()) return 0;

    if (!cmd_run(&cmd)) return false;
  }
  else
    nob_log(INFO, "Up to date: %s", test_exe);

  // Run tests if requested
  if (run) {
      nob_log(INFO, "Running tests...");
      Cmd cmd = {0};
      cmd_append(&cmd, test_exe);
      if (!cmd_run(&cmd))
      {
          nob_log(ERROR, "Tests failed");
          return false;
      }
  }
  else
  {
      nob_log(INFO, "Build complete. Run with: %s", test_exe);
      nob_log(INFO, "Or rebuild and run with: ./nob -test -run");
  }

  return true;
}

bool build_and_run_mt_tests(bool use_valgrind, bool use_tsan)
{
  nob_log(INFO, "========================================");
  nob_log(INFO, "Building Multithreaded Test Suite");
  nob_log(INFO, "========================================");

  if (!build_mt_test_suite(use_tsan))
  {
    nob_log(ERROR, "Failed to build MT test suite");
    return false;
  }

  nob_log(INFO, "");
  nob_log(INFO, "========================================");
  nob_log(INFO, "Running Multithreaded Tests");
  nob_log(INFO, "========================================");

  bool all_passed = true;

  // Run test_latin_squares_mt
  if (!run_mt_test("Latin Squares MT", BINDIR "test_latin_squares_mt", use_valgrind, use_tsan))
  {
    all_passed = false;
  }

  nob_log(INFO, "");

  // Run test_taxicab_method_mt
  if (!run_mt_test("Taxicab Method MT", BINDIR "test_taxicab_method_mt", use_valgrind, use_tsan))
  {
    all_passed = false;
  }

  nob_log(INFO, "");
  nob_log(INFO, "========================================");
  if (all_passed)
  {
    nob_log(INFO, "ALL TESTS PASSED!");
  }
  else
  {
    nob_log(ERROR, "SOME TESTS FAILED");
  }
  nob_log(INFO, "========================================");

  return all_passed;
}

bool build_mt_test_suite(bool use_tsan)
{
  if (!mkdir_if_not_exists(temp_sprintf("%smt_tests", odir))) return false;

  // Define source files needed for MT tests
  typedef struct {
    const char *name;
    const char **sources;
    size_t num_sources;
    const char *output;
  } TestTarget;

  // Sources for test_latin_squares_mt
  static const char *latin_sources[] = {
    SRCDIR "find_latin_squares_mt.c",
    SRCDIR "find_latin_square.c",
    SRCDIR "latin_squares.c",
    SRCDIR "pow_m_sqr.c",
    SRCDIR "arithmetic.c",
    SRCDIR "taxicab.c",
    SRCDIR "permut.c",
    SRCDIR "print_grid.c",
    SRCDIR "test/test_latin_squares_mt.c"
  };

  // Sources for test_taxicab_method_mt
  static const char *taxicab_sources[] = {
    SRCDIR "taxicab_method_mt.c",
    SRCDIR "taxicab_method.c",
    SRCDIR "taxicab_method_common.c",
    SRCDIR "find_latin_squares_mt.c",
    SRCDIR "find_latin_square.c",
    SRCDIR "find_sets.c",
    SRCDIR "latin_squares.c",
    SRCDIR "pow_m_sqr.c",
    SRCDIR "arithmetic.c",
    SRCDIR "serialize.c",
    SRCDIR "taxicab.c",
    SRCDIR "permut.c",
    SRCDIR "print_grid.c",
    SRCDIR "test/test_taxicab_method_mt.c"
  };

  TestTarget targets[] = {
    {
      .name = "test_latin_squares_mt",
      .sources = latin_sources,
      .num_sources = sizeof(latin_sources) / sizeof(latin_sources[0]),
      .output = BINDIR "test_latin_squares_mt"
    },
    {
      .name = "test_taxicab_method_mt",
      .sources = taxicab_sources,
      .num_sources = sizeof(taxicab_sources) / sizeof(taxicab_sources[0]),
      .output = BINDIR "test_taxicab_method_mt"
    }
  };

  size_t num_targets = sizeof(targets) / sizeof(targets[0]);

  for (size_t t = 0; t < num_targets; ++t)
  {
    TestTarget *target = &targets[t];
    nob_log(INFO, "Building %s...", target->name);

    // Compile each source file
    const char *objects[32]; // Assuming max 32 sources
    for (size_t i = 0; i < target->num_sources; ++i)
    {
      // Generate object file path
      const char *src = target->sources[i];

      // Extract filename without extension
      const char *filename = src;
      const char *last_slash = strrchr(src, '/');
      if (last_slash) filename = last_slash + 1;

      char obj_name[256];
      snprintf(obj_name, sizeof(obj_name), "%smt_tests/%s", odir, filename);

      // Replace .c with .o
      char *dot = strrchr(obj_name, '.');
      if (dot) strcpy(dot, ".o");

      objects[i] = strdup(obj_name);

      // Check if recompilation is needed
      const char *dep_sources[] = {src};
      bool rebuild = nob_needs_rebuild(objects[i], dep_sources, 1);
      rebuild |= needs_rebuild(objects[i], deps.items, deps.count);

      if (rebuild)
      {
        nob_log(INFO, "  Compiling %s", src);

        Cmd compile_cmd = {0};
        cmd_append(&compile_cmd, "gcc");
        cmd_append(&compile_cmd, src);
        cmd_append(&compile_cmd, "-c");
        cmd_append(&compile_cmd, "-o", objects[i]);

        // Add flags
        cmd_append(&compile_cmd, "-Wall", "-Wextra", "-pedantic");
        cmd_append(&compile_cmd, "-pthread");
        cmd_append(&compile_cmd, "-I" IDIR, "-I.");

        if (use_tsan)
          cmd_append(&compile_cmd, "-fsanitize=thread");

        if (debug)
          cmd_append(&compile_cmd, "-ggdb", "-O0");
        else
          cmd_append(&compile_cmd, "-O2");

        if (no_gui)
          cmd_append(&compile_cmd, "-D__NO_GUI__");

        if (!cmd_run(&compile_cmd))
        {
          nob_log(ERROR, "Failed to compile %s", src);
          return false;
        }
      }
      else
        nob_log(INFO, "  Up to date: %s", objects[i]);
    }

    // Link the test executable
    bool needs_link = nob_needs_rebuild(target->output, objects, target->num_sources);

    if (needs_link)
    {
      nob_log(INFO, "  Linking %s", target->output);

      Cmd link_cmd = {0};
      cmd_append(&link_cmd, "gcc");

      if (use_tsan)
        cmd_append(&link_cmd, "-fsanitize=thread");

      cmd_append(&link_cmd, "-o", target->output);

      for (size_t i = 0; i < target->num_sources; ++i)
        cmd_append(&link_cmd, objects[i]);

      cmd_append(&link_cmd, "-pthread");
      cmd_append(&link_cmd, "-lm", "-lgmp");

      if (!no_gui)
        cmd_append(&link_cmd, "-lcurses");
      else
        cmd_append(&link_cmd, "-D__NO_GUI__");

      if (!cmd_run(&link_cmd))
      {
        nob_log(ERROR, "Failed to link %s", target->output);
        return false;
      }
    }
    else
      nob_log(INFO, "  Up to date: %s", target->output);

    // Free dynamically allocated object paths
    for (size_t i = 0; i < target->num_sources; ++i)
      free((void*)objects[i]);
  }

  return true;
}

bool run_mt_test(const char *test_name, const char *test_exe, bool use_valgrind, bool use_tsan)
{
  nob_log(INFO, "Running %s...", test_name);

  Cmd test_cmd = {0};

  if (use_valgrind && !use_tsan)
  {
    nob_log(INFO, "  With Valgrind memory checking");
    cmd_append(&test_cmd, "valgrind");
    cmd_append(&test_cmd, "--leak-check=full");
    cmd_append(&test_cmd, "--error-exitcode=1");
    cmd_append(&test_cmd, "--quiet");
  }

  cmd_append(&test_cmd, test_exe);

  bool result = cmd_run(&test_cmd);

  if (result)
  {
    nob_log(INFO, "  PASSED: %s", test_name);
  }
  else
  {
    nob_log(ERROR, "  FAILED: %s", test_name);
  }

  return result;
}

#define NOB_IMPLEMENTATION
#include "nob.h"
#define FLAG_IMPLEMENTATION
#include "flag.h"
