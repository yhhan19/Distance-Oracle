#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#define MAX_string_buffer_len 65536
#define MAX_pointer_buffers 8
#define MAX_nodes ((int) 1e6)
#define EARTH_RADIUS 6371009

typedef struct node {
    long long node2id;
    int node2ind, color;
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
    struct heap_node *child, *sibling;
} heap_node;

bst_node *BST_NULL;
void ***pointer_buffers;
int *avail_pointer_buffer, max_buffers, bst_node_count, 
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
    n->lat = lat;
    n->lon = lon;
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
    free_buffer((void **) queue);
}

void connect_nodes(network *net, long long way, node *from, node *to) {
    edge *e = new_edge(way, from, to);
    adjacent_node *adj = new_adjacent_node(e);
    adj->next = net->adjacent_lists[from->node2ind];
    net->adjacent_lists[from->node2ind] = adj;
    net->edge_count ++;
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
    free_buffer((void **) stack);
}

int traverse_network_from(network *net, node *source, node **queue) {
    int head = 0, tail = 0;
    queue[tail ++] = source;
    source->active = 1;
    while (head < tail) {
        node *cur = queue[head ++];
        adjacent_node *p = net->adjacent_lists[cur->node2ind];
        while (p != NULL) {
            edge *e = p->e;
            node *to = e->to;
            if (to->active == NULL) {
                to->active = 1;
                queue[tail ++] = to;
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
    free_buffer((void **) queue);
}

network *new_network_from(const char *name) {
    FILE *fin = fopen(name, "r");
    network *net = (network *) malloc(sizeof(network));
    new_nodes_from(net, fin);
    new_edges_from(net, fin);
    fclose(fin);
    mark_components(net);
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

heap_node *new_heap_node() {
    
    heap_node_count ++;
}

void link_to_parent(
    node *n0, node *n, double temp, node **parent, double *dist) 
{
    parent[n0->node2ind] = n;
    dist[n0->node2ind] = temp;
}

void shortest_paths_from(
    network *net, node *source, node **parent, double *dist) 
{
    heap_node *root = NULL;
    link_to_parent(source, NULL, 0, parent, dist);
    source->active = heap_insert(root, source);
    while (root != NULL) {
        node *n = extract_min(root);
        n->active = NULL;
        adjacent_node *p = net->adjacent_lists[n->node2ind];
        while (p != NULL) {
            edge *e = p->e;
            node *n0 = e->to;
            double temp = dist[n->node2ind] + e->weight;
            if (n0->active == NULL) {
                link_to_parent(n0, n, temp, parent, dist);
                n0->active = heap_insert(root, n0);
            }
            else {
                if (temp < dist[n0->node2ind]) {
                    link_to_parent(n0, n, temp, parent, dist);
                    n0->active = dcrease_key(root, n0, temp);
                }
            }
            p = p->next;
        }
    }
}

void shortest_paths(network *net) {
    int i;
    node **parent = (node **) new_buffer();
    double *dist = (double *) malloc(sizeof(double) * net->node_count);
    for (i = 0; i < net->node_count; i ++) {
        node *n = net->ind2node[i];
        if (n->color == net->greatest_component) {
            shortest_paths_from(net, n, parent, dist);
        }
    }
}

void init() {
    init_pointer_buffers();
    init_bst();
    init_network();
    printf("inited\n");
}

void clear() {
    clear_network();
    clear_bst();
    clear_pointer_buffers();
    printf("clear\n");
}

void test() {
    network *net = new_network_from("map-beijing.osm");
    shortest_paths(net);
    free_network(net);
}

int main() {
    init();
    test();
    clear();
    return EXIT_SUCCESS;
}
