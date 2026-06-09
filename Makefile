CC := gcc

SRCDIR := src
BINDIR := bin
ODIR := $(SRCDIR)/obj
IDIR := include .
LDIR := libs
MDIR := $(SRCDIR)/main

CCFLAGS := -Wall -Wextra -O0# -pedantic
IFLAGS := $(addprefix -I, $(IDIR))
LFLAGS += -lm
LFLAGS += -lgmp -lcurses
CFLAGS := $(CCFLAGS) $(IFLAGS)# -D__NO_GUI__
DBFLAGS := -ggdb# -D__NO_GUI__

SRC := $(wildcard $(SRCDIR)/*.c)
OFILES := $(SRC:$(SRCDIR)/%.c=$(ODIR)/%.o)
DB_OFILES := $(OFILES:$(ODIR)/%.o=$(ODIR)/db/%.o)
HFILES := $(wildcard $(firstword $(IDIR))/*.h)

NAME := main
TRGT := $(BINDIR)/$(NAME)
TRGT_DB := $(TRGT)-db

.PHONY: all run db

all: run Makefile

run: $(TRGT) Makefile
	./$<

db: $(TRGT_DB) Makefile

$(TRGT): $(OFILES) $(MDIR)/$(NAME).c | $(BINDIR) $(ODIR)
	$(CC) $^ -o $@ $(CFLAGS) $(LFLAGS)
$(TRGT_DB): $(DB_OFILES) $(MDIR)/$(NAME).c | $(BINDIR) $(ODIR)
	$(CC) $^ -o $@ $(CFLAGS) $(DBFLAGS) $(LFLAGS)

$(ODIR)/%.o: $(SRCDIR)/%.c $(HFILES) Makefile | $(ODIR)
	$(CC) $< -c -o $@ $(CFLAGS)
$(ODIR)/%_db.o: $(SRCDIR)/%.c $(HFILES) Makefile | $(ODIR)
	$(CC) $< -c -o $@ $(CFLAGS) $(DBFLAGS)

$(BINDIR) $(ODIR) output $(MDIR):
	mkdir -p "$@"
