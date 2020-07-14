#include "utils.h"

void init() {
    srand(time(0));
    init_pointer_buffers();
    init_bst();
    init_heap();
    init_network();
    printf("main: inited\n");
}

void clear() {
    clear_network();
    clear_heap();
    clear_bst();
    clear_pointer_buffers();
    printf("main: clear\n");
}

void test() {
    
    network *net = new_network_from("map-shanghai.osm");
    double d = network_distance(net,
        31.2983874, 121.4996445, 31.1942597, 121.5944074);
    wspd *w = new_wspd(1.9, net);
    printf("main: %lf [wspd error]\n", check_wspd(w, net));
    free_wspd(w);
    free_network(net);
    /*
    network *net = new_network_from("map-dc.osm");
    double d = network_distance(net,
        39.0019850, -76.9166379, 38.8958150, -77.0066764);
    sp_vector *sp = new_sp_vector(net->node_count);
    init_sp_vector(sp);
    shortest_paths_from(net, sp, net->ind2node[0]);
    free_sp_vector(sp);
    */
}

int main() {
    init();
    test();
    clear();
    return EXIT_SUCCESS;
}
