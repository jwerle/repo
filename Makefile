
SRC = $(wildcard src/*.c)
SRC += $(wildcard deps/*.c)
SRC += deps/libgit2.dylib
OBJ = $(SRC:.c=.o)
PREFIX = /usr/local
BIN = repo
CFLAGS = -std=c99 -lm -I deps -I include

repo: $(SRC)
	$(CC) $^ main.c $(CFLAGS) -o $@

install:
	install $(BIN) $(PREFIX)/bin

uninstall:
	rm $(PREFIX/bin/$(BIN)

clean:
	rm ./repo repo-test
	rm -f $(BIN) $(OBJ)

test: $(filter-out src/main.c, $(SRC) test/repo.c)
	$(CC) $^ $(CFLAGS) -o repo-test
	@echo
	@repo-test

.PHONY: clean install uninstall test repo
