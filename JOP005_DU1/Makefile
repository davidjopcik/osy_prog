CC = gcc
CFLAGS = -O2 -Wall

all: gennum

gen.o: gen.c gen.h
	$(CC) $(CFLAGS) -c gen.c -o gen.o

libgen.a: gen.o
	ar rcs libgen.a gen.o

gennum: gennum.c gen.h libgen.a
	$(CC) $(CFLAGS) gennum.c -L. -lgen -o gennum

clean:
	rm -f *.o libgen.a gennum