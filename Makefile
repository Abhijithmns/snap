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

.PHONY: all clean install uninstall clangd
