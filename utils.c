#include "utils.h"

double sphere_dist(node *a, node *b) {
	//spherical law of cosines
	double t = sin(a->lat) * sin(b->lat) 
        + cos(a->lat) * cos(b->lat) * cos(a->lon - b->lon);
	if (t > 1) t = 1;
	return EARTH_RADIUS * sqrt(2 - 2 * t);
}
