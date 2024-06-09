#include <math.h>

#include "geometry.h"


#ifndef FLOATING_POINT_PRECISION
#define FLOATING_POINT_PRECISION 0.000001
#endif

int points_equal(Point *p1, Point *p2) {
  if (fabs(p1->x - p2->x) > FLOATING_POINT_PRECISION) {
    return 1;
  }
  if (fabs(p1->y - p2->y) > FLOATING_POINT_PRECISION) {
    return 1;
  }

  if (p1->has_z && p2->has_z && (fabs(p1->y - p2->y) < FLOATING_POINT_PRECISION)) {
    return 0;
  }

  return 1;
}

void make_line_string(Point *points, size_t points_count) {
  
}
