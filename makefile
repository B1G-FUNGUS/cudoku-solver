CC=gcc
TBX_DIR=lib/toolbox/src
CFLAGS=-lncurses -I$(TBX_DIR)
DEPS = $(TBX_DIR)/clargs.h
OBJ = cudoku.o clargs.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

cudoku: src/cudoku.c $(TBX_DIR)/clargs.c
	$(CC) -o $@ $^ $(CFLAGS)
