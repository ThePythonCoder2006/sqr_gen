CC := gcc

SRCDIR := src
BINDIR := bin
ODIR := $(SRCDIR)/obj
IDIR := include .
LDIR := libs

CCFLAGS := -Wall -Wextra -O0# -pedantic
IFLAGS := $(addprefix -I, $(IDIR))
ifeq ($(OS),Windows_NT)
	LFLAGS += $(addprefix -L, $(LDIR))
else
	LFLAGS += -lm
endif
LFLAGS += -lgmp -lcurses
CFLAGS := $(CCFLAGS) $(IFLAGS)# -D__NO_GUI__
DBFLAGS := -ggdb# -D__NO_GUI__

SRC := $(wildcard $(SRCDIR)/*.c)
OFILES := $(SRC:$(SRCDIR)/%.c=$(ODIR)/%.o)
DB_OFILES := $(OFILES:%.o=%_db.o)
HFILES := $(wildcard $(firstword $(IDIR))/*.h)

TRGT := $(BINDIR)/main
TRGT_DB := $(TRGT:%=%_db)

.PHONY: all run db

all: run Makefile

run: $(TRGT) Makefile
	./$<

db: $(TRGT_DB) Makefile

$(TRGT): $(OFILES) | $(BINDIR) $(ODIR)
	$(CC) $^ -o $@ $(CFLAGS) $(LFLAGS)
$(TRGT_DB): $(DB_OFILES) | $(BINDIR) $(ODIR)
	$(CC) $^ -o $@ $(CFLAGS) $(DBFLAGS) $(LFLAGS)

$(ODIR)/%.o: $(SRCDIR)/%.c $(HFILES) Makefile | $(ODIR)
	$(CC) $< -c -o $@ $(CFLAGS)
$(ODIR)/%_db.o: $(SRCDIR)/%.c $(HFILES) Makefile | $(ODIR)
	$(CC) $< -c -o $@ $(CFLAGS) $(DBFLAGS)

$(BINDIR) $(ODIR):
	mkdir -p "$@"
