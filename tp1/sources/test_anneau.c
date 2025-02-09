#include <stdint.h>
#include <mpi.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    int nbp, rang, token;
    
    MPI_Init(&argc, &argv);                 
    MPI_Comm_size(MPI_COMM_WORLD, &nbp);    
    MPI_Comm_rank(MPI_COMM_WORLD, &rang);   

    if (nbp < 2) {
        printf("Error: Need at least 2 processes!\n");
        fflush(stdout);
        MPI_Finalize();
        return 1;
    }

    if (rang == 0) {
        token = 1;
        printf("Process %d initialized token = %d\n", rang, token);
        fflush(stdout);
        MPI_Send(&token, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
    }

    // 所有进程都要执行这个部分，包括 0 号进程
    MPI_Recv(&token, 1, MPI_INT, (rang - 1 + nbp) % nbp, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    token++;  
    printf("Process %d received token = %d\n", rang, token);
    fflush(stdout);
    MPI_Send(&token, 1, MPI_INT, (rang + 1) % nbp, 0, MPI_COMM_WORLD);

    if (rang == 0) {
        MPI_Recv(&token, 1, MPI_INT, nbp - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Process %d final token = %d\n", rang, token);
        fflush(stdout);
    }

    MPI_Finalize();  
    return 0;
}

