#include "maze.h"

std::vector<Cell *> AStarSeq(Maze &maze);
std::vector<std::vector<Cell *>> SolveInterMaze(std::vector<Maze> &mazes);
std::vector<std::vector<Cell *>> SolveIntraMaze(std::vector<Maze> &mazes);
std::vector<std::vector<Cell *>> SolveOuterMaze(std::vector<Maze> &mazes);
std::vector<std::vector<Cell *>> SolveSeq(std::vector<Maze> &mazes);
std::vector<std::vector<Cell *>> SolveSelected(std::vector<Maze> &mazes, const std::string &mode);
std::vector<std::vector<Cell *>> SolveMPI(std::vector<Maze> &mazes);
std::vector<std::vector<Cell *>> SolveOMP(std::vector<Maze>& mazes);
bool PathEqual(const std::vector<Cell*>& a, const std::vector<Cell*>& b);
bool PathsEqual(const std::vector<std::vector<Cell*>>& a, const std::vector<std::vector<Cell*>>& b);