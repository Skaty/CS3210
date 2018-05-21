#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "training_consts.h"
#include "datastructs.h"
#include "field_training.h"
#include "player_training.h"
#include "utils.h"

/**
 * Entry point of application
 * @param  argc [description]
 * @param  argv [description]
 * @return      exit code
 */
int main(int argc, char *argv[]) {
    int rank, processes;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &processes);

    if (processes != 12) {
        printf("There must be exactly 12 processes running!\n");
        MPI_Finalize();
        return 0;
    }

    if (rank == 0) {
        // Field process
        field_main();
    } else {
        // Player process
        player_main(rank);
    }

    // We are done with the game
    MPI_Finalize();
    return 0;
}
