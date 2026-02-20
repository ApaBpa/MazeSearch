#include "maze.h"

#include <vector>
#include <random>
#include <iostream>

Cell::Cell(int posX, int posY)
    : posX(posX), posY(posY), inMaze(false), inFrontier(false) {}

Passage::Passage()
    : c1(nullptr), c2(nullptr) {}
Passage::Passage(Cell *c1, Cell *c2)
    : c1(c1), c2(c2) {}

Maze::Maze(int sizeX, int sizeY)
    : sizeX(sizeX), sizeY(sizeY), start(NULL), finish(NULL) {}

void Maze::MakeGrid()
{
    cells.resize(sizeY);
    for (int y = 0; y < sizeY; y++)
    {
        for (int x = 0; x < sizeX; x++)
        {
            cells[y].emplace_back(x, y);
        }
    }
}

void Maze::MakePassage(Cell *a, Cell *b)
{
    passages.emplace_back(a, b);
}

/// @brief Print maze to console (don't use this)
void PrintAsciiMaze(const Maze &maze)
{
    int height = 2 * maze.height() + 1;
    int width = 2 * maze.width() + 1;
    std::vector<std::string> ASCIIMaze(height, std::string(width, '#'));
    // Print all walkable cells
    for (int y = 0; y < maze.height(); y++)
    {
        for (int x = 0; x < maze.width(); x++)
        {
            const Cell &c = maze.cells[y][x];
            if (c.inMaze)
            {
                int gx = 2 * c.posX + 1;
                int gy = 2 * c.posY + 1;
                ASCIIMaze[gy][gx] = ' ';
            }
        }
    }
    // Print passages (remove '#' between cells)
    for (const Passage &p : maze.passages)
    {
        if (!p.c1 || !p.c2)
            continue; // Skip if Passage not properly defined
        int x1 = p.c1->posX;
        int y1 = p.c1->posY;
        int x2 = p.c2->posX;
        int y2 = p.c2->posY;
        int gx1 = 2 * x1 + 1;
        int gy1 = 2 * y1 + 1;
        int gx2 = 2 * x2 + 1;
        int gy2 = 2 * y2 + 1;
        int wx = (gx1 + gx2) / 2; // wall x between centers
        int wy = (gy1 + gy2) / 2; // wall y between centers
        ASCIIMaze[wy][wx] = ' ';  // remove that wall
    }
    for (const auto &row : ASCIIMaze)
    {
        std::cout << row << '\n';
    }
}
