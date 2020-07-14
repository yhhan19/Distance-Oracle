#include "utils.h"

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
