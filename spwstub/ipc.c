#include "ipc.h"
#include "spw_packet.h"

#include <complex.h>
#include <stdio.h>
#include <unistd.h>
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
