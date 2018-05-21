#include <mpi.h>
#include <stdio.h>
#include "field_match.h"
#include "player_match.h"

/**
 * Entry point of the application
 * @param  argc [description]
 * @param  argv [description]
 * @return      [description]
 */
int main(int argc, char *argv[]) {
    int rank, processes;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &processes);

    if (processes != 34) {
        printf("There must be exactly 34 processes!\n");
        MPI_Finalize();
        return 0;
    }

    if (rank < 12) {
        // field process
        field_main(rank);
    } else {
        // player process
        player_main(rank);
    }

    MPI_Finalize();

    return 0;
}
