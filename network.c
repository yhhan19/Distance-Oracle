#include "utils.h"

node *new_node(long long id, double lat, double lon) {
    node *n = (node *) malloc(sizeof(node));
    node_count ++;
    n->node2id = id;
    n->lat = lat;
    n->lon = lon;
    n->in = n->out = 0;
    n->active = NULL;
    return n;
}

void free_node(node *n) {
    free(n);
    node_count --;
}

edge *new_edge(long long way, node *from, node *to) {
    edge *e = (edge *) malloc(sizeof(edge));
    edge_count ++;
    e->way = way;
    e->from = from;
    e->to = to;
    e->weight = sphere_dist(from, to);
    return e;
}

void free_edge(edge *e) {
    free(e);
    edge_count --;
}

void init_network() {
    node_count = edge_count = 0;
    adjacent_node_count = 0;
}

void clear_network() {
    if (node_count != 0 || edge_count != 0) 
        printf("%d %d %d\n", node_count, edge_count, adjacent_node_count);
    assert(node_count == 0 && edge_count == 0);
    assert(adjacent_node_count == 0);
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
    free_edge(adj->e);
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
        node *n = new_node(id, lat * PI / 180, lon * PI / 180);
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
                if (from != to) {
                    connect_nodes(net, way_id, from, to);
                    if (oneway == 0) {
                        connect_nodes(net, way_id, to, from);
                    }
                }
            }
        }
    }
    printf("network: %d edges\n", net->edge_count);
    free_buffer((void **) stack);
}

adjacent_node *match_edge(network *net, node *n0, node *n1) {
    adjacent_node *p = net->adjacent_lists[n0->node2ind];
    while (p != NULL) {
        edge *e = p->e;
        if (e->to == n1) break;
        p = p->next;
    }
    return p;
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
    int i, sum, max, argmax, count, max2;
    for (i = 0, sum = 0, max = 0, count = 0; i < net->node_count; i ++) {
        if (net->ind2node[i]->active == NULL) {
            int tail = traverse_network_from(net, net->ind2node[i], queue);
            int j;
            for (j = 0; j < tail; j ++) 
                queue[j]->color = count;
            count ++;
            sum += tail;
            if (tail > max) {
                max2 = max;
                max = tail;
                argmax = i;
            }
            else if (tail > max2) {
                max2 = tail;
            }
        }
    }
    assert(sum == net->node_count);
    net->component_count = count;
    net->greatest_component_size = max;
    net->greatest_component = net->ind2node[argmax]->color;
    for (i = 0; i < net->node_count; i ++)
        net->ind2node[i]->active = NULL;
    printf("network: %d components\n", count);
    printf("network: %d nodes [main component]\n", max);
    printf("network: %d nodes [2nd component]\n", max2);
    free_buffer((void **) queue);
}

void simplify(network *net) {
    int i, count = 0;
    for (i = 0; i < net->node_count; i ++) {
        node *n = net->ind2node[i];
        if (n->color == net->greatest_component) {
            adjacent_node *p = net->adjacent_lists[n->node2ind], *q;
            while (p != NULL) {
                q = p->next;
                while (q != NULL) {
                    if (q->e->to->node2ind < p->e->to->node2ind 
                    || (q->e->to->node2ind == p->e->to->node2ind 
                    && q->e->weight < p->e->weight)) {
                        edge *temp = p->e;
                        p->e = q->e;
                        q->e = temp;
                    }
                    q = q->next;
                }
                p = p->next;
            }
            p = net->adjacent_lists[n->node2ind];
            while (p != NULL) {
                while (p->next != NULL && p->e->to == p->next->e->to) {
                    q = p->next;
                    p->next = q->next;
                    free_adjacent_node(q);
                    count ++;
                }
                p = p->next;
            }
        }
    }
    printf("network: %d edges simplified\n", count);
}

void depth_traverse_network_from(
    network *net, node *source, node **queue, int *tail) 
{
    adjacent_node *p = net->adjacent_lists[source->node2ind];
    while (p != NULL) {
        if (p->e->to->active == NULL) {
            p->e->to->active = (void *) 1;
            depth_traverse_network_from(net, p->e->to, queue, tail);
        }
        p = p->next;
    }
    queue[(* tail) ++] = source;
}

network *reverse_network(network *net) {
    network *rn = (network *) malloc(sizeof(network));
    rn->node_count = net->node_count;
    rn->ind2node = (node **) malloc(
        sizeof(node *) * net->node_count);
    rn->id2node = new_bst();
    int i;
    for (i = 0; i < net->node_count; i ++) {
        node *n = net->ind2node[i];
        rn->ind2node[i] = new_node(n->node2id, n->lat, n->lon);
        rn->ind2node[i]->node2ind = i;
        rn->ind2node[i]->in = n->in;
        rn->ind2node[i]->out = n->out;
        rn->ind2node[i]->active = NULL;
        rn->id2node = bst_insert(rn->id2node, rn->ind2node[i]);
    }
    rn->edge_count = net->edge_count;
    rn->adjacent_lists = (adjacent_node **) malloc(
        sizeof(adjacent_node *) * rn->node_count);
    for (i = 0; i < rn->node_count; i ++) 
        rn->adjacent_lists[i] = NULL;
    for (i = 0; i < net->node_count; i ++) {
        adjacent_node *p = net->adjacent_lists[i];
        while (p != NULL) {
            edge *e = new_edge(p->e->way, 
                rn->ind2node[p->e->to->node2ind], 
                rn->ind2node[p->e->from->node2ind]);
            adjacent_node *adj = new_adjacent_node(e);
            adj->next = rn->adjacent_lists[e->from->node2ind];
            rn->adjacent_lists[e->from->node2ind] = adj;
            p = p->next;
        }
    }
    return rn;
}

void strong_connected_component(network *net) {
    net->component_count = 0;
    net->greatest_component = net->greatest_component_size = 0;
    network *rnet = reverse_network(net);
    node **queue = (node **) new_buffer();
    node **stack = (node **) new_buffer();
    int i, tail = 0, top = 0;
    for (i = 0; i < net->node_count; i ++) {
        node *n = net->ind2node[i];
        if (n->active == NULL) {
            n->active = (void *) 1;
            depth_traverse_network_from(net, n, queue, & tail);
        }
    }
    for (i = 0; i < tail; i ++) {
        queue[i]->active = NULL;
        stack[top ++] = rnet->ind2node[queue[i]->node2ind];
    }
    int sum = 0, temp = 0, argmax = -1;
    while (top > 0) {
        node *n = stack[-- top];
        if (n->active == NULL) {
            n->active = (void *) 1;
            tail = 0;
            depth_traverse_network_from(rnet, n, queue, & tail);
            for (i = 0; i < tail; i ++) {
                node *m = net->ind2node[queue[i]->node2ind];
                m->color = net->component_count;
            }
            if (tail > net->greatest_component_size) {
                temp = net->greatest_component_size;
                net->greatest_component_size = tail;
                net->greatest_component = net->component_count;
            }
            else if (tail > temp) {
                temp = tail;
            }
            net->component_count ++;
        }
    }
    printf("network: %d components\n", net->component_count);
    printf("network: %d nodes [main component]\n", net->greatest_component_size);
    printf("network: %d nodes [2nd component]\n", temp);
    free_network(rnet);
    free_buffer((void **) queue);
    free_buffer((void **) stack);
}

network *one_component(network *net) {
    network *sn = (network *) malloc(sizeof(network));
    sn->node_count = net->greatest_component_size;
    sn->ind2node = (node **) malloc(
        sizeof(node *) * net->greatest_component_size);
    sn->id2node = new_bst();
    int i, j;
    int *map = (int *) malloc(sizeof(int) * net->node_count);
    for (i = 0, j = 0; i < net->node_count; i ++) {
        node *n = net->ind2node[i];
        if (n->color == net->greatest_component) {
            node *cpy = new_node(n->node2id, n->lat, n->lon);
            cpy->node2ind = j;
            cpy->in = n->in;
            cpy->out = n->out;
            cpy->active = NULL;
            map[i] = j;
            sn->ind2node[j ++] = cpy;
            sn->id2node = bst_insert(sn->id2node, cpy);
        }
    }
    sn->edge_count = 0;
    sn->adjacent_lists = (adjacent_node **) malloc(
        sizeof(adjacent_node *) * sn->node_count);
    for (i = 0; i < sn->node_count; i ++) 
        sn->adjacent_lists[i] = NULL;
    for (i = 0; i < net->node_count; i ++) {
        adjacent_node *p = net->adjacent_lists[i];
        while (p != NULL) {
            if (p->e->from->color == p->e->to->color 
             && p->e->from->color == net->greatest_component) {
                edge *cpy = new_edge(p->e->way, 
                    sn->ind2node[map[p->e->from->node2ind]], 
                    sn->ind2node[map[p->e->to->node2ind]]);
                sn->edge_count ++;
                adjacent_node *adj = new_adjacent_node(cpy);
                adj->next = sn->adjacent_lists[cpy->from->node2ind];
                sn->adjacent_lists[cpy->from->node2ind] = adj;
            }
            p = p->next;
        }
    }
    strong_connected_component(sn);
    return sn;
}

void check_network_distance(network *net) {
    int i, min_lat = -1, min_lon = -1, max_lat = -1, max_lon = -1, count = 0;
    for (i = 0; i < net->node_count; i ++) {
        node *n = net->ind2node[i];
        if (n->color != net->greatest_component) continue;
        if (min_lat == -1) {
            min_lat = max_lat = min_lon = max_lon = i;
        }
        else {
            if (n->lat < net->ind2node[min_lat]->lat) min_lat = i;
            if (n->lat > net->ind2node[max_lat]->lat) max_lat = i;
            if (n->lon < net->ind2node[min_lon]->lon) min_lon = i;
            if (n->lon > net->ind2node[max_lon]->lon) max_lon = i;
        }
        count ++;
    }
    network_distance_rad(net, net->ind2node[min_lat], net->ind2node[max_lat]);
    network_distance_rad(net, net->ind2node[min_lon], net->ind2node[max_lon]);
    double L = 0, R = INF;
    while (R - L > 1e-6) {
        double M = (L + R) / 2;
        bst_node *root = bounded_shortest_paths_from(net, net->ind2node[min_lat], M);
        if (count / 2 < root->s) R = M; else L = M;
        free_bst(root);
    }
    printf("distance: %lf (m) [half graph]\n", L);
}

network *new_network_from(const char *name) {
    FILE *fin = fopen(name, "r");
    network *net = (network *) malloc(sizeof(network));
    new_nodes_from(net, fin);
    new_edges_from(net, fin);
    fclose(fin);
    strong_connected_component(net);
    //simplify(net);
    network *snet = one_component(net);
    free_network(net);
    printf("network: loaded\n");
    return snet;
}

void free_network(network *net) {
    int i;
    for (i = 0; i < net->node_count; i ++) {
        adjacent_node *p = net->adjacent_lists[i];
        while (p != NULL) {
            adjacent_node *temp = p;
            p = p->next;
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
