#include "ipc.h"
#include "spw_packet.h"

#include <complex.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/errno.h>

int32_t write_pipe(pipe_fd to, const Message* const msg) {
    int w = write(to, msg, sizeof(MessageHeader) + msg->s_header.s_payload_len);
    if(w == -1) {
        return -1;
    }
    return 0;
}

int32_t read_pipe(pipe_fd from, Message* const msg) {
    MessageHeader msg_header;
    int head_bytes = read(from, &msg_header, sizeof(MessageHeader));
    if(head_bytes != sizeof(MessageHeader) || head_bytes == -1) return 1; // empty

    msg->s_header = msg_header;

    int msg_bytes = read(from, msg->s_payload, msg_header.s_payload_len);
    if(msg_bytes != msg_header.s_payload_len || msg_bytes == -1) {
        printf("is -1, but type is %d, len %d", msg->s_header.s_type, msg->s_header.s_payload_len);
        return -1;
    }
    return head_bytes + msg_bytes;
}

int32_t write_tx_pipe(pipe_fd to, const Packet* const packet) {
    int32_t fd_to_write = to;
    if(fd_to_write == 0) return -3;

    int w = write(fd_to_write, packet, sizeof(PacketHeader) + packet->s_header.s_payload_len);
    if(w == -1) {
        return -1;
    }
    return 0;
}

int32_t read_rx_pipe(pipe_fd from, Packet* const packet) {
    PacketHeader header;
    int32_t head_bytes = read(from, &header, sizeof(PacketHeader));
    if(head_bytes != sizeof(PacketHeader) || head_bytes == -1) return 1; // empty   
    packet->s_header = header;

    int32_t bytes = read(from, packet->s_payload, header.s_payload_len);
    if(bytes != header.s_payload_len || bytes == -1) {
        return -1;
    }
    return head_bytes + bytes;
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

int32_t push_to_fifo(void* self, Packet* packet) {
    ChildProcess* pr = (ChildProcess*) self;
    Message msg = {
        .s_header = { .s_type = LINK, .s_payload_len = packet->s_header.s_payload_len}
    };
    memcpy(msg.s_payload, packet->s_payload, packet->s_header.s_payload_len);
    write_pipe(pr->to_parent_write, &msg);
}

int32_t rx_duty(ChildProcess *pr) {
    printf("[RX] started duty\n");
    Message msg;
    msg.s_header.s_type = -1;
    printf("[RX] Listening for parent %d and outer rx %d\n", pr->from_parent_read, pr->outer);
    while(msg.s_header.s_type != STOP){
        int32_t r = poll_parent(pr, &msg);
        if(r == 1) {
            Packet packet;
            r = poll_rx(pr, &packet);
            if(r == 1) continue;
            if(r == -1) return -1;
            printf("non empty RX pipe\n");
            push_to_fifo(pr, &packet);
        }
        else if(r == -1) return -1;
        else if (msg.s_header.s_type == STOP) {
            break;
        } else {
            printf("[RX] non supported control\n");
            continue;
        }

    }

    printf("[RX] finished duty\n");
    Message stop =  {.s_header = {.s_type = STOP}};
    write_pipe(pr->to_parent_write, &stop);
    return 0;
}

void send_test_packet(ChildProcess *pr) {
    Packet packet = {.s_header = {.s_payload_len = 10} };
    for(int32_t i = 0; i < packet.s_header.s_payload_len; ++i) {
        packet.s_payload[i] = i;
    }
    write_tx_pipe(pr->outer, &packet);
}

int32_t tx_duty(ChildProcess *pr) {
    printf("[TX] started duty\n");
    Message msg;
    msg.s_header.s_type = -1;
        printf("[TX] Listening for parent %d and writing outer tx %d\n", pr->from_parent_read, pr->outer);
    while(msg.s_header.s_type != STOP){
        int r = poll_parent(pr, &msg);
        if(r == -1) {
            fprintf(stderr, "do_child_payload err %d\n", r);
            return -1;
        }
        if(r == 1) continue;

        if(msg.s_header.s_type == PARENT_CONTROL){
            printf("[TX] sending test packet\n");
            send_test_packet(pr);
        } else if (msg.s_header.s_type == STOP) {
            break;
        } else {
            return -1;
        }
    }
    printf("[TX] finished duty\n");
    Message stop =  {.s_header = {.s_type = STOP}};
    write_pipe(pr->to_parent_write, &stop);
    return 0;
}

Message create_console_msg(char c) {
    Message msg;
    msg.s_header.s_type = CONSOLE_CONTROL;
    msg.s_header.s_payload_len = sizeof(char);
    memcpy(msg.s_payload, &c, sizeof(char));
    return msg;
}

int32_t console_duty(ChildProcess *pr) {
    while(true) {
        printf("Enter command: \n");
        char c;
        scanf(" %c", &c);
        Message msg = create_console_msg(c);
        write_pipe(pr->to_parent_write, &msg);
        if(c == '9') {
            break;
        }
    }
    printf("[CONSOLE] finished duty\n");
    Message stop =  {.s_header = {.s_type = STOP}};
    write_pipe(pr->to_parent_write, &stop);
    return 0;
}