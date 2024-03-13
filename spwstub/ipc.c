#include "ipc.h"
#include "interprocess.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/errno.h>

int send(void * self, local_id dst, const Message * msg){
    ch_process* pr = (ch_process*) self;
    if(pr->is_rx) return -1;

    int32_t fd_to_write = pr->outer;
    if(fd_to_write == 0) return -1;

    int w = write(fd_to_write, msg, sizeof(MessageHeader) + msg->s_header.s_payload_len);
    if(w == -1) {
        return -1;
    }
    return 0;
}



int receive(void * self, local_id from, Message * msg){
    ch_process* pr = (ch_process*) self;

    int32_t fd_to_read = pr->to_parent_read;
    if(from != 0)
        fd_to_read = pr->outer;

    MessageHeader msg_header;
    int head_bytes = read(fd_to_read, &msg_header, sizeof(MessageHeader));
    if(head_bytes != sizeof(MessageHeader) || head_bytes == -1) return 1; // empty

    msg->s_header = msg_header;

    int msg_bytes = read(fd_to_read, msg->s_payload, msg_header.s_payload_len);
    if(msg_bytes != msg_header.s_payload_len || msg_bytes == -1) {
        printf("is -1, but type is %d, len %d", msg->s_header.s_type, msg->s_header.s_payload_len);
        return -1;
    }

    return head_bytes + msg_bytes;
}


int receive_any(void * self, Message * msg){
    ch_process* pr = (ch_process*) self;
    int r = receive(self, 0, msg);
    if(r == 1) {
        if(pr->is_rx) {
            int r = receive(self, 1, msg);
            if(r == 1) return 1;
            return r;
        }
    }
    return r;
}
