#ifndef DATASTRUCTS_H
#define DATASTRUCTS_H

#define PITCH_WIDTH 96
#define PITCH_LENGTH 128

/**
 * Represents a coordinate
 */
typedef struct coords {
    unsigned char x;
    unsigned char y;
} coordinates;

/**
 * Used mainly in training to store state
 * of the game
 */
typedef struct ply {
    coordinates previous;
    coordinates position;
    int onball;
    int gotball;
    int distance;
    int timesonball;
    int timesgotball;
} player;

/**
 * Stores player's attributes
 */
typedef struct plyattr {
    int speed;
    int dribbling;
    int kick;
} playerattr;

/**
 * A structure used to send/receive information about players
 * Used only in the match version.
 */
typedef struct plypkt {
    unsigned char isvalid;
    coordinates position;
    coordinates ballpos;
    short challenge;
} playerpacket;

#endif
