#ifndef UTILS_H
#define UTILS_H

#include "datastructs.h"

#define max(a,b) \
  ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b; })

#define min(a,b) \
  ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b; })

void init_coordinates_for_rank(coordinates* res, int rank, int xcount, int ycount);
int is_positions_equal(coordinates pos1, coordinates pos2);
void copy_position(coordinates* src, coordinates* dst);
int distance(coordinates* pos1, coordinates* pos2);
int run_towards_coordinates(coordinates* result, coordinates* src, coordinates* dst, int maxdist);
int get_field_rank_from_coordinates(coordinates* coord);

#endif
