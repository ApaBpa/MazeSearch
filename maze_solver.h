#include "maze.h"

std::vector<Cell *> AStar(Maze &maze);
std::vector<std::vector<Cell *>> SolveSeq(std::vector<Maze> &mazes);
std::vector<std::vector<Cell *>> SolveMPI(std::vector<Maze> &mazes);
std::vector<std::vector<Cell *>> SolveOMP(std::vector<Maze>& mazes);