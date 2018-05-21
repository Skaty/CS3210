#include <stdlib.h>
#include "utils.h"
/**
 * Utilities - helper functions
 */

/**
 * Given an integer, this method returns the sign of the integer
 * @param  num [description]
 * @return     -1 if num is negative, 1 if num is positive
 */
int get_sign(int num) {
    return (num > 0) - (num < 0);
}

/**
 * Determines if two positions are exactly equal.
 */
int is_positions_equal(coordinates pos1, coordinates pos2) {
    return (pos1.x == pos2.x) && (pos1.y == pos2.y);
}

/**
 * Copies the position from source to destination
 */
void copy_position(coordinates* src, coordinates* dst) {
    dst->x = src->x;
    dst->y = src->y;
}

/**
 * Gets the Manhattan distance between two coordinates.
 */
int distance(coordinates* pos1, coordinates* pos2) {
    return abs(pos1->x - pos2->x) + abs(pos1->y - pos2->y);
}

/**
 * Systematic method of calculating initial coordinates based on the
 * player's rank
 */
void init_coordinates_for_rank(coordinates* res, int rank, int xcount, int ycount) {
    int xspacing = PITCH_LENGTH / xcount;
    int yspacing = PITCH_WIDTH / ycount;
    res->x = ((rank / ycount) * xspacing) % PITCH_LENGTH;
    res->y = (rank * yspacing) % PITCH_WIDTH;
}

/**
 * Retrieves the field rank from
 * a given ball coordinates
 */
int get_field_rank_from_coordinates(coordinates* coord) {
    int offsetX = coord->x / 32;
    int offsetY = coord->y / 32;

    return (offsetY * 4) + offsetX;
}

/**
 * Advances entity at src towards dst. Returns the final coordinates, taking
 * into account the dst coordinates and the maximum distance that the entity can travel.
 * @param result  a pointer where the return value can be written to
 * @param src     the current coordinates of the entity
 * @param dst     the destination coordinates
 * @param maxdist maximum distance that the entity can travel
 */
int run_towards_coordinates(coordinates* result, coordinates* src, coordinates* dst, int maxdist) {
    int delx = (int)(dst->x) - (int)(src->x);
    int dely = (int)(dst->y) - (int)(src->y);
    int remainingdist = maxdist;

    // Move x-wards first before y-wards.
    int actualdelx = get_sign(delx) * min(abs(delx), remainingdist);
    remainingdist -= abs(actualdelx);
    int actualdely = get_sign(dely) * min(abs(dely), remainingdist);

    result->x = (unsigned char)((int)(src->x) + actualdelx);
    result->y = (unsigned char)((int)(src->y) + actualdely);

    return maxdist - remainingdist;
}
