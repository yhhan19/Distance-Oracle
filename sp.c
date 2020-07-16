#include "utils.h"

sp_vector *new_sp_vector(int node_count) {
    sp_vector *sp = (sp_vector *) malloc(sizeof(sp_vector));
    sp->node_count = node_count;
    sp->parent = (node **) malloc(sizeof(node *) * node_count);
    sp->dist = (double *) malloc(sizeof(double) * node_count);
    sp->type = (int *) malloc(sizeof(int) * node_count);
    return sp;
}

void init_sp_vector(sp_vector *sp) {
    sp->type_count = 0;
    int i;
    for (i = 0; i < sp->node_count; i ++) {
        sp->parent[i] = NULL;
        sp->dist[i] = -1;
        sp->type[i] = -1;
    }
    sp->heap = NULL;
}

void free_sp_vector(sp_vector *sp) {
    free(sp->parent);
    free(sp->dist);
    free(sp->type);
    assert(sp->heap == NULL);
    free(sp);
}

void update_sp_vector(sp_vector *sp, node *n0, node *n, double temp)
{
    sp->parent[n0->node2ind] = n;
    sp->dist[n0->node2ind] = temp;
    if (n == NULL) {

    }
    else if (sp->parent[n->node2ind] == NULL) {
        if (sp->type[n0->node2ind] == -1)
            sp->type[n0->node2ind] = sp->type_count ++;
    }
    else {
        if (sp->type[n0->node2ind] == -1)
            sp->type[n0->node2ind] = sp->type[n->node2ind];
    }
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
    int i = 0;
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
        i ++;
    }
    assert(sp->heap == NULL);
}

bst_node *bounded_shortest_paths_from(network *net, node *source, double bound) {
    bst_node *root = BST_NULL;
    heap_node *heap = new_heap_node(source, 0);
    while (heap != NULL && heap->key < bound) {
        node *n = heap->n;
        double key = heap->key;
        n->active = NULL;
        heap = extract_min(heap);
        root = bst_insert(root, n);
        adjacent_node *p = net->adjacent_lists[n->node2ind];
        while (p != NULL) {
            edge *e = p->e;
            double temp = key + e->weight;
            if (bst_search(root, e->to->node2id) == NULL) {
                if (e->to->active == NULL) {
                    e->to->active = new_heap_node(e->to, temp);
                    heap = heap_merge(heap, (heap_node *) (e->to->active));
                }
                else {
                    double d = ((heap_node *) e->to->active)->key;
                    if (temp < d) {
                        heap = decrease_key(heap, (heap_node *) (e->to->active), temp);
                    }
                }
            }
            p = p->next;
        }
    }
    while (heap != NULL) {
        node *n = heap->n;
        n->active = NULL;
        heap = extract_min(heap);
    }
    assert(heap == NULL);
    return root;
}

void shortest_paths(network *net) {
    clock_t time = clock();
    FILE *spout = fopen("sp.txt", "wb");
    sp_vector *sp = new_sp_vector(net->node_count);
    int i, percent = 0;
    for (i = 0; i < net->node_count; i ++) {
        init_sp_vector(sp);
        node *n = net->ind2node[i];
        if (n->color == net->greatest_component) {
            shortest_paths_from(net, sp, n);
            fwrite(sp->dist, sizeof(double), net->node_count, spout);
        }
        if (i * 100 >= net->node_count * percent) {
            printf("\rsp: %3d%% [compute & write]", ++ percent);
        }
    }
    printf("\n");
    time = clock() - time;
    printf("sp: %d (ms) [compute & write]\n", time);
    free_sp_vector(sp);
    fclose(spout);
}

void check_shortest_paths(network *net) {
    clock_t time = clock();
    FILE *spin = fopen("sp.txt", "rb");
    sp_vector *sp = new_sp_vector(net->node_count);
    double *dist = (double *) malloc(sizeof(double) * net->node_count);
    int i, j, percent = 0;
    for (i = 0; i < net->node_count; i ++) {
        init_sp_vector(sp);
        node *n = net->ind2node[i];
        if (n->color == net->greatest_component) {
            shortest_paths_from(net, sp, n);
            fread(dist, sizeof(double), net->node_count, spin);
            for (j = 0; j < net->node_count; j ++) {
                assert(dist[j] == sp->dist[j]);
            }
        }
        if (i * 100 >= net->node_count * percent) {
            printf("\rsp: %3d%% [read & check]", ++ percent);
        }
    }
    printf("\n");
    time = clock() - time;
    printf("sp: %d (ms) [read & check]\n", time);
    free_sp_vector(sp);
    fclose(spin);
}

float **read_shortest_paths(network *net) {
    float **ret = (float **) malloc(sizeof(float *) * net->node_count);
    clock_t time = clock();
    FILE *spin = fopen("sp.txt", "rb");
    sp_vector *sp = new_sp_vector(net->node_count);
    double *dist = (double *) malloc(sizeof(double) * net->node_count);
    int i, j, percent = 0;
    for (i = 0; i < net->node_count; i ++) {
        ret[i] = NULL;
        init_sp_vector(sp);
        node *n = net->ind2node[i];
        if (n->color == net->greatest_component) {
            ret[i] = (float *) malloc(sizeof(float) * net->node_count);
            fread(dist, sizeof(double), net->node_count, spin);
            for (j = 0; j < net->node_count; j ++) 
                ret[i][j] = (float) dist[j];
        }
        if (i * 100 >= net->node_count * percent) {
            printf("\rsp: %3d%% [read]", ++ percent);
        }
    }
    printf("\n");
    time = clock() - time;
    printf("sp: %d (ms) [read]\n", time);
    free_sp_vector(sp);
    fclose(spin);
    return ret;
}

void free_matrix(float **a, int N) {
    int i;
    for (i = 0; i < N; i ++) {
        if (a[i] != NULL) free(a[i]);
    }
    free(a);
}

node *nearest_neighbor(network *net, node *n) {
    int i;
    double min = -1;
    node *argmin = NULL;
    for (i = 0; i < net->node_count; i ++) {
        node *n0 = net->ind2node[i];
        if (n0->color == net->greatest_component) {
            double temp = sphere_dist(n0, n);
            if (argmin == NULL || temp < min) {
                min = temp;
                argmin = n0;
            }
        }
    }
    return argmin;
}

double network_distance_rad(network *net, node *source, node *target) {
    node *s0 = nearest_neighbor(net, source);
    node *t0 = nearest_neighbor(net, target);
    sp_vector *sp = new_sp_vector(net->node_count);
    init_sp_vector(sp);
    shortest_paths_from(net, sp, s0);
    printf("distance: %lf (m) [great-circle] ", sphere_dist(s0, t0));
    printf("%lf (m) [in-network]\n", sp->dist[t0->node2ind]);
    double ret =  sp->dist[t0->node2ind];
    free_sp_vector(sp);
    return ret;
}

double network_distance_deg(network *net, 
    double lat0, double lon0, double lat1, double lon1) 
{
    node *source = new_node(0, lat0 * PI / 180, lon0 * PI / 180);
    node *target = new_node(0, lat1 * PI / 180, lon1 * PI / 180);
    node *s0 = nearest_neighbor(net, source);
    node *t0 = nearest_neighbor(net, target);
    free_node(source);
    free_node(target);
    sp_vector *sp = new_sp_vector(net->node_count);
    init_sp_vector(sp);
    shortest_paths_from(net, sp, s0);
    printf("distance: %lf (m) [great-circle] ", sphere_dist(s0, t0));
    printf("%lf (m) [in-network]\n", sp->dist[t0->node2ind]);
    double ret =  sp->dist[t0->node2ind];
    free_sp_vector(sp);
    return ret;
}
