#include "utils.h"

void init_heap() {
    heap_node_count = 0;
}

void clear_heap() {
    assert(heap_node_count == 0);
}

heap_node *new_heap_node(struct node *n, double key) {
    heap_node *h = (heap_node *) malloc(sizeof(heap_node));
    heap_node_count ++;
    h->n = n;
    h->key = key;
    h->head = NULL;
    h->next = h->prev = NULL;
    return h;
}

void free_heap_node(heap_node *h) {
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
