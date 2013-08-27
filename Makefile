
LIBGIT = libgit2/build/libgit2.dylib
SRC = $(wildcard src/*.c)
SRC += $(wildcard deps/*.c)
SRC += $(LIBGIT)
OBJ = $(SRC:.c=.o)
PREFIX = /usr/local
BIN = repo
CFLAGS = -std=c99 -lm -I deps -I include  -I libgit2/include

CMDS = ls clone

all: repo $(CMDS)

repo: git
	$(CC) $(SRC) main.c $(CFLAGS) -o $@

%: cmd/%.c
	$(CC) $(SRC) $< $(CFLAGS) -o repo-$@

git:
	rm -rf ./libgit2/build && mkdir ./libgit2/build
	cd ./libgit2/build && cmake .. && cmake --build .

install:
	install $(BIN) $(PREFIX)/bin

uninstall:
	rm $(PREFIX/bin/$(BIN)

clean:
	rm -f $(BIN) 
	rm -f $(filter-out $(LIBGIT), $(OBJ))
	rm -f repo-*
	rm -rf libgit2/build

test: $(filter-out src/main.c, $(SRC) test/repo.c)
	$(CC) $^ $(CFLAGS) -o repo-test
	@echo
	@repo-test

.PHONY: clean install uninstall test repo cmds deps git
