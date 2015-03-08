LIBS=-lm
CFLAGS +=  $(LIBS) -I./src/ -I./deps/rlite/src/ -I./deps/linenoise/

PREFIX?=/usr/local
INSTALL_BIN=$(PREFIX)/bin
INSTALL=install
DEPS=./deps/rlite/src/libhirlite.a ./deps/rlite/deps/lua/src/liblua.a

all: rlite-cli
	echo

rlite-cli: rlite linenoise src/rlite-cli.o src/sds.o
	$(CC) -o rlite-cli ./deps/linenoise/linenoise.o rlite-cli.o sds.o $(DEPS) $(CFLAGS)

rdb2rld: rlite src/rdb2rld.o src/rdb2rldf.o src/sds.o
	$(CC) -o rdb2rld rdb2rldf.o rdb2rld.o sds.o $(DEPS) $(CFLAGS)

.c.o:
	$(CC) $(ARCH) $(DEBUG) $(CFLAGS) -c $<

linenoise:
	cd deps/linenoise ; $(MAKE)

rlite:
	cd deps/rlite ; $(MAKE) all

test: tests/rdb2rld-test.o rlite src/rdb2rldf.o src/sds.o
	$(CC) -o rdb2rld-test rdb2rldf.o rdb2rld-test.o sds.o $(DEPS) $(CFLAGS)
	./rdb2rld-test

clean:
	cd deps/linenoise ; $(MAKE) clean
	cd deps/rlite ; $(MAKE) clean
	rm -f *.o rlite-cli rdb2rld

install: rlite-cli
	$(INSTALL) rlite-cli $(INSTALL_BIN)

uninstall:
	rm -f $(INSTALL_BIN)/rlite-cli
