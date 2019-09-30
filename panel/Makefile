BIN = panel

CFLAGS = -Os -std=gnu11 -Wall
LDLIBS = -lfcgi -ljzon -lctemplate -lconfig

.PHONY: all clean server lighttpd nginx

all: $(BIN)

clean:
	rm -f $(BIN)

server: $(BIN)
	@spawn-fcgi -n -s /tmp/panel.sock -- ./$(BIN)

lighttpd: $(BIN)
	@lighttpd -Df ./lighttpd.conf

nginx:
	@nginx -c `pwd`/nginx.conf
