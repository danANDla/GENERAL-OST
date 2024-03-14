#include "ipc.h"

#include <inttypes.h>
#include <stdbool.h>

#ifndef INTERPROCESS_HPP
#define INTERPROCESS_HPP


typedef struct {
    bool is_rx;
    uint64_t pid;
    uint64_t ppid;

    pipe_fd from_parent_read;
    pipe_fd to_parent_write;
    pipe_fd outer;
} ChildProcess;

void sender_duty();
int32_t rx_duty(ChildProcess *pr);
int32_t tx_duty(ChildProcess *pr);
int32_t console_duty(ChildProcess *pr);

#endif