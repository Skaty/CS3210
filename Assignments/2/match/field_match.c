#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "datastructs.h"
#include "utils.h"
#include "consts_match.h"

// Global variables -- prevents excessive passing
// of commonly used variables
int field_rank = -1, playerKicked;
playerpacket* prevpositions = NULL;
playerpacket* players = NULL;

// Score for Team A and Team B respectively
int score[2] = {0, 0};

MPI_Comm fieldcomm, playercomm;

/**
 * Contains code for a field process
 * in the match.
 */
playerpacket* field_malloc_players() {
    int array_sz = NUM_PLAYERS + 1;
    if (field_rank == 0) {
        array_sz = NUM_FIELDS * (NUM_PLAYERS + 1);
    }

    return (playerpacket*)malloc(array_sz * sizeof(playerpacket));
}

/**
 * Instantiates the positions of the players
 */
void field_init_players() {
    int i;

    for (i = 1; i <= NUM_PLAYERS; i++) {
        init_coordinates_for_rank(&(players[i].position), i-1, 8, 3);
    }
}

/**
 * Instantiate the field process.
 * Instantiates the following: communicators & heap memory for arrays
 */
void field_init() {
    MPI_Comm_split(MPI_COMM_WORLD, 0, field_rank, &fieldcomm);
    int i;
    MPI_Comm dummycomm;

    for (i = 0; i < NUM_FIELDS; i++) {
        if (i == field_rank) {
            MPI_Comm_split(MPI_COMM_WORLD, i, 0, &playercomm);
        } else {
            MPI_Comm_split(MPI_COMM_WORLD, MPI_UNDEFINED, MPI_UNDEFINED, &dummycomm);
        }
    }

    // ignore split by players
    MPI_Comm_split(MPI_COMM_WORLD, MPI_UNDEFINED, MPI_UNDEFINED, &dummycomm);

    if (field_rank == 0) {
        prevpositions = (playerpacket*)malloc((NUM_PLAYERS + 1) * sizeof(playerpacket));
    }

    players = field_malloc_players();
}

/**
 * Gather information from players
 */
void field_gather_from_players() {
    int pkt_sz = sizeof(playerpacket);
    MPI_Gather(players, pkt_sz, MPI_BYTE, players, pkt_sz, MPI_BYTE, 0, playercomm);
}

/**
 * Gathers information about players from
 * each field process. Only run by master.
 */
void field_master_gather() {
    int packetsize = (NUM_PLAYERS + 1) * sizeof(playerpacket);
    MPI_Gather(players, packetsize, MPI_BYTE, players, packetsize, MPI_BYTE, 0, fieldcomm);
}

/**
 * Master processing method. After all information
 * is gathered from field processes, this method
 * will collate the information and determine the 'ball winner'.
 */
void field_master_execute(coordinates* ball) {
    int fieldIdx, playerIdx, offset = 0;
    int maxChallenge = -1;
    playerKicked = -1;

    for (fieldIdx = 0; fieldIdx < NUM_FIELDS; fieldIdx++) {
        for (playerIdx = 1; playerIdx <= NUM_PLAYERS; playerIdx++) {
            offset++;
            if (players[offset].isvalid == 1) {
                memcpy(&(players[playerIdx]), &(players[offset]), sizeof(playerpacket));
                if (players[playerIdx].challenge > maxChallenge) {
                    copy_position(&(players[playerIdx].ballpos), ball);
                    playerKicked = playerIdx;
                    maxChallenge = players[playerIdx].challenge;
                }
            }
        }

        // First item in each field's array is invalid
        // Only there because of MPI Gather.
        offset++;
    }
}

/**
 * Determines if a goal has been scored by
 * a player.
 */
void field_check_ball_goal(coordinates* ball) {
    if ((ball->x == 0 || ball->x == 127) && (ball->y >= 43 && ball->y <= 51)) {
        ball->x = 64;
        ball->y = 32;
        if (playerKicked <= NUM_PLAYERS/2) {
            score[0] += 1;
        } else {
            score[1] += 1;
        }
    }
}

/**
 * Prints out round statistics. Run by master process.
 */
void field_print_status(coordinates* ball, int roundId) {
    int playerIdx = 0;
    printf("%d\n", roundId);
    printf("%d %d\n", ball->x, ball->y);

    for (playerIdx = 1; playerIdx <= NUM_PLAYERS; playerIdx++) {
        int reachedBall = (players[playerIdx].challenge != -1);
        int kickedBall = (playerIdx == playerKicked);
        printf("%d ", playerIdx - 1);
        printf("%d %d ", prevpositions[playerIdx].position.x, prevpositions[playerIdx].position.y);
        printf("%d %d ", players[playerIdx].position.x, players[playerIdx].position.y);
        printf("%d %d %d\n", reachedBall, kickedBall, players[playerIdx].challenge);
    }
}

/**
 * Executes code for one round of the match.
 * All phases included.
 */
void field_execute_rounds(int roundOffset) {
    // Plan:
    // 1. Master field broadcasts ball coordinates
    // 2. Every field process gathers info from
    //    each team about the players that landed there
    // 3. Field 0 calls gather on all field processes.
    // 4. Field 0 does calculation for ball possessions
    coordinates ball = {64, 32};
    int current_round = 0;

    for (current_round = 1; current_round <= HALVE_ROUNDS; current_round++) {
        if (field_rank == 0) {
            field_check_ball_goal(&ball);
            memcpy(prevpositions, players, (NUM_PLAYERS + 1) * sizeof(playerpacket));
        }

        MPI_Bcast(&ball, sizeof(coordinates), MPI_BYTE, 0, MPI_COMM_WORLD);

        field_gather_from_players();
        field_master_gather();
        if (field_rank == 0) {
            field_master_execute(&ball);
            field_print_status(&ball, roundOffset + current_round);
        }
    }
}

/**
 * Entry point for field processes.
 */
void field_main(int rank) {
    field_rank = rank;
    int halves = NUM_HALVES;
    field_init();

    while (halves != 0) {
        int round_offset = (NUM_HALVES - halves) * HALVE_ROUNDS;
        halves--;
        if (field_rank == 0) {
            field_init_players();
        }
        field_execute_rounds(round_offset);
    }

    free(players);
    if (prevpositions != NULL) {
        free(prevpositions);
    }
}
