# Assumes Linux or WSL environment with g++ and contents of /dependencies installed.
CC      := g++
MPICC   := mpic++
CFLAGS  := -O2 -std=c++17 -fopenmp -I./dependencies
LDFLAGS := -lpthread

SRC := $(wildcard *.cpp)

# Target file names
SEQ_TARGET := maze_seq
OMP_TARGET := maze_omp
MPI_TARGET := maze_mpi

SEQ_OBJ := $(SRC:%.cpp=build/seq/%.o)
OMP_OBJ := $(SRC:%.cpp=build/omp/%.o)
MPI_OBJ := $(SRC:%.cpp=build/mpi/%.o)

.PHONY: all seq omp mpi clean

all: $(SEQ_TARGET) $(OMP_TARGET) $(MPI_TARGET)

# --- Compile rules ---
build/seq/%.o: %.cpp
	mkdir -p build/seq
	$(CC) $(CFLAGS) -c -o $@ $<

build/omp/%.o: %.cpp
	mkdir -p build/omp
	$(CC) $(CFLAGS) -DUSE_OMP -c -o $@ $<

build/mpi/%.o: %.cpp
	mkdir -p build/mpi
	$(MPICC) $(CFLAGS) -DUSE_MPI -c -o $@ $<

# --- Link rules ---
$(SEQ_TARGET): $(SEQ_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OMP_TARGET): $(OMP_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(MPI_TARGET): $(MPI_OBJ)
	$(MPICC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf build $(SEQ_TARGET) $(OMP_TARGET) $(MPI_TARGET)