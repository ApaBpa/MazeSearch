#include "mpi_controller.h"

#ifdef USE_MPI
#include "mpi.h"

#include <stdexcept>

enum class WorkerCommand : int {
    Generate = 1,
    Solve = 2,
    Shutdown = 3,
};

enum class SolveModeCode : int {
    Inter = 1,
    Intra = 2,
    Combined = 3,
    Sequential = 4,
};

static SolveModeCode ModeToCode(const std::string& mode) {
    if (mode == "inter") return SolveModeCode::Inter;
    if (mode == "intra") return SolveModeCode::Intra;
    if (mode == "combined") return SolveModeCode::Combined;
    return SolveModeCode::Sequential;
}

static std::string CodeToMode(SolveModeCode mode) {
    if (mode == SolveModeCode::Inter) return "inter";
    if (mode == SolveModeCode::Intra) return "intra";
    if (mode == SolveModeCode::Combined) return "combined";
    return "sequential";
}

void MpiBroadcastGenerateCommand(int width, int height, int num_mazes, uint32_t seed) {
    int cmd = static_cast<int>(WorkerCommand::Generate);
    int payload[3] = {width, height, num_mazes};
    unsigned long long seed_payload = static_cast<unsigned long long>(seed);
    MPI_Bcast(&cmd, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(payload, 3, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&seed_payload, 1, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
}

void MpiBroadcastSolveCommand(const std::string& mode) {
    int cmd = static_cast<int>(WorkerCommand::Solve);
    int mode_code = static_cast<int>(ModeToCode(mode));
    MPI_Bcast(&cmd, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&mode_code, 1, MPI_INT, 0, MPI_COMM_WORLD);
}

void MpiRunWorkerLoop(const std::function<void(int, int, int, uint32_t)>& onGenerate,
                      const std::function<void(const std::string&)>& onSolve) {
    while (true) {
        int cmd_int = 0;
        MPI_Bcast(&cmd_int, 1, MPI_INT, 0, MPI_COMM_WORLD);
        WorkerCommand cmd = static_cast<WorkerCommand>(cmd_int);

        if (cmd == WorkerCommand::Generate) {
            int payload[3] = {0, 0, 0};
            unsigned long long seed_payload = 0;
            MPI_Bcast(payload, 3, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Bcast(&seed_payload, 1, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
            onGenerate(payload[0], payload[1], payload[2], static_cast<uint32_t>(seed_payload));
            continue;
        }

        if (cmd == WorkerCommand::Solve) {
            int mode_int = 0;
            MPI_Bcast(&mode_int, 1, MPI_INT, 0, MPI_COMM_WORLD);
            onSolve(CodeToMode(static_cast<SolveModeCode>(mode_int)));
            continue;
        }

        if (cmd == WorkerCommand::Shutdown) {
            break;
        }

        throw std::runtime_error("Unknown MPI worker command");
    }
}
#endif
