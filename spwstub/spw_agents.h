#ifndef SPW_AGENTS_H
#define SPW_AGENTS_H

#include "spw_interface.h"

int32_t rx_duty(ChildProcess *pr);
int32_t tx_duty(ChildProcess *pr);
int32_t console_duty(ChildProcess *pr);
int32_t parent_duty(SpWInterface* const spw_int);

#endif