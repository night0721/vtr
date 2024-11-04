.POSIX:
.SUFFIXES:

VERSION = 1.0
TARGET = vtr
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

CFLAGS = -Os -march=native -mtune=native -pipe -s -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE

SRC = src/vtr.c
INCLUDE = include

$(TARGET): $(SRC)
	$(CC) $(SRC) -o $@ $(CFLAGS) -I$(INCLUDE) $(LDFLAGS)

dist:
	mkdir -p $(TARGET)-$(VERSION)
	cp -R README.md $(MANPAGE) $(TARGET) $(TARGET)-$(VERSION)
	tar -cf $(TARGET)-$(VERSION).tar $(TARGET)-$(VERSION)
	gzip $(TARGET)-$(VERSION).tar
	rm -rf $(TARGET)-$(VERSION)

install: $(TARGET)
	mkdir -p $(DESTDIR)$(BINDIR)
	cp -p $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	chmod 755 $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	rm $(DESTDIR)$(BINDIR)/$(TARGET)

clean:
	rm $(TARGET)

all: $(TARGET)

.PHONY: all dist install uninstall clean
