#include "spw_utils.h"
#include "ipc.h"
#include "spw_interface.h"
#include "spw_packet.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

int32_t wait_all_stop(const SpWInterface* const spw_int) {
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

int32_t wait_started(const SpWInterface * const spw_int, const pipe_fd from) {
    Message msg = {.s_header = {.s_type = STOP}}; 
    while (msg.s_header.s_type != START) {
            int32_t r = read_pipe(from, &msg);
            if(r == 1) continue;
            if(r == -1) return -1;
    }
    return 0;
}

void stop_agents(const SpWInterface* const spw_int) {
    write_pipe(spw_int->to_tx_write, &DEFAULT_STOP_MESSAGE);
    write_pipe(spw_int->to_rx_write, &DEFAULT_STOP_MESSAGE);
    wait_all_stop(spw_int);
}

int32_t enable_rx(SpWInterface* const spw_int) {
    write_pipe(spw_int->to_rx_write, &DEFAULT_START_MESSAGE);
    if(wait_started(spw_int, spw_int->from_rx_read) != 0) return -1; 
    return 0;
}

int32_t enable_tx(SpWInterface* const spw_int) {
    write_pipe(spw_int->to_tx_write, &DEFAULT_START_MESSAGE);
    if(wait_started(spw_int, spw_int->from_tx_read) != 0) return -1; 
    return 0;
}


int32_t poll_children(SpWInterface* const spw_int, Message* msg) {
    int32_t r = read_pipe(spw_int->from_console_read, msg);
    if(r == 1) {
        r = read_pipe(spw_int->from_rx_read, msg);
        if(r == 1) return 1;
    }
    return r;
}

int32_t poll_parent(void *self, Message* msg) {
    ChildProcess* pr = (ChildProcess*) self;
    int32_t r = read_pipe(pr->from_parent_read, msg);
    if(r == 1) {
        return 1;
    }
    if(r == -1) {
        fprintf(stderr, "do_child_payload err %d\n", r);
        return -1;
    }
    return r;
}

int32_t poll_rx(void* self, Packet* packet) {
    ChildProcess* pr = (ChildProcess*) self;
    int32_t r = read_rx_pipe(pr->outer, packet);
    if(r == 1) {
        return 1;
    }
    if(r == -1) {
        fprintf(stderr, "do_child_payload err %d\n", r);
        return -1;
    }
    return r;
}

Packet retrieve_packet_from_msg(const Message* const msg) {

    // printf("retrieving packet from message: [");
    // for(uint16_t i = 0; i < msg->s_header.s_payload_len; ++i) {
    //     printf("%2d ", msg->s_payload[i]);
    // }
    // printf("]\n");

    Packet packet;
    memcpy(&packet.s_header, msg->s_payload, sizeof(PacketHeader));
    memcpy(packet.s_payload, msg->s_payload + sizeof(PacketHeader), packet.s_header.s_payload_len);
    return packet;
}

void print_packet(char* const pr, const Message * const msg){
    Packet packet = retrieve_packet_from_msg(msg);
    printf("%s: [", pr);
    for(uint16_t i = 0; i < packet.s_header.s_payload_len; ++i) {
        #ifndef DEBUG_DECODE_SPW_CHARACTERS
            printf(" %2c", packet.s_payload[i].b);
        #endif 
        
        #ifdef DEBUG_DECODE_SPW_CHARACTERS
            char* str = malloc(sizeof(char) * 4);
            decode_character(str, &packet.s_payload[i]);
            printf(" %s", str);
            free(str);
        #endif 
    }
    printf(" ]\n");
}

void process_handshake_state(SpWInterface* const spw_int, Packet* packet) {
    for(uint16_t i = 0; i < packet->s_header.s_payload_len; ++i) {
        SpWCharcter ch = packet->s_payload[i];
        CharacterType t = get_charater_type(&ch);
        if(t == NULL_CH && spw_int->state == STARTED) {
            spw_int->state = CONNECTING;
            printf("[PARENT] READY -> CONNECTING\n");
        }
        else if(t == FCT_CH && spw_int->state == CONNECTING) {
            spw_int->state = RUN; 
            printf("[PARENT] CONNECTING -> RUN\n");
        }
    }
}

int32_t send_packet_tx(const pipe_fd tx, const Packet* const packet) {
    Message msg = {.s_header = {.s_type = PARENT_CONTROL, .s_payload_len = sizeof(PacketHeader) + packet->s_header.s_payload_len}};
    memcpy(msg.s_payload, packet, sizeof(PacketHeader) + packet->s_header.s_payload_len);
    return write_pipe(tx, &msg);
}

void handshake_send(SpWInterface* const spw_int) {
    if(spw_int->state == STARTED) {
        send_packet_tx(spw_int->to_tx_write, &NULL_PACKET);
    } else if (spw_int->state == CONNECTING) {
        send_packet_tx(spw_int->to_tx_write, &FCT_PACKET);
    }
}
 
void process_packet(SpWInterface* const spw_int, const Message* const msg) {
    Packet packet = retrieve_packet_from_msg(msg);
    if(spw_int->state != RUN) process_handshake_state(spw_int, &packet);
}

void process_link_msg(SpWInterface* const spw_int, const Message* const msg) {

    switch (spw_int->state) {
        case OFF: 
            break;
        case READY:
            break;
        case STARTED:
        case CONNECTING:
                #ifdef DEBUG_HANDSHAKE_PACKETS
                    print_packet("[PARENT] received handshake", msg);
                #endif
                process_packet(spw_int, msg);
            break;
        case RUN:
                #ifdef DEBUG_DATA_PACKETS
                    print_packet("[PARENT] recived from link", msg);
                #endif
            break;
        case ERROR_RESET:
            break;
        case ERROR_WAIT:
            break;
    }
}

int32_t send_packet_to_neigh(const SpWInterface* const spw, char* ch) {
    if(spw->state != RUN) {
        fprintf(stderr, "[PARENT] send fail: link connection not established\n");
        return -1;
    }
    send_packet_tx(spw->to_tx_write, &NULL_PACKET);
    return 0;
}

int32_t push_to_fifo(void* self, Packet* packet){
    ChildProcess* pr = (ChildProcess*) self;
    Message msg = {
        .s_header = { .s_type = LINK, .s_payload_len = sizeof(PacketHeader) + packet->s_header.s_payload_len }
    };
    memcpy(msg.s_payload, packet, sizeof(PacketHeader) + packet->s_header.s_payload_len);
    write_pipe(pr->to_parent_write, &msg);
    return 0;
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
