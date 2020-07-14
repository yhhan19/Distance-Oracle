#include "utils.h"

quad_node *new_quad_node(wspd *w, quad_node *f, int i) {
    quad_node *n = (quad_node *) malloc(sizeof(quad_node));
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
    n->type = WHITE;
    n->data = NULL;
    n->size = 0;
    return n;
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
    int ret = 1;
    switch (n->type) {
        case WHITE: 
            n->data = data;
            n->type = BLACK;
            n->size = 1;
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
            ret = quad_insert(w, n->child[id(n, data)], data);
            if (ret) {
                n->size ++;
                n->type = GRAY;
            }
            break;
        case GRAY: 
            ret = quad_insert(w, n->child[id(n, data)], data);
            if (ret) n->size += ret;
            break;
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
    free(root);
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

int cmp(wspd *w, node *p, node *q, pair *pr) {
    char ca[CLEN], cb[CLEN];
    pt_z4_code(w, ca, p, q, w->depth);
    pr_z4_code(cb, pr);
    return z4_code_cmp(ca, cb);
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
    if (pair_contain(w->list + l, p, q))
        return w->list + l;
    if (l >= 1 && pair_contain(w->list + l - 1, p, q))
        return w->list + l - 1;
    return NULL;
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
        printf("wspd: %d [array expand]\n", w->size);
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
    double r = MAX(du, dv) / 2;
    double d = node_dist(u, v);
    return (d >= s * r);
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
    if (well_separated(u, v, w->s)) {
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

double check_query(wspd *w, node *a, node *b) {
    pair *p = bin_search(w, a, b);
    assert(p != NULL);
    double e = sqrt(sphere_dist(a, b)) / sqrt(sphere_dist(p->u->data, p->v->data));
    return e;
}

double check_wspd(wspd *w, network *net) {
    int i, j;
    double min = 1, max = 1;
    for (i = 0; i < w->tail; i ++) {
        pair *p = w->list + i;
        assert(well_separated(p->u, p->v, w->s));
    }
    for (i = 0; i < w->tail - 1; i ++) {
        char temp[CLEN], temp2[CLEN];
        pr_z4_code(temp, w->list + i);
        pr_z4_code(temp2, w->list + i + 1);
        assert(strcmp(temp, temp2) == -1);
    }
    for (i = 0; i < net->node_count; i += 500) {
        for (j = 0; j < net->node_count; j += 500) {
            if (i == j) continue;
            double e = check_query(w, net->ind2node[i], net->ind2node[j]);
            min = MIN(min, e);
            max = MAX(max, e);
        }
    }
    return MAX((1 / min) - 1, 1 - max);
}

wspd *new_wspd(double eps, network *net) {
    wspd *w = (wspd *) malloc(sizeof(wspd));
    w->s = 4 / eps;
    w->tail = 0; w->size = 256;
    w->list = (pair *) malloc(sizeof(pair) * w->size);
    w->depth = 0;
    w->root = new_quad_node(w, NULL, -1);
    int i;
    w->root->p.lat = w->root->p.lon = PI;
    w->root->q.lat = w->root->q.lon = - PI;
    for (i = 0; i < net->node_count; i ++) {
        w->root->p.lat = MIN(w->root->p.lat, net->ind2node[i]->lat);
        w->root->q.lat = MAX(w->root->q.lat, net->ind2node[i]->lat);
        w->root->p.lon = MIN(w->root->p.lon, net->ind2node[i]->lon);
        w->root->q.lon = MAX(w->root->q.lon, net->ind2node[i]->lon);
    }
    quad_node *u = w->root;
    for (i = 0; i < net->node_count; i += 500) {
        if (! quad_contain(w->root, net->ind2node[i])) continue;
        quad_insert(w, w->root, net->ind2node[i]);
    }
    printf("wspd: %d nodes [quadtree]\n", w->root->size);
    printf("wspd: %d layers [quadtree]\n", w->depth);
    build(w, w->root, w->root);
    printf("wspd: %d pairs\n", w->tail);
    return w;
}

void free_wspd(wspd *w) {
    free_quad(w->root);
    free(w->list);
    free(w);
}
