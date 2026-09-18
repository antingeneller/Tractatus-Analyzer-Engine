# Tractatus Logico-Philosophicus -- analysis engine
#
#   make            -> tractatus-tty   (Arch Linux / POSIX TTY interface)
#   make gui        -> tractatus.exe   (Win32 GUI, cross-compiled with mingw-w64)
#   make all        -> both
#   make install    -> /usr/local/bin/tractatus-tty
#
# Arch packages needed:  base-devel  (and mingw-w64-gcc for the .exe)

CC      ?= cc
CFLAGS  ?= -O2 -std=c99 -Wall -Wextra -pedantic
MINGW   ?= x86_64-w64-mingw32-gcc
PREFIX  ?= /usr/local

CORE    = src/tractatus_data.c src/tractatus_engine.c

all: tractatus-tty

tractatus-tty: src/main_tty.c $(CORE) src/tractatus.h
	$(CC) $(CFLAGS) -o $@ src/main_tty.c $(CORE)

gui: tractatus.exe

tractatus.exe: src/main_gui.c $(CORE) src/tractatus.h
	$(MINGW) -O2 -std=c99 -mwindows -o $@ src/main_gui.c $(CORE) -lgdi32 -luser32

both: tractatus-tty tractatus.exe

install: tractatus-tty
	install -Dm755 tractatus-tty $(DESTDIR)$(PREFIX)/bin/tractatus-tty

clean:
	rm -f tractatus-tty tractatus.exe

.PHONY: all gui both install clean
