CXX := g++
CXXFLAGS := -g -pthread -std=c++11 -Wall
LDFLAGS := -pthread

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
  LDFLAGS += -lrt
endif

all: socket_srv

socket_srv: socket_srv.o
	$(CXX) socket_srv.o $(LDFLAGS) -o $@

socket_srv.o: socket_srv.cpp
	$(CXX) -c $(CXXFLAGS) socket_srv.cpp -o $@

clean:
	rm -f socket_srv socket_srv.o image.img out.bin runme socket_cl