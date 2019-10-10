#ifndef _ELEMENTARY_H_
#define _ELEMENTARY_H_

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MAX_nodes ((int) 1e7)
#define EARTH_RADIUS 6371009
#define PI 3.14159265359

typedef struct node {
    long long node2id;
    int node2ind, color, in, out;
    void *active;
    double lat, lon;
} node;

typedef struct edge {
    long long way;
    struct node *from, *to;
    double weight;
} edge;

int node_count, edge_count;

node *new_node(long long id, double lat, double lon) {
    node *n = (node *) malloc(sizeof(node));
    node_count ++;
    n->node2id = id;
    n->lat = lat * PI / 180;
    n->lon = lon * PI / 180;
    n->in = n->out = 0;
    n->active = NULL;
    return n;
}

void free_node(node *n) {
    free(n);
    node_count --;
}

double dist(node *a, node *b) {
	//spherical law of cosines
	double t = sin(a->lat) * sin(b->lat) 
        + cos(a->lat) * cos(b->lat) * cos(a->lon - b->lon);
	assert(t <= 1);
	return EARTH_RADIUS * sqrt(2 - 2 * t);
}

edge *new_edge(long long way, node *from, node *to) {
    edge *e = (edge *) malloc(sizeof(edge));
    edge_count ++;
    e->way = way;
    e->from = from;
    e->to = to;
    e->weight = dist(from, to);
    return e;
}

void free_edge(edge *e) {
    free(e);
    edge_count --;
}

#endif
