PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

SOURCES = $(wildcard *.c)
INCLUDES = $(wildcard *.h) version.h
OBJECTS = $(SOURCES:.c=.o)
DEPS = libxml-2.0

CFLAGS = -O3 -Wall -Wextra
CFLAGS += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_GNU_SOURCE
CFLAGS += $(shell pkg-config --cflags $(DEPS))
LDFLAGS = -lm $(shell pkg-config --libs $(DEPS))

# Check on what system we are running
SYSTEM = $(shell uname -s)
ifeq ($(SYSTEM),Linux)
    CFLAGS += -DSYS_LINUX
endif
ifeq ($(SYSTEM),Darwin)
    CFLAGS += -DSYS_MACOSX
endif
ifeq ($(SYSTEM),CYGWIN_NT-5.1)
    CFLAGS += -D__CYGWIN__
endif


# Targets
all: switchres

version.h: version.sh
	@echo "  GEN   $@"
	@$(shell sh version.sh)

%.o: %.c $(INCLUDES)
	@echo "  CC    $@"
	@$(CC) -c $(CFLAGS) $< -o $@

switchres: $(OBJECTS) $(INCLUDES)
	@echo "  LD    $@"
	@$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

install: switchres
	install -D -m0755 switchres $(BINDIR)/switchres

clean:
	rm -f *.o switchres version.h

.PHONY: all clean install
