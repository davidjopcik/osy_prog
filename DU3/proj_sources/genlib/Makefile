CC=gcc
CFLAGS=-O2 -Wall -Wextra -std=c11
AR=ar
RANLIB=ranlib

all: libgen.a

libgen.a: gen.o
	$(AR) rcs $@ $^
	$(RANLIB) $@

gen.o: gen.c gen.h
	$(CC) $(CFLAGS) -c gen.c

clean:
	rm -f *.o libgen.a
