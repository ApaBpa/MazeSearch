make clean
make maze_mpi
echo "Build Complete..."
MPI_NP=${MPI_NP:-6}
mpirun -np "$MPI_NP" ./maze_mpi