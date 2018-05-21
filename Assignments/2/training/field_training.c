#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "training_consts.h"
#include "datastructs.h"
#include "utils.h"
/**
* Contains code for the field process
*/

/**
 * Instantiate the position of all players before the game
 */
void field_init_players(player* players) {
    int i;
    players[BALL_ID].position.x = 64;
    players[BALL_ID].position.y = 32;

    for (i = 1; i <= NUM_PLAYERS; i++) {
        init_coordinates_for_rank(&(players[i].position), i, 4, 3);
        players[i].distance = 0;
    }
}

/**
 * Performs the Ball phase: sends ball coordinates to all players
 */
void field_ball_phase(coordinates* ballcoords) {
    int i;
    for (i = 0; i < NUM_PLAYERS; i++) {
        MPI_Send(ballcoords, sizeof(coordinates), MPI_BYTE, i + 1, 0, MPI_COMM_WORLD);
    }
}

/**
 * Main function for receiving updated player coordinates.
 * Also handles the ball challenge.
 */
void field_update_phase(player* players) {
    coordinates buffer[2];
    int playersremaining = NUM_PLAYERS, kicker_rank = -1;
    MPI_Status status;

    // Copy previous ball's location
    copy_position(&(players[BALL_ID].position), &(players[BALL_ID].previous));

    // Wait for messages from players
    while (playersremaining != 0) {
        MPI_Recv(buffer, sizeof(coordinates) * 2, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        int cur_rank = status.MPI_SOURCE;
        player *cur_player = &players[cur_rank];

        // Copy player's current coordinates as previous coordinate
        copy_position(&(cur_player->position), &(cur_player->previous));
        copy_position(&(buffer[0]), &(cur_player->position));

        if (is_positions_equal(buffer[0], players[0].position)) {
            // player is on the ball
            if (kicker_rank == -1 || rand() % 2 == 0) {
                // player won the ball "interim"
                kicker_rank = cur_rank;
                copy_position(&(buffer[1]), &(players[0].position));
            }
            cur_player->onball = 1;
            cur_player->timesonball += 1;
        } else {
            cur_player->gotball = 0;
            cur_player->onball = 0;
        }

        cur_player->distance += distance(&(cur_player->position), &(cur_player->previous));

        playersremaining--;
    }

    if (kicker_rank != -1) {
        players[kicker_rank].gotball = 1;
        players[kicker_rank].timesgotball += 1;
    }
}

/**
 * Prints out the state of the game
 */
void field_print_phase(int cur_round, player* players) {
    int i;
    printf("%d\n", cur_round);
    printf("%d %d\n", players[BALL_ID].position.x, players[BALL_ID].position.y);

    for (i = 1; i < NUM_PLAYERS; i++) {
        printf("%d %d %d ", i - 1, players[i].previous.x, players[i].previous.y);
        printf("%d %d ", players[i].position.x, players[i].position.y);
        printf("%d %d ", players[i].onball, players[i].gotball);
        printf("%d ", players[i].distance);
        printf("%d %d\n", players[i].timesonball, players[i].timesgotball);
    }
}

/**
 * Executes a single round of the training match
 */
void field_execute_rounds(int cur_round, player* players) {
    field_ball_phase(&(players[BALL_ID].position));
    field_update_phase(players);
    field_print_phase(cur_round, players);
}

/**
 * Entry point for field process application
 */
void field_main() {
    int roundsleft = ROUNDS;
    player* players = (player*)calloc((NUM_PLAYERS + 1), sizeof(player));
    field_init_players(players);
    srand(time(NULL));
    while (roundsleft != 0) {
        roundsleft--;
        field_execute_rounds(ROUNDS-roundsleft, players);
    }

    // free memory
    free(players);
}
