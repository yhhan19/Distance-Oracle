#include "utils.h"

list_node *new_list_node(void *data) {
    list_count ++;
    list_node *p = (list_node *) malloc(sizeof(list_node));
    p->data = data;
    p->next = NULL;
    return p;
}

void free_list_node(list_node *p) {
    list_count --;
    free(p);
}

quad_node *new_quad_node(wspd *w, quad_node *f, int i) {
    quad_node *n = (quad_node *) malloc(sizeof(quad_node));
    quad_count ++;
    if (f == NULL) {
        n->depth = 0;
        n->code = 0;
    }
    else {
        node p = f->p, q = f->q, r;
        r.lat = (p.lat + q.lat) / 2;
        r.lon = (p.lon + q.lon) / 2; 
        switch (i) {
            case NW: 
                n->p.lat = p.lat; n->p.lon = r.lon;
                n->q.lat = r.lat; n->q.lon = q.lon;
                break;
            case NE: 
                n->p.lat = r.lat; n->p.lon = r.lon;
                n->q.lat = q.lat; n->q.lon = q.lon;
                break;
            case SW: 
                n->p.lat = p.lat; n->p.lon = p.lon;
                n->q.lat = r.lat; n->q.lon = r.lon;
                break;
            case SE: 
                n->p.lat = r.lat; n->p.lon = p.lon;
                n->q.lat = q.lat; n->q.lon = r.lon;
                break;
        }
        n->depth = f->depth + 1;
        n->code = f->code * 4 + i;
    }
    if (n->depth > w->depth) 
        w->depth = n->depth;
    for (i = 0; i < 4; i ++) 
        n->child[i] = NULL;
    n->desc = NULL;
    n->type = WHITE;
    n->data = NULL;
    n->size = 0;
    return n;
}

void free_quad_node(quad_node *p) {
    quad_count --;
    free(p);
}

int quad_contain(quad_node *r, node *p) {
    if (p->lat < r->p.lat || p->lat >= r->q.lat) return 0;
    if (p->lon < r->p.lon || p->lon >= r->q.lon) return 0;
    return 1;
}

int id(quad_node *n, node *data) {
    node r;
    r.lat = (n->p.lat + n->q.lat) / 2;
    r.lon = (n->p.lon + n->q.lon) / 2; 
    if (data->lat < r.lat) 
        return (data->lon < r.lon) ? SW : NW;
    else 
        return (data->lon < r.lon) ? SE : NE;
}

int quad_insert(wspd *w, quad_node *n, node *data) {
    int ret;
    switch (n->type) {
        case WHITE: 
            ret = 1;
            n->data = data;
            n->type = BLACK;
            break;
        case BLACK: 
            if (n->depth == DEP) return 0;
            n->child[NW] = new_quad_node(w, n, NW);
            n->child[NE] = new_quad_node(w, n, NE);
            n->child[SW] = new_quad_node(w, n, SW);
            n->child[SE] = new_quad_node(w, n, SE);
            n->child[id(n, n->data)]->data = n->data;
            n->child[id(n, n->data)]->type = BLACK;
            n->child[id(n, n->data)]->size = 1;
        case GRAY: 
            ret = quad_insert(w, n->child[id(n, data)], data);
            if (ret) n->type = GRAY;
    }
    if (ret) {
        n->size ++;
        list_node *p = new_list_node((void *) data);
        p->next = n->desc;
        n->desc = p;
        double d0 = w->net->dist[n->data->node2ind][data->node2ind];
        double d1 = w->net->dist[data->node2ind][n->data->node2ind];
        n->diam = MAX(n->diam, MAX(d0, d1));
    }
    return ret;
}

quad_node *quad_search(quad_node *n, node *data) {
    quad_node *ret;
    switch (n->type) {
        case WHITE: 
            ret = NULL;
            break;
        case BLACK: 
            ret = n;
            break;
        case GRAY: 
            return quad_search(n->child[id(n, data)], data);
            break;
    }
    return ret;
}

void free_quad(quad_node *root) {
    if (root == NULL) return ;
    int i;
    for (i = 0; i < 4; i ++) {
        free_quad(root->child[i]);
    }
    list_node *p = root->desc, *q = NULL;
    while (p != NULL) {
        q = p;
        p = p->next;
        free_list_node(q);
    }
    free_quad_node(root);
}

int pair_contain(pair *p, node *a, node *b) {
    return quad_contain(p->u, a) && quad_contain(p->v, b);
}

int pr_z4_code(char *ret, pair *p) {
    long long cu = p->u->code, cv = p->v->code;
    int du = p->u->depth, dv = p->v->depth;
    int u[CLEN], v[CLEN], i;
    for (i = 0; i < du; i ++) {
        u[i] = cu % 4;
        cu = cu / 4;
    }
    for (i = 0; i < dv; i ++) {
        v[i] = cv % 4;
        cv = cv / 4;
    }
    int len = 0;
    while (du > 0 || dv > 0) {
        du --; dv --;
        ret[len ++] = (du < 0) ? '4' : u[du] + '0';
        ret[len ++] = (dv < 0) ? '4' : v[dv] + '0';
    }
    ret[len ++] = '\0';
    return len;
}

void pt_z4_code(wspd *w, char *ret, node *p, node *q, int len) {
    double lxp = w->root->p.lat, lyp = w->root->p.lon, 
        rxp = w->root->q.lat, ryp = w->root->q.lon;
    double lxq = w->root->p.lat, lyq = w->root->p.lon, 
        rxq = w->root->q.lat, ryq = w->root->q.lon;
    int i;
    for (i = 0; i < len; i ++) {
        int cp = 0, cq = 0;
        double myp = (lyp + ryp) / 2;
        if (p->lon < myp) {
            ryp = myp;
            cp = cp * 2;
        }
        else {
            lyp = myp;
            cp = cp * 2 + 1;
        }
        double mxp = (lxp + rxp) / 2;
        if (p->lat < mxp) {
            rxp = mxp;
            cp = cp * 2;
        }
        else {
            lxp = mxp;
            cp = cp * 2 + 1;
        }
        double myq = (lyq + ryq) / 2;
        if (q->lon < myq) {
            ryq = myq;
            cq = cq * 2;
        }
        else {
            lyq = myq;
            cq = cq * 2 + 1;
        }
        double mxq = (lxq + rxq) / 2;
        if (q->lat < mxq) {
            rxq = mxq;
            cq = cq * 2;
        }
        else {
            lxq = mxq;
            cq = cq * 2 + 1;
        }
        ret[i * 2] = cp + '0';
        ret[i * 2 + 1] = cq + '0';
    }
    ret[len * 2] = '\0';
}

int z4_code_cmp(char *a, char *b) {
    char ca, cb;
    do {
        ca = *a ++;
        cb = *b ++;
        if (ca == '\0') return ca - cb;
    } while (ca == cb || ca == '4' || cb == '4');
    return ca - cb;
}

int z4_code_cmp_len(char *a, char *b) {
    char ca, cb;
    char *pa = a, *pb = b;
    do {
        ca = *a ++;
        cb = *b ++;
        if (ca == '\0') return a - pa;
    } while (ca == cb || ca == '4' || cb == '4');
    return a - pa;
}

int cmp(wspd *w, node *p, node *q, pair *pr) {
    char ca[CLEN], cb[CLEN];
    pt_z4_code(w, ca, p, q, w->depth);
    pr_z4_code(cb, pr);
    return z4_code_cmp(ca, cb);
}

int cmp_len(wspd *w, node *p, node *q, pair *pr) {
    char ca[CLEN], cb[CLEN];
    pt_z4_code(w, ca, p, q, w->depth);
    pr_z4_code(cb, pr);
    return z4_code_cmp_len(ca, cb);
}

pair *bin_search(wspd *w, node *p, node *q) {
    int l = 0, r = w->tail - 1;
    while (l < r) {
        int m = (l + r) / 2;
        int delta = cmp(w, p, q, w->list + m);
        if (delta < 0) r = m - 1;
        if (delta > 0) l = m + 1;
        if (delta == 0) return w->list + m;
    }
    int i, ret = -1, max = -1;
    for (i = l - 1; i <= l + 1; i ++) {
        if (i < 0 || i >= w->tail) continue;
        int len = cmp_len(w, p, q, w->list + i);
        if (len > max) {
            max = len;
            ret = i;
        }
    }
    /*
    if (! pair_contain(w->list + ret, p, q)) {
        char ca[CLEN], cb[CLEN];
        printf("%d %d\n", p->node2ind, q->node2ind);
        pt_z4_code(w, ca, p, q, w->depth);
        pr_z4_code(cb, w->list + ret);
        printf("%s\n%s\n", ca, cb);
        scanf("%c");
    }
    */
    return w->list + ret;
    /*
    if (pair_contain(w->list + l, p, q))
        return w->list + l;
    if (l >= 1 && pair_contain(w->list + l - 1, p, q))
        return w->list + l - 1;
    return NULL;
    */
}

void add_pair(wspd *w, quad_node *u, quad_node *v) {
    if (w->tail == w->size) {
        w->size *= 2;
        pair *temp = (pair *) malloc(sizeof(pair) * w->size);
        int i;
        for (i = 0; i < w->tail; i ++) 
            temp[i] = w->list[i];
        free(w->list);
        w->list = temp;
        if (w->size >= (int) 1e6) 
            printf("wspd: %d [array expanded]\n", w->size);
    }
    w->list[w->tail].u = u;
    w->list[w->tail].v = v;
    w->tail ++;
}

double diameter(quad_node *u) {
    if (u->type == BLACK) 
        return 0;
    else 
        return sphere_dist(& (u->p), & (u->q));
}

double node_dist(quad_node *u, quad_node *v) {
    node nu[4], nv[4];
    nu[0] = u->p; nu[3] = u->q;
    nu[1].lat = u->p.lat; nu[1].lon = u->q.lon;
    nu[2].lat = u->q.lat; nu[2].lon = u->p.lon;
    nv[0] = v->p; nv[3] = v->q;
    nv[1].lat = v->p.lat; nv[1].lon = v->q.lon;
    nv[2].lat = v->q.lat; nv[2].lon = v->p.lon;
    double ret = -1;
    int i, j;
    for (i = 0; i < 4; i ++) {
        for (j = 0; j < 4; j ++) {
            double d = sphere_dist(nu + i, nv + j);
            if (ret < 0 || d < ret) ret = d;
        }
    }
    return ret;
}

int well_separated(quad_node *u, quad_node *v, double s) {
    double du = diameter(u), dv = diameter(v);
    double diam = MAX(du, dv);
    double dist = node_dist(u, v);
    return (dist >= s * diam);
}

int net_well_separated(wspd *w, quad_node *u, quad_node *v) {
    double diam = MAX(u->diam, v->diam);
    double dist = w->net->dist[u->data->node2ind][v->data->node2ind];
    return (dist >= w->s * diam);
}

int level(quad_node *u) {
    if (u->type == GRAY) 
        return u->depth;
    else 
        return INF;
}

void build(wspd *w, quad_node *u, quad_node *v) {
    if (u == v && u->type != GRAY && v->type != GRAY) 
        return ;
    if (u->type == WHITE || v->type == WHITE) 
        return ;
    if (net_well_separated(w, u, v)) {
        add_pair(w, u, v);
    }
    else {
        int i;
        if (level(u) > level(v)) {
            for (i = 0; i < 4; i ++) {
                build(w, u, v->child[i]);
            }
        }
        else {
            for (i = 0; i < 4; i ++) {
                build(w, u->child[i], v);
            }
        }
    }
}

pair *check_query(wspd *w, node *a, node *b) {
    pair *p = bin_search(w, a, b);
    assert(p != NULL);
    return p;
}

void check_wspd(wspd *w, network *net, int step) {
    int i, j, percent;
    double min = 1, max = 1;
    long long total = 0;
    for (i = 0, percent = 0; i < w->tail; i ++) {
        pair *p = w->list + i;
        assert(net_well_separated(w, p->u, p->v));
        total += (long long) p->u->size * p->v->size;
        if ((long long) i * 100 >= (long long) w->tail * percent) {
            printf("\rwspd: %3d%% [separate check]", ++ percent);
        }
    }
    printf("\n");
    assert(total == (long long) net->node_count * (net->node_count - 1));
    for (i = 0, percent = 0; i < w->tail - 1; i ++) {
        char temp[CLEN], temp2[CLEN];
        pr_z4_code(temp, w->list + i);
        pr_z4_code(temp2, w->list + i + 1);
        assert(strcmp(temp, temp2) == -1);
        if ((long long) i * 100 >= (long long) (w->tail - 1) * percent) {
            printf("\rwspd: %3d%% [order check]", ++ percent);
        }
    }
    printf("\n");
    clock_t time = clock();
    long long times = 0;
    for (i = 0, percent = 0; i < net->node_count; i += step) {
        for (j = 0; j < net->node_count; j += step) {
            if (i == j) continue;
            pair *p = check_query(w, net->ind2node[i], net->ind2node[j]);
            double e = w->net->dist[i][j];
            e /= w->net->dist[p->u->data->node2ind][p->v->data->node2ind];
            min = MIN(min, e);
            max = MAX(max, e);
            if (percent < 10000 && (times ++ * 10000 >= 
            (long long) net->node_count / step * (net->node_count / step - 1) * percent)) {
                printf("\rwspd: %5.2lf%% [query check]", (double) ++ percent / 100);
            }
        }
    }
    printf("\n");
    time = clock() - time;
    printf("wspd: %lf [wspd error]\n", MAX(max - 1, 1 - min));
    printf("wspd: %lf (ms) [query time]\n", (double) time / times);
    printf("wspd: checked\n");
}

wspd *new_wspd(network *net, double eps) {
    wspd *w = (wspd *) malloc(sizeof(wspd));
    w->net = net;
    w->s = 2 / eps;
    w->tail = 0; w->size = 256;
    w->list = (pair *) malloc(sizeof(pair) * w->size);
    w->depth = 0;
    w->root = new_quad_node(w, NULL, -1);
    int i;
    w->root->p.lat = w->root->p.lon = PI;
    w->root->q.lat = w->root->q.lon = - PI;
    for (i = 0; i < net->node_count; i ++) {
        node *n = net->ind2node[i];
        if (n->color != net->greatest_component) continue;
        w->root->p.lat = MIN(w->root->p.lat, net->ind2node[i]->lat);
        w->root->q.lat = MAX(w->root->q.lat, net->ind2node[i]->lat);
        w->root->p.lon = MIN(w->root->p.lon, net->ind2node[i]->lon);
        w->root->q.lon = MAX(w->root->q.lon, net->ind2node[i]->lon);
    }
    clock_t time = clock();
    for (i = 0; i < net->node_count; i ++) {
        node *n = net->ind2node[i];
        if (n->color == net->greatest_component) {
            quad_insert(w, w->root, net->ind2node[i]);
        }
    }
    time = clock() - time;
    printf("wspd: %d nodes %d layers [quadtree]\n", w->root->size, w->depth);
    printf("wspd: %d (ms) [insertion time]\n", time);
    time = clock();
    build(w, w->root, w->root);
    time = clock() - time;
    printf("wspd: %d pairs (%lf)\n", w->tail, 
        (double) w->tail / w->root->size / (w->root->size - 1));
    printf("wspd: %d (ms) [build time]\n", time);
    printf("wspd: built\n");
    return w;
}

void clear_wspd() {
    assert(list_count == 0);
    assert(quad_count == 0);
}

void free_wspd(wspd *w) {
    free_quad(w->root);
    free(w->list);
    free(w);
}
