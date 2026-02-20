# Assumes Linux or WSL environment with g++ and contents of /dependencies installed.

CC      := g++
CFLAGS  := -O2 -std=c++17 -fopenmp -I./dependencies
LDFLAGS := -fopenmp -lpthread
SRC     := $(wildcard *.cpp)
OBJ     := $(SRC:.cpp=.o)
TARGET  := maze

.PHONY: all run clean
all: $(TARGET)
$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)
%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@
run: $(TARGET)
	./$(TARGET)
clean:
	rm -f $(OBJ) $(TARGET)