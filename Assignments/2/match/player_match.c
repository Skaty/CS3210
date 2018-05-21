#include <mpi.h>
#include <stdlib.h>
#include <time.h>

#include "datastructs.h"
#include "utils.h"
#include "consts_match.h"

int plyrank = 0;
coordinates ball = {0, 0};
coordinates mycoords = {0, 0};
coordinates goal = {0, 47};
coordinates initial_position = {0, 0};
playerpacket packetbuffer;

// Player attributes
playerattr attributes = {5, 5, 5};

MPI_Comm fieldcomms[NUM_FIELDS];
MPI_Comm playercomms;

/**
 * Instantiate the player process: communicators and goal location. Only runs once.
 */
void player_init() {
    int i;
    MPI_Comm dummycomm;

    // Init PRNG
    srand(time(NULL) + plyrank);

    if (plyrank < TEAM_B_START_RANK) {
        // in Team A
        goal.x = 127;
    }
    // Ignore field-only split
    MPI_Comm_split(MPI_COMM_WORLD, MPI_UNDEFINED, MPI_UNDEFINED, &dummycomm);

    for (i = 0; i < NUM_FIELDS; i++) {
        MPI_Comm_split(MPI_COMM_WORLD, i, plyrank, &(fieldcomms[i]));
    }

    MPI_Comm_split(MPI_COMM_WORLD, plyrank % TEAM_B_START_RANK, plyrank, &playercomms);

    // Init attributes
    attributes.speed = (rand() % 10) + 1;
    attributes.dribbling = (rand() % min(10, 15 - attributes.speed - 1)) + 1;
    attributes.kick = 15 - attributes.speed - attributes.dribbling;
}

/**
 * Main logic on what the player should do next.
 */
int player_perform_move() {
    int balldist = distance(&mycoords, &ball);
    int balldistfromhome = distance(&ball, &initial_position);
    run_towards_coordinates(&(packetbuffer.position), &mycoords, &ball, attributes.speed);

    if (is_positions_equal(packetbuffer.position, ball) == 1) {
        // Ball is reachable. Kick it towards goal.
        run_towards_coordinates(&(packetbuffer.ballpos), &ball, &goal, 2 * attributes.kick);
        packetbuffer.challenge = ((rand() % 10) + 1) * attributes.dribbling;
    } else {
        packetbuffer.challenge = -1;
    }

    if (attributes.speed * 2 < balldist && attributes.speed * 2 < balldistfromhome) {
        // Player too slow and thus, might not be competitive
        copy_position(&mycoords, &(packetbuffer.position));
    }

    copy_position(&(packetbuffer.position), &mycoords);

    return 0;
}

/**
 * Send new information to relevant field
 */
void player_gather_field() {
    int field_idx;
    int posfield_idx = get_field_rank_from_coordinates(&(packetbuffer.position));
    int pktsize = sizeof(playerpacket);
    for (field_idx = 0; field_idx < NUM_FIELDS; field_idx++) {
        packetbuffer.isvalid = (posfield_idx == field_idx);
        MPI_Gather(&packetbuffer, pktsize, MPI_BYTE, NULL, pktsize, MPI_BYTE, 0, fieldcomms[field_idx]);
    }
}

/**
 * Executes all relevant procedures for a single round.
 */
void player_execute_rounds() {
    int roundsleft = HALVE_ROUNDS;
    while (roundsleft != 0) {
        roundsleft--;
        MPI_Bcast(&ball, sizeof(coordinates), MPI_BYTE, 0, MPI_COMM_WORLD);
        player_perform_move();
        player_gather_field();
    }
}

/**
 * Instantiates player's initial position
 */
void player_init_pos() {
    init_coordinates_for_rank(&mycoords, plyrank - NUM_FIELDS, NUM_PLAYERS_WIDTH, NUM_PLAYERS_HEIGHT);
    copy_position(&mycoords, &initial_position);
}

/**
 * Entry point for all player processes
 */
void player_main(int rank) {
    plyrank = rank;
    int halves = NUM_HALVES;
    player_init();

    while (halves != 0) {
        halves--;
        player_init_pos();
        player_execute_rounds();
    }
}
