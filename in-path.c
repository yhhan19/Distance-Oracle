#include "utils.h"

void init() {
    srand(time(0));
    init_pointer_buffers();
    init_bst();
    init_heap();
    init_network();
    printf("main: initialized\n");
}

void clear() {
    clear_wspd();
    clear_network();
    clear_heap();
    clear_bst();
    clear_pointer_buffers();
    printf("main: all clear\n");
}

void test() {
    network *net = new_network_from("map-beijing-2.osm");
    check_network_distance(net);
    //shortest_paths(net);
    //check_shortest_paths(net);
    net->dist = read_shortest_paths(net);
    wspd *w = new_wspd(net, 0.2);
    check_wspd(w, net, 10);
    free_wspd(w);
    free_matrix(net->dist, net->node_count);
    free_network(net);
}

int main() {
    init();
    test();
    clear();
    return EXIT_SUCCESS;
}

/*
    network *net = new_network_from("map-shanghai.osm");
    double d = network_distance(net,
        31.2983874, 121.4996445, 31.1942597, 121.5944074);
    network *net = new_network_from("map-dc.osm");
    double d = network_distance(net,
        39.0019850, -76.9166379, 38.8958150, -77.0066764);
    sp_vector *sp = new_sp_vector(net->node_count);
    init_sp_vector(sp);
    shortest_paths_from(net, sp, net->ind2node[0]);
    free_sp_vector(sp);
*/