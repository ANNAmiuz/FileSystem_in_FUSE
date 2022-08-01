# Set you prefererred CFLAGS/compiler compiler here.
# Our github runner provides gcc-10 by default.
CC ?= cc
CFLAGS ?= -g -Wall -O2

# this target should build all executables for all tests
all:
	$(CC) $(CFLAGS) -o memfs memfs.c `pkg-config fuse --cflags --libs`

check: all
	$(MAKE) -C tests check
