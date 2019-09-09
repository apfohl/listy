BIN = panel

CFLAGS = -Os -std=gnu11 -Wall
LDLIBS = -lfcgi -ljzon

.PHONY: all clean server

all: $(BIN)

clean:
	rm -f $(BIN)

server:
	@lighttpd -Df ./lighttpd.conf
