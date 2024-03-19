#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "ipc.h"

#ifndef SPW_NODE_HPP
#define SPW_NODE_HPP

enum SpWState {
    OFF = 0,
    ERROR_RESET,
    ERROR_WAIT,
    READY,
    STARTED,
    CONNECTING,
    RUN
};

typedef struct {
    void** data;
    uint32_t head;
    uint32_t tail;
    uint32_t size;
} Fifo;

typedef struct {
    uint32_t id;
    bool auto_start;
    enum SpWState state;
    ChildProcess tx;
    ChildProcess rx;
    ChildProcess console;
    Fifo rx_fifo;
    pipe_fd to_rx_write;
    pipe_fd from_rx_read;
    pipe_fd to_tx_write;
    pipe_fd from_tx_read;
    pipe_fd to_console_write;
    pipe_fd from_console_read;
} SpWInterface;

void start_link(SpWInterface* interface);
void powerup_link(SpWInterface* spw_int, pipe_fd tx, pipe_fd rx);
void stop_interface(SpWInterface* const spw_int);

#endif