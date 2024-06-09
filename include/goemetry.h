#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <stdbool.h>

typedef struct {
  double x;
  double y;
  double z;
  bool has_z;
} Point;

typedef struct {
  Point *points;
  size_t points_count;
  bool is_closed;
} LineString;

#endif

