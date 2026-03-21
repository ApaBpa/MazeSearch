#include "maze.h"

std::vector<Cell *> AStar(Maze &maze);
std::vector<std::vector<Cell *>> SolveSeq(std::vector<Maze> &mazes);
std::vector<std::vector<Cell *>> SolveMPI(std::vector<Maze> &mazes);
std::vector<std::vector<Cell *>> SolveOMP(std::vector<Maze>& mazes);
bool PathEqual(const std::vector<Cell*>& a, const std::vector<Cell*>& b);
bool PathsEqual(const std::vector<std::vector<Cell*>>& a, const std::vector<std::vector<Cell*>>& b);