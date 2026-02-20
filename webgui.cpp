#include "webgui.h"
#include "httplib.h"
#include "maze.h"
#include "maze_generator.h"
#include "maze_solver.h"
#include <chrono>
#include <sstream>
#include <mutex>
#include <algorithm>

static Maze currentMaze(0, 0);
static MazeGenerator generator;
static std::mutex mazeMutex;

// Helper function to get integer parameters from the request
static int GetIntParam(const httplib::Request req, const std::string& key, int defaultValue) {
    if (req.has_param(key)) {
        try {
            return std::stoi(req.get_param_value(key));
        } catch (const std::exception& e) {
            std::cerr << "Invalid parameter for " << key << ": " << e.what() << "\n";
        }
    }
    return defaultValue;
}

static std::string MazeToJSON(const Maze& maze, const double generationTime) {
    std::ostringstream oss;
    oss << "{"; // JSON object
    oss << "\"width\":" << maze.width() << ",";
    oss << "\"height\":" << maze.height() << ",";
    oss << "\"generationTime\":" << generationTime << ",";
    oss << "\"start\":{\"x\":" << maze.start->posX << ",\"y\":" << maze.start->posY << "}" << ",";
    oss << "\"finish\":{\"x\":" << maze.finish->posX << ",\"y\":" << maze.finish->posY << "}" << ",";
    oss << "\"cells\":";
    oss << "["; // JSON array
    // Serialize cells (fill JSON array with cells)
    for (size_t y = 0; y < maze.height(); ++y) {
        for (size_t x = 0; x < maze.width(); ++x) {
            if(x > 0 || y > 0) oss << ",";   // Add comma before each cell except the first
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
    for (size_t i = 0; i < maze.passages.size(); ++i) {
        if(i > 0) oss << ",";   // Add comma before each passage except the first
        const Passage& passage = maze.passages[i];
        oss << "{";
        oss << "\"c1\":{" << "\"x\":" << passage.c1->posX << "," << "\"y\":" << passage.c1->posY << "},";
        oss << "\"c2\":{" << "\"x\":" << passage.c2->posX << "," << "\"y\":" << passage.c2->posY << "}";
        oss << "}";
    }
    oss << "]";
    oss << "}";
    return oss.str();
}

static std::string SolveToJSON(const std::vector<Cell*>& path, double solvingTime) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"solvingTime\":" << solvingTime << ",";
    oss << "\"path\":";
    oss << "["; // JSON array
    for (size_t i = 0; i < path.size(); ++i) {
        if(i > 0) oss << ",";   // Add comma before each cell except the first
        const Cell* cell = path[i];
        oss << "{";
        oss << "\"x\":" << cell->posX << ",";
        oss << "\"y\":" << cell->posY;
        oss << "}";
    }
    oss << "]";
    oss << "}";
    return oss.str();
}

void StartWebServer(){
    httplib::Server server;

    // Import HTML files from "/static"
    server.set_mount_point("/", "./static");

    server.Post("/generate", [](const httplib::Request& req, httplib::Response& res){
        int width = std::max(2, std::min(500, GetIntParam(req, "width", 20)));
        int height = std::max(2, std::min(500, GetIntParam(req, "height", 20)));
        
        std::lock_guard<std::mutex> lock(mazeMutex);
        currentMaze = Maze(width, height);
        currentMaze.MakeGrid();
        
        auto t0 = std::chrono::high_resolution_clock::now();

        generator.GeneratePrim(currentMaze);

        auto t1 = std::chrono::high_resolution_clock::now();
        double generationTime = std::chrono::duration<double, std::milli>(t1 - t0).count();

        res.set_content(MazeToJSON(currentMaze, generationTime), "application/json");
    });

    server.Post("/solve", [](const httplib::Request& req, httplib::Response& res){
        std::string algo = req.has_param("algorithm")
            ? req.get_param_value("algorithm") : "astar";

        std::lock_guard<std::mutex> lock(mazeMutex);
        if (currentMaze.width() == 0 || currentMaze.height() == 0){res.set_content("{\"error\":\"No maze generated yet.\"}", "application/json"); return;}

        std::vector<Cell*> path;
        auto t0 = std::chrono::high_resolution_clock::now();

        if (algo == "astar") {
            path = AStar(currentMaze);
        } else{
            res.set_content("{\"error\":\"Unknown algorithm.\"}", "application/json");
            return;
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        double solvingTime = std::chrono::duration<double, std::milli>(t1 - t0).count();

        res.set_content(SolveToJSON(path, solvingTime), "application/json");
    });

    int port = 8080;
    printf("Server running at http://localhost:%d\n", port);
    server.listen("0.0.0.0", port);
}