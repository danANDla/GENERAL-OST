#include "ipc.h"
#include "interprocess.h"

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

int32_t receive_any(void * self, Message * msg){
    ChildProcess* pr = (ChildProcess*) self;
    int32_t r = read_pipe(pr->from_parent_read, msg);
    if(r == 1) {
        if(pr->is_rx) {
            int r = read_pipe(pr->outer, msg);
            if(r == 1) return 1;
            return r;
        }
        return 1;
    }
    return r;
}