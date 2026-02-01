# sqr_gen

Generating magic squares of powers in c using the [taxicab method](https://wismuth.com/magic/squares-of-nth-powers.html).

# Quick start

## Recommended build tool: `nob`

For more info see [nob.h](github.com/tsoding/nob.h) and [flag.h](github.com/tsoding/flag.h).

Bootstrap nob (only the first ever time)
```(shell)
gcc nob.c -o nob
```

Build with:
```
./nob [OPTIONS] [TYPE]
```
Where:
OPTIONS:
* -help: show help to stdout
* -debug: enables debug symbols
* -no-gui: disables the curses based GUI
* -run: executes (or gdbs) the build

TYPE:
* empty: release build
* db: enables debug symbols (alternative to the `-debug` flag)

## Legacy build tool: `make`

Compile and run with
```(shell)
make
```

Build with debug symbols and no curses:
```(shell)
make db
```
