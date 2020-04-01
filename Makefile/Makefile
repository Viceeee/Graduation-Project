CC = gcc
CFLAGS = -I.
DEPS = SimpleFS.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

SimpleFS:SimpleFS.o Simplefunc.o
	$(CC) -o SimpleFS SimpleFS.o Simplefunc.o