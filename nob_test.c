/*
 * Nob build script for permut.c unit tests
 *
 * Bootstrap:
 *   gcc nob_test.c -o nob_test
 *
 * Build and run tests:
 *   ./nob_test
 */

#define NOB_STRIP_PREFIX
#include "nob.h"

#include <stdbool.h>

#define SRCDIR "src/"
#define ODIR SRCDIR "obj/test/"
#define IDIR "include"
#define BINDIR "bin/"

bool compile_source(const char *src_path, const char *obj_path, bool debug) {
    Cmd cmd = {0};

    cmd_append(&cmd, "gcc");
    cmd_append(&cmd, "-c");
    cmd_append(&cmd, "-I" IDIR);
    cmd_append(&cmd, "-I.");
    cmd_append(&cmd, "-Wall");
    cmd_append(&cmd, "-Wextra");

    if (debug) {
        cmd_append(&cmd, "-ggdb", "-O0");
    } else {
        cmd_append(&cmd, "-O2");
    }

    cmd_append(&cmd, "-o", obj_path);
    cmd_append(&cmd, src_path);

    if (!cmd_run_sync_and_reset(&cmd)) {
        return false;
    }

    return true;
}

int main(int argc, char **argv) {
    GO_REBUILD_URSELF(argc, argv);

    // Parse command line arguments
    bool debug = false;
    bool run = false;

    shift(argv, argc); // Skip program name

    while (argc > 0) {
        const char *arg = shift(argv, argc);
        if (strcmp(arg, "-debug") == 0 || strcmp(arg, "-d") == 0) {
            debug = true;
        } else if (strcmp(arg, "-run") == 0 || strcmp(arg, "-r") == 0) {
            run = true;
        } else if (strcmp(arg, "-help") == 0 || strcmp(arg, "-h") == 0) {
            printf("Usage: ./nob_test [OPTIONS]\n");
            printf("OPTIONS:\n");
            printf("  -debug, -d    Build with debug symbols\n");
            printf("  -run, -r      Run tests after building\n");
            printf("  -help, -h     Show this help message\n");
            return 0;
        }
    }

    const char* odir = temp_sprintf(ODIR"%s", debug ? "db/" : "");

    // Ensure directories exist
    if (!mkdir_if_not_exists(odir)) return 1;
    if (!mkdir_if_not_exists(BINDIR)) return 1;

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

    const char* objects[] = {
        temp_sprintf("%spermut.o", odir),
        temp_sprintf("%spow_m_sqr.o", odir),
        temp_sprintf("%staxicab.o", odir),
        temp_sprintf("%slatin_squares.o", odir),
        temp_sprintf("%sarithmetic.o", odir),
        temp_sprintf("%sprint_grid.o", odir),
        temp_sprintf("%stest_permut.o", odir)
    };

    size_t num_sources = sizeof(sources) / sizeof(sources[0]);

    // Compile each source file
    for (size_t i = 0; i < num_sources; ++i) {
        // Check if recompilation is needed
        bool rebuild = nob_needs_rebuild(objects[i], sources, num_sources);

        if (rebuild) {
            nob_log(INFO, "Compiling %s", sources[i]);
            if (!compile_source(sources[i], objects[i], debug)) {
                nob_log(ERROR, "Failed to compile %s", sources[i]);
                return 1;
            }
        } else {
            nob_log(INFO, "Up to date: %s", objects[i]);
        }
    }

    // Link all objects into the test executable
    bool needs_link = nob_needs_rebuild(test_exe, objects, sizeof(objects) / sizeof(objects[0]));

    if (needs_link) {
        nob_log(INFO, "Linking %s", test_exe);

        Cmd cmd = {0};
        cmd_append(&cmd, "gcc");

        if (debug) {
            cmd_append(&cmd, "-ggdb");
        }

        cmd_append(&cmd, "-o", test_exe);

        for (size_t i = 0; i < num_sources; ++i) {
            cmd_append(&cmd, objects[i]);
        }

        cmd_append(&cmd, "-lm");
        cmd_append(&cmd, "-lgmp");
        cmd_append(&cmd, "-lcurses");

        if (!cmd_run_sync_and_reset(&cmd)) {
            nob_log(ERROR, "Failed to link %s", test_exe);
            return 1;
        }
    } else {
        nob_log(INFO, "Up to date: %s", test_exe);
    }

    // Run tests if requested
    if (run) {
        nob_log(INFO, "Running tests...");
        Cmd cmd = {0};
        cmd_append(&cmd, test_exe);
        if (!cmd_run_sync_and_reset(&cmd)) {
            nob_log(ERROR, "Tests failed");
            return 1;
        }
    } else {
        nob_log(INFO, "Build complete. Run with: %s", test_exe);
        nob_log(INFO, "Or rebuild and run with: ./nob_test -run");
    }

    return 0;
}

#define NOB_IMPLEMENTATION
#include "nob.h"
