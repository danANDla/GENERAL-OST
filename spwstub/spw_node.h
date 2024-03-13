#include <inttypes.h>
#include <stdbool.h>

#include "interprocess.h"

#ifndef SPW_NODE_HPP
#define SPW_NODE_HPP

enum SpWState {
    OFF = 0,
    ERROR_RESET,
    ERROR_WIAT,
    READY,
    STARTED,
    CONNECTING,
    RUN
};

struct CablePair {
    pipe_fd inner_to_read;
    pipe_fd inner_to_write;
    pipe_fd outer; 
};

struct SpWInterface {
    uint32_t id;
    bool auto_start;
    enum SpWState state;
    struct CablePair tx;
    struct CablePair rx;
};

void start_link(struct SpWInterface* interface);

#endif