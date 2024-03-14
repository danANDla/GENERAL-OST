#include "interprocess.h"
#include "ipc.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

int32_t rx_duty(ChildProcess *pr) {
    printf("RX started duty\n");
    Message msg;
    msg.s_header.s_type = -1;
    while(msg.s_header.s_type != STOP){
        int32_t r = receive_any(pr, &msg);
        if(r == -1) {
            fprintf(stderr, "do_child_payload err %d\n", r);
            return -1;
        }
        if(r == 1) continue;

        if(msg.s_header.s_type == LINK){
            write_pipe(pr->to_parent_write, &msg);
        } else if (msg.s_header.s_type == STOP) {
            break;
        } else {
            return -1;
        }
    }
    printf("[RX] finished duty\n");
    Message stop =  {.s_header = {.s_type = STOP}};
    write_pipe(pr->to_parent_write, &stop);
    return 0;
}

int32_t tx_duty(ChildProcess *pr) {
    printf("[TX] started duty\n");
    Message msg;
    msg.s_header.s_type = -1;
    while(msg.s_header.s_type != STOP){
        int r = receive_any(pr, &msg);
        if(r == -1) {
            fprintf(stderr, "do_child_payload err %d\n", r);
            return -1;
        }
        if(r == 1) continue;

        if(msg.s_header.s_type == LINK){
            write_pipe(pr->to_parent_write, &msg);
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