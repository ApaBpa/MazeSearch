#pragma once
#include "maze.h"
#include <vector>
#include <random>

class MazeGenerator{
public:
    MazeGenerator();
    void GeneratePrim(Maze& maze);

private:
    std::mt19937 rng;
    void AddFrontierNeighbours(Maze& maze, std::vector<Cell*>& frontier, Cell* c);
};