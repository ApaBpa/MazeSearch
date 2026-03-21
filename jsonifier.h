#include "maze_solver.h"
#include "maze.h"
#include "stdio.h"
#include <sstream>
#include <vector>

std::string MazesToJSON(const std::vector<Maze>& mazes, const double generationTime);
std::string PathsToJSON(const std::vector<std::vector<Cell *>>& paths, double solvingTime);