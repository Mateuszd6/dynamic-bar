DEBUG = true
CC = cc
CFLAGS = -std=c99 -pedantic -O3 # -O0 -g3
CWARNINGS = -Wall -Wextra -Wshadow
CSANITIZERS = # -fsanitize=address,undefined
CPLATFORM = -D_POSIX_C_SOURCE=200809L

DESTDIR = /usr/local

.PHONY : all clean

all : dbar

clean :
	@-rm -f obj/*.o dbar

install : dbar
	mkdir -p $(DESTDIR)/bin
	cp -f dbar $(DESTDIR)/bin
	chmod 755 $(DESTDIR)/bin/dbar
	cp -f dbar-refresh $(DESTDIR)/bin
	chmod 755 $(DESTDIR)/bin/dbar-refresh
	cp -f dynamic-bar-run $(DESTDIR)/bin
	chmod 755 $(DESTDIR)/bin/dynamic-bar-run

uninstall:
	rm -f $(DESTDIR)/bin/st
	rm -f $(DESTDIR)$(MANPREFIX)/man1/st.1

dbar : $(shell ls *.c *.h 2>/dev/null)
	${CC} ${CFLAGS} ${CPLATFORM} ${CWARNINGS} ${CSANITIZERS} dbar.c -o dbar
