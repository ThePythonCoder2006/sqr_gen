# sqr_gen

Generating magic squares of powers in c using the [taxicab method](https://wismuth.com/magic/squares-of-nth-powers.html).

By default tries to use the full taxicab method which includes the taxicab permutation. I have not been able to make this method work under the current implementation with my current hardware. One can produce square (whose size is sufficiently big) by using the O(mu) enabled by the `-no-taxi-method` flag of `main` and providing reasonable `-sum` parameter.

Well optimized taxicabs for the taxicab method are provided (and used if `-new-taxi` is not enabled and sizes are correct) in `know`

Results of each run are compiled in binary format in `output/[method type]-[DDMMYYYY]-[time]` as well as some text information. The binary files may be read using `bin/viewer`, which is build by `nob`

# Quick start

## Recommended build tool: `nob`

For more info see [nob.h](github.com/tsoding/nob.h) and [flag.h](github.com/tsoding/flag.h).

Bootstrap nob (only the first ever time)
```(shell)
gcc nob.c -o nob
```

Build with:
```
./nob [OPTIONS] -- [ARGS]
```
Where:
OPTIONS:
* `-help`:   show help on stdout
* `-debug`:  enables debug build
* `-no-gui`: disables the curses based GUI
* `-run`:    executes the build
* `-unit`:   builds and runs unit tests

ARGS: arguments for main:
* `-mt`:            use multithreaded search for the latin square enumeration
* `-threads <int>`: the number of threads to use for the latin square enumeration
        Default:  4
* `-new-taxi`:      find new taxicabs satifiying the condition
* `-p <double>`:    the minimal number of expected solutions from the taxicabs
        Default:  0.000010
* `-sum <int>`:     the maximal magic sum of the pair of taixcabs
        Default:  18446744073709551615
* `-regen`:         regenerate the list of all latin squares
* -no-taxi-method        wether to use the taxicab method or not
* `-help`:          show help message on stdout
* `-r <int>`:       value of r
        Default:  3
* `-s <int>`:       value of s
        Default:  4
* `-d <int>`:       value of d
        Default:  2

Required sets: Number of compatible sets to find (default: 32)

Usage of `viewer`:
```(shell)
 ./bin/viewer [OPTIONS] [PATH]
```

Options:
* `-help`:                           print this help on stdout
* `-sqr <str>`:                      to display a square of powers. takes M_name as value
        Default:                     sq
* `-taxi <str> ... -taxi <str> ...`: to display 2 taxicabs. Write the files names as -taxi=a_name -taxi=b_name, and pass the base_file_name as path
* `-l <str>`:                        set to non-null to write the read matrix to a latex file of your chosing
        Default:

## Legacy build tool: `make`

`make` by defaults builds `main` and main only. To build `viewer` define the `NAME` variable to `viewer`. Setting `NAME=main` achieves nothing, as it is it's default value.

Compile and run with
```(shell)
make NAME=[name]
```

Build with debug symbols and no curses:
```(shell)
make db NAME=[name]
```

