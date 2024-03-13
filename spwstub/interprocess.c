#include "interprocess.h"
#include "ipc.h"

#include <stdio.h>

int32_t listener_duty(ch_process *pr) {
    Message msg;
    msg.s_header.s_type = -1;
    while(msg.s_header.s_type != STOP){
        int r = receive_any(pr, &msg);

        if(r == -1) {
            fprintf(stderr, "do_child_payload err %d\n", r);
            return -1;
        }

        if(r == 1) continue;
        // cmp_clock(pr, msg.s_header.s_local_time + 1);

        if(msg.s_header.s_type == TRANSFER){
            int transfer = do_transfer(pr, &msg);
            if(transfer == 1) return 0;
            if(transfer == -1) return -1;
        }
    }
    return 0;
}
