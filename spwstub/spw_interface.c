#include "spw_interface.h"
#include "ipc.h"
#include "spw_agents.h"
#include "spw_utils.h"

#define DEBUG_ALL_PIPE_DATA


void powerup_link(SpWInterface* const spw_int, pipe_fd tx, pipe_fd rx) {
    if(spw_int->state != OFF) return;
    spw_int->state = ERROR_RESET;
    create_pipes(spw_int, tx, rx);
    spw_int->state = ERROR_WAIT;
    create_forks(spw_int);
    enable_rx(spw_int);
    spw_int->state = READY;
}

void start_link(SpWInterface* interface) {
    if(interface->state != READY) return;
    interface->state = STARTED;
    parent_duty(interface);
    stop_interface(interface);
}

void stop_interface(SpWInterface* const spw_int) {
    stop_agents(spw_int);
    close_pipes(spw_int);
    spw_int->state = OFF;
}