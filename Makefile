CC := gcc

SRCDIR := ./src
BINDIR := ./bin
ODIR := $(SRCDIR)/obj
IDIR := ./include
LDIR := ./libs

CCFLAGS := -Wall -Wextra -pedantic -ggdb
IFLAGS := -I$(IDIR)
LFLAGS := -L$(LDIR) -lgmp -lcurses
CFLAGS := $(CCFLAGS) $(IFLAGS)# -D__DEBUG__

SRC := $(wildcard $(SRCDIR)/*.c)
OFILES := $(SRC:$(SRCDIR)/%.c=$(ODIR)/%.o)
HFILES := $(wildcard $(IDIR)/*.h)

TRGT := $(BINDIR)/main.exe

.PHONY: all run db

all: run Makefile

run: $(TRGT) $(HFILES) Makefile
	./$<

db: $(TRGT) Makefile
	gdb ./$<

$(TRGT): $(OFILES) | $(BINDIR) $(ODIR)
	$(CC) $^ -o $@ $(CFLAGS) $(LFLAGS)

$(ODIR)/%.o: $(SRCDIR)/%.c $(HFILES) Makefile | $(ODIR)
	$(CC) $< -c -o $@ $(CFLAGS)

$(BINDIR) $(ODIR):
	mkdir "$@"

# curses: $(BINDIR)/test_curses
#   ./$^

# $(BINDIR)/test_curses: test_curses.c
#   $(CC) $< -o $@ $(CFLAGS) 

# GPT: GPT.c $(SRCDIR)/pow_m_sqr.c $(SRCDIR)/latin_squares.c
#  	$(CC) $^ -o $@ $(CFLAGS)