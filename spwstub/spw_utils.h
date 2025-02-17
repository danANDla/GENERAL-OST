#ifndef SPW_STATE_H
#define SPW_STATE_H

#include "ipc.h"
#include "spw_interface.h"

// #define DEBUG_HANDSHAKE_PACKETS
// #define DEBUG_DECODE_SPW_CHARACTERS
#define DEBUG_DATA_PACKETS

void create_pipes(SpWInterface* const spw_int, pipe_fd tx, pipe_fd rx);
void close_pipes(const SpWInterface* const spw_int);
void create_forks(SpWInterface* const spw_int);
int32_t enable_rx(SpWInterface* const spw_int);
int32_t enable_tx(SpWInterface* const spw_int);
void handshake_send(SpWInterface* const spw_int);
int32_t poll_children(SpWInterface* const spw_int, Message* msg);
int32_t poll_parent(void *self, Message* msg);
int32_t poll_rx(void* self, Packet* packet);
Packet retrieve_packet_from_msg(const Message* const msg);
int32_t send_image(SpWInterface* const spw_int, char* path);
int32_t send_char(SpWInterface* const spw_int, char ch);
int32_t push_to_fifo(void* self, Packet* packet);
void process_link_msg(SpWInterface* const spw_int, const Message* const msg);
void stop_agents(const SpWInterface* const spw_int);
void print_packet(char* const pr, const Packet * const packet);

int32_t send_from_queue(ChildProcess* const tx, Fifo* const queue);
int32_t put_packet_in_queue(Fifo * const queue, const Message *const msg);
int32_t flush_rx_fifo(SpWInterface* const spw_int);
int32_t img_from_fifo(SpWInterface* const spw_int);

#endif