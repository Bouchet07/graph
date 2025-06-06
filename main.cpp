#include "graph.h"
#include <chrono>

int main(){
    CNet ring = CNet::ring(30, 2);
    ring.plot();

    return 0;
}