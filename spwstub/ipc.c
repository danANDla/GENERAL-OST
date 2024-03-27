#define _GNU_SOURCE
#include "ipc.h"
#include "spw_packet.h"
#include <complex.h>
#include <linux/limits.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>

Message DEFAULT_START_MESSAGE =  {
    .s_header = {
        .s_payload_len = 0,
        .s_type = START,
    }
};

Message DEFAULT_STOP_MESSAGE =  {
    .s_header = {
        .s_payload_len = 0,
        .s_type = STOP,
    }
};

static int32_t PIPE_CAP;

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

int32_t write_tx_pipe(ChildProcess* const pr, const Packet* const packet) {
    pipe_fd to = pr->outer;
    if(to == 0) return -3;

    struct timeval tv;
    gettimeofday(&tv,NULL);
    int64_t cur = 1000000 * tv.tv_sec + tv.tv_usec;
    while(cur - pr->last_send < USECOND_TX_DELAY) {
        gettimeofday(&tv,NULL);
        cur = 1000000 * tv.tv_sec + tv.tv_usec;
    }
    pr->last_send = cur;

    int w = write(to, packet, sizeof(PacketHeader) + packet->s_header.s_payload_len);
    if(w == -1) {
        int sz;
        ioctl(to, FIONREAD, &sz);
        printf("failed to write (bytes in pipe: %d)\n", sz);
        return -1;
    }
    return 0;
}

int32_t is_available_to_write(pipe_fd to, int32_t to_write) {
    int32_t sz;
    ioctl(to, FIONREAD, &sz);
    if((PIPE_CAP - sz) - to_write > 0) return 1;
    return 0;
}

int32_t init_pipe_cap(pipe_fd from) {
    int32_t sz;
    fcntl(from, F_GETPIPE_SZ);
    PIPE_CAP = sz;
}

int32_t read_rx_pipe(pipe_fd from, Packet* const packet) {
    PacketHeader header;
    int32_t head_bytes = read(from, &header, sizeof(PacketHeader));
    if(head_bytes != sizeof(PacketHeader) || head_bytes == -1) return 1; // empty   
    packet->s_header = header;

    int32_t bytes = read(from, packet->s_payload, header.s_payload_len);
    if(bytes != header.s_payload_len || bytes == -1) {
        printf("is thisexpected %d, received %d\n", header.s_payload_len, bytes);
        return -1;
    }
    return head_bytes + bytes;
}
