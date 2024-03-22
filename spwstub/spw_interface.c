#include "spw_interface.h"
#include "ipc.h"
#include "spw_agents.h"
#include "spw_utils.h"

#define DEBUG_ALL_PIPE_DATA


void powerup_link(SpWInterface* const spw_int, const pipe_fd tx, const pipe_fd rx) {
    if(spw_int->state != OFF) return;
    spw_int->state = ERROR_RESET;
    create_pipes(spw_int, tx, rx);
    spw_int->state = ERROR_WAIT;
    create_forks(spw_int);
    enable_rx(spw_int);
    spw_int->state = READY;
}

void start_link(SpWInterface* const spw_int) {
    if(spw_int->state != READY) return;
    enable_tx(spw_int);
    spw_int->state = STARTED;
    parent_duty(spw_int);
    stop_interface(spw_int);
}

void stop_interface(SpWInterface* const spw_int) {
    stop_agents(spw_int);
    close_pipes(spw_int);
    spw_int->state = OFF;
}