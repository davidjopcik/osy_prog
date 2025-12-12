CFLAGS = -g -Wall

.PHONY: clean all

all: gttest fgtest

gttest: gthr.c main.c gtswtch.o gthr.h Makefile
	$(CC) $(CFLAGS) $(filter %.c %.o, $^) -o $@

fgtest: gthr.c fg.c gtswtch.o gthr.h Makefile
	$(CC) $(CFLAGS) $(filter %.c %.o, $^) -o $@

.S.o:
	as -o $@ $^

clean:
	rm -f *.o gttest fgtest