CXX := g++
CXXFLAGS := -g -pthread -std=c++11 -Wall
LDFLAGS := -pthread

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
  LDFLAGS += -lrt
endif

.PHONY: all clean

all: socket_srv socket_cl

socket_srv: socket_srv.o
	$(CXX) socket_srv.o $(LDFLAGS) -o $@

socket_cl: socket_cl.o
	$(CXX) socket_cl.o $(LDFLAGS) -o $@

socket_srv.o: socket_srv.cpp
	$(CXX) -c $(CXXFLAGS) socket_srv.cpp -o $@

socket_cl.o: socket_cl.cpp
	$(CXX) -c $(CXXFLAGS) socket_cl.cpp -o $@

clean:
	rm -f socket_srv socket_srv.o socket_cl socket_cl.o image.img out.bin runme runme.xz