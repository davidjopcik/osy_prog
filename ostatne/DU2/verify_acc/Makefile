CC=gcc
CFLAGS=-O2 -Wall -Wextra -std=c11 -fPIC

all: libverify.so

libverify.so: verify_acc.o
	$(CC) -shared -o $@ $^

verify_acc.o: verify_acc.c verify.h
	$(CC) $(CFLAGS) -c verify_acc.c

clean:
	rm -f *.o libverify.so