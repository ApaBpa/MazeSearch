#include "webgui.h"
#include "httplib.h"
#include "maze.h"
#include "maze_generator.h"
#include "solve_a_star.h"
#include "jsonifier.h"
#include <fstream>
#include <chrono>
#include <mutex>
#include <algorithm>
#include <deque>

// Runtime choose solver via request parameter (default outer sequential)
// valid modes: inter, intra, outer, seq

static std::vector<Maze> mazes;
static MazeGenerator generator;
static std::mutex maze_mutex;
static std::vector<std::vector<Cell *>> previous_paths; // Store the previously solved paths to avoid re-solving
static bool solution_cached = false;

static std::deque<std::string> log_lines;
static std::mutex log_mutex;

static void AppendLog(const std::string &line){
    std::lock_guard<std::mutex> lock(log_mutex);
    if (log_lines.size() >= 1000) {
        log_lines.pop_front();
    }
    log_lines.push_back(line);
}

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

// Helper function to get string parameters from the request
static std::string GetStrParam(const httplib::Request req, const std::string& key, const std::string& defaultValue) {
    if (req.has_param(key)) {
        return req.get_param_value(key);
    }
    return defaultValue;
}

// Request handler for maze generation
void RegisterGenerateHandler (httplib::Server& server){
    server.Post("/generate", [](const httplib::Request& req, httplib::Response& res){
        int width = std::max(2, std::min(500, GetIntParam(req, "width", 20)));
        int height = std::max(2, std::min(500, GetIntParam(req, "height", 20)));
        int num_mazes = std::max(1, std::min(100, GetIntParam(req, "numMazes", 1)));
        std::lock_guard<std::mutex> lock(maze_mutex);

        if(num_mazes == 0){
            res.set_content("{\"error\":\"num_mazes must be greater than 0.\"}", "application/json");
            return;
        }

        mazes.clear();
        mazes.reserve(num_mazes);
        
        auto t0 = std::chrono::high_resolution_clock::now(); // Kosher??
        for (int i = 0; i < num_mazes; ++i) {
            Maze maze(width, height);
            generator.GeneratePrim(maze);
            mazes.push_back(std::move(maze));
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        double generationTime = std::chrono::duration<double, std::milli>(t1 - t0).count();

        AppendLog("[GEN] Generated " + std::to_string(num_mazes) + " maze(s) " + std::to_string(width) + "x" + std::to_string(height) + " in " + std::to_string(generationTime) + " ms");

        previous_paths.clear();
        solution_cached = false;
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

        auto t0 = std::chrono::high_resolution_clock::now();    // Kosher??
        std::string mode = GetStrParam(req, "mode", "outer");
        std::vector<std::vector<Cell *>> paths = SolveSelected(mazes, mode);
        auto t1 = std::chrono::high_resolution_clock::now();
        double solvingTime = std::chrono::duration<double, std::milli>(t1 - t0).count();

        AppendLog("[SOL] Mode=" + GetStrParam(req, "mode", "outer") + " solved " + std::to_string(mazes.size()) + " maze(s) in " + std::to_string(solvingTime) + " ms");

        if (solution_cached && PathsEqual(paths, previous_paths)) {
            res.set_content(PathsToJSON(previous_paths, solvingTime), "application/json");
            AppendLog("[SOL] Cached result returned");
            return;
        }

        previous_paths = paths;
        solution_cached = true;
        res.set_content(PathsToJSON(paths, solvingTime), "application/json");
    });
}

void StartWebServer(){
    httplib::Server server;

    server.Get("/icons/favicon.ico", [](const httplib::Request&, httplib::Response& res) {
        std::ifstream f("./icons/favicon.ico", std::ios::binary);
        if (!f) { res.status = 404; return; }
        std::string bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        res.set_content(std::move(bytes), "image/x-icon");
    });

    server.Get("/favicon.ico", [](const httplib::Request&, httplib::Response& res) {
        res.set_redirect("/icons/favicon.ico");
    });

    // Import HTML files from "/static"
    server.set_mount_point("/", "./static");
    
    server.Get("/logs", [](const httplib::Request&, httplib::Response& res){
        std::lock_guard<std::mutex> lock(log_mutex);
        std::string json = "[";
        bool first = true;
        for (const auto &line : log_lines) {
            if (!first) json += ",";
            first = false;
            // basic JSON string escaping for quotes/backslash/newline
            std::string txt;
            for (char c : line) {
                if (c == '\\') txt += "\\\\";
                else if (c == '"') txt += "\\\"";
                else if (c == '\n') txt += "\\n";
                else if (c == '\r') txt += "\\r";
                else txt += c;
            }
            json += "\"" + txt + "\"";
        }
        json += "]";
        res.set_content(json, "application/json");
    });

    RegisterGenerateHandler(server);

    RegisterSolveHandler(server);

    int port = 8080;
    printf("Server running at http://localhost:%d\n", port);
    server.listen("0.0.0.0", port);
}