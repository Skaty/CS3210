#include <mpi.h>
#include <stdlib.h>
#include <time.h>

#include "training_consts.h"
#include "datastructs.h"
#include "utils.h"

/**
 * Contains methods for player processes
 */

void player_execute_round(int rnd, coordinates* playercoords, coordinates* ballcoords) {
    coordinates buffer[] = {{0, 0}, {0,0}};
    int sendsize = 0;

    // Get ball, determine path and next position
    MPI_Recv(ballcoords, sizeof(coordinates), MPI_BYTE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    run_towards_coordinates(&(buffer[0]), playercoords, ballcoords, 10);
    copy_position(&(buffer[0]), playercoords);

    if (is_positions_equal(buffer[0], *ballcoords) == 1) {
        buffer[1].x = rand() % 128;
        buffer[1].y = rand() % 64;
        sendsize = sizeof(coordinates) * 2;
    } else {
        // not on ball
        sendsize = sizeof(coordinates);
    }

    MPI_Send(buffer, sendsize, MPI_BYTE, BALL_ID, 0, MPI_COMM_WORLD);
}

void player_main(int myrank) {
    coordinates mycoords = {0, 0};
    init_coordinates_for_rank(&mycoords, myrank, 4, 3);

    coordinates ballcoords = {0, 0};
    int roundsleft = ROUNDS;

    srand(time(NULL));

    while (roundsleft != 0) {
        roundsleft--;
        player_execute_round(ROUNDS-roundsleft, &mycoords, &ballcoords);
    }
}
