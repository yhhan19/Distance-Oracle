#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define SW 0
#define SE 1
#define NW 2
#define NE 3
#define WHITE 0
#define BLACK 1
#define GRAY 2
#define SQRT2 1.4142135623731
#define INF 2147483647
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))
#define ABS(a) (((a)>0)?(a):(-(a)))
#define CLEN 64
#define DEP 30
#define MAX_nodes ((int) 1e7)
#define EARTH_RADIUS 6371009.0
#define PI 3.14159265359
#define MAX_string_buffer_len 65536
#define MAX_pointer_buffers 8

char string_buffer[MAX_string_buffer_len];
void ***pointer_buffers;
int *avail_pointer_buffer, max_buffers;

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

typedef struct adjacent_node {
    edge *e;
    struct adjacent_node *next;
} adjacent_node;

typedef struct network {
    int node_count, edge_count, greatest_component;
    node **ind2node;
    struct bst_node *id2node;
    struct adjacent_node **adjacent_lists;
} network;

typedef struct heap_node {
    struct node *n;
    double key;
    struct heap_node *head, *next, *prev;
} heap_node;

typedef struct bst_node {
    struct node *n;
    struct bst_node *left, *right;
    int h, s;
} bst_node;

typedef struct sp_vector {
    int node_count, type_count;
    double *dist;
    node **parent;
    int *type;
    heap_node *heap;
} sp_vector;

bst_node *BST_NULL;
int bst_node_count, node_count, edge_count, 
    heap_node_count, adjacent_node_count;

typedef struct quad_node {
    struct quad_node *child[4];
    int type, depth, size;
    long long code;
    node p, q, *data;
} quad_node;

typedef struct pair {
    struct quad_node *u, *v;
} pair;

typedef struct wspd {
    quad_node *root;
    pair *list;
    int size, tail, depth;
    double s;
} wspd;

double sphere_dist(node *a, node *b);
sp_vector *new_sp_vector(int node_count);
void init_sp_vector(sp_vector *sp);
void free_sp_vector(sp_vector *sp);
void update_sp_vector(sp_vector *sp, node *n0, node *n, double temp);
node *extract_sp_vector(sp_vector *sp);
void shortest_paths_from(network *net, sp_vector *sp, node *source);
void shortest_paths(network *net);
node *nearest_neighbor(network *net, node *n);
double network_distance(network *net, 
    double lat0, double lon0, double lat1, double lon1);
node *new_node(long long id, double lat, double lon);
void free_node(node *n);
edge *new_edge(long long way, node *from, node *to);
void free_edge(edge *e);
void init_network();
void clear_network();
adjacent_node *new_adjacent_node(edge *e);
void free_adjacent_node(adjacent_node *adj);
void new_nodes_from(network *net, FILE *fin);
void connect_nodes(network *net, long long way, node *from, node *to);
void new_edges_from(network *net, FILE *fin);
adjacent_node *match_edge(network *net, node *n0, node *n1);
int traverse_network_from(network *net, node *source, node **queue);
void mark_components(network *net);
void simplify(network *net);
network *new_network_from(const char *name);
void free_network(network *net);
void init_heap();
void clear_heap();
heap_node *new_heap_node(struct node *n, double key);
void free_heap_node(heap_node *h);
heap_node *heap_merge(heap_node *h0, heap_node *h1);
heap_node *extract_min(heap_node *h);
heap_node *decrease_key(heap_node *h0, heap_node *h1, double key);
void init_pointer_buffers();
void clear_pointer_buffers();
void** new_buffer();
void free_buffer(void ** buffer);
bst_node *new_bst_node(struct node *n);
void free_bst_node(bst_node *b);
void init_bst();
void clear_bst();
bst_node *new_bst();
void traverse_bst(bst_node *root, bst_node **queue, int *tail);
void free_bst(bst_node *root);
void update(bst_node *root);
bst_node *rotate_right(bst_node *root);
bst_node *rotate_left(bst_node *root);
bst_node *re_balance(bst_node *root);
bst_node *bst_insert(bst_node *root, node *n);
bst_node *bst_search(bst_node *root, long long id);
quad_node *new_quad_node(wspd *w, quad_node *f, int i);
int quad_contain(quad_node *r, node *p);
int id(quad_node *n, node *data);
int quad_insert(wspd *w, quad_node *n, node *data);
quad_node *quad_search(quad_node *n, node *data);
void free_quad(quad_node *root);
int pair_contain(pair *p, node *a, node *b);
int pr_z4_code(char *ret, pair *p);
void pt_z4_code(wspd *w, char *ret, node *p, node *q, int len);
int z4_code_cmp(char *a, char *b);
int cmp(wspd *w, node *p, node *q, pair *pr);
pair *bin_search(wspd *w, node *p, node *q);
void add_pair(wspd *w, quad_node *u, quad_node *v);
double diameter(quad_node *u);
double node_dist(quad_node *u, quad_node *v);
int well_separated(quad_node *u, quad_node *v, double s);
int level(quad_node *u);
void build(wspd *w, quad_node *u, quad_node *v);
double check_query(wspd *w, node *a, node *b);
double check_wspd(wspd *w, network *net);
wspd *new_wspd(double eps, network *net);
void free_wspd(wspd *w);
void init();
void clear();
void test();

#endif