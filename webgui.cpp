#include "webgui.h"
#include "httplib.h"
#include "maze.h"
#include "maze_generator.h"
#include "maze_solver.h"
#include <chrono>
#include <sstream>
#include <mutex>
#include <algorithm>

#if defined(USE_MPI)
    #define SOLVE SolveMPI
#elif defined(USE_OMP)
    #define SOLVE SolveOMP
#else
    #define SOLVE SolveSeq
#endif

static std::vector<Maze> mazes;
static MazeGenerator generator;
static std::mutex maze_mutex;

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

static std::string MazesToJSON(const std::vector<Maze>& mazes, const double generationTime) {
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

static std::string PathsToJSON(const std::vector<std::vector<Cell *>>& paths, double solvingTime) {
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

// Request handler for maze generation
void RegisterGenerateHandler (httplib::Server& server, unsigned int num_mazes){
    server.Post("/generate", [num_mazes](const httplib::Request& req, httplib::Response& res){
        int width = std::max(2, std::min(500, GetIntParam(req, "width", 20)));
        int height = std::max(2, std::min(500, GetIntParam(req, "height", 20)));
        std::lock_guard<std::mutex> lock(maze_mutex);

        if(num_mazes == 0){
            res.set_content("{\"error\":\"num_mazes must be greater than 0.\"}", "application/json");
            return;
        }

        mazes.clear();
        mazes.reserve(num_mazes);
        
        auto t0 = std::chrono::high_resolution_clock::now(); // IDK if auto is cool here?

        for (int i = 0; i < num_mazes; ++i) {
            Maze maze(width, height);
            generator.GeneratePrim(maze);
            mazes.push_back(std::move(maze));
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        double generationTime = std::chrono::duration<double, std::milli>(t1 - t0).count();

        // Serialize the first maze to JSON and send it in the response
        res.set_content(MazesToJSON(mazes, generationTime), "application/json");
    });
}

// Post request handler for maze solving
void RegisterSolveHandler (httplib::Server& server){
    server.Post("/solve", [](const httplib::Request& req, httplib::Response& res){
        std::lock_guard<std::mutex> lock(maze_mutex);
        
        if (mazes.empty()) {
            res.set_content("{\"error\":\"No maze generated yet.\"}", "application/json");
            return;
        }

        auto t0 = std::chrono::high_resolution_clock::now();

        std::vector<std::vector<Cell *>> paths = SOLVE(mazes);

        auto t1 = std::chrono::high_resolution_clock::now();
        double solvingTime = std::chrono::duration<double, std::milli>(t1 - t0).count();

        res.set_content(PathsToJSON(paths, solvingTime), "application/json");
    });
}

void StartWebServer(){
    httplib::Server server;

    // Import HTML files from "/static"
    server.set_mount_point("/", "./static");
    
    RegisterGenerateHandler(server, 1);

    RegisterSolveHandler(server);

    int port = 8080;
    printf("Server running at http://localhost:%d\n", port);
    server.listen("0.0.0.0", port);
}