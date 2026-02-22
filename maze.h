#pragma once
#include <vector>
#include <random>

/// @brief A Cell is any position in the maze.
/// By default, there are walls between every Cell
class Cell {
public:
    int posX, posY;

    bool inMaze = false;
    bool inFrontier = false;

    bool operator==(const Cell& other) const {
        return posX == other.posX && posY == other.posY;
    }

    Cell(int posX, int posY);
};

/// @brief A Passage is between two cells where there are no walls
class Passage {
public:
    Cell* c1;
    Cell* c2;

    Passage();
    Passage(Cell* c1, Cell* c2);
};

class Maze{
public:
    Maze(int sizeX, int sizeY);
    
    int width() const {return sizeX;}
    int height() const {return sizeY;}
    
    std::vector<std::vector<Cell>> cells;   // All cells of the maze, in a grid
    std::vector<Passage> passages;

    void MakePassage(Cell* a, Cell* b);
    void MakeGrid();

    Cell *start;
    Cell *finish;
private:
    int sizeX, sizeY;
};

void PrintAsciiMaze(const Maze& maze);
