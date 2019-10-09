#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MAX_string_buffer_len 65536
#define MAX_pointer_buffers 8
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

typedef struct network {
    int node_count, edge_count, greatest_component;
    node **ind2node;
    struct bst_node *id2node;
    struct adjacent_node **adjacent_lists;
} network;

typedef struct adjacent_node {
    edge *e;
    struct adjacent_node *next;
} adjacent_node;

typedef struct bst_node {
    node *n;
    struct bst_node *left, *right;
    int h, s;
} bst_node;

typedef struct heap_node {
    node *n;
    double key;
    struct heap_node *head, *next, *prev;
} heap_node;

typedef struct sp_vector {
    double *dist;
    node **parent;
    network *net;
} sp_vector;

bst_node *BST_NULL;
void ***pointer_buffers;
int *avail_pointer_buffer, max_buffers,
    bst_node_count, heap_node_count,
    node_count, edge_count, adjacent_node_count;
char string_buffer[MAX_string_buffer_len];

void init_pointer_buffers() {
    max_buffers = 0;
    avail_pointer_buffer = (int *) malloc(
        sizeof(int) * MAX_pointer_buffers);
    pointer_buffers = (void ***) malloc(
        sizeof(void **) * MAX_pointer_buffers);
    int i;
    for (i = 0; i < MAX_pointer_buffers; i ++) {
        pointer_buffers[i] = (void **) malloc(
            sizeof(void *) * MAX_nodes);
        avail_pointer_buffer[i] = 1;
    }
}

void clear_pointer_buffers() {
    int i;
    for (i = 0; i < MAX_pointer_buffers; i ++) {
        assert(avail_pointer_buffer[i]);
        free(pointer_buffers[i]);
    }
    free(pointer_buffers);
}

void** new_buffer() {
    int i;
    int temp = 0;
    for (i = 0; i < MAX_pointer_buffers; i ++)
        if (! avail_pointer_buffer[i])
            temp ++;
    if (temp > max_buffers) max_buffers = temp;
    for (i = 0; i < MAX_pointer_buffers; i ++)
        if (avail_pointer_buffer[i]) {
            avail_pointer_buffer[i] = 0;
            return pointer_buffers[i];
        }
    assert(0);
}

void free_buffer(void ** buffer) {
    int i;
    for (i = 0; i < MAX_pointer_buffers; i ++)
        if (buffer == pointer_buffers[i]) {
            avail_pointer_buffer[i] = 1;
            return ;
        }
    assert(0);
}

bst_node *new_bst_node(node *n) {
    bst_node *b = (bst_node *) malloc(sizeof(bst_node));
    bst_node_count ++;
    b->n = n;
    b->left = b->right = BST_NULL;
    b->h = 0;
    b->s = 1;
    return b;
}

void free_bst_node(bst_node *b) {
    free(b);
    bst_node_count --;
}

void init_bst() {
    bst_node_count = 0;
    BST_NULL = new_bst_node(NULL);
    BST_NULL->left = BST_NULL->right = BST_NULL;
    BST_NULL->h = -1;
    BST_NULL->s = 0;
}

void clear_bst() {
    free_bst_node(BST_NULL);
    assert(bst_node_count == 0);
}

bst_node *new_bst() {
    return BST_NULL;
}

void traverse_bst(bst_node *root, bst_node **queue, int *tail) {
    if (root == BST_NULL) 
        return ;
    traverse_bst(root->left, queue, tail);
    queue[(*tail) ++] = root;
    traverse_bst(root->right, queue, tail);
}

void free_bst(bst_node *root) {
    bst_node **queue = (bst_node **) new_buffer();
    int head, tail = 0;
    traverse_bst(root, queue, & tail);
    for (head = 0; head < tail; head ++) 
        free_bst_node(queue[head]);
    free_buffer((void **) queue);
}

void update(bst_node *root) {
    root->s = root->left->s + root->right->s + 1;
    if (root->left->h > root->right->h)
        root->h = root->left->h + 1;
    else
        root->h = root->right->h + 1;
}

bst_node *rotate_right(bst_node *root) {
    bst_node *temp = root->left;
    root->left = temp->right;
    temp->right = root;
    update(root); update(temp);
    return temp;
}

bst_node *rotate_left(bst_node *root) {
    bst_node *temp = root->right;
    root->right = temp->left;
    temp->left = root;
    update(root); update(temp);
    return temp;
}

bst_node *re_balance(bst_node *root) {
    if (root->left->h > root->right->h + 1) {
        if (root->left->left->h >= root->left->right->h)
            root = rotate_right(root);
        else {
            root->left = rotate_left(root->left);
            root = rotate_right(root);
        }
    }
    if (root->right->h > root->left->h + 1) {
        if (root->right->right->h >= root->right->left->h)
            root = rotate_left(root);
        else {
            root->right = rotate_right(root->right);
            root = rotate_left(root);
        }
    }
    return root;
}

bst_node *bst_insert(bst_node *root, node *n) {
    if (root == BST_NULL) {
        bst_node *temp = new_bst_node(n);
        return temp;
    }
    if (n->node2id < root->n->node2id)
        root->left = bst_insert(root->left, n);
    else
        root->right = bst_insert(root->right, n);
    update(root);
    root = re_balance(root);
    return root;
}

bst_node *bst_search(bst_node *root, long long id) {
    if (root == BST_NULL) 
        return NULL;
    if (id == root->n->node2id) 
        return root;
    if (id < root->n->node2id) 
        return bst_search(root->left, id);
    else 
        return bst_search(root->right, id);
}


void init_network() {
    node_count = edge_count = 0;
    adjacent_node_count = 0;
}

void clear_network() {
    assert(node_count == 0 && edge_count == 0);
    assert(adjacent_node_count == 0);
}

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

adjacent_node *new_adjacent_node(edge *e) {
    adjacent_node *adj = 
        (adjacent_node *) malloc(sizeof(adjacent_node));
    adjacent_node_count ++;
    adj->e = e;
    adj->next = NULL;
    return adj;
}

void free_adjacent_node(adjacent_node *adj) {
    free(adj);
    adjacent_node_count --;
}

void new_nodes_from(network *net, FILE *fin) {
    net->id2node = new_bst();
    char *s = string_buffer;
    s[0] = 0;
    while (strcmp(s, "<way") != 0) {
        fscanf(fin, "%s", s);
        long long id;
        double lat, lon;
        while (strcmp(s, "<way") != 0 && strcmp(s, "<node") != 0) {
            if (s[0] == 'i' && s[1] == 'd') 
                sscanf(s, "id=\"%lld\"", & id);
            if (s[0] == 'l' && s[1] == 'a' && s[2] == 't')  
                sscanf(s, "lat=\"%lf\"", & lat);
            if (s[0] == 'l' && s[1] == 'o' && s[2] == 'n')  
                sscanf(s, "lon=\"%lf\"", & lon);
            fscanf(fin, "%s", s);
        }
        node *n = new_node(id, lat, lon);
        net->id2node = bst_insert(net->id2node, n);
    }
    bst_node **queue = (bst_node **) new_buffer();
    int head = 0, tail = 0;
    traverse_bst(net->id2node, queue, & tail);
    net->node_count = net->id2node->s;
    net->ind2node = (node **) malloc(
        sizeof(node *) * net->node_count);
    for ( ; head < tail; head ++) {
        node *n = queue[head]->n;
        n->node2ind = head;
        net->ind2node[head] = n;
    }
    printf("network: %d nodes\n", net->node_count);
    free_buffer((void **) queue);
}

void connect_nodes(network *net, long long way, node *from, node *to) {
    edge *e = new_edge(way, from, to);
    adjacent_node *adj = new_adjacent_node(e);
    adj->next = net->adjacent_lists[from->node2ind];
    net->adjacent_lists[from->node2ind] = adj;
    net->edge_count ++;
    from->out ++;
    to->in ++;
}

void new_edges_from(network *net, FILE *fin) {
    net->adjacent_lists = (adjacent_node **) malloc(
        sizeof(adjacent_node *) * net->node_count);
    int i;
    for (i = 0; i < net->node_count; i ++) 
        net->adjacent_lists[i] = NULL;
    net->edge_count = 0;
    node **stack = (node **) new_buffer();
    int top;
    char *s = string_buffer;
    s[0] = 0;
    while (strcmp(s, "<relation") != 0) {
        fscanf(fin, "%s", s);
        int oneway = 0, highway = 0;
        long long way_id, node_id;
        top = 0;
        while (strcmp(s, "<relation") != 0 && strcmp(s, "<way") != 0) {
            if (s[0] == 'i' && s[1] == 'd') 
                sscanf(s, "id=\"%lld\"", & way_id);
            if (strcmp(s, "<nd") == 0) {
                fscanf(fin, "%s", s);
                sscanf(s, "ref=\"%lld\"", & node_id);
                bst_node *temp = bst_search(net->id2node, node_id);
                stack[top ++] = temp->n;
            }
            if (strcmp(s, "<tag") == 0) {
                fscanf(fin, "%s", s);
                highway |= strcmp(s, "k=\"highway\"") == 0;
                if (strcmp(s, "k=\"oneway\"") == 0) {
                    fscanf(fin, "%s", s);
                    oneway |= strcmp(s, "v=\"yes\"/>") != 0;
                }
            }
            fscanf(fin, "%s", s);
        }
        if (highway == 1) {
            while (top > 1) {
                node *to = stack[-- top];
                node *from = stack[top - 1];
                connect_nodes(net, way_id, from, to);
                if (oneway == 0) {
                    connect_nodes(net, way_id, to, from);
                }
            }
        }
    }
    printf("network: %d edges\n", net->edge_count);
    free_buffer((void **) stack);
}

int traverse_network_from(network *net, node *source, node **queue) {
    int head = 0, tail = 0;
    queue[tail ++] = source;
    source->active = (void *) 1;
    while (head < tail) {
        node *n = queue[head ++];
        adjacent_node *p = net->adjacent_lists[n->node2ind];
        while (p != NULL) {
            edge *e = p->e;
            node *n0 = e->to;
            if (n0->active == NULL) {
                n0->active = (void *) 1;
                queue[tail ++] = n0;
            }
            p = p->next;
        }
    }
    return tail;
}

void mark_components(network *net) {
    node **queue = (node **) new_buffer();
    int i, sum, max, argmax, count;
    for (i = 0, sum = 0, max = 0, count = 0; i < net->node_count; i ++) {
        if (net->ind2node[i]->active == NULL) {
            int tail = traverse_network_from(net, net->ind2node[i], queue);
            int j;
            for (j = 0; j < tail; j ++) 
                queue[j]->color = count;
            count ++;
            sum += tail;
            if (tail > max) {
                max = tail;
                argmax = i;
            }
        }
    }
    assert(sum == net->node_count);
    net->greatest_component = net->ind2node[argmax]->color;
    for (i = 0; i < net->node_count; i ++)
        net->ind2node[i]->active = NULL;
    printf("greatest component: %d nodes\n", max);
    free_buffer((void **) queue);
}

void simplify(network *net) {
    int i, count = 0;
    for (i = 0; i < net->node_count; i ++) {
        node *n = net->ind2node[i];
        if (n->color == net->greatest_component && 
            n->in == n->out && n->out == 2) 
        {
            node *a = net->adjacent_lists[i]->e->to;
            node *b = net->adjacent_lists[i]->next->e->to;
            if (a == b) continue;
            adjacent_node *p = net->adjacent_lists[a->node2ind];
            while (p != NULL) {
                edge *e = p->e;
                if (e->to == n) break;
                p = p->next;
            }
            adjacent_node *q = net->adjacent_lists[b->node2ind];
            while (q != NULL) {
                edge *e = q->e;
                if (e->to == n) break;
                q = q->next;
            }
            if (p != NULL && q != NULL) {
                p->e->to = b;
                q->e->to = a;
                n->color = -1;
                count ++;
            }
        }
    }
    printf("redundancy: %d nodes\n", count);
    for (i = 0; i < net->node_count; i ++) 
        if (net->ind2node[i]->color == net->greatest_component) 
            break;
    node **queue = (node **) new_buffer();
    int tail = traverse_network_from(net, net->ind2node[i], queue);
    printf("simplified: %d nodes\n", tail);
    free_buffer((void **) queue);
}

network *new_network_from(const char *name) {
    FILE *fin = fopen(name, "r");
    network *net = (network *) malloc(sizeof(network));
    new_nodes_from(net, fin);
    new_edges_from(net, fin);
    fclose(fin);
    mark_components(net);
    //simplify(net);
    printf("loaded\n");
    return net;
}

void free_network(network *net) {
    int i;
    for (i = 0; i < net->node_count; i ++) {
        adjacent_node *p = net->adjacent_lists[i];
        while (p != NULL) {
            adjacent_node *temp = p;
            p = p->next;
            free_edge(temp->e);
            free_adjacent_node(temp);
        }
    }
    free(net->adjacent_lists);
    for (i = 0; i < net->node_count; i ++) 
        free_node(net->ind2node[i]);
    free(net->ind2node);
    free_bst(net->id2node);
    free(net);
}

void init_heap() {
    heap_node_count = 0;
}

void clear_heap() {
    assert(heap_node_count == 0);
}

heap_node *new_heap_node(node *n, double key) {
    heap_node *h = (heap_node *) malloc(sizeof(heap_node));
    heap_node_count ++;
    h->n = n;
    h->key = key;
    h->head = NULL;
    h->next = h->prev = NULL;
    n->active = (void *) h;
    return h;
}

void free_heap_node(heap_node *h) {
    h->n->active = NULL;
    free(h);
    heap_node_count --;
}

heap_node *heap_merge(heap_node *h0, heap_node *h1) {
    if (h0 == NULL) 
        return h1;
    if (h1 == NULL) 
        return h0;
    if (h0->key < h1->key) {
        h1->next = h0->head;
        if (h0->head != NULL) 
            h0->head->prev = h1;
        h0->head = h1;
        h1->prev = h0;
        return h0;
    }
    else {
        h0->next = h1->head;
        if (h1->head != NULL) 
            h1->head->prev = h0;
        h1->head = h0;
        h0->prev = h1;
        return h1;
    }
}

heap_node *extract_min(heap_node *h) {
    heap_node *p = h->head, *q;
    free_heap_node(h);
    if (p == NULL) return NULL;
    heap_node **queue = (heap_node **) new_buffer();
    heap_node **stack = (heap_node **) new_buffer();
    int head = 0, tail = 0, top;
    while (p != NULL) {
        queue[tail ++] = p;
        p = p->next;
        q = queue[tail - 1];
        q->prev = q->next = NULL;
    }
    while (tail > 1) {
        for (top = 0; head < tail; ) {
            p = queue[head ++];
            q = queue[head ++];
            if (head == tail + 1) q = NULL;
            stack[top ++] = heap_merge(p, q);
        }
        head = tail = 0;
        while (top > 0) 
            queue[tail ++] = stack[-- top];
    }
    free_buffer((void **) queue);
    free_buffer((void **) stack);
    return queue[0];
}

heap_node *decrease_key(heap_node *h0, heap_node *h1, double key) {
    assert(key < h1->key);
    if (h1->prev == NULL) {
        assert(h0 == h1);
        h0->key = key;
        return h0;
    }
    else {
        if (h1->prev->next == h1) {
            h1->prev->next = h1->next;
            if (h1->next != NULL) 
                h1->next->prev = h1->prev;
        }
        else {
            assert(h1->prev->head == h1);
            h1->prev->head = h1->next;
            if (h1->next != NULL) 
                h1->next->prev = h1->prev;
        }
        h1->prev = h1->next = NULL;
        h1->key = key;
        return heap_merge(h0, h1);
    }
}

heap_node *heap_insert(heap_node *h0, node *n, double key) {
    heap_node *h1 = new_heap_node(n, key);
    return heap_merge(h0, h1);
}

void link_to_parent(sp_vector *sp, node *n0, node *n, double temp) 
{
    sp->parent[n0->node2ind] = n;
    sp->dist[n0->node2ind] = temp;
}

sp_vector *new_sp_vector(network *net) {
    sp_vector *sp = (sp_vector *) malloc(sizeof(sp_vector));
    sp->parent = (node **) malloc(sizeof(node *) * net->node_count);
    sp->dist = (double *) malloc(sizeof(double) * net->node_count);
    sp->net = net;
    return sp;
}

void init_sp_vector(sp_vector *sp) {
    int i;
    for (i = 0; i < sp->net->node_count; i ++) {
        sp->parent[i] = NULL;
        sp->dist[i] = -1;
    }
}

void free_sp_vector(sp_vector *sp) {
    free(sp->parent);
    free(sp->dist);
    free(sp);
}

void shortest_paths_from(
    network *net, node *source, sp_vector *sp) 
{
    heap_node *heap = NULL;
    link_to_parent(sp, source, NULL, 0);
    heap = heap_insert(heap, source, 0);
    while (heap != NULL) {
        node *n = heap->n;
        double key = heap->key;
        heap = extract_min(heap);
        adjacent_node *p = net->adjacent_lists[n->node2ind];
        while (p != NULL) {
            edge *e = p->e;
            node *n0 = e->to;
            heap_node *h = (heap_node *) (n0->active);
            double temp = key + e->weight;
            if (sp->dist[n0->node2ind] < 0 || temp < sp->dist[n0->node2ind]) {
                link_to_parent(sp, n0, n, temp);
                if (h == NULL) 
                    heap = heap_insert(heap, n0, temp);
                else 
                    heap = decrease_key(heap, h, temp);
            }
            p = p->next;
        }
    }
    assert(heap_node_count == 0);
}

void shortest_paths(network *net) {
    int i;
    sp_vector *sp = new_sp_vector(net);
    for (i = 0; i < net->node_count; i ++) {
        init_sp_vector(sp);
        node *n = net->ind2node[i];
        if (n->color == net->greatest_component) {
            shortest_paths_from(net, n, sp);
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
    sp_vector *sp = new_sp_vector(net);
    init_sp_vector(sp);
    shortest_paths_from(net, s0, sp);
    printf("great-circle distance: %lf (m)\n", dist(s0, t0));
    printf("network distance: %lf (m)\n", sp->dist[t0->node2ind]);
    node *p = t0;
    while (p != NULL) {
        printf("%d ", p->node2ind);
        p = sp->parent[p->node2ind];
    }
    printf("\n");
}

void init() {
    srand(time(0));
    init_pointer_buffers();
    init_bst();
    init_heap();
    init_network();
    printf("inited\n");
}

void clear() {
    clear_network();
    clear_heap();
    clear_bst();
    clear_pointer_buffers();
    printf("clear\n");
}

void test() {
    network *net = new_network_from("map-shanghai.osm");
    //shortest_paths(net);
    double nd = network_distance(net,
        31.2983874, 121.4996445, 31.1942597, 121.5944074);
    free_network(net);
}

int main() {
    init();
    test();
    clear();
    return EXIT_SUCCESS;
}
