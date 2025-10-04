CC=gcc
CFLAGS=-O2 -Wall -Wextra -std=c11
INCS=-I../genlib -I../verify_acc

all: gennum verbank

gennum: gennum.c ../genlib/libgen.a
	$(CC) $(CFLAGS) $(INCS) -o $@ gennum.c ../genlib/libgen.a

verbank: verbank.c
	$(CC) $(CFLAGS) $(INCS) -L../verify_acc -o $@ verbank.c -lverify

clean:
	rm -f gennum verbank
