#include "spw_node.h"

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h> 
#include <fcntl.h>

int pipe2(int fildes[2], int flags);

void interprocess_init(struct SpWInterface* const spw_int, pipe_fd tx, pipe_fd rx) {
    int32_t inner_tx[2];
    if(pipe2(inner_tx, O_NONBLOCK) < 0) exit(0);
    spw_int->tx = (struct CablePair) {
        .inner_to_read = inner_tx[0],
        .inner_to_write = inner_tx[1],
        .outer = tx
    };

    int32_t inner_rx[2];
    if(pipe2(inner_rx, O_NONBLOCK) < 0) exit(0);
    spw_int->rx = (struct CablePair) {
        .inner_to_read = inner_rx[0],
        .inner_to_write = inner_rx[1],
        .outer = rx
    };
}

void powerup_link(struct SpWInterface* interface) {
    if(interface->state != OFF) return;
    interface->state = READY;
    // start listening;
}

void start_link(struct SpWInterface* interface) {
    if(interface->state != READY) return;

    interface->state = STARTED;
    // start sending nulls;
}
