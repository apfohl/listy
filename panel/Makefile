BIN = panel

CFLAGS = -g -std=gnu11 -Wall
LDLIBS = -lfcgi -ljzon -lctemplate

.PHONY: all clean server

all: $(BIN)

clean:
	rm -f $(BIN)

server: $(BIN)
	@lighttpd -Df ./lighttpd.conf
