#ifndef SPW_NODE_HPP
#define SPW_NODE_HPP

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "ipc.h"
#include "spw_packet.h"

enum {
    FIFO_MSG_SZ = 500
};

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
    Packet data[FIFO_MSG_SZ];
    uint32_t head;
    uint32_t tail;
    uint32_t size;
} Fifo;

typedef struct {
    uint32_t id;
    bool auto_start;
    bool fct;
    enum SpWState state;
    ChildProcess tx;
    ChildProcess rx;
    ChildProcess console;
    pipe_fd to_rx_write;
    pipe_fd from_rx_read;
    pipe_fd to_tx_write;
    pipe_fd from_tx_read;
    pipe_fd to_console_write;
    pipe_fd from_console_read;
    Fifo tx_fifo;
    Fifo rx_fifo;
} SpWInterface;

void start_link(SpWInterface* const interface);
void powerup_link(SpWInterface* const spw_int, const pipe_fd tx, const pipe_fd rx);
void stop_interface(SpWInterface* const spw_int);

#endif