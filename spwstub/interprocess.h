#include <inttypes.h>
#include <stdbool.h>

#ifndef INTERPROCESS_HPP
#define INTERPROCESS_HPP

typedef int32_t pipe_fd;

typedef struct {
    bool is_rx;
    uint64_t pid;
    uint64_t ppid;

    pipe_fd to_parent_read;
    pipe_fd to_parent_write;
    pipe_fd outer;
} ch_process;

int32_t listener_duty();
void sender_duty();

#endif