CFLAGS +=  -I./src/ -I./deps/rlite/src/ -I./deps/linenoise/

PREFIX?=/usr/local
INSTALL_BIN=$(PREFIX)/bin
INSTALL=install

all: rlite-cli
	echo

rlite-cli: rlite linenoise src/rlite-cli.o src/sds.o
	$(CC) -o rlite-cli ./deps/rlite/src/libhirlite.a ./deps/rlite/deps/lua/src/liblua.a ./deps/linenoise/linenoise.o rlite-cli.o sds.o $(CFLAGS)

.c.o:
	$(CC) $(ARCH) $(DEBUG) $(CFLAGS) -c $<

linenoise:
	cd deps/linenoise ; $(MAKE)

rlite:
	cd deps/rlite ; $(MAKE) all

clean:
	cd deps/linenoise ; $(MAKE) clean
	cd deps/rlite ; $(MAKE) clean
	rm -f *.o rlite-cli

install: rlite-cli
	$(INSTALL) rlite-cli $(INSTALL_BIN)

uninstall:
	rm -f $(INSTALL_BIN)/rlite-cli
