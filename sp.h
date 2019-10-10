#ifndef _SP_H_
#define _SP_H_

#include "heap.h"
#include "network.h"

typedef struct sp_vector {
    int node_count;
    double *dist;
    node **parent;
    heap_node *heap;
} sp_vector;

sp_vector *new_sp_vector(int node_count) {
    sp_vector *sp = (sp_vector *) malloc(sizeof(sp_vector));
    sp->node_count = node_count;
    sp->parent = (node **) malloc(sizeof(node *) * node_count);
    sp->dist = (double *) malloc(sizeof(double) * node_count);
    return sp;
}

void init_sp_vector(sp_vector *sp) {
    int i;
    for (i = 0; i < sp->node_count; i ++) {
        sp->parent[i] = NULL;
        sp->dist[i] = -1;
    }
    sp->heap = NULL;
}

void free_sp_vector(sp_vector *sp) {
    free(sp->parent);
    free(sp->dist);
    assert(sp->heap == NULL);
    free(sp);
}

void update_sp_vector(sp_vector *sp, node *n0, node *n, double temp)
{
    sp->parent[n0->node2ind] = n;
    sp->dist[n0->node2ind] = temp;
    if (n0->active != NULL) {
        sp->heap = decrease_key(sp->heap, (heap_node *) (n0->active), temp);
    }
    else {
        n0->active = new_heap_node(n0, temp);
        sp->heap = heap_merge(sp->heap, (heap_node *) (n0->active));
    }
}

node *extract_sp_vector(sp_vector *sp) {
    node *n = sp->heap->n;
    n->active = NULL;
    sp->heap = extract_min(sp->heap);
    return n;
}

void shortest_paths_from(network *net, sp_vector *sp, node *source) {
    update_sp_vector(sp, source, NULL, 0);
    while (sp->heap != NULL) {
        node *n = extract_sp_vector(sp);
        double key = sp->dist[n->node2ind];
        adjacent_node *p = net->adjacent_lists[n->node2ind];
        while (p != NULL) {
            edge *e = p->e;
            double temp = key + e->weight;
            if (sp->dist[e->to->node2ind] < 0 
            || temp < sp->dist[e->to->node2ind]) 
                update_sp_vector(sp, e->to, n, temp);
            p = p->next;
        }
    }
    assert(sp->heap == NULL);
}

void shortest_paths(network *net) {
    int i;
    sp_vector *sp = new_sp_vector(net->node_count);
    for (i = 0; i < net->node_count; i ++) {
        init_sp_vector(sp);
        node *n = net->ind2node[i];
        if (n->color == net->greatest_component) {
            shortest_paths_from(net, sp, n);
        }
    }
    free_sp_vector(sp);
}

node *nearest_neighbor(network *net, node *n) {
    int i;
    double min = -1;
    node *argmin = NULL;
    for (i = 0; i < net->node_count; i ++) {
        node *n0 = net->ind2node[i];
        if (n0->color == net->greatest_component) {
            double temp = dist(n0, n);
            if (argmin == NULL || temp < min) {
                min = temp;
                argmin = n0;
            }
        }
    }
    printf("%lf\n", min);
    return argmin;
}

double network_distance(network *net, 
    double lat0, double lon0, double lat1, double lon1) 
{
    node *source = new_node(0, lat0, lon0);
    node *target = new_node(0, lat1, lon1);
    node *s0 = nearest_neighbor(net, source);
    node *t0 = nearest_neighbor(net, target);
    free_node(source);
    free_node(target);
    sp_vector *sp = new_sp_vector(net->node_count);
    init_sp_vector(sp);
    shortest_paths_from(net, sp, s0);
    printf("great-circle distance: %lf (m)\n", dist(s0, t0));
    printf("network distance: %lf (m)\n", sp->dist[t0->node2ind]);
    return sp->dist[t0->node2ind];
}

#endif
