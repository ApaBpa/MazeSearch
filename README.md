# *Work in progress
Currently, only the sequential and MPI solver is implemented

## Dependencies
* httplib.h [https://github.com/yhirose/cpp-httplibt] should exist in a folder dependencies/
* C++ compiler (g++)
* mpic++ or equivalent
* OpenMP

## Build (Assuming Linux or WSL)
`git pull`

Install dependencies

`make clean && make maze_mpi`

## Run (Assuming Linux or WSL)
`MPI_NP=4 mpirun -np $MPI_NP ./maze_mpi`

..should produce this output:

"Server running at http://localhost:8080"

If you want to test different process counts, change `MPI_NP` (for example `MPI_NP=8`).

## Usage
First, we need to generate a maze by clicking the 'generate' button.
The maze generator will sequentially generate the specified number of mazes with the given size (TODO) using Prim's minimum spanning tree algorithm.

Once the maze(s) are generated, we can solve them by clicking the 'solve' button. This will solve all the generated mazes using the A* algorithm. If we built using MPI, it will solve multiple mazes in parallel by initializing one thread per maze, and then reducing the result into a vector of paths from start to finish of each maze.