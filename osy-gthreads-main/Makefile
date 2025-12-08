CFLAGS = -g -Wall

.PHONY: clean all

all: gttest heater

gttest: gthr.c main.c gtswtch.o gthr.h Makefile
	$(CC) $(CFLAGS) $(filter %.c %.o, $^) -o $@

heater: gthr.c heater.c gtswtch.o gthr.h Makefile
	$(CC) $(CFLAGS) $(filter %.c %.o, $^) -o $@

.S.o:
	as -o $@ $^

clean:
	rm -f *.o gttest heater
