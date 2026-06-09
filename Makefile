# tools
CC = gcc

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# libs
# remove the Xinerama parts if you don't want Xinerama support
LIBS = -lX11 -lXinerama

# flags
CPPFLAGS = -DXINERAMA
# CFLAGS = -std=c99 -Wall -Wextra -O0 -g -fdiagnostics-color=always # debug
CFLAGS = -std=c99 -Wall -Wextra -Os -fdiagnostics-color=always
LDFLAGS = ${LIBS}

# files
SRC = snap.c

all: snap

# rules
snap:
	${CC} ${CPPFLAGS} ${CFLAGS} ${SRC} -o snap ${LDFLAGS}

clean:
	rm -rf snap

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f snap $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/snap

uninstall: 
	rm -f $(DESTDIR)$(PREFIX)/bin/snap

help:
	@echo "snap - A minimal X11 screenshot utility"
	@echo "======================================="
	@echo ""
	@echo "Building:"
	@echo "  make - Compile snap"
	@echo "  make clean - Remove build artifacts"
	@echo "  make install - Install snap binary"
	@echo "  make uninstall - Uninstall snap binary"



.PHONY: all clean install uninstall help
