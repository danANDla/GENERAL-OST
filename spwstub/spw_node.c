#include "spw_node.h"
#include "ipc.h"

#include <complex.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h> 
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#define DEBUG_ALL_PIPE_DATA

int pipe2(int fildes[2], int flags);

void create_pipes(SpWInterface* const spw_int, pipe_fd tx, pipe_fd rx) {
    int32_t inner_tx_to_parent[2];
    if(pipe2(inner_tx_to_parent, O_NONBLOCK) < 0) exit(0);
    int32_t inner_tx_from_parent[2];
    if(pipe2(inner_tx_from_parent, O_NONBLOCK) < 0) exit(0);
    spw_int->tx = (ChildProcess) {
        .is_rx = false,
        .from_parent_read = inner_tx_from_parent[0],
        .to_parent_write = inner_tx_to_parent[1],
        .outer = tx
    };
    spw_int->from_tx_read = inner_tx_to_parent[0];
    spw_int->to_tx_write = inner_tx_from_parent[1];
    printf("[PARENT] %2d -> %2d [TX]\n", inner_tx_from_parent[1], inner_tx_from_parent[0]);
    printf("[PARENT] %2d <- %2d [TX]\n", inner_tx_to_parent[0], inner_tx_to_parent[1]);

    int32_t inner_rx_to_parent[2];
    if(pipe2(inner_rx_to_parent, O_NONBLOCK) < 0) exit(0);
    int32_t inner_rx_from_parent[2];
    if(pipe2(inner_rx_from_parent, O_NONBLOCK) < 0) exit(0);
    spw_int->rx = (ChildProcess) {
        .is_rx = true,
        .from_parent_read = inner_rx_from_parent[0],
        .to_parent_write = inner_rx_to_parent[1],
        .outer = rx
    };
    spw_int->from_rx_read = inner_rx_to_parent[0];
    spw_int->to_rx_write = inner_rx_from_parent[1];
    printf("[PARENT] %2d -> %2d [RX]\n", inner_rx_from_parent[1], inner_rx_from_parent[0]);
    printf("[PARENT] %2d <- %2d [RX]\n", inner_rx_to_parent[0], inner_rx_to_parent[1]);

    int32_t nonblock_console_to_parent[2];
    if(pipe2(nonblock_console_to_parent, O_NONBLOCK) < 0) exit(0);
    int32_t nonblock_console_from_parent[2];
    if(pipe2(nonblock_console_from_parent, O_NONBLOCK) < 0) exit(0);
    spw_int->console = (ChildProcess) {
        .is_rx = false,
        .from_parent_read = nonblock_console_from_parent[0],
        .to_parent_write = nonblock_console_to_parent[1],
        .outer = -1
    };
    spw_int->from_console_read = nonblock_console_to_parent[0];
    spw_int->to_console_write = nonblock_console_from_parent[1];
    printf("[PARENT] %2d -> %2d [CONSOLE]\n", nonblock_console_from_parent[1], nonblock_console_from_parent[0]);
    printf("[PARENT] %2d <- %2d [CONSOLE]\n", nonblock_console_to_parent[0], nonblock_console_to_parent[1]);
}

void close_pipes(const SpWInterface* const spw_int) {
    close(spw_int->to_tx_write);
    close(spw_int->tx.from_parent_read);
    close(spw_int->tx.to_parent_write);
    close(spw_int->from_tx_read);

    close(spw_int->to_rx_write);
    close(spw_int->rx.from_parent_read);
    close(spw_int->rx.to_parent_write);
    close(spw_int->from_rx_read);

    close(spw_int->to_console_write);
    close(spw_int->console.from_parent_read);
    close(spw_int->console.to_parent_write);
    close(spw_int->from_console_read);
}

void create_forks(SpWInterface* const spw_int) {
    if(fork() == 0){
        spw_int->console.pid = getpid();
        spw_int->console.ppid = getppid();
        if(console_duty(&spw_int->console) != 0) exit(-1);
        exit(0);
    }

    if(fork() == 0){
        spw_int->rx.pid = getpid();
        spw_int->rx.ppid = getppid();
        if(rx_duty(&spw_int->rx) != 0) exit(-1);
        exit(0);
    }

    if(fork() == 0){
        spw_int->tx.pid = getpid();
        spw_int->tx.ppid = getppid();
        if(tx_duty(&spw_int->tx) != 0) exit(-1);
        exit(0);
    }
}

int32_t wait_all_stop(SpWInterface* const spw_int) {
    pipe_fd read_fd[3] = {spw_int->from_console_read, spw_int->from_rx_read, spw_int->from_tx_read};
    bool is_stop[3] = {false, false, false};
    uint8_t stoped = 0;
    while(stoped < 3) {
        for(int32_t i = 0; i < 3; ++i) {
            if(is_stop[i]) continue;
            Message msg;
            int32_t r = read_pipe(read_fd[i], &msg);
            if(r == 1) continue;
            if(msg.s_header.s_type == STOP) {
                // printf("[PARENT] received stop from %d\n", i);
                is_stop[i] = true;
                stoped++;
            }
        }
    }
    printf("[PARENT] received all stoped\n");
    return 0;
}

void stop_interface(SpWInterface* const spw_int) {
    Message stop_msg = {
        .s_header = {
            .s_magic = MESSAGE_MAGIC,
            .s_payload_len = 0,
            .s_type = STOP,
        }
    };
    write_pipe(spw_int->to_tx_write, &stop_msg);
    write_pipe(spw_int->to_rx_write, &stop_msg);
    wait_all_stop(spw_int);
    close_pipes(spw_int);
}

int32_t poll_children(SpWInterface* const spw_int, Message* msg) {
    int32_t r = read_pipe(spw_int->from_console_read, msg);
    if(r == 1) {
        r = read_pipe(spw_int->from_rx_read, msg);
        if(r == 1) return 1;
    }
    return r;
}

void process_link_msg(const SpWInterface* const spw_int, const Message* const msg) {
    switch (spw_int->state) {
        case OFF: 
            break;
        case READY:
            break;
        case STARTED:
            #ifdef DEBUG_ALL_PIPE_DATA
                printf("[PARENT] Received from LINK: len=%d, first 4 bytes: %d\n", msg->s_header.s_payload_len, *msg->s_payload);
            #endif
            break;
        case CONNECTING:
            break;
        case RUN:
            break;
        case ERROR_RESET:
            break;
        case ERROR_WAIT:
            break;
    }
}

int32_t send_packet_to_neigh(const SpWInterface* const spw, char* ch) {
    Message msg = {.s_header = {.s_type = PARENT_CONTROL, .s_payload_len = 1}};
    memcpy(msg.s_payload, ch, 1);
    write_pipe(spw->to_tx_write, &msg);
}

int32_t parent_duty(SpWInterface* const spw_int) {
    bool received_stop_from_console = false;
    Message msg;
    while(!received_stop_from_console){
        int32_t r = poll_children(spw_int, &msg);

        if(r == -1) {
            fprintf(stderr, "do_child_payload err %d\n", r);
            return -1;
        }
        if(r == 1) continue;

        if(msg.s_header.s_type == CONSOLE_CONTROL){
            char ch = *(char *)msg.s_payload;
            if(ch == '9') received_stop_from_console = true;
            else send_packet_to_neigh(spw_int, &ch);
        } else if (msg.s_header.s_type == LINK) {
            process_link_msg(spw_int, &msg);
        } else {
            return -1;
        }
    }
    return 0;
}

void powerup_link(SpWInterface* spw_int, pipe_fd tx, pipe_fd rx) {
    if(spw_int->state != OFF) return;
    spw_int->state = READY;
    create_pipes(spw_int, tx, rx);
}

void start_link(SpWInterface* interface) {
    if(interface->state != READY) return;
    interface->state = STARTED;
    create_forks(interface);
    parent_duty(interface);
    stop_interface(interface);
}

void* queue_read(Fifo *queue) {
    if (queue->tail == queue->head) {
        return NULL;
    }
    void* handle = queue->data[queue->tail];
    queue->data[queue->tail] = NULL;
    queue->tail = (queue->tail + 1) % queue->size;
    return handle;
}

int queue_write(Fifo *queue, void* handle) {
    if (((queue->head + 1) % queue->size) == queue->tail) {
        return -1;
    }
    queue->data[queue->head] = handle;
    queue->head = (queue->head + 1) % queue->size;
    return 0;
}