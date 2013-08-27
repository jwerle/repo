
OS := $(shell uname)

ifeq ($(OS), Darwin)
LIBGIT = libgit2/build/libgit2.dylib
else
LIBGIT = libgit2/build/libgit2.so.0
endif

SRC = $(wildcard src/*.c)
SRC += $(wildcard deps/*.c)
SRC += $(LIBGIT)
OBJ = $(SRC:.c=.o)
PREFIX = /usr/local
BIN = repo
CFLAGS = -std=c99 -lm -L./libgit2/build -Wl,-rpath=./libgit2/build -lgit2 -I deps -I include -I libgit2/include

CMDS = ls clone

all: repo $(CMDS)

repo: 
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
