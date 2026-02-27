#include "maze_generator.h"
#include "maze.h"
#include <stdexcept>
#include <random>

static const int dirX[4] = { 1, 0, -1, 0 };
static const int dirY[4] = { 0, 1,  0, -1 };

MazeGenerator::MazeGenerator()
{
    std::random_device rd;
    rng.seed(rd());
}

/// @brief Generate maze with prim's algorithm https://weblog.jamisbuck.org/2011/1/10/maze-generation-prim-s-algorithm
void MazeGenerator::GeneratePrim(Maze &maze)
{
    printf("Generating maze with Prim's algorithm...\n");
    if (maze.width() < 1 || maze.height() < 1 || maze.cells.empty() || maze.cells[0].empty()) {
        throw std::runtime_error("Maze dimensions or cells not initialized");
    }

    std::vector<Cell*> frontier;

    // Pick random starting position
    std::uniform_int_distribution<int> distX(0, maze.width() - 1);
    std::uniform_int_distribution<int> distY(0, maze.height() - 1);
    int startX = distX(rng);
    int startY = distY(rng);

    Cell *start = &maze.cells[startY][startX];
    Cell *lastVisited = start;
    start->inMaze = true;

    AddFrontierNeighbours(maze, frontier, start);

    while (!frontier.empty())
    {
        std::uniform_int_distribution<int> randF(0, (int)frontier.size() - 1);
        int idx = randF(rng);
        Cell *nextF = frontier[idx]; // Frontier cell

        // Find neighbours of current frontier cell and update who the neighbours are
        std::vector<Cell *> mazeNeighbours;
        for (int dir = 0; dir < 4; dir++)
        {
            int nx = nextF->posX + dirX[dir];
            int ny = nextF->posY + dirY[dir];

            if (nx < 0 || nx >= maze.width() || ny < 0 || ny >= maze.height())
                continue;

            Cell *neighbour = &maze.cells[ny][nx];
            if (neighbour->inMaze)
            {
                mazeNeighbours.emplace_back(neighbour);
            }
        }

        if (!mazeNeighbours.empty())
        {
            std::uniform_int_distribution<int> randN(0, mazeNeighbours.size() - 1);
            Cell *neighbour = mazeNeighbours[randN(rng)];

            maze.MakePassage(nextF, neighbour);

            nextF->inMaze = true;
            nextF->inFrontier = false;

            lastVisited = nextF;

            AddFrontierNeighbours(maze, frontier, nextF);
        }

        frontier[idx] = frontier.back();
        frontier.pop_back();
    }

    maze.start = start;
    maze.finish = lastVisited;
}

/// @brief Finds the four neighbours of the current cell
/// @param c Current Cell
void MazeGenerator::AddFrontierNeighbours(Maze &maze, std::vector<Cell*> &frontier, Cell* c)
{
    for (int dir = 0; dir < 4; dir++)
    {
        int neighbourX = c->posX + dirX[dir];
        int neighbourY = c->posY + dirY[dir];
        if (neighbourX < 0 || neighbourX >= maze.width() || neighbourY < 0 || neighbourY >= maze.height())
            continue;
        Cell &neighbour = maze.cells[neighbourY][neighbourX];
        if (!neighbour.inMaze && !neighbour.inFrontier)
        {
            neighbour.inFrontier = true;
            frontier.emplace_back(&neighbour);
        }
    }
}
