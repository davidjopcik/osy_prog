CC=gcc
CFLAGS=-O2 -Wall -Wextra -std=c11 -fPIC

all: libverify.so

libverify.so: verify_rc.o
	$(CC) -shared -o $@ $^

verify_rc.o: verify_rc.c verify.h
	$(CC) $(CFLAGS) -c verify_rc.c

clean:
	rm -f *.o libverify.so