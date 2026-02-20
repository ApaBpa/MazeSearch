#include "maze_solver.h"
#include "maze.h"
#include "omp.h"
#include "stdio.h"

#include <algorithm>
#include <unordered_map>
#include <vector>

std::vector<Cell *> FindNeighbours(const Maze &maze, const Cell *c){
    std::vector<Cell *> neighbours;

    for (const Passage &p : maze.passages)
    {
        if (p.c1 == c)
            neighbours.emplace_back(p.c2);
        else if (p.c2 == c)
            neighbours.emplace_back(p.c1);
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

void ReconstructPath(std::unordered_map<Cell*, Cell*> &parentOf, Cell *current, std::vector<Cell*> &path){
    while (current != nullptr){
        path.push_back(current);
        current = parentOf.count(current) ? parentOf[current] : nullptr;
    }
    std::reverse(path.begin(), path.end());
}

/// @brief A* search algorithm https://www.geeksforgeeks.org/dsa/a-search-algorithm/
/// @param maze 
/// @return Vector of Cell* in the order they are visited, starting with the start cell and ending with the finish cell. {} if no path is found.
std::vector<Cell *> AStar(Maze &maze){
    if (!maze.start || !maze.finish) return {};
    if (maze.start == maze.finish) return { maze.start };

    // Initialize the open and closed sets, and the gCost and fCost maps
    Cell *start = maze.start;
    Cell *finish = maze.finish;
    std::vector<Cell*> openSet;
    std::vector<Cell*> closedSet;

    std::unordered_map<Cell*, float> gCost;
    std::unordered_map<Cell*, float> fCost;
    std::unordered_map<Cell*, Cell*> parentOf;

    gCost[start] = 0;
    fCost[start] = Heuristic(start, finish);
    openSet.push_back(start);

    while (!openSet.empty()){
        // Find the cell in open set with lowest fCost
        Cell *current = openSet[0];
        for (Cell *cell : openSet){
            if (fCost[cell] < fCost[current]){
                current = cell;
            }
        }

        // If we reached the finish, reconstruct the path and return it
        if (current == finish){
            // Reconstruct path
            std::vector<Cell *> path;
            ReconstructPath(parentOf, current, path);
            return path;
        }

        // Move current from open set to closed set
        openSet.erase(std::remove(openSet.begin(), openSet.end(), current), openSet.end());
        closedSet.push_back(current);

        for(Cell *neighbour : FindNeighbours(maze, current)){
            if (std::find(closedSet.begin(), closedSet.end(), neighbour) != closedSet.end()){
                continue; // Ignore the neighbour which is already evaluated
            }

            float tentativeGCost = gCost[current] + ManhattanDistance(current, neighbour);
            bool inOpen = (std::find(openSet.begin(), openSet.end(), neighbour) != openSet.end());

            // Best path to neighbour found so far, record it
            if (!inOpen || tentativeGCost < gCost[neighbour]) {
                parentOf[neighbour] = current;
                gCost[neighbour] = tentativeGCost;
                fCost[neighbour] = tentativeGCost + Heuristic(neighbour, finish);
                if (!inOpen) openSet.push_back(neighbour);
            }
        }
    }
    
    return {}; // No path found
}


