#include "httplib.h"

#include "maze.h"
#include "maze_generator.h"
#include "webgui.h"

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <omp.h>

int main(int argc, char** argv) {
    StartWebServer();
    return 0;
    /*
    httplib::Server svr;
    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("Hello from C++!", "text/plain");
    });
    printf("Server running at http://localhost:8080\n");
    svr.listen("0.0.0.0", 8080);

    int numMazes = 10;
    int mazeW = 40;
    int mazeH = 40;

    int numThreads = 1;

    for (int i = 1; i < argc; ++i) {
        std::string s = argv[i];
        if (s == "--mazes" && i + 1 < argc) {numMazes = std::stoi(argv[++i]);} 
        else if (s == "--width" && i + 1 < argc) {mazeW = std::stoi(argv[++i]);} 
        else if (s == "--height" && i + 1 < argc) {mazeH = std::stoi(argv[++i]);} 
        else if (s == "--threads" && i + 1 < argc) {numThreads = std::stoi(argv[++i]);} 
        else if (s == "--help") {
            std::cout << "Usage: maze.exe [--mazes N] [--width W] [--height H] [--threads T]\n";
            return 0;
        } 
        else {std::cerr << "Bad or missing arguments (ignored): " << s << "\n";}
    }

    if (numMazes <= 0 || mazeW <= 0 || mazeH <= 0 || numThreads <= 0) {
        std::cerr << "Numeric arguments must be positive.\n";
        return 1;
    }
    
    // Create vector of mazes
    std::vector<Maze> allMazes;
    allMazes.reserve(numMazes);
    
    // Creat team of threads
    omp_set_num_threads(numThreads);
    for (int i = 0; i < numMazes; ++i) allMazes.emplace_back(mazeW, mazeH);

    // Parallel section (outer maze generation)
    #pragma omp parallel for if(numThreads>1)
    for(int i = 0; i < numMazes; i++){
        MazeGenerator mg;
        allMazes[i].MakeGrid();
        mg.GeneratePrim(allMazes[i]);
    }
    
    std::cout << "Generated " << numMazes << " mazes of size: " << mazeW << "x" << mazeH << "\n";

    int current = 0;
    Maze displayedMaze = allMazes.empty() ? Maze(mazeW, mazeH) : allMazes[current];

    return 0;
    */
}



