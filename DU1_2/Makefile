CXX := g++
CXXFLAGS := -g -Wall -Wextra -pthread -std=c++11

SOURCES=$(wildcard *.cpp)
OBJS=$(SOURCES:%.cpp=%.o)
TARGETS=$(SOURCES:%.cpp=%)

.PHONY: all clean

all:$(TARGETS)

run: gennum 
	./gennum 1234567891 5


%.o: %.cpp
	$(CXX) $(CXXFLAGS)

clean:
	rm -rf $(TARGETS)
