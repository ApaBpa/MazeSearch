#include "maze_solver.h"
#include "maze.h"
#include "omp.h"
#include "mpi.h"
#include "stdio.h"

#include <algorithm>
#include <unordered_map>
#include <vector>

std::vector<Cell*> FindNeighbours(const Maze &maze, const Cell c) {
    std::vector<Cell*> neighbours;

    for (const Passage &p : maze.passages) {
        if (p.c1 && *p.c1 == c && p.c2) {
            neighbours.emplace_back(p.c2);
        } else if (p.c2 && *p.c2 == c && p.c1) {
            neighbours.emplace_back(p.c1);
        }
    }
    return neighbours;
}

float Heuristic(const Cell *a, const Cell *b){
#ifdef USE_EUCLIDEAN
    return sqrt(pow(a->posX - b->posX, 2) + pow(a->posY - b->posY, 2));
#else
    return abs(a->posX - b->posX) + abs(a->posY - b->posY);
#endif
}

float EuclideanDistance(const Cell *a, const Cell *b){
    return sqrt(pow(a->posX - b->posX, 2) + pow(a->posY - b->posY, 2));
}

float ManhattanDistance(const Cell *a, const Cell *b){
    return abs(a->posX - b->posX) + abs(a->posY - b->posY);
}

void ReconstructPath(std::unordered_map<Cell*, Cell*> &parent_of, Cell *current, std::vector<Cell*> &path){
    while (current != nullptr){
        path.push_back(current);
        current = parent_of.count(current) ? parent_of[current] : nullptr;
    }
    std::reverse(path.begin(), path.end());
}

/// @brief A* search algorithm https://www.geeksforgeeks.org/dsa/a-search-algorithm/
/// @param maze 
/// @return Vector of Cell in the order they are visited, starting with the start cell and ending with the finish cell. {} if no path is found.
std::vector<Cell *> AStar(Maze &maze){
    if (!maze.start || !maze.finish) return {};
    if (maze.start == maze.finish) return { maze.start };

    // Initialize the open and closed sets, and the g_cost and f_cost maps
    Cell *start = maze.start;
    Cell *finish = maze.finish;
    std::vector<Cell*> open_set;
    std::vector<Cell*> closed_set;

    std::unordered_map<Cell*, float> g_cost;
    std::unordered_map<Cell*, float> f_cost;
    std::unordered_map<Cell*, Cell*> parent_of;

    g_cost[start] = 0;
    f_cost[start] = Heuristic(start, finish);
    open_set.push_back(start);

    while (!open_set.empty()){
        // Find the cell in open set with lowest f_cost
        Cell *current = open_set[0];
        for (Cell *cell : open_set){
            if (f_cost[cell] < f_cost[current]){
                current = cell;
            }
        }

        // If we reached the finish, reconstruct the path and return it
        if (current == finish){
            // Reconstruct path
            std::vector<Cell *> path;
            ReconstructPath(parent_of, current, path);
            return path;
        }

        // Move current from open set to closed set
        open_set.erase(std::remove(open_set.begin(), open_set.end(), current), open_set.end());
        closed_set.push_back(current);

        for(Cell* neighbour : FindNeighbours(maze, *current)){
            if (std::find(closed_set.begin(), closed_set.end(), neighbour) != closed_set.end()){
                continue; // Ignore the neighbour which is already evaluated
            }

            float tentative_g_cost = g_cost[current] + ManhattanDistance(current, neighbour);
            bool inOpen = (std::find(open_set.begin(), open_set.end(), neighbour) != open_set.end());

            // Best path to neighbour found so far, record it
            if (!inOpen || tentative_g_cost < g_cost[neighbour]) {
                parent_of[neighbour] = current;
                g_cost[neighbour] = tentative_g_cost;
                f_cost[neighbour] = tentative_g_cost + Heuristic(neighbour, finish);
                if (!inOpen) open_set.push_back(neighbour);
            }
        }
    }
    
    return {}; // No path found
}

#if not defined(USE_MPI) && not defined(USE_OMP)
/// @brief Solves mutiple mazes sequentially.
/// @param mazes 
/// @return Vector of paths, where each path is a vector of Cell representing the order of cells from start to finish for each maze.
std::vector<std::vector<Cell *>> SolveSeq(std::vector<Maze>& mazes){
    std::vector<std::vector<Cell *>> results;
    for (Maze &maze : mazes){
        results.push_back(AStar(maze));
    }
    return results;
}
#endif

#ifdef USE_MPI
/// @brief Solves mutiple mazes in parallel using MPI. Each process will solve a subset of all mazes. Expected speedup ~ number of cores on the CPU.
/// @param mazes 
/// @return Vector of paths, where each path is a vector of Cell representing the order of cells from start to finish for each maze. The order of paths corresponds to the order of mazes in the input vector.
std::vector<std::vector<Cell *>> SolveMPI(std::vector<Maze>& mazes){
    int my_rank, size;

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int num_mazes = mazes.size();
    int mazesPerProcess = (num_mazes + size - 1) / size;

    int startIndex = my_rank * mazesPerProcess;
    int endIndex = std::min(startIndex + mazesPerProcess, num_mazes);

    // Solve local mazes
    std::vector<std::vector<Cell *>> localResults;
    for (int i = startIndex; i < endIndex; i++){
        localResults.push_back(AStar(mazes[i]));
    }

    // Manual gather on node=0
    if (my_rank == 0){
        std::vector<std::vector<Cell *>> allResults = localResults;

        for (int source = 1; source < size; source++){
            int numPaths;
            MPI_Recv(&numPaths, 1, MPI_INT, source, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            for (int i = 0; i < numPaths; ++i) {
                int pathSize;
                MPI_Recv(&pathSize, 1, MPI_INT, source, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                std::vector<Cell *> path(pathSize);
                MPI_Recv(path.data(), pathSize * sizeof(Cell *), MPI_BYTE, source, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                allResults.push_back(path);
            }
        }

        return allResults;
    } 
    else {
        int numPaths = localResults.size();
        MPI_Send(&numPaths, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        for (const std::vector<Cell *>& path : localResults) {
            int pathSize = path.size();
            MPI_Send(&pathSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send(path.data(), pathSize * sizeof(Cell *), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
        }

        return {};
    }
}
#endif