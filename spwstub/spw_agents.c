#include "ipc.h"

#include <stdio.h>

#include "spw_utils.h"
#include "string.h"

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
