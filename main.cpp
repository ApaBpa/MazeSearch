#include "httplib.h"

#include "maze.h"
#include "maze_generator.h"
#include "webgui.h"
#ifdef USE_MPI
#include "mpi.h"
#endif

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <omp.h>

int main(int argc, char** argv) {
#ifdef USE_MPI
    MPI_Init(&argc, &argv);
#endif
    StartWebServer();
#ifdef USE_MPI
    MPI_Finalize();
#endif
    return 0;
}



