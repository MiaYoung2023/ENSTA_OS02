#include <stdint.h>
#include <mpi.h>
#include <stdio.h>
#include <math.h>

int get_neighbor(int rank, int k) {
    return rank ^ (1 << k); // k-irritation
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // dimension de hypercube
    int d = (int)log2(size); // nombre de taches

    // nombre de taches doit être une puissance de 2
    if ((1 << d) != size) {
        if (rank == 0) {
            printf("Error: Number of tasks must be a power of 2!\n");
        }
        MPI_Finalize();
        return -1;
    }

    int token = -1; // initial token value

    if (rank == 0) {
        token = 2048; // value donné par user
        printf("Task %d starts with token %d\n", rank, token);
    }

    double start_time = MPI_Wtime();

    // diffusion du token
    for (int step = 0; step < d; step++) {
        int offset = 1 << step; // 2^step
        if (rank < offset) {
            //processus that send the token
            int target = rank + offset;
            MPI_Send(&token, 1, MPI_INT, target, 0, MPI_COMM_WORLD);
            printf("Task %d sent token %d to Task %d\n", rank, token, target);
        } else if (rank < 2 * offset) {
            //processus that receive the token
            int source = rank - offset;
            MPI_Recv(&token, 1, MPI_INT, source, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("Task %d received token %d from Task %d\n", rank, token, source);
        }
    }

    double end_time = MPI_Wtime();

    // 计算并打印执行时间（如果是rank 0）
    if (rank == 0) {
        printf("Token diffusion took %f seconds\n", end_time - start_time);
    }
    
    MPI_Finalize();
    return 0;
}
