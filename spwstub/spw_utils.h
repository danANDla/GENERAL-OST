#ifndef SPW_STATE_H
#define SPW_STATE_H

#include "spw_interface.h"

void create_pipes(SpWInterface* const spw_int, pipe_fd tx, pipe_fd rx);
void close_pipes(const SpWInterface* const spw_int);
void create_forks(SpWInterface* const spw_int);
int32_t poll_children(SpWInterface* const spw_int, Message* msg);
int32_t poll_parent(void *self, Message* msg);
int32_t poll_rx(void* self, Packet* packet);
int32_t send_packet_to_neigh(const SpWInterface* const spw, char* ch);
void process_link_msg(const SpWInterface* const spw_int, const Message* const msg);
void stop_agents(const SpWInterface* const spw_int);

#endif