CC=gcc
CFLAGS=-O2 -Wall -Wextra -std=c11
LDFLAGS=
INCS=-I../genlib -I../verify_acc

all: gennum verbank

gennum: gennum.c ../genlib/libgen.a
	$(CC) $(CFLAGS) $(INCS) -o $@ gennum.c ../genlib/libgen.a

# POZOR: -L nastav na adresár s libverify.so (využijeme LD_LIBRARY_PATH pri behu)
verbank: verbank.c
	$(CC) $(CFLAGS) $(INCS) -L../verify_acc -o $@ verbank.c -lverify

clean:
	rm -f gennum verbank