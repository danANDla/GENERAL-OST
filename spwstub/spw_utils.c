#include "spw_utils.h"
#include "ipc.h"
#include "spw_agents.h"
#include "spw_interface.h"
#include "spw_packet.h"
#include "../image/image.h"
#include "../bmp_util/bmp_reader.h"
#include "../bmp_util/bmp_writer.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int pipe2(int fildes[2], int flags);

int32_t send_packet_to_link(ChildProcess* const tx, pipe_fd pipe_to_tx, const Packet* const packet);
int32_t read_fifo(Fifo* const queue, Packet* const packet);

void create_pipes(SpWInterface* const spw_int, pipe_fd tx, pipe_fd rx) {
    int32_t inner_tx_to_parent[2];
    if(pipe2(inner_tx_to_parent, O_NONBLOCK) < 0) exit(0);
    int32_t inner_tx_from_parent[2];
    if(pipe2(inner_tx_from_parent, O_NONBLOCK) < 0) exit(0);
    spw_int->tx = (ChildProcess) {
        .is_rx = false,
        .from_parent_read = inner_tx_from_parent[0],
        .to_parent_write = inner_tx_to_parent[1],
        .outer = tx,
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
        .outer = rx,
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
        .outer = -1,
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
        if(tx_duty(&spw_int->tx, &spw_int->tx_fifo) != 0) exit(-1);
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
    Packet packet;
    memcpy(&packet.s_header, msg->s_payload, sizeof(PacketHeader));
    memcpy(packet.s_payload, msg->s_payload + sizeof(PacketHeader), packet.s_header.s_payload_len);
    return packet;
}

void print_packet(char* const pr, const Packet * const packet){
    printf("%s: (len=%d) [", pr, packet->s_header.s_payload_len);
    uint16_t until = 20;
    if(packet->s_header.s_payload_len < until) 
        until = packet->s_header.s_payload_len;
    for(uint16_t i = 0; i < until; ++i) {
        #ifndef DEBUG_DECODE_SPW_CHARACTERS
            printf(" %2d", packet->s_payload[i].b);
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

int32_t flush_rx_fifo(SpWInterface* const spw_int) {
    int32_t r = 0;
    Packet packet;
    int32_t cnt = 0;
    while(read_fifo(&spw_int->rx_fifo, &packet) == 0) {
        cnt++;
        print_packet("packet", &packet);
    }
    printf("read from queue: %d packets\n", cnt);
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

void handshake_send(SpWInterface* const spw_int) {
    if(spw_int->state == STARTED) {
        send_packet_to_link(&spw_int->tx, spw_int->to_tx_write, &NULL_PACKET);
    } else if (spw_int->state == CONNECTING) {
        send_packet_to_link(&spw_int->tx, spw_int->to_tx_write, &FCT_PACKET);
    }
}
 
void process_packet(SpWInterface* const spw_int, const Message* const msg) {
    Packet packet = retrieve_packet_from_msg(msg);
    if(spw_int->state != RUN) process_handshake_state(spw_int, &packet);
}

void process_link_msg(SpWInterface* const spw_int, const Message* const msg) {
    #ifdef DEBUG_HANDSHAKE_PACKETS
        Packet p = retrieve_packet_from_msg(msg);
    #endif
    switch (spw_int->state) {
        case OFF: 
            break;
        case READY:
            break;
        case STARTED:
        case CONNECTING:
                #ifdef DEBUG_HANDSHAKE_PACKETS
                    print_packet("[PARENT] received handshake", &p);
                #endif
                process_packet(spw_int, msg);
            break;
        case RUN:
            put_packet_in_queue(&spw_int->rx_fifo, msg);
            break;
        case ERROR_RESET:
            break;
        case ERROR_WAIT:
            break;
    }
}

int32_t sep_to_packets(const void *const src, const size_t sz, Packet** const packets, size_t* const num_of_packets) {
    size_t step = MAX_PACKET_LEN;
    *num_of_packets = sz / step;
    if(sz % step) *num_of_packets += 1;
    Packet* res = malloc(sizeof(Packet) * *num_of_packets);
    for(size_t i = 0; i < *num_of_packets - 1; ++i) {
        res[0].s_header = (PacketHeader) {.s_payload_len = step};
        memcpy(res->s_payload, src + i * step, step);
    }
    res[*num_of_packets - 1].s_header = (PacketHeader) {.s_payload_len = sz % step};
    memcpy(res->s_payload, src + (*num_of_packets - 1) * step, sz % step);
    *packets = res;
    return 0;
}

int32_t send_packet_to_tx(const pipe_fd pipe_to_tx, const Packet* const packet) {
    Message msg = {.s_header = {.s_type = PARENT_CONTROL, .s_payload_len = sizeof(PacketHeader) + packet->s_header.s_payload_len}};
    memcpy(msg.s_payload, packet, sizeof(PacketHeader) + packet->s_header.s_payload_len);
    return write_pipe(pipe_to_tx, &msg);
}

int32_t send_packet_to_link(ChildProcess* const tx, pipe_fd pipe_to_tx, const Packet* const packet) {
    #ifdef SEND_THROUGH_BUFF
        return send_packet_to_tx(pipe_to_tx, packet);
    #endif
    #ifndef SEND_THROUGH_BUFF
        return write_tx_pipe(tx, packet);
    #endif
}

int32_t send_data_to_link(SpWInterface* const spw_int, const void *const src, const int64_t sz) {
    Packet p;
    int64_t step = MAX_PACKET_PAYLOAD_LEN;
    int64_t packets_n = sz / step;
    if(sz % step) packets_n += 1;
    for(int64_t i = 0; i < packets_n - 1; ++i) {
        printf("[PARENT] sending %ld packet\n", i);
        p.s_header.s_payload_len = step;
        memcpy(p.s_payload, src + i * step, step);
        send_packet_to_link(&spw_int->tx, spw_int->to_tx_write, &p);
    }
    p.s_header.s_payload_len = sz % step;
    memcpy(p.s_payload, src + (packets_n - 1) * step, sz % step);
    printf("[PARENT] sending %ld packet\n", packets_n - 1);

    return send_packet_to_link(&spw_int->tx, spw_int->to_tx_write, &p);
}

int32_t send_image(SpWInterface* const spw_int, char* path) {
    struct Image img;
    FILE* f = fopen(path, "rb");
    from_bmp(f, &img);
    size_t sz = sizeof(struct Pixel) * img.height * img.width + sizeof(img.height) + sizeof(img.width);
    void* data = malloc(sz);
    memcpy(data, &img.height, sizeof(uint64_t));
    size_t offset = sizeof(uint64_t) ;
    memcpy(data + offset, &img.width, sizeof(uint64_t));
    offset += sizeof(uint64_t);
    memcpy(data + offset, img.pixels, sizeof(struct Pixel) * img.height * img.width);
    int32_t r = send_data_to_link(spw_int, data, sz);
    free(data);
    return r;
}

int32_t send_char(SpWInterface* const spw_int, char ch) {
    return send_data_to_link(spw_int, &ch,1); 
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

int32_t get_q_free(const Fifo * const queue) {
    if(queue->head == queue->tail) return queue->size;

    if(queue->head >= queue->tail) return queue->size - queue->head + queue->tail;
    return queue->tail - queue->head;
}

int32_t send_from_queue(ChildProcess* const tx, Fifo* const queue) {
    if (queue->tail == queue->head) {
        return -1;
    }
    printf("[TX] will send from queue (%d)\n", get_q_free(queue));
    Packet packet = queue->data[queue->tail];
    if(!is_available_to_write(tx->outer, packet.s_header.s_payload_len + sizeof(PacketHeader))) {
        printf("[TX] pipe_is_busy (packets in head=%d, tail=%d)\n", queue->head, queue->tail);
        return -1;
    }
    queue->data[queue->tail].s_header.s_payload_len = 0;
    queue->tail = (queue->tail + 1) % queue->size;

    return write_tx_pipe(tx, &packet);
}

int32_t put_packet_in_queue(Fifo * const queue, const Message *const msg) {
    if (((queue->head + 1) % queue->size) == queue->tail) {
        return -1;
    }
    printf("[PARENT] putting msg in queue(%d)\n", get_q_free(queue));
    Packet p = retrieve_packet_from_msg(msg);
    queue->data[queue->head] = p;
    queue->head = (queue->head + 1) % queue->size;
    return 0;
}

int32_t read_fifo(Fifo* const queue, Packet* const packet) {
    if (queue->tail == queue->head) {
        return -1;
    }
    *packet = queue->data[queue->tail];
    queue->data[queue->tail].s_header.s_payload_len = 0;
    queue->tail = (queue->tail + 1) % queue->size;
    return 0;
}

int32_t img_from_fifo(SpWInterface* const spw_int) {
    Packet packet;
    int32_t r = read_fifo(&spw_int->rx_fifo, &packet);
    struct Image image;

    uint16_t payload_offset = 0;
    memcpy(&image.height, packet.s_payload, sizeof(image.height));
    payload_offset += sizeof(image.height);
    memcpy(&image.width, packet.s_payload + payload_offset, sizeof(image.width));
    payload_offset += sizeof(image.width);
    printf("image width=%ld,height=%ld\n", image.width, image.height);

    image.pixels = malloc(sizeof(struct Pixel) * image.width * image.height);

    uint8_t pixel_buf[3];
    uint8_t buf_sz = 0;
    size_t pixel_offset = 0;

    uint32_t cnt = 0;
    while(r == 0) {
        for(int i = payload_offset; i < packet.s_header.s_payload_len; ++i) {
            memcpy(&pixel_buf[buf_sz], &packet.s_payload[i].b, 1);
            buf_sz++;
            if(buf_sz == 3) {
                memcpy(image.pixels + pixel_offset, pixel_buf, 3 * sizeof(uint8_t));
                pixel_offset++;
                buf_sz = 0;
                cnt++;
            }
        }
        if(buf_sz != 0) printf("syoma\n");
        payload_offset = 0;
        r = read_fifo(&spw_int->rx_fifo, &packet);
    }
    printf("captured %d pixels\n", cnt);

    FILE* res_file = fopen("results/swiss.bmp", "wb");
    if(res_file != NULL)
        to_bmp(res_file, &image);

    free(image.pixels);
    fclose(res_file);

    return 0;
}