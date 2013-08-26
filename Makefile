
LIBGIT = deps/libgit2.dylib
SRC = $(wildcard src/*.c)
SRC += $(wildcard deps/*.c)
SRC += $(LIBGIT)
OBJ = $(SRC:.c=.o)
PREFIX = /usr/local
BIN = repo
CFLAGS = -std=c99 -lm -I deps -I include 

CMDS = ls

all: repo $(CMDS)

repo: 
	$(CC) $(SRC) main.c $(CFLAGS) -o $@

%: cmd/%.c
	$(CC) $(SRC) $< $(CFLAGS) -o repo-$@

install:
	install $(BIN) $(PREFIX)/bin

uninstall:
	rm $(PREFIX/bin/$(BIN)

clean:
	rm -f $(BIN) 
	rm -f $(filter-out $(LIBGIT), $(OBJ))
	rm -f repo-*

test: $(filter-out src/main.c, $(SRC) test/repo.c)
	$(CC) $^ $(CFLAGS) -o repo-test
	@echo
	@repo-test

.PHONY: clean install uninstall test repo cmds deps
