#ifndef SPW_LAYER_H
#define SPW_LAYER_H

#include "../swic.h"
#include <inttypes.h>

#define MAX_WORMHOLE_ADDRESS_LENGTH 4
#define TRY 100
#define SWIC_SPEED 10

#ifdef __cplusplus
extern "C"
{
#endif

int8_t spw_is_ready_to_transmit(const SWIC_SEND interface);
int8_t spw_send_data(const SWIC_SEND interface, const uint8_t* const data, const uint32_t sz);
int8_t spw_hw_init();

#ifdef __cplusplus
}
#endif

#endif // SPW_LAYER_H
