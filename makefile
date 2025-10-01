CC=g++
CFLAGS=-O2 -std=c++11

.PHONY: all clean

all: numgen

numgen: numgen.cpp
	$(CC) $(CFLAGS) numgen.cpp -o numgen

clean:
	rm -f numgen
