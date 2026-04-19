#pragma once

#include <cstdint>
#include <functional>
#include <string>

#ifdef USE_MPI
void MpiBroadcastGenerateCommand(int width, int height, int num_mazes, uint32_t seed);
void MpiBroadcastSolveCommand(const std::string& mode);
void MpiRunWorkerLoop(const std::function<void(int, int, int, uint32_t)>& onGenerate,
                      const std::function<void(const std::string&)>& onSolve);
#endif
