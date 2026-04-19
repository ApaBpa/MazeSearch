#include "solve_a_star.h"
#include "maze.h"
#include "stdio.h"

#ifdef USE_OMP
#include "omp.h"
#endif
#ifdef USE_MPI
#include "mpi.h"
#endif

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <limits>
#include <deque>
#include <vector>
#include <stdexcept>

std::vector<Cell*> FindNeighbours(const Maze &maze, const Cell c) {
    std::vector<Cell*> neighbours;

    for (const Passage &p : maze.passages) {
        if (p.c1 && *p.c1 == c && p.c2) {
            neighbours.emplace_back(p.c2);
        } else if (p.c2 && *p.c2 == c && p.c1) {
            neighbours.emplace_back(p.c1);
        }
    }
    return neighbours;
}

float Heuristic(const Cell *a, const Cell *b){
#ifdef USE_EUCLIDEAN
    return sqrt(pow(a->posX - b->posX, 2) + pow(a->posY - b->posY, 2));
#else
    return abs(a->posX - b->posX) + abs(a->posY - b->posY);
#endif
}

float EuclideanDistance(const Cell *a, const Cell *b){
    return sqrt(pow(a->posX - b->posX, 2) + pow(a->posY - b->posY, 2));
}

float ManhattanDistance(const Cell *a, const Cell *b){
    return abs(a->posX - b->posX) + abs(a->posY - b->posY);
}

void ReconstructPath(std::unordered_map<Cell*, Cell*> &parent_of, Cell *current, std::vector<Cell*> &path){
    while (current != nullptr){
        path.push_back(current);
        current = parent_of.count(current) ? parent_of[current] : nullptr;
    }
    std::reverse(path.begin(), path.end());
}

/// @brief A* search algorithm https://www.geeksforgeeks.org/dsa/a-search-algorithm/
/// @param maze 
/// @return Vector of Cell in the order they are visited, starting with the start cell and ending with the finish cell. {} if no path is found.
std::vector<Cell *> AStarSeq(Maze &maze){
    if (!maze.start || !maze.finish) return {};
    if (maze.start == maze.finish) return { maze.start };

    // Initialize the open and closed sets, and the g_cost and f_cost maps
    Cell *start = maze.start;
    Cell *finish = maze.finish;
    std::vector<Cell*> open_set;
    std::vector<Cell*> closed_set;

    std::unordered_map<Cell*, float> g_cost;
    std::unordered_map<Cell*, float> f_cost;
    std::unordered_map<Cell*, Cell*> parent_of;

    g_cost[start] = 0;
    f_cost[start] = Heuristic(start, finish);
    open_set.push_back(start);

    while (!open_set.empty()){
        // Find the cell in open set with lowest f_cost
        Cell *current = open_set[0];
        for (Cell *cell : open_set){
            if (f_cost[cell] < f_cost[current]){
                current = cell;
            }
        }

        // If we reached the finish, reconstruct the path and return it
        if (current == finish){
            // Reconstruct path
            std::vector<Cell *> path;
            ReconstructPath(parent_of, current, path);
            return path;
        }

        // Move current from open set to closed set
        open_set.erase(std::remove(open_set.begin(), open_set.end(), current), open_set.end());
        closed_set.push_back(current);

        for(Cell* neighbour : FindNeighbours(maze, *current)){
            if (std::find(closed_set.begin(), closed_set.end(), neighbour) != closed_set.end()){
                continue; // Ignore the neighbour which is already evaluated
            }

            float tentative_g_cost = g_cost[current] + ManhattanDistance(current, neighbour);
            bool inOpen = (std::find(open_set.begin(), open_set.end(), neighbour) != open_set.end());

            // Best path to neighbour found so far, record it
            if (!inOpen || tentative_g_cost < g_cost[neighbour]) {
                parent_of[neighbour] = current;
                g_cost[neighbour] = tentative_g_cost;
                f_cost[neighbour] = tentative_g_cost + Heuristic(neighbour, finish);
                if (!inOpen) open_set.push_back(neighbour);
            }
        }
    }
    
    return {}; // No path found
}

/// @brief Solves multiple mazes sequentially (outer maze parallelism).
std::vector<std::vector<Cell *>> SolveSeq(std::vector<Maze>& mazes){
    std::vector<std::vector<Cell *>> results;
    for (Maze &maze : mazes){
        results.push_back(AStarSeq(maze));
    }
    return results;
}

#ifdef USE_MPI
struct CellCoord {
    int x;
    int y;
};

static Cell* GetCellPtr(Maze &maze, int posX, int posY);

/// @brief Solves mutiple mazes in parallel using MPI. Each process will solve a subset of all mazes. Expected speedup ~ number of cores on the CPU.
/// @param mazes 
/// @return Vector of paths, where each path is a vector of Cell representing the order of cells from start to finish for each maze. The order of paths corresponds to the order of mazes in the input vector.
std::vector<std::vector<Cell *>> SolveMPI(std::vector<Maze>& mazes){
    int my_rank, size;

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int num_mazes = mazes.size();
    int mazesPerProcess = (num_mazes + size - 1) / size;

    int startIndex = my_rank * mazesPerProcess;
    int endIndex = std::min(startIndex + mazesPerProcess, num_mazes);

    // Solve local mazes
    std::vector<std::vector<Cell *>> localResults;
    for (int i = startIndex; i < endIndex; i++){
        localResults.push_back(AStarSeq(mazes[i]));
    }

    // Manual gather on node=0
    if (my_rank == 0){
        std::vector<std::vector<Cell *>> allResults = localResults;

        for (int source = 1; source < size; source++){
            int numPaths;
            MPI_Recv(&numPaths, 1, MPI_INT, source, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            for (int i = 0; i < numPaths; ++i) {
                int pathSize;
                MPI_Recv(&pathSize, 1, MPI_INT, source, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                std::vector<CellCoord> coords(pathSize);
                if (pathSize > 0) {
                    MPI_Recv(coords.data(), pathSize * sizeof(CellCoord), MPI_BYTE, source, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }

                std::vector<Cell *> path;
                path.reserve(pathSize);
                int globalMazeIndex = source * mazesPerProcess + i;
                for (const CellCoord& coord : coords) {
                    if (coord.x < 0 || coord.y < 0 || globalMazeIndex < 0 || globalMazeIndex >= num_mazes) {
                        path.push_back(nullptr);
                    } else {
                        path.push_back(GetCellPtr(mazes[globalMazeIndex], coord.x, coord.y));
                    }
                }

                allResults.push_back(std::move(path));
            }
        }

        return allResults;
    } 
    else {
        int numPaths = localResults.size();
        MPI_Send(&numPaths, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        for (const std::vector<Cell *>& path : localResults) {
            int pathSize = path.size();
            MPI_Send(&pathSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

            std::vector<CellCoord> coords;
            coords.reserve(path.size());
            for (Cell* cell : path) {
                if (cell) {
                    coords.push_back({cell->posX, cell->posY});
                } else {
                    coords.push_back({-1, -1});
                }
            }

            if (pathSize > 0) {
                MPI_Send(coords.data(), pathSize * sizeof(CellCoord), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
            }
        }

        return {};
    }
}

/// @brief Solves multiple mazes in parallel (inter-maze): MPI process farm.
std::vector<std::vector<Cell *>> SolveInterMaze(std::vector<Maze>& mazes){
    return SolveMPI(mazes);
}

std::vector<Cell *> HDA(Maze& maze);

#ifdef USE_MPI
/// @brief Solves one maze at a time with HDA (intra-maze parallelism).
std::vector<std::vector<Cell *>> SolveIntraMaze(std::vector<Maze>& mazes){
    std::vector<std::vector<Cell *>> results;
    results.reserve(mazes.size());
    for (Maze &maze : mazes){
        results.push_back(HDA(maze));
    }
    return results;
}
#endif

std::vector<std::vector<Cell *>> SolveSelected(std::vector<Maze>& mazes, const std::string &mode){
    if (mode == "inter"){
        // UI "inter" means inter-maze parallelism.
        printf("Solving Maze [INTER-MAZE]...\n");
        return SolveInterMaze(mazes);
    } else if (mode == "intra"){
        // UI "intra" means intra-maze parallelism.
        printf("Solving Maze [INTRA-MAZE]...\n");
        return SolveIntraMaze(mazes);
    } else if (mode == "combined"){
        printf("Solving Maze [COMBINED] (fallback to INTER-MAZE only; full combined not implemented yet)...\n");
        return SolveInterMaze(mazes);
    } else if (mode == "sequential"){
        printf("Solving Maze [SEQUENTIAL]...\n");
        return SolveSeq(mazes);
    }

    throw std::invalid_argument("Invalid mode: " + mode);
}

struct HDAMessage {
    int posX, posY;
    float tentative_g_cost;
    int parentX, parentY;
};

struct ParentEntry {
    int childX, childY;
    int parentX, parentY;
};

static const int MSG_TAG = 1;

/// @brief Creates a hash for a cell based on coordinates. Used for hashing cells in the HDA algorithm.
/// @return Hash value for each cell. Should be unique but not guarenteed.
static int HDAHash(const Cell *cell, int num_ranks){
    // 73856093 and 19349663 are large primes
    // ^ is bitwise XOR
    size_t h = (size_t)(cell -> posX * 73856093) ^ (cell -> posY * 19349663);
    return h % num_ranks;
}

static Cell* GetCellPtr(Maze &maze, int posX, int posY){
    // Get pointer to cell at (posX, posY) in maze.
    if (posX < 0 || posX >= maze.width() || posY < 0 || posY >= maze.height()) return nullptr;
    return &maze.cells[posY][posX];
}

struct HDAState {
    std::vector<Cell*> open_set;
    std::unordered_set<Cell*> closed_set;
    std::unordered_map<Cell*, float> g_cost;
    std::unordered_map<Cell*, float> f_cost;
    std::unordered_map<Cell*, Cell*> parent_of;
    int msgs_received = 0;
};

static void DrainHDAInbox(Maze& maze, Cell* finish, HDAState& state){
    MPI_Status status;
    int flag = 0;
    MPI_Iprobe(MPI_ANY_SOURCE, MSG_TAG, MPI_COMM_WORLD, &flag, &status);
    while (flag){
        HDAMessage msg;
        MPI_Recv(&msg, sizeof(HDAMessage), MPI_BYTE, status.MPI_SOURCE, MSG_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        state.msgs_received++;

        Cell* neighbour = GetCellPtr(maze, msg.posX, msg.posY);
        Cell* parent = GetCellPtr(maze, msg.parentX, msg.parentY);
        if (!neighbour || !parent) {
            MPI_Iprobe(MPI_ANY_SOURCE, MSG_TAG, MPI_COMM_WORLD, &flag, &status);
            continue;
        }

        auto it = state.g_cost.find(neighbour);
        if (it == state.g_cost.end() || msg.tentative_g_cost < it->second){
            state.g_cost[neighbour] = msg.tentative_g_cost;
            state.f_cost[neighbour] = msg.tentative_g_cost + Heuristic(neighbour, finish);
            state.parent_of[neighbour] = parent;
            state.open_set.push_back(neighbour);
        }

        MPI_Iprobe(MPI_ANY_SOURCE, MSG_TAG, MPI_COMM_WORLD, &flag, &status);
    }
}

/// @brief Hash distributed A* search algorithm. Each process is responsible for a subset of cells based on the hash of their coordinates.
/// Processes communicate to share information about the best path found so far.
/// @param maze
std::vector<Cell *> HDA(Maze& maze){
    int my_rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (!maze.start || !maze.finish) return {};
    if (maze.start == maze.finish) return { maze.start };

    Cell * start = maze.start;
    Cell * finish = maze.finish;

    // Per-rank state
    HDAState state;

    // Send buffer and pending MPI requests
    std::deque<HDAMessage> send_buffer;
    std::vector<MPI_Request> pending_sends;

    float best_local_g_cost = std::numeric_limits<float>::infinity();

    int msgs_sent = 0;
    int msgs_received = 0;

    // Seed: only the rank responsible for the start cell initializes the open set
    if (HDAHash(start, size) == my_rank){
        state.g_cost[start] = 0;
        state.f_cost[start] = Heuristic(start, finish);
        state.open_set.push_back(start);
    }

    // Main loop
    while (true){
        DrainHDAInbox(maze, finish, state);

        // Synchronization block: check if all processes are idle (open set empty) and if so, terminate
        int local_idle = state.open_set.empty() ? 1 : 0;
        int global_idle = 0;
        int total_sent = 0;
        int total_received = 0;

        MPI_Allreduce(&local_idle, &global_idle, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
        MPI_Allreduce(&msgs_sent, &total_sent, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
        MPI_Allreduce(&state.msgs_received, &total_received, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

        if (global_idle == size && total_sent == total_received) break;

        if (state.open_set.empty()) continue;

        auto best = std::min_element(state.open_set.begin(), state.open_set.end(), [&](Cell* a, Cell* b){
            return state.f_cost[a] < state.f_cost[b];
        });

        Cell* current = *best;
        state.open_set.erase(best);

        if (state.closed_set.count(current)) continue;

        if(!state.g_cost.count(current) || state.g_cost[current] >= best_local_g_cost) continue;

        if (current == finish){
            best_local_g_cost = state.g_cost[current];
            continue;
        }

        for (Cell* neighbour : FindNeighbours(maze, *current)) {
            if (state.closed_set.count(neighbour)) continue;

            float tentative_g_cost = state.g_cost[current] + ManhattanDistance(current, neighbour);

            if (tentative_g_cost >= best_local_g_cost) continue;

            int owner = HDAHash(neighbour, size);

            if (owner == my_rank) {
                auto it = state.g_cost.find(neighbour);
                if (it == state.g_cost.end() || tentative_g_cost < it->second) {
                    state.g_cost[neighbour] = tentative_g_cost;
                    state.f_cost[neighbour] = tentative_g_cost + Heuristic(neighbour, finish);
                    state.parent_of[neighbour] = current;
                    state.open_set.push_back(neighbour);
                }
            } else {
                send_buffer.push_back({neighbour->posX, neighbour->posY, tentative_g_cost, current->posX, current->posY});
                MPI_Request req;
                MPI_Isend(&send_buffer.back(), sizeof(HDAMessage), MPI_BYTE, owner, MSG_TAG, MPI_COMM_WORLD, &req);
                pending_sends.push_back(req);
                msgs_sent++;

                if ((int)pending_sends.size() >= 64) {
                    MPI_Waitall((int)pending_sends.size(), pending_sends.data(), MPI_STATUSES_IGNORE);
                    pending_sends.clear();
                    send_buffer.clear();
                }
            }
        }
    }

    // Flush any remaining messages, before gathering
    if (!pending_sends.empty()) {
        MPI_Waitall((int)pending_sends.size(), pending_sends.data(), MPI_STATUSES_IGNORE);
        pending_sends.clear();
        send_buffer.clear();
    }

    std::vector<ParentEntry> local_parents;
    local_parents.reserve(state.parent_of.size());
    for (auto& [child, parent] : state.parent_of){
        local_parents.push_back({child->posX, child->posY, parent->posX, parent->posY});
    }

    // Send parent information to rank 0 for path reconstruction.
    if (my_rank != 0){
        int num_parents = local_parents.size();
        MPI_Send(&num_parents, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        if (num_parents > 0){
            MPI_Send(local_parents.data(), num_parents * sizeof(ParentEntry), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
        }
        return {};
    }

    // Rank 0 gathers parent information from all ranks and reconstructs the path
    std::unordered_map<Cell*, Cell*> merged = state.parent_of;

    for (int source = 1; source < size; source++){
        int num_parents = 0;
        MPI_Recv(&num_parents, 1, MPI_INT, source, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (num_parents == 0) continue;

        std::vector<ParentEntry> entries(num_parents);
        MPI_Recv(entries.data(), num_parents * sizeof(ParentEntry), MPI_BYTE, source, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        for (const ParentEntry& entry : entries){
            Cell* child = GetCellPtr(maze, entry.childX, entry.childY);
            Cell* parent = GetCellPtr(maze, entry.parentX, entry.parentY);
            if (child && parent){
                merged[child] = parent;
            }
        }
    }

    std::vector<Cell*> path;
    ReconstructPath(merged, finish, path);
    return path;
}
#endif

#ifdef USE_OMP
std::vector<std::vector<Cell *>> SolveOMP(std::vector<Maze>& mazes){
    std::printf("Not implemented yet\n");
    return {};
}
#endif

/// @brief Helper to check if paths are equal
/// @return True if paths are equal, false otherwise
bool PathEqual(const std::vector<Cell*>& a, const std::vector<Cell*>& b){
    if (a.size() != b.size()) return false;
    else if (a == b) return true;   // Same pointer

    for (size_t i = 0; i < a.size(); i++){
        if (!a[i] || !b[i]){    // Nullptr check
            if (a[i] != b[i]) return false;
            continue;
        }
        if (a[i]->posX != b[i]->posX || a[i]->posY != b[i]->posY) return false;
    }
    return true;
}

/// @brief Wrapper to check if path vectors are equal
/// @return True if all paths are equal, false otherwise
bool PathsEqual(const std::vector<std::vector<Cell*>>& a, const std::vector<std::vector<Cell*>>& b){
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); i++){
        if (!PathEqual(a[i], b[i])) return false;
    }
    return true;
}