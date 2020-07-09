#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include <string.h>

#define SW 0
#define SE 1
#define NW 2
#define NE 3
#define WHITE 0
#define BLACK 1
#define GRAY 2
#define SQRT2 1.4142135623730950488016887242097
#define INF 2147483647
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))
#define ABS(a) (((a)>0)?(a):(-(a)))
#define RAD ((double) (RAND_MAX + 1))
#define CLEN 64

typedef struct point {
    double x, y;
} point;

typedef struct quad_node {
    struct quad_node *child[4];
    int type, depth, size;
    long long code;
    point p, q, *data;
} quad_node;

typedef struct pair {
    struct quad_node *u, *v;
} pair;

typedef struct trie_node {
    struct trie_node *child[6];
    int data;
} trie_node;

typedef struct wspd {
    trie_node *dict;
    quad_node *root;
    pair *list;
    int size, tail, count, depth;
} wspd;

double frand() {
    return rand() + (double) rand() / (double) (RAND_MAX + 1);
}

double dist2(point *u, point *v) {
    return (u->x - v->x) * (u->x - v->x) + (u->y - v->y) * (u->y - v->y);
}

point *init(int N) {
    point *input = (point *) malloc(sizeof(point) * N);
    int i;
    for (i = 0; i < N; i ++) {
        input[i].x = frand();
        input[i].y = frand();
    }
    return input;
}

quad_node *new_quad_node(wspd *w, quad_node *f, int i) {
    quad_node *n = (quad_node *) malloc(sizeof(quad_node));
    if (f == NULL) {
        n->p.x = 0; n->p.y = 0;
        n->q.x = RAD; n->q.y = RAD;
        n->depth = 0;
        n->code = 0;
    }
    else {
        point p = f->p, q = f->q, r;
        r.x = (p.x + q.x) / 2;
        r.y = (p.y + q.y) / 2; 
        switch (i) {
            case NW: 
                n->p.x = p.x; n->p.y = r.y;
                n->q.x = r.x; n->q.y = q.y;
                break;
            case NE: 
                n->p.x = r.x; n->p.y = r.y;
                n->q.x = q.x; n->q.y = q.y;
                break;
            case SW: 
                n->p.x = p.x; n->p.y = p.y;
                n->q.x = r.x; n->q.y = r.y;
                break;
            case SE: 
                n->p.x = r.x; n->p.y = p.y;
                n->q.x = q.x; n->q.y = r.y;
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

int contain(quad_node *r, point *p) {
    if (p->x < r->p.x || p->x >= r->q.x) return 0;
    if (p->y < r->p.y || p->y >= r->q.y) return 0;
    return 1;
}

int id(quad_node *n, point *data) {
    point r;
    r.x = (n->p.x + n->q.x) / 2;
    r.y = (n->p.y + n->q.y) / 2; 
    if (data->x < r.x) 
        return (data->y < r.y) ? SW : NW;
    else 
        return (data->y < r.y) ? SE : NE;
}

void quad_insert(wspd *w, quad_node *n, point *data) {
    switch (n->type) {
        case WHITE: 
            n->data = data;
            n->type = BLACK;
            n->size = 1;
            break;
        case BLACK: 
            n->child[NW] = new_quad_node(w, n, NW);
            n->child[NE] = new_quad_node(w, n, NE);
            n->child[SW] = new_quad_node(w, n, SW);
            n->child[SE] = new_quad_node(w, n, SE);
            n->child[id(n, n->data)]->data = n->data;
            n->child[id(n, n->data)]->type = BLACK;
            n->child[id(n, n->data)]->size = 1;
            n->type = GRAY;
            n->size ++;
            quad_insert(w, n->child[id(n, data)], data);
            break;
        case GRAY: 
            n->size ++;
            quad_insert(w, n->child[id(n, data)], data);
            break;
    }
}

quad_node *quad_search(quad_node *n, point *data) {
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

trie_node *new_trie_node(wspd *w) {
    trie_node *p = (trie_node *) malloc(sizeof(trie_node));
    int i;
    for (i = 0; i < 6; i ++) {
        p->child[i] = NULL;
    }
    p->data = -1;
    w->count ++;
    return p;
}

void trie_insert(wspd *w, int _p) {
    pair *p = w->list + _p;
    long long cu = p->u->code, cv = p->v->code;
    int du = p->u->depth, dv = p->v->depth;
    int min = MIN(du, dv);
    int u[64], v[64], i;
    for (i = 0; i < du; i ++) {
        u[i] = cu % 4;
        cu = cu / 4;
    }
    for (i = 0; i < dv; i ++) {
        v[i] = cv % 4;
        cv = cv / 4;
    }
    trie_node *root = w->dict;
    while (du > 0 || dv > 0) {
        du --;
        dv --;
        if (du < 0){
            if (root->child[5] == NULL) {
                root->child[5] = new_trie_node(w);
            }
            root = root->child[5];
        }
        else {
            if (root->child[u[du]] == NULL) {
                root->child[u[du]] = new_trie_node(w);
            }
            root = root->child[u[du]];
        }
        if (dv < 0) {
            if (root->child[5] == NULL) {
                root->child[5] = new_trie_node(w);
            }
            root = root->child[5];
        }
        else {
            if (root->child[v[dv]] == NULL) {
                root->child[v[dv]] = new_trie_node(w);
            }
            root = root->child[v[dv]];
        }
    }
    assert(root->child[4] == NULL);
    root->child[4] = new_trie_node(w);
    root->child[4]->data = _p;
}

pair *trie_search(wspd *w, point *p, point *q) {
    double lxp = 0, lyp = 0, rxp = RAD, ryp = RAD;
    double lxq = 0, lyq = 0, rxq = RAD, ryq = RAD;
    trie_node *root = w->dict;
    int i;
    for (i = 0; i < w->depth; i ++) {
        int cp = 0, cq = 0;
        double myp = (lyp + ryp) / 2;
        if (p->y < myp) {
            ryp = myp;
            cp = cp * 2;
        }
        else {
            lyp = myp;
            cp = cp * 2 + 1;
        }
        double mxp = (lxp + rxp) / 2;
        if (p->x < mxp) {
            rxp = mxp;
            cp = cp * 2;
        }
        else {
            lxp = mxp;
            cp = cp * 2 + 1;
        }
        double myq = (lyq + ryq) / 2;
        if (q->y < myq) {
            ryq = myq;
            cq = cq * 2;
        }
        else {
            lyq = myq;
            cq = cq * 2 + 1;
        }
        double mxq = (lxq + rxq) / 2;
        if (q->x < mxq) {
            rxq = mxq;
            cq = cq * 2;
        }
        else {
            lxq = mxq;
            cq = cq * 2 + 1;
        }
        if (root->child[cp] != NULL) {
            root = root->child[cp];
        }
        else if (root->child[5] != NULL) {
            root = root->child[5];
        }
        else {
            return w->list + root->child[4]->data;
        }
        if (root->child[cq] != NULL) {
            root = root->child[cq];
        }
        else if (root->child[5] != NULL) {
            root = root->child[5];
        }
        else {
            return w->list + root->child[4]->data;
        }
    }
    return w->list + root->child[4]->data;
}

void free_trie(trie_node *root) {
    if (root == NULL) return ;
    int i;
    for (i = 0; i < 6; i ++) {
        free_trie(root->child[i]);
    }
    free(root);
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

void pt_z4_code(char *ret, point *p, point *q, int len) {
    double lxp = 0, lyp = 0, rxp = RAD, ryp = RAD;
    double lxq = 0, lyq = 0, rxq = RAD, ryq = RAD;
    int i;
    for (i = 0; i < len; i ++) {
        int cp = 0, cq = 0;
        double myp = (lyp + ryp) / 2;
        if (p->y < myp) {
            ryp = myp;
            cp = cp * 2;
        }
        else {
            lyp = myp;
            cp = cp * 2 + 1;
        }
        double mxp = (lxp + rxp) / 2;
        if (p->x < mxp) {
            rxp = mxp;
            cp = cp * 2;
        }
        else {
            lxp = mxp;
            cp = cp * 2 + 1;
        }
        double myq = (lyq + ryq) / 2;
        if (q->y < myq) {
            ryq = myq;
            cq = cq * 2;
        }
        else {
            lyq = myq;
            cq = cq * 2 + 1;
        }
        double mxq = (lxq + rxq) / 2;
        if (q->x < mxq) {
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

int cmp(wspd *w, point *p, point *q, pair *pr) {
    char ca[CLEN], cb[CLEN];
    pt_z4_code(ca, p, q, w->depth);
    pr_z4_code(cb, pr);
    return z4_code_cmp(ca, cb);
}

pair *bin_search(wspd *w, point *p, point *q) {
    int l = 0, r = w->tail - 1;
    while (l < r) {
        int m = (l + r) / 2;
        int delta = cmp(w, p, q, w->list + m);
        if (delta < 0) r = m - 1;
        if (delta > 0) l = m + 1;
        if (delta == 0) return w->list + m;
    }
    int i;
    for (i = l - 1; i <= l; i ++) {
        if (i < 0 || i >= w->tail) continue;
        pair *A = w->list + i;
        if (contain(A->u, p) && contain(A->v, q)) return A;
    }
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
    }
    w->list[w->tail].u = u;
    w->list[w->tail].v = v;
    w->tail ++;
    //trie_insert(w, w->tail - 1);
}

int well_separated(quad_node *u, quad_node *v, double s) {
    point cu, cv;
    double du, dv;
    if (u->type == BLACK) {
        cu.x = u->data->x;
        cu.y = u->data->y;
        du = 0;
    }
    else {
        cu.x = (u->p.x + u->q.x) / 2;
        cu.y = (u->p.y + u->q.y) / 2;
        du = (u->q.x - u->p.x) * SQRT2;
    }
    if (v->type == BLACK) {
        cv.x = v->data->x;
        cv.y = v->data->y;
        dv = 0;
    }
    else {
        cv.x = (v->p.x + v->q.x) / 2;
        cv.y = (v->p.y + v->q.y) / 2;
        dv = (v->q.x - v->p.x) * SQRT2;
    }
    double r = MAX(du, dv) / 2;
    double d = sqrt(dist2(& cu, & cv)) - MAX(du, dv);
    return (d >= s * r);
}

int level(quad_node *u) {
    if (u->type == GRAY) 
        return u->depth;
    else 
        return INF;
}

void build(wspd *w, quad_node *u, quad_node *v, double s) {
    if (u == v && u->type != GRAY && v->type != GRAY) 
        return ;
    if (u->type == WHITE || v->type == WHITE) 
        return ;
    if (well_separated(u, v, s)) {
        add_pair(w, u, v);
    }
    else {
        int i;
        if (level(u) > level(v)) {
            for (i = 0; i < 4; i ++) {
                build(w, u, v->child[i], s);
            }
        }
        else {
            for (i = 0; i < 4; i ++) {
                build(w, u->child[i], v, s);
            }
        }
    }
}

void check_wspd(point *input, int N, wspd *w) {
    int i, j;
    for (i = 0; i < w->tail - 1; i ++) {
        char temp[CLEN], temp2[CLEN];
        pr_z4_code(temp, w->list + i);
        pr_z4_code(temp2, w->list + i + 1);
        assert(strcmp(temp, temp2) == -1);
    }
    for (i = 0; i < N; i ++) {
        for (j = 0; j < N; j ++) {
            if (i == j) continue;
            int a = i, b = j;
            pair *p = bin_search(w, input + a, input + b);
            assert(p != NULL);
            assert(contain(p->u, input + a));
            assert(contain(p->v, input + b));
        }
    }
}

wspd *new_wspd(point *input, int N, double s) {
    wspd *w = (wspd *) malloc(sizeof(wspd));
    w->tail = 0; w->size = 256;
    w->list = (pair *) malloc(sizeof(pair) * w->size);
    w->dict = new_trie_node(w);
    w->count = 0; w->depth = 0;
    w->root = new_quad_node(w, NULL, -1);
    int i, j;
    for (i = 0; i < N; i ++) {
        quad_insert(w, w->root, input + i);
    }
    build(w, w->root, w->root, s);
    printf("%lf\n", (double) w->tail / N / (N - 1));
    //check_wspd(input, N, w);
    return w;
}

void free_wspd(wspd *w) {
    free_quad(w->root);
    free_trie(w->dict);
    free(w->list);
    free(w);
}

double diam(point *input, int N, double eps) {
    wspd *w = new_wspd(input, N, 2 / eps);
    int i;
    double ret = -1;
    for (i = 0; i < w->tail; i ++) {
        double d2 = dist2(w->list[i].u->data, w->list[i].v->data);
        if (ret < 0 || d2 > ret) ret = d2;
    }
    free_wspd(w);
    return sqrt(ret);
}

double _diam(point *input, int N) {
    int i, j;
    double ret = -1;
    for (i = 0; i < N; i ++) {
        for (j = i + 1; j < N; j ++) {
            double d2 = dist2(input + i, input + j);
            if (ret < 0 || d2 > ret) ret = d2;
        }
    }
    return sqrt(ret);
}

void test() {
    int N = (int) 1e4;
    point *input = init(N);
    double eps = 0.1;
    printf("%lf\n", diam(input, N, eps) / _diam(input, N));
    free(input);
}

int main() {
    srand(time(0));
    test();
    return 0;
}

/*

long long z_code(point *p, int len) {
    double lx = 0, ly = 0, rx = RAD, ry = RAD;
    long long ret = 0;
    int i;
    for (i = 0; i < len; i ++) {
        double my = (ly + ry) / 2;
        if (p->y < my) {
            ry = my;
            ret = ret * 2;
        }
        else {
            ly = my;
            ret = ret * 2 + 1;
        }
        double mx = (lx + rx) / 2;
        if (p->x < mx) {
            rx = mx;
            ret = ret * 2;
        }
        else {
            lx = mx;
            ret = ret * 2 + 1;
        }
    }
    return ret;
}

int prefix(long long a, int la, long long b, int lb) {
    int ca[64], cb[64], i, j;
    for (i = 0; i < la; i ++) {
        ca[i] = a % 4;
        a = a / 4;
    }
    for (i = 0; i < lb; i ++) {
        cb[i] = b % 4;
        b = b / 4;
    }
    while (lb > 0) {
        lb --;
        la --;
        if (la < 0 || ca[la] != cb[lb]) return 0;
    }
    return 1;
}

pair *search_pair(wspd *w, point *p, point *q) {
    long long cp = z_code(p, w->depth);
    long long cq = z_code(q, w->depth);
    pair *ret = NULL;
    int i;
    for (i = 0; i < w->tail; i ++) {
        quad_node *u = w->list[i].u, *v = w->list[i].v;
        if (prefix(cp, w->depth, u->code, u->depth) 
        && prefix(cq, w->depth, v->code, v->depth)) {
            assert(ret == NULL);
            ret = w->list + i;
        }
    }
    assert(ret != NULL);
    return ret;
}

*/