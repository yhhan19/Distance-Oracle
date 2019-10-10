#include "sp.h"

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
    double d = network_distance(net,
        31.2983874, 121.4996445, 31.1942597, 121.5944074);
    /*
    network *net = new_network_from("map-dc.osm");
    double d = network_distance(net,
        39.0019850, -76.9166379, 38.8958150, -77.0066764
    );
    */
    free_network(net);
}

int main() {
    init();
    test();
    clear();
    return EXIT_SUCCESS;
}
