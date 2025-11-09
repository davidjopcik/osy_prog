# --- Makefile (Linux) ---

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2 -g -pthread
LDFLAGS  ?= -pthread -lrt

TARGET := socket_srv
SRC    := socket_srv.cpp
OBJ    := $(SRC:.cpp=.o)
DEPS   := $(OBJ:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) $(LDFLAGS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ) $(DEPS) image.img out.bin runme

.PHONY: all clean
-include $(DEPS)