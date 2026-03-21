#include "maze_solver.h"
#include "maze.h"
#include "stdio.h"
#include <sstream>
#include <vector>

std::string MazesToJSON(const std::vector<Maze>& mazes, const double generationTime) {
    std::ostringstream oss;
    oss << "{"; // JSON object
    oss << "\"numMazes\":" << mazes.size() << ",";
    oss << "\"generationTime\":" << generationTime << ",";
    oss << "\"mazes\":[";

    for (size_t i = 0; i < mazes.size(); i++){
        if(i > 0) oss << ",";   // Add comma before each maze except the first
        const Maze &maze = mazes[i];

        oss << "{";
        oss << "\"width\":" << maze.width() << ",";
        oss << "\"height\":" << maze.height() << ",";
        oss << "\"start\":{\"x\":" << maze.start->posX << ",\"y\":" << maze.start->posY << "}" << ",";
        oss << "\"finish\":{\"x\":" << maze.finish->posX << ",\"y\":" << maze.finish->posY << "}" << ",";
        oss << "\"cells\":";
        oss << "["; // JSON array
        bool firstCell = true;
        // Serialize cells (fill JSON array with cells)
        for (size_t y = 0; y < maze.height(); ++y) {
            for (size_t x = 0; x < maze.width(); ++x) {
                if (!firstCell) oss << ",";
                firstCell = false;

                const Cell& cell = maze.cells[y][x];
                oss << "{";
                oss << "\"x\":" << cell.posX << ",";
                oss << "\"y\":" << cell.posY << ",";
                oss << "\"inMaze\":" << (cell.inMaze ? "true" : "false") << ",";
                oss << "\"inFrontier\":" << (cell.inFrontier ? "true" : "false");
                oss << "}";
            }
        }
        oss << "],";
        // Serialize passages (fill JSON array with passages)
        oss << "\"passages\":";
        oss << "["; // JSON array
        for (size_t j = 0; j < maze.passages.size(); ++j) {
            if(j > 0) oss << ",";   // Add comma before each passage except the first
            const Passage& passage = maze.passages[j];
            oss << "{";
            oss << "\"c1\":{" << "\"x\":" << passage.c1->posX << "," << "\"y\":" << passage.c1->posY << "},";
            oss << "\"c2\":{" << "\"x\":" << passage.c2->posX << "," << "\"y\":" << passage.c2->posY << "}";
            oss << "}";
        }
        oss << "]";
        oss << "}";
    }
    
    oss << "]"; // End of mazes array
    oss << "}";

    return oss.str();
}

std::string PathsToJSON(const std::vector<std::vector<Cell *>>& paths, double solvingTime) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"solvingTime\":" << solvingTime << ",";
    oss << "\"paths\":[";
    for (size_t i = 0; i < paths.size(); ++i) {
        if(i > 0) oss << ",";   // Add comma before each cell except the first
        const std::vector<Cell *>& path = paths[i];
        
        oss << "["; // JSON array for the path
        for(size_t j = 0; j < path.size(); j++){
            if(j > 0) oss << ",";   // Add comma before each cell except the first
            const Cell& cell = *path[j];
            oss << "{";
            oss << "\"x\":" << cell.posX << ",";
            oss << "\"y\":" << cell.posY;
            oss << "}";
        }
        oss << "]"; // End single path
    }

    oss << "]";
    oss << "}";

    return oss.str();
}
